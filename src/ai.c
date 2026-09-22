#include "game.h"
#include "ai_parallel.h"
#include "ai_strategy.h"
#include "ml.h"
#include "ai_belief.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Hidden ranks are erased before evaluating or sampling any search board. */
#define SAMPLES 16
#define ROOT_WIDTH 24
#define BEAM 10
#define WIN 10000.0f
static const float worth[12]={1000,7,4,12,7,9,12,17,24,35,50,9};
#include "ai_force.h"
typedef struct {Move move;float score;} Candidate;
static int neighbors(int s,int n[4]) {
    int k=0;if(s%10)n[k++]=s-1;if(s%10<9)n[k++]=s+1;if(s>=10)n[k++]=s-10;if(s<90)n[k++]=s+10;return k;
}
static float exchange(int a,int d) {int c=combat_result(a,d);return c>0?worth[d]:(c<0?-worth[a]:worth[d]-worth[a]);}
/* Determinized search knows the hypothetical ranks already, so it cannot
   discover the value of information by itself. Pay for NEW observations only,
   weighted by uncertainty and the cost/role of the probing unit. */
static float information_gain(const Game *view,float p[100][12],Move m) {
    Piece a=view->board[m.from],d=view->board[m.to];
    if(d.side<0||d.revealed)return 0;
    float entropy=0;
    for(int r=0;r<12;r++)if(p[m.to][r]>0)entropy-=p[m.to][r]*log2f(p[m.to][r]);
    float rate=a.rank==SCOUT?worth[SCOUT]:a.rank==SERGEANT?1.5f:a.rank==LIEUTENANT?1.1f:.35f;
    /* Preserve scarce miners and spies for their special combat roles. */
    if(a.rank==MINER||a.rank==SPY)rate=.25f;
    /* Keep reconnaissance available, but avoid spending scouts merely to
       observe weak targets once a substantial advantage is established. */
    float advantage=force_advantage(view);
    return entropy*rate*(advantage>.2f?.6f:1.0f);
}
/* Moving one square only discloses mobility. A quiet scout ray also gives
   away its exact rank, permanently removing its bluff value. Charge this
   once, using only the army composition and identities already revealed to
   the opponent. Keep it a soft cost so urgent defense or escape can win. */
static float scout_disclosure_cost(const Game *view,Move m) {
    Piece a=view->board[m.from];
    if(a.rank!=SCOUT||a.revealed||view->board[m.to].side>=0)return 0;
    if(abs(m.to%10-m.from%10)+abs(m.to/10-m.from/10)<=1)return 0;
    int unknown[12];
    for(int r=SPY;r<=MARSHAL;r++)unknown[r]=army_counts[r]-view->captured[a.side][r];
    for(int s=0;s<100;s++){
        Piece own=view->board[s];
        if(own.side==a.side&&own.revealed&&own.rank>=SPY&&own.rank<=MARSHAL)unknown[own.rank]--;
    }
    int total=0;for(int r=SPY;r<=MARSHAL;r++)total+=unknown[r]>0?unknown[r]:0;
    if(total<=0)return 0;
    float scout_fraction=(float)(unknown[SCOUT]>0?unknown[SCOUT]:0)/total;
    return 5*worth[SCOUT]*(1-scout_fraction);
}
/* High officers surrender lasting information on their first surviving
   combat. Scale that price by the still-hidden mobile army, not the clock:
   late deployment and endgames should not preserve secrecy at any cost. */
static float officer_disclosure_cost(const Game *view,float p[100][12],Move m){
    Piece a=view->board[m.from],d=view->board[m.to];
    if((a.rank!=GENERAL&&a.rank!=MARSHAL)||a.revealed||d.side!=1-a.side)return 0;
    int hidden[12]={0},total=0;
    for(int s=0;s<100;s++){
        Piece own=view->board[s];
        if(own.side==a.side&&!own.revealed&&movable(own)){hidden[own.rank]++;total++;}
    }
    if(total<=hidden[a.rank])return 0; /* Already inferable from public losses. */
    float survives=0;
    for(int r=SPY;r<=BOMB;r++)if(combat_result(a.rank,r)>0)survives+=p[m.to][r];
    float uncertainty=1-(float)hidden[a.rank]/total;
    return 1.5f*worth[a.rank]*fminf(1,total/24.0f)*uncertainty*survives;
}
/* The last miners are the remaining route through a bomb screen. Do not pay
   for information with them when the target has moved (so cannot be a bomb
   or flag). Known recaptures and actual bomb/flag probes keep their priority. */
static float scarce_miner_probe(const Game *view,float p[100][12],Move m){
    Piece a=view->board[m.from],d=view->board[m.to];
    if(a.rank!=MINER||d.side!=1-a.side||d.revealed||!d.moved)return 0;
    int left=army_counts[MINER]-view->captured[a.side][MINER];
    if(left<1||left>2||view->captured[1-a.side][BOMB]>=army_counts[BOMB])return 0;
    float failure=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(MINER,r)<=0)failure+=p[m.to][r];
    return worth[MINER]*4*failure/left;
}
/* A rare spy is often absent from a small set of sampled armies. Do not
   interpret that absence as permission to expose the marshal to an unknown
   adjacent unit. This premium uses public spy possibilities, never true ranks. */
static float spy_exposure(const Game *view,float p[100][12],Move m,bool after) {
    Piece a=view->board[m.from];if(a.rank!=MARSHAL)return 0;
    int square=after?m.to:m.from,nb[4],n=neighbors(square,nb);float risk=0;
    for(int i=0;i<n;i++) {
        int e=nb[i];
        Piece enemy=view->board[e];if(enemy.side!=1-a.side||p[e][SPY]<=0)continue;
        risk+=enemy.revealed?2.0f:1.0f;
    }
    return fminf(risk,2)*worth[MARSHAL]*.9f;
}
static void insert(Candidate *list,int *count,int limit,Move m,float score) {
    int at=*count;if(at==limit){if(score<=list[at-1].score)return;at--;}else (*count)++;
    while(at>0&&score>list[at-1].score){list[at]=list[at-1];at--;}
    list[at]=(Candidate){m,score};
}
static void public_board(const Game *g,Game *view,int remaining[12]) {
    *view=*g;int enemy=1-g->turn;
    for(int r=0;r<12;r++)remaining[r]=army_counts[r]-g->captured[enemy][r];
    for(int s=0;s<100;s++)if(g->board[s].side==enemy) {
        if(g->board[s].revealed)remaining[g->board[s].rank]--;
        else view->board[s].rank=-2;
    }
    /* Public casualties and movement can identify a rank without combat.
       Propagate only logically forced identities, never a high prior. Remove
       each inferred piece from the unassigned bag before sampling. */
    bool changed;
    do{
        changed=false;
        for(int s=0;s<100;s++)if(view->board[s].side==enemy&&view->board[s].rank==-2){
            int possible=0,rank=-1;
            for(int r=0;r<12;r++)if(remaining[r]>0&&
                (!view->board[s].moved||(r!=FLAG&&r!=BOMB))){possible++;rank=r;}
            if(possible==1){
                view->board[s].rank=rank;view->board[s].revealed=true;
                remaining[rank]--;changed=true;
            }
        }
    }while(changed);
}
static float prior(const Game *view,int s,int rank) {
    return ai_rank_prior(view,s,rank);
}
static void probabilities(const Game *view,const int remaining[12],float p[100][12]) {
    memset(p,0,100*12*sizeof(float));
    for(int s=0;s<100;s++)if(view->board[s].side==1-view->turn) {
        if(view->board[s].rank>=0){p[s][view->board[s].rank]=1;continue;}
        float total=0;
        for(int r=0;r<12;r++){p[s][r]=fmaxf(remaining[r],0)*prior(view,s,r);total+=p[s][r];}
        if(total>0)for(int r=0;r<12;r++)p[s][r]/=total;
    }
}
static void sample_board(const Game *view,const int remaining[12],Game *sample,uint32_t *rng) {
    *sample=*view;
    /* Assign fixed ranks first, without replacement, preserving the public army.
       A moved identity can never be assigned a flag or a bomb. */
    int order[12]={FLAG,BOMB,SPY,SCOUT,MINER,SERGEANT,LIEUTENANT,CAPTAIN,MAJOR,COLONEL,GENERAL,MARSHAL};
    for(int k=0;k<12;k++){int r=order[k];for(int j=0;j<remaining[r];j++) {
        float weights[100]={0},total=0;
        for(int s=0;s<100;s++)if(sample->board[s].rank==-2){weights[s]=prior(view,s,r);total+=weights[s];}
        if(total<=0)break;
        float pick=(float)(game_random(rng)%1000000)/1000000.0f*total;
        for(int s=0;s<100;s++)if(weights[s]>0){pick-=weights[s];if(pick<=0){sample->board[s].rank=r;break;}}
    }}
    /* Allow sparse test positions with an incomplete casualty ledger. */
    for(int s=0;s<100;s++)if(sample->board[s].rank==-2)sample->board[s].rank=SCOUT;
}
static float evaluate(const Game *g,int side,bool flag_known) {
    if(g->winner==GAME_DRAW)return 0;
    if(g->winner>=0){
        if(g->winner!=side)return -WIN;
        if(flag_known||g->combat!=1||g->defend_rank!=FLAG)return WIN;
        /* Guessed hidden flags are information opportunities, not proven mates.
           Otherwise the search invents winning plans it cannot execute later. */
        Game approximation=*g;approximation.winner=-1;
        return evaluate(&approximation,side,true)+35;
    }
    float score=0;int mobile[2]={0},alive[2][12]={{0}},flag[2]={-1,-1},guard[2]={15,15};
    for(int s=0;s<100;s++)if(g->board[s].side>=0){Piece p=g->board[s];alive[p.side][p.rank]++;if(p.rank==FLAG)flag[p.side]=s;}
    for(int s=0;s<100;s++){Piece p=g->board[s];if(p.side<0)continue;
        float v=force_value(alive,p.side,p.rank);
        if(movable(p)) {
            mobile[p.side]++;
            int advance=p.side==COMPUTER?s/10:9-s/10;
            v+=advance*.18f;
            int nb[4],n=neighbors(s,nb),freedom=0;
            for(int k=0;k<n;k++)if(!is_lake(nb[k])&&g->board[nb[k]].side!=p.side)freedom++;
            if(freedom&&p.rank>=MINER&&flag[p.side]>=0){int f=flag[p.side],d=abs(s%10-f%10)+abs(s/10-f/10);if(d<guard[p.side])guard[p.side]=d;}
            v+=freedom*.16f;
            /* Connected troops can recapture invaders and escort miners. */
            int support=0;
            for(int k=0;k<n;k++){Piece friend=g->board[nb[k]];if(friend.side==p.side&&movable(friend))support++;}
            v+=fminf(2,support)*.22f;
            if(flag[1-p.side]>=0){int f=flag[1-p.side];int dist=abs(s%10-f%10)+abs(s/10-f/10);v+=1.6f/(dist+1);}
        }
        score+=p.side==side?v:-v;
    }
    if(!mobile[side])return -WIN;
    if(!mobile[1-side])return WIN;
    /* Keep a reserve able to intercept miners before the last bomb falls. */
    score-=fmaxf(0,guard[side]-3)*1.8f;
    score+=fmaxf(0,guard[1-side]-3)*1.8f;
    /* Penalize the best available enemy capture across the WHOLE army, not
       just the moved unit. Subtract a recapturable attacker's material value. */
    float hanging[2]={0,0};
    for(int s=0;s<100;s++){Piece victim=g->board[s];if(victim.side<0||victim.rank==BOMB)continue;
        /* A sampled hidden flag is only a hypothesis. Crediting its threat
           at full flag value, then valuing its capture as an observation,
           makes waiting beside it score higher than actually taking it. */
        if(victim.rank==FLAG&&victim.side!=side&&!flag_known)continue;
        const int dx[4]={-1,1,0,0},dz[4]={0,0,-1,1};
        for(int dir=0;dir<4;dir++)for(int step=1;step<10;step++) {
            int x=s%10+dx[dir]*step,z=s/10+dz[dir]*step;if(x<0||x>9||z<0||z>9)break;
            int t=z*10+x;if(is_lake(t))break;Piece attacker=g->board[t];if(attacker.side<0)continue;
            if(attacker.side!=victim.side&&movable(attacker)&&(step==1||attacker.rank==SCOUT)&&combat_result(attacker.rank,victim.rank)>=0){
                float loss=worth[victim.rank];int result=combat_result(attacker.rank,victim.rank);
                bool recapture=result==0;
                int nb[4],n=neighbors(s,nb);
                for(int k=0;k<n;k++){Piece defender=g->board[nb[k]];if(defender.side==victim.side&&movable(defender)&&combat_result(defender.rank,attacker.rank)>=0)recapture=true;}
                if(recapture&&victim.rank!=FLAG)loss=fmaxf(0,loss-worth[attacker.rank]);
                if(loss>hanging[victim.side])hanging[victim.side]=loss;
            }
            break;
        }
    }
    score-=(g->turn==side?.28f:.85f)*hanging[side];
    score+=(g->turn==side?.85f:.28f)*hanging[1-side];
    return score;
}
static float repetition_penalty(const Game *g,Move m) {
    int side=g->turn,id=g->board[m.from].id;float penalty=0;
    int count=g->history_count[side]<8?g->history_count[side]:8;
    for(int i=0;i<count;i++)if(g->history_id[side][i]==id&&g->history[side][i].from==m.to)penalty+=.75f;
    return penalty;
}
static float move_order(const Game *g,Move m) {
    Piece a=g->board[m.from],d=g->board[m.to];float score=0;
    if(d.side>=0)score=exchange(a.rank,d.rank)*12+4;
    score+=(m.to/10-m.from/10)*(a.side==COMPUTER?1:-1)*.1f;
    int nb[4],n=neighbors(m.from,nb);
    for(int i=0;i<n;i++){Piece e=g->board[nb[i]];if(e.side==1-a.side&&movable(e)&&combat_result(e.rank,a.rank)>=0)score+=worth[a.rank]*3;}
    /* Quiet retreat must survive beam selection in the simulated replies too,
       not just receive a safety bonus at the real move's root. */
    if(a.rank>=COLONEL&&a.rank<=MARSHAL){
        int nearest=100,after=100;
        for(int s=0;s<100;s++){
            Piece e=g->board[s];if(e.side!=1-a.side||!e.revealed||!movable(e)||combat_result(e.rank,a.rank)<=0)continue;
            int d=abs(s%10-m.from%10)+abs(s/10-m.from/10);
            if(d<nearest){nearest=d;after=abs(s%10-m.to%10)+abs(s/10-m.to/10);}
        }
        if(nearest<=4){
            int retreat=(m.from/10-m.to/10)*(a.side==COMPUTER?1:-1);
            score+=worth[a.rank]*(after-nearest+2*retreat);
        }
    }
    return score-repetition_penalty(g,m);
}
static float tactical_search(const Game *g,int depth,int side,float alpha,float beta,int *budget,bool flag_known){
    float best=evaluate(g,side,flag_known);
    if(g->winner>=0||depth<=0||(*budget)--<=0)return best;
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);
    if(!n)return g->turn==side?-WIN:WIN;
    bool maximizing=g->turn==side;
    if(maximizing){if(best>=beta)return best;if(best>alpha)alpha=best;}
    else {if(best<=alpha)return best;if(best<beta)beta=best;}
    Candidate captures[6];int count=0;
    for(int i=0;i<n;i++)if(g->board[moves[i].to].side>=0)
        insert(captures,&count,6,moves[i],move_order(g,moves[i]));
    for(int i=0;i<count&&*budget>0;i++){
        Game child=*g;game_apply(&child,captures[i].move);
        float value=tactical_search(&child,depth-1,side,alpha,beta,budget,flag_known);
        if(maximizing){if(value>best)best=value;if(best>alpha)alpha=best;}
        else {if(value<best)best=value;if(best<beta)beta=best;}
        if(alpha>=beta)break;
    }
    return best;
}
/* Each hypothetical world has its own cache. Include repetition and move
   history: identical piece placement alone is NOT the same search position. */
#define TT_SIZE 4096
typedef struct {uint64_t key;float value;Move move;int depth,bound;bool valid;} SearchEntry;
typedef struct {SearchEntry *table;int budget;bool aborted;int hits,completed;} SearchContext;
static uint64_t hash_word(uint64_t h,int value){return (h^(uint32_t)value)*UINT64_C(1099511628211);}
static uint64_t search_key(const Game *g){
    uint64_t h=UINT64_C(14695981039346656037);
    for(int s=0;s<100;s++){Piece p=g->board[s];h=hash_word(h,p.rank);h=hash_word(h,p.side);h=hash_word(h,p.id);h=hash_word(h,p.revealed+2*p.moved);}
    h=hash_word(h,g->turn);h=hash_word(h,g->winner);h=hash_word(h,g->ply);
    h=hash_word(h,g->combat);h=hash_word(h,g->attack_rank);h=hash_word(h,g->defend_rank);
    h=hash_word(h,g->last_move.from);h=hash_word(h,g->last_move.to);
    for(int side=0;side<2;side++){
        for(int r=0;r<12;r++)h=hash_word(h,g->captured[side][r]);
        h=hash_word(h,g->last_id[side]);h=hash_word(h,g->last_from[side]);h=hash_word(h,g->last_to[side]);h=hash_word(h,g->repetitions[side]);
        h=hash_word(h,g->history_count[side]);
        for(int k=0;k<8;k++){h=hash_word(h,g->history[side][k].from);h=hash_word(h,g->history[side][k].to);h=hash_word(h,g->history_id[side][k]);}
    }
    return h;
}
static float search(const Game *g,int depth,int side,float alpha,float beta,SearchContext *ctx,bool flag_known) {
    if(ctx->budget--<=0){ctx->aborted=true;return 0;}
    if(g->winner>=0)return evaluate(g,side,flag_known);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);
    if(!n)return g->turn==side?-WIN:WIN;
    if(depth==0){
        float value=tactical_search(g,3,side,alpha,beta,&ctx->budget,flag_known);
        if(ctx->budget<=0)ctx->aborted=true;
        return value;
    }
    float original_alpha=alpha,original_beta=beta;uint64_t key=0;
    SearchEntry *entry=NULL;Move preferred={-1,-1};
    if(ctx->table){
        key=search_key(g);entry=&ctx->table[key&(TT_SIZE-1)];
        if(entry->valid&&entry->key==key){
            preferred=entry->move;ctx->hits++;
            if(entry->depth>=depth){
                if(entry->bound==0)return entry->value;
                if(entry->bound==1&&entry->value>alpha)alpha=entry->value;
                if(entry->bound==2&&entry->value<beta)beta=entry->value;
                if(alpha>=beta)return entry->value;
            }
        }
    }
    Candidate best_moves[BEAM];int count=0;
    for(int i=0;i<n;i++)insert(best_moves,&count,BEAM,moves[i],move_order(g,moves[i]));
    /* Reorder the selected set without changing its membership. A cached move
       must not silently change which branches a selective search considers. */
    for(int i=1;i<count;i++)if(best_moves[i].move.from==preferred.from&&best_moves[i].move.to==preferred.to){
        Candidate first=best_moves[i];memmove(best_moves+1,best_moves,(size_t)i*sizeof(Candidate));best_moves[0]=first;break;
    }
    float best=g->turn==side?-1e20f:1e20f;
    Move best_move=moves[0];
    for(int i=0;i<count;i++) {
        Game child=*g;game_apply(&child,best_moves[i].move);
        float v=search(&child,depth-1,side,alpha,beta,ctx,flag_known);
        if(ctx->aborted)return 0;
        if(g->turn==side){if(v>best){best=v;best_move=best_moves[i].move;}if(best>alpha)alpha=best;}
        else {if(v<best){best=v;best_move=best_moves[i].move;}if(best<beta)beta=best;}
        if(beta<=alpha)break;
    }
    if(entry)*entry=(SearchEntry){key,best,best_move,depth,best<=original_alpha?2:best>=original_beta?1:0,true};
    return best;
}
static float deepen_search(const Game *g,int depth,int side,SearchContext *ctx,bool flag_known,float *levels){
    float best=evaluate(g,side,flag_known);
    if(levels)levels[0]=best;
    /* After the initial fallback, extend by complete reply pairs. This also
       avoids spending the budget on every intervening odd horizon. */
    for(int d=1;d<=depth;d=d==1?2:d+2){
        float value=search(g,d,side,-1e20f,1e20f,ctx,flag_known);
        if(ctx->aborted)break;
        best=value;ctx->completed=d;
        if(levels)levels[d]=best;
    }
    return best;
}
/* Route maps use public observations and land paths. Friendly movable troops
   can be traversed at extra planning cost; fixed friendly defenses cannot. */
static void route_map(const Game *view,float p[100][12],int rank,float dist[100]) {
    bool used[100]={false};
    for(int s=0;s<100;s++) {
        dist[s]=1000;
        if(view->board[s].side==1-view->turn){
            float gain=0,win=0;
            for(int r=0;r<12;r++){gain+=p[s][r]*(r==FLAG?35:exchange(rank,r));if(combat_result(rank,r)>0)win+=p[s][r];}
            /* Officers hunt living threats; scouts/miners investigate static
               defenses. This prevents aimless marches into probable bombs. */
            if(rank>=CAPTAIN&&!view->board[s].moved&&!view->board[s].revealed&&p[s][BOMB]>.06f)continue;
            if(gain>0||win>.55f)dist[s]=-fminf(10,gain*.35f)-win*2;
            else if(!view->board[s].revealed&&rank<=MINER)dist[s]=1;
        }
        if(rank>=CAPTAIN&&view->board[s].side==view->turn&&view->board[s].rank==MINER){
            int advance=view->turn==COMPUTER?s/10:9-s/10;
            if(advance>=4)dist[s]=1.0f-advance*.35f;
        }
    }
    for(int k=0;k<100;k++) {
        int s=-1;for(int t=0;t<100;t++)if(!used[t]&&!is_lake(t)&&(s<0||dist[t]<dist[s]))s=t;
        if(s<0||dist[s]>=999)break;
        used[s]=true;
        int nb[4],n=neighbors(s,nb);
        for(int i=0;i<n;i++){int t=nb[i];Piece e=view->board[t];if(is_lake(t))continue;
            if(e.side==view->turn&&(e.rank==BOMB||e.rank==FLAG))continue;
            float cost=e.side==view->turn?2.5f:1;
            if(e.side==1-view->turn){if(p[t][BOMB]>.99f&&rank!=MINER)continue;cost+=2;}
            if(dist[t]>dist[s]+cost)dist[t]=dist[s]+cost;
        }
    }
}
static float expected_threat(const Game *g,float p[100][12],Move m,bool after) {
    Piece a=g->board[m.from];int square=after?m.to:m.from;float risk=0;
    for(int s=0;s<100;s++)if(g->board[s].side==1-a.side&&(!after||s!=m.to)) {
        int dx=s%10-square%10,dz=s/10-square/10;if(dx&&dz)continue;
        int distance=abs(dx)+abs(dz);if(!distance)continue;
        int step=dx?(dx>0?1:-1):(dz>0?10:-10);bool clear=true;
        for(int t=square+step;t!=s;t+=step)if(is_lake(t)||(g->board[t].side>=0&&(!after||t!=m.from))){clear=false;break;}
        if(!clear)continue;
        float threat=0;for(int r=1;r<=10;r++)if(distance==1||r==SCOUT) {
            int result=combat_result(r,a.rank);
            if(result>=0)threat+=p[s][r]*(result>0?worth[a.rank]:fmaxf(0,worth[a.rank]-worth[r]));
        }
        if(threat>risk)risk=threat;
    }
    return risk;
}
/* Give reconnaissance a route through our own formation. Compare complete
   before/after maps: comparing two squares in the old map falsely rewards
   leaving the mover's own occupied square, even on a sideways shuffle. */
static float recon_progress(const Game *view,float p[100][12],Move m,const float before[100]){
    Piece a=view->board[m.from];
    if(view->board[m.to].side>=0)return 0;
    bool unknown=false,scout=false;
    for(int s=0;s<100;s++){
        Piece e=view->board[s];
        if(e.side==1-view->turn&&!e.revealed)unknown=true;
        if(e.side==view->turn&&e.rank==SCOUT)scout=true;
    }
    if(!unknown||!scout)return 0;
    if(a.rank!=SCOUT&&expected_threat(view,p,m,true)>expected_threat(view,p,m,false)+1)return 0;
    Game next=*view;next.board[m.from]=empty_piece();next.board[m.to]=a;
    float after[100];route_map(&next,p,SCOUT,after);
    float best=0;
    for(int s=0;s<100;s++)if(view->board[s].side==view->turn&&view->board[s].rank==SCOUT){
        int target=s==m.from?m.to:s;
        if(before[s]>=999||after[target]>=999)continue;
        float progress=before[s]-after[target];
        if(s==m.from)best=progress;
        else if(a.rank!=SCOUT)best=fmaxf(best,progress);
    }
    /* One step and a long disclosure receive the same maximum incentive. */
    return fmaxf(-6,fminf(6,best*4));
}
/* Moving from one threatened square to another is not a free exchange of
   risks: a capture hands the reply to a different, possibly stronger support
   piece. Price that reply independently of exposure at the starting square. */
static float supported_capture_cost(const Game *view,float p[100][12],Move m){
    Piece a=view->board[m.from],d=view->board[m.to];
    if(a.rank<COLONEL||a.rank>MARSHAL||d.side!=1-a.side)return 0;
    float worst=0;int nb[4],n=neighbors(m.to,nb);
    for(int k=0;k<n;k++){
        int e=nb[k];Piece support=view->board[e];
        if(e==m.from||support.side!=1-a.side||support.revealed)continue;
        float defeat=0;
        for(int r=SPY;r<=MARSHAL;r++)if(combat_result(r,a.rank)>0)defeat+=p[e][r];
        if(defeat>0)worst=fmaxf(worst,fmaxf(defeat,support.moved?.5f:.25f));
    }
    return 2*worth[a.rank]*worst;
}
static void land_distances(int source,int distance[100]) {
    int queue[100],head=0,tail=0;for(int s=0;s<100;s++)distance[s]=100;
    distance[source]=0;queue[tail++]=source;
    while(head<tail){int s=queue[head++],nb[4],n=neighbors(s,nb);
        for(int k=0;k<n;k++){int t=nb[k];if(is_lake(t)||distance[t]!=100)continue;distance[t]=distance[s]+1;queue[tail++]=t;}
    }
}
/* Masked ranks still occupy squares. In particular a scout cannot see through
   an unidentified piece when checking a public tactical threat. */
static bool public_legal(const Game *g,Move m,int side){
    if(!game_legal(g,m,side))return false;
    int dx=m.to%10-m.from%10,step=dx?(dx>0?1:-1):(m.to>m.from?10:-10);
    for(int s=m.from+step;s!=m.to;s+=step)if(g->board[s].side>=0)return false;
    return true;
}
static Game optimistic_move(const Game *view,Move m){
    Game next=*view;Piece a=next.board[m.from],d=next.board[m.to];
    bool reverse=next.last_id[a.side]==a.id&&m.from==next.last_to[a.side]&&m.to==next.last_from[a.side];
    next.repetitions[a.side]=reverse?next.repetitions[a.side]+1:1;
    next.last_id[a.side]=a.id;next.last_from[a.side]=m.from;next.last_to[a.side]=m.to;
    a.moved=true;
    next.board[m.from]=empty_piece();
    int result=d.side<0||!d.revealed?1:combat_result(a.rank,d.rank);
    if(result>0)next.board[m.to]=a;
    else if(result==0)next.board[m.to]=empty_piece();
    return next;
}
/* Preserve unresolved contact threats across turns, including support behind
   a tempting capture. A quiet move elsewhere does not erase a possible spy
   or marshal. Count the worst unknown attacker per officer, not every
   adjacent square as an independent simultaneous capture. */
static float approaching_officer_risk_from(const Game *view,const Game *intent,float p[100][12]) {
    int side=view->turn;float risk=0;
    for(int s=0;s<100;s++){
        Piece officer=view->board[s];
        if(officer.side!=side||officer.rank<COLONEL||officer.rank>MARSHAL)continue;
        int anchor=s;
        for(int t=0;t<100;t++)if(intent->board[t].side==side&&intent->board[t].id==officer.id){anchor=t;break;}
        float worst=0;int nb[4],count=neighbors(s,nb);
        for(int k=0;k<count;k++){
            int e=nb[k];Piece hunter=view->board[e];
            if(hunter.side!=1-side||hunter.revealed)continue;
            float defeat=0;
            for(int r=SPY;r<=MARSHAL;r++)if(combat_result(r,officer.rank)>0)defeat+=p[e][r];
            if(defeat<=0)continue;
            /* Keep intent evidence as an extra premium: otherwise a retreat
               from a recent pursuer towards an old unknown scout looks just
               as dangerous as standing still. Baseline contact risk never
               expires when this short history rolls over. */
            bool approached=false;int hcount=intent->history_count[1-side];if(hcount>8)hcount=8;
            int past_anchor=anchor;
            for(int age=0;age<hcount;age++){
                /* At our turn, the latest enemy move followed our latest
                   move. Rewind one own move per older enemy observation. */
                if(age>0&&age<=intent->history_count[side]){
                    int own=(intent->history_count[side]-age)%8;
                    if(intent->history_id[side][own]==officer.id)past_anchor=intent->history[side][own].from;
                }
                int h=(intent->history_count[1-side]-1-age)%8;
                Move past=intent->history[1-side][h];
                if(intent->history_id[1-side][h]==hunter.id&&past.to==e&&
                   abs(e%10-past_anchor%10)+abs(e/10-past_anchor/10)==1&&
                   abs(past.from%10-past_anchor%10)+abs(past.from/10-past_anchor/10)>1)approached=true;
            }
            float floor=officer.revealed?(hunter.moved?.5f:.25f):(hunter.moved?.25f:.1f);
            if(officer.revealed&&approached)floor+=.5f;
            float loss=4*worth[officer.rank]*fmaxf(floor,defeat);
            if(loss>worst)worst=loss;
        }
        risk+=worst;
    }
    return risk;
}
static float approaching_officer_risk(const Game *view,float p[100][12]) {
    return approaching_officer_risk_from(view,view,p);
}
static int officer_exits(const Game *view,float p[100][12],int s){
    int nb[4],n=neighbors(s,nb),exits=0;Piece officer=view->board[s];
    for(int k=0;k<n;k++){
        Move escape={s,nb[k]};
        if(view->board[escape.to].side>=0||!public_legal(view,escape,officer.side))continue;
        if(expected_threat(view,p,escape,true)>.1f*worth[officer.rank]||spy_exposure(view,p,escape,true)>0)continue;
        exits++;
    }
    return exits;
}
/* Credit a teammate for opening the first usable exit of a threatened
   officer. Avoid rewarding pointless shuffles when an exit already exists. */
static float officer_clearance(const Game *view,float p[100][12],Move m){
    Piece d=view->board[m.to],a=view->board[m.from];
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))return 0;
    Game next=optimistic_move(view,m);float bonus=0;int nb[4],n=neighbors(m.from,nb);
    for(int k=0;k<n;k++){
        int s=nb[k];Piece officer=view->board[s];
        if(officer.side!=view->turn||officer.rank<COLONEL||officer.rank>MARSHAL||!officer.revealed)continue;
        Move standing={s,s};
        if(expected_threat(view,p,standing,false)<=0&&spy_exposure(view,p,standing,false)<=0)continue;
        if(!officer_exits(view,p,s)&&officer_exits(&next,p,s))bonus=fmaxf(bonus,.6f*worth[officer.rank]);
    }
    return bonus;
}
/* Reject a publicly provable loss next turn if another move avoids it.
   Unknown root combats get their BEST outcome, so this never claims that an
   uncertain attack necessarily loses. No hidden identity enters this check. */
static bool immediate_defeat(const Game *view,Move m){
    Game next=optimistic_move(view,m);int side=view->turn,enemy=1-side;
    Piece target=view->board[m.to];
    if(target.side==enemy&&target.revealed&&target.rank==FLAG)return false;
    if(target.side==enemy&&!target.revealed&&!target.moved)return false; /* May capture the hidden flag NOW. */
    if(!game_moves(&next,side,NULL)&&game_moves(&next,enemy,NULL))return true;
    for(int from=0;from<100;from++){
        Piece a=next.board[from];if(a.side!=enemy||!a.revealed||!movable(a))continue;
        for(int to=0;to<100;to++){
            Piece d=next.board[to];if(d.side!=side||combat_result(a.rank,d.rank)<0||!public_legal(&next,(Move){from,to},enemy))continue;
            if(d.rank==FLAG)return true;
            Game reply=next;reply.board[from]=empty_piece();
            reply.board[to]=combat_result(a.rank,d.rank)>0?a:empty_piece();
            if(!game_moves(&reply,side,NULL)&&game_moves(&reply,enemy,NULL))return true;
        }
    }
    return false;
}
static float last_mobile_risk(const Game *view,float p[100][12],Move m){
    int mobile=0;for(int s=0;s<100;s++)if(view->board[s].side==view->turn&&movable(view->board[s]))mobile++;
    if(mobile!=1)return false;
    Piece a=view->board[m.from],d=view->board[m.to];
    /* Preserve a chance to win immediately against an unidentified flag. */
    if(d.side>=0&&p[m.to][FLAG]>0)return false;
    int enemy_mobile=0;
    for(int r=SPY;r<=MARSHAL;r++)enemy_mobile+=army_counts[r]-view->captured[1-view->turn][r];
    if(d.side>=0&&enemy_mobile==1&&p[m.to][a.rank]>.999f)return 0; /* Rank inferred from casualties: forced draw. */
    if(d.side>=0&&d.revealed&&combat_result(a.rank,d.rank)==0){
        bool other_mobile=false;
        for(int s=0;s<100;s++)if(s!=m.to&&view->board[s].side==1-view->turn)
            for(int r=SPY;r<=MARSHAL;r++)if(p[s][r]>0)other_mobile=true;
        if(!other_mobile)return false; /* A proven mutual immobilisation is a draw. */
    }
    float attack_loss=0;
    if(d.side>=0)for(int r=1;r<12;r++)if(combat_result(a.rank,r)<=0)attack_loss+=p[m.to][r];
    Game next=optimistic_move(view,m);
    float reply_loss=0;
    for(int s=0;s<100;s++)if(next.board[s].side==1-view->turn){
        int saved=next.board[s].rank;float danger=0;
        for(int r=SPY;r<=MARSHAL;r++)if(p[s][r]>0&&combat_result(r,a.rank)>=0){
            if(enemy_mobile==1&&r==a.rank)continue; /* The opponent can only trade to a draw. */
            next.board[s].rank=r;
            if(public_legal(&next,(Move){s,m.to},1-view->turn))danger+=p[s][r];
        }
        next.board[s].rank=saved;
        /* An unidentified unit deliberately approaching the exposed marshal
           is stronger spy evidence than a remote unidentified blocker. */
        if(a.rank==MARSHAL&&!view->board[s].revealed&&view->combat==2&&view->last_move.to==s&&view->last_move.from>=0){
            int old=view->last_move.from;
            int before=abs(old%10-m.from%10)+abs(old/10-m.from/10);
            int after=abs(s%10-m.from%10)+abs(s/10-m.from/10);
            if(after<=2&&after<before)danger=fminf(1,danger*4);
        }
        reply_loss=fmaxf(reply_loss,danger);
    }
    return attack_loss+(1-attack_loss)*reply_loss;
}
static float known_unanswered_loss_at(const Game *next,int side,int victim){
    Piece target=next->board[victim];
    if(target.side!=side||!movable(target))return 0;
    for(int s=0;s<100;s++){
        Piece e=next->board[s];if(e.side!=1-side)continue;
        if(target.rank==SPY&&e.moved&&!e.revealed){
            Game probe=*next;probe.board[s].rank=SPY;
            /* Every mobile rank kills an attacked spy, including a mutual
               spy exchange. Knowing the exact attacking rank is unnecessary. */
            if(public_legal(&probe,(Move){s,victim},e.side))
                return 4*(next->captured[1-side][MARSHAL]<army_counts[MARSHAL]?worth[MARSHAL]:worth[SPY]);
        }
        if(!e.revealed||!movable(e))continue;
        int result=combat_result(e.rank,target.rank);
        bool key_spy_trade=target.rank==SPY&&result==0&&next->captured[1-side][MARSHAL]<army_counts[MARSHAL];
        if(result<=0&&!key_spy_trade)continue;
        if(!public_legal(next,(Move){s,victim},e.side))continue;
        if(key_spy_trade)return 4*worth[MARSHAL];
        /* Recapturing the spy does not refund the marshal it just killed. */
        if(target.rank==MARSHAL&&e.rank==SPY)return 4*(worth[MARSHAL]-worth[SPY]);
        Game reply=*next;reply.board[s]=empty_piece();reply.board[victim]=e;
        bool recapture=false;
        for(int t=0;t<100;t++){Piece guard=reply.board[t];if(guard.side==side&&movable(guard)&&combat_result(guard.rank,e.rank)>=0&&public_legal(&reply,(Move){t,victim},side)){recapture=true;break;}}
        if(!recapture){
            float value=worth[target.rank];
            if(target.rank==SPY&&next->captured[1-side][MARSHAL]<army_counts[MARSHAL])value=worth[MARSHAL];
            return value*4;
        }
    }
    return 0;
}
static float known_unanswered_loss(const Game *view,Move m){
    Game next=*view;Piece a=next.board[m.from],d=next.board[m.to];next.board[m.from]=empty_piece();
    if(d.side<0||(d.revealed&&combat_result(a.rank,d.rank)>0))next.board[m.to]=a;
    else if(d.revealed&&combat_result(a.rank,d.rank)==0)next.board[m.to]=empty_piece();
    /* A known captured marshal no longer justifies preserving our spy as
       an anti-marshal reserve. Unknown combat must not assume this success. */
    if(d.side>=0&&d.revealed&&combat_result(a.rank,d.rank)>=0)next.captured[d.side][d.rank]++;
    float loss=0;
    int flag=-1,mobile=0;
    for(int s=0;s<100;s++)if(view->board[s].side==a.side){
        if(view->board[s].rank==FLAG)flag=s;
        if(movable(view->board[s]))mobile++;
    }
    for(int victim=0;victim<100;victim++){
        Piece target=next.board[victim];
        /* An unrelated move must not abandon our last routes through bombs.
           This uses public inventory, not the identities of hidden enemies. */
        bool scarce_miner=target.side==a.side&&target.rank==MINER&&
            army_counts[MINER]-view->captured[a.side][MINER]<=2&&
            view->captured[1-a.side][BOMB]<army_counts[BOMB];
        bool active_spy=target.rank==SPY&&next.captured[1-a.side][MARSHAL]<army_counts[MARSHAL];
        bool local_guard=mobile<=3&&flag>=0&&abs(victim%10-flag%10)+abs(victim/10-flag/10)<=3;
        bool raid_victim=false;
        int raider=view->last_move.to;
        if(view->combat==1&&raider>=0&&raider<100&&target.side==a.side){
            Piece enemy=next.board[raider];
            raid_victim=enemy.side==1-a.side&&enemy.revealed&&enemy.rank>=SERGEANT&&enemy.rank<=MARSHAL&&
                combat_result(enemy.rank,target.rank)>0&&public_legal(&next,(Move){raider,victim},enemy.side);
        }
        float weight=victim==m.to||target.rank>=COLONEL||scarce_miner||active_spy||local_guard||raid_victim?1:.25f;
        loss=fmaxf(loss,weight*known_unanswered_loss_at(&next,a.side,victim));
    }
    return loss;
}
static float public_defense_relief(const Game *view,float p[100][12],Move m,float before){
    Game next=optimistic_move(view,m);next.turn=1-view->turn;
    float relief=before-ai_flag_risk(&next,view->turn);
    Piece target=view->board[m.to];float success=0;
    if(target.side>=0)for(int r=0;r<12;r++)
        if(combat_result(view->board[m.from].rank,r)>0)success+=p[m.to][r];
    /* A guard that can be removed immediately without a recapture is not
       reliable new flag coverage. Keep negative relief and all tactical
       capture scores: this removes a fictitious bonus, not legal sacrifices. */
    if((target.side>=0&&success<.999f)||known_unanswered_loss_at(&next,view->turn,m.to)>0)
        relief=fminf(0,relief);
    return relief;
}
/* Stage the spy two steps from an identified marshal, never beside a piece
   that can simply take it. Unknown identities provide no target information. */
static bool spy_square_unsafe(const Game *view,int square,int side){
    for(int e=0;e<100;e++){
        Piece enemy=view->board[e];if(enemy.side!=1-side)continue;
        if(!enemy.revealed&&!enemy.moved)continue;
        Game probe=*view;if(!enemy.revealed)probe.board[e].rank=SPY;
        if(public_legal(&probe,(Move){e,square},1-side))return true;
    }
    return false;
}
static float spy_mission(const Game *view){
    int spy=-1,marshal=-1,side=view->turn;
    for(int s=0;s<100;s++){
        Piece p=view->board[s];
        if(p.side==side&&p.rank==SPY)spy=s;
        if(p.side==1-side&&p.revealed&&p.rank==MARSHAL)marshal=s;
    }
    if(spy<0||marshal<0)return 100;
    float dist[100];bool done[100]={false};for(int s=0;s<100;s++)dist[s]=100;dist[spy]=0;
    for(int iteration=0;iteration<100;iteration++){
        int s=-1;for(int t=0;t<100;t++)if(!done[t]&&(s<0||dist[t]<dist[s]))s=t;
        if(s<0||dist[s]>=100)break;
        done[s]=true;
        if(abs(s%10-marshal%10)+abs(s/10-marshal/10)==2)return dist[s];
        int nb[4],n=neighbors(s,nb);
        for(int k=0;k<n;k++){
            int t=nb[k];Piece p=view->board[t];
            if(is_lake(t)||p.side==1-side||(p.side==side&&!movable(p)))continue;
            Game probe=*view;probe.board[spy]=empty_piece();probe.board[t]=(Piece){SPY,side,-1,true,true};
            bool danger=spy_square_unsafe(&probe,t,side);
            if(danger)continue;
            /* A mobile ally can clear the corridor; bombs cannot. */
            float cost=p.side==side?3:1;
            if(dist[t]>dist[s]+cost)dist[t]=dist[s]+cost;
        }
    }
    return 100;
}
static float spy_hunt(const Game *view,Move m){
    if(view->board[m.to].side>=0)return 0;
    Game next=*view;next.board[m.to]=next.board[m.from];next.board[m.from]=empty_piece();
    if(next.board[m.to].rank==SPY&&spy_square_unsafe(&next,m.to,view->turn))return 0;
    float before=spy_mission(view),after=spy_mission(&next);
    if(after>=100)return 0;
    return fmaxf(0,fminf(12,6*(fminf(before,after+2)-after)));
}
static void defense_maps(const Game *view,float p[100][12],float defense[12][100]) {
    memset(defense,0,12*100*sizeof(float));int flag=-1;
    for(int s=0;s<100;s++)if(view->board[s].side==view->turn&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return;
    int home[100];land_distances(flag,home);
    for(int e=0;e<100;e++)if(view->board[e].side==1-view->turn&&home[e]<=6){
        float mobile=1-p[e][BOMB]-p[e][FLAG];
        float urgency=(7-home[e])*mobile*(1+2*p[e][MINER]);
        int distance[100];land_distances(e,distance);
        for(int r=1;r<=10;r++){
            float can_stop=0;for(int enemy=1;enemy<=10;enemy++)if(combat_result(r,enemy)>=0)can_stop+=p[e][enemy];
            for(int s=0;s<100;s++)defense[r][s]+=urgency*can_stop*.8f*fmaxf(0,10-distance[s]);
        }
    }
}
/* Once stronger ranks are gone, even a sergeant or major can lead the hunt.
   Pursue identified mobile targets instead of probing stationary blockers. */
static float dominant_hunt(const Game *view,Move m){
    Piece a=view->board[m.from],d=view->board[m.to];int side=view->turn;
    if(a.rank<SERGEANT||a.rank>MARSHAL)return 0;
    if(a.rank==MARSHAL&&view->captured[1-side][SPY]<army_counts[SPY])return 0;
    for(int r=a.rank;r<=MARSHAL;r++)if(army_counts[r]>view->captured[1-side][r])return 0;
    if(d.side>=0&&!d.moved&&(!d.revealed||!movable(d)))return 0;
    int dist[100],queue[100],head=0,tail=0;
    for(int s=0;s<100;s++)dist[s]=100;
    for(int s=0;s<100;s++){
        Piece p=view->board[s];
        if(p.side==1-side&&((!p.revealed&&p.moved)||(p.revealed&&movable(p)))){dist[s]=0;queue[tail++]=s;}
    }
    while(head<tail){
        int s=queue[head++],nb[4],n=neighbors(s,nb);
        for(int k=0;k<n;k++){
            int t=nb[k];if(is_lake(t)||dist[t]!=100||(view->board[t].side>=0&&t!=m.from))continue;
            dist[t]=dist[s]+1;queue[tail++]=t;
        }
    }
    if(dist[m.from]>=100||dist[m.to]>=100)return 0;
    return 3.0f*(dist[m.from]-dist[m.to]);
}
/* With at most two weaker mobile enemies left, take a certainly mobile
   target when doing so neither leaves nor creates a detected flag emergency.
   This avoids sampled stationary identities postponing a free liquidation. */
static Move cleanup_capture(const Game *view,const Move *moves,int n){
    int side=view->turn,mobile=0;
    for(int r=SPY;r<=MARSHAL;r++)mobile+=army_counts[r]-view->captured[1-side][r];
    if(mobile<1||mobile>2||ai_flag_risk(view,side)>0)return (Move){-1,-1};
    Move best={-1,-1};int best_rank=-1;
    for(int i=0;i<n;i++){
        Move m=moves[i];Piece a=view->board[m.from],d=view->board[m.to];
        if(a.rank<SERGEANT||a.rank>=COLONEL||d.side!=1-side||(!d.moved&&(!d.revealed||!movable(d))))continue;
        bool dominant=true;for(int r=a.rank;r<=MARSHAL;r++)if(army_counts[r]>view->captured[1-side][r])dominant=false;
        if(!dominant)continue;
        Game next=*view;next.board[m.from]=empty_piece();next.board[m.to]=a;next.turn=1-side;
        if(ai_flag_risk(&next,side)>0)continue;
        int rank=d.revealed?d.rank:0;
        if(rank>best_rank){best=m;best_rank=rank;}
    }
    return best;
}
/* Remove the possible miner BEFORE its escort can use an opened bomb gate.
   A recaptured interceptor may be worth more than an intact army with a lost
   flag. This premium uses only public motion, rank odds and known escorts. */
static float gate_interception(const Game *view,float p[100][12],Move m){
    int side=view->turn,flag=-1;Piece a=view->board[m.from],d=view->board[m.to];
    if(d.side!=1-side||(!d.moved&&!d.revealed)||p[m.to][MINER]<=0)return 0;
    float win=0;for(int r=0;r<12;r++)if(combat_result(a.rank,r)>0)win+=p[m.to][r];
    if(win<.999f)return 0; /* Do not justify a blind losing probe. */
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return 0;
    int gates[4],ng=neighbors(flag,gates);
    for(int j=0;j<ng;j++)if(view->board[gates[j]].side!=side||view->board[gates[j]].rank!=BOMB)return 0;
    int distance[100];land_distances(flag,distance);if(distance[m.to]>4)return 0;
    Game next=*view;next.board[m.from]=empty_piece();next.board[m.to]=a;
    bool recaptured=false;
    for(int e=0;e<100;e++){
        Piece escort=next.board[e];
        if(escort.side==1-side&&escort.revealed&&escort.rank>=COLONEL&&escort.rank<=MARSHAL&&
           combat_result(escort.rank,a.rank)>0&&game_legal(&next,(Move){e,m.to},1-side))recaptured=true;
    }
    return recaptured?480.0f*p[m.to][MINER]:0;
}
/* Assign the sole superior officer to a known raider near our flag. This
   mission is independent of bomb enclosure and of the enemy flag's location. */
static float recall_officer(const Game *view,Move m){
    Piece a=view->board[m.from];int side=view->turn,flag=-1;
    if(a.rank<COLONEL||a.rank>MARSHAL)return 0;
    Piece target=view->board[m.to];
    if(target.side>=0&&(!target.revealed||combat_result(a.rank,target.rank)<=0))return 0;
    Game next=optimistic_move(view,m);
    for(int e=0;e<100;e++){
        Piece enemy=next.board[e];
        if(enemy.side==1-side&&enemy.revealed&&movable(enemy)&&combat_result(enemy.rank,a.rank)>=0&&
           public_legal(&next,(Move){e,m.to},enemy.side))return 0;
    }
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return 0;
    float best=0;
    int home[100];land_distances(flag,home);
    for(int e=0;e<100;e++){
        Piece enemy=view->board[e];
        if(enemy.side!=1-side||!enemy.revealed||enemy.rank<MAJOR||enemy.rank>MARSHAL||a.rank<=enemy.rank||home[e]>8)continue;
        /* A distant recall must not consume the tempo needed by local
           defenders to stop an escorted miner already at the flag. */
        if(home[e]<=4&&abs(m.from%10-e%10)+abs(m.from/10-e/10)>4){
            bool urgent_miner=false;
            for(int t=0;t<100;t++){
                Piece p=view->board[t];
                if(t!=e&&p.side==1-side&&home[t]<=4&&
                   ((!p.revealed&&p.moved)||(p.revealed&&p.rank==MINER))){
                    for(int g=0;g<100;g++){
                        Piece guard=view->board[g];
                        if(g!=m.from&&guard.side==side&&movable(guard)&&guard.rank>=MINER&&
                           abs(g%10-t%10)+abs(g/10-t/10)<=2)urgent_miner=true;
                    }
                }
            }
            if(urgent_miner)continue;
        }
        bool other_guard=false;
        for(int s=0;s<100;s++)if(s!=m.from&&view->board[s].side==side&&view->board[s].rank>enemy.rank&&view->board[s].rank<=MARSHAL)other_guard=true;
        if(other_guard)continue;
        float dist[100];bool done[100]={false};for(int s=0;s<100;s++)dist[s]=100;dist[e]=0;
        for(int step=0;step<100;step++){
            int s=-1;for(int t=0;t<100;t++)if(!done[t]&&(s<0||dist[t]<dist[s]))s=t;
            if(s<0||dist[s]>=100)break;
            done[s]=true;int nb[4],n=neighbors(s,nb);
            for(int k=0;k<n;k++){
                int t=nb[k];Piece p=view->board[t];
                if(is_lake(t)||(p.side==side&&!movable(p)))continue;
                if(p.side==1-side&&t!=e&&(!p.revealed||combat_result(a.rank,p.rank)<=0))continue;
                float cost=p.side==side&&t!=m.from?3:p.side==1-side?2:1;
                if(dist[t]>dist[s]+cost)dist[t]=dist[s]+cost;
            }
        }
        /* Remote redeployment needs a stronger incentive than local routing:
           nearby officers already have clearance and tactical plans. */
        int travel=abs(m.from%10-e%10)+abs(m.from/10-e/10);
        float priority=travel>=8?160.0f:48.0f;
        if(dist[m.from]<100&&dist[m.to]<dist[m.from])best=fmaxf(best,priority*(dist[m.from]-dist[m.to]));
    }
    return best;
}
static float escorted_defense(const Game *view,Move m){
    int flag=-1,side=view->turn;Piece a=view->board[m.from];
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0||a.rank<MINER)return 0;
    Game next=*view;next.board[m.from]=empty_piece();next.board[m.to]=a;
    for(int e=0;e<100;e++)if(next.board[e].side==1-side&&next.board[e].revealed&&movable(next.board[e])&&combat_result(next.board[e].rank,a.rank)>0&&game_legal(&next,(Move){e,m.to},1-side))return 0;
    float best=0;
    for(int e=0;e<100;e++){
        Piece invader=view->board[e];
        if(invader.side!=1-side||(!invader.moved&&!invader.revealed)||(invader.revealed&&invader.rank!=MINER))continue;
        if(abs(e%10-flag%10)+abs(e/10-flag/10)>6)continue;
        bool escort=false;
        for(int t=0;t<100;t++)if(view->board[t].side==1-side&&view->board[t].revealed&&view->board[t].rank>=COLONEL&&view->board[t].rank<=MARSHAL&&abs(t%10-e%10)+abs(t/10-e/10)<=2)escort=true;
        if(!escort)continue;
        int dist[100];land_distances(e,dist);
        if(dist[m.from]<=6&&dist[m.to]<dist[m.from])best=fmaxf(best,32.0f*(dist[m.from]-dist[m.to]));
    }
    return best;
}
#include "ai_intercept.h"
#include "ai_assault.h"
#include "ai_breach.h"
#include "ai_flagrace.h"
#include "ai_guardtempo.h"
#include "ai_guardrelief.h"
#include "ai_officerteam.h"
#include "ai_raiders.h"
#include "ai_continuity.h"
#include "ai_marshalguard.h"
#include "ai_armycare.h"
#include "ai_pincer.h"
#include "ai_spyteam.h"
typedef struct {
    const Game *worlds;const Candidate *moves;int side,depth,budget;
    bool flag_known;float downside;
    float base[SAMPLES],risk[SAMPLES],result[ROOT_WIDTH],plan[ROOT_WIDTH];
    float levels[ROOT_WIDTH][SAMPLES][9];
    int completed[ROOT_WIDTH][SAMPLES];
} RootSearch;
static void evaluate_branch(int i,void *context) {
    RootSearch *work=context;
    SearchEntry *table=calloc(TT_SIZE,sizeof(SearchEntry));
    for(int j=0;j<SAMPLES;j++) {
        Game child=work->worlds[j];game_apply(&child,work->moves[i].move);
        if(table)memset(table,0,TT_SIZE*sizeof(SearchEntry));
        SearchContext ctx={.table=table,.budget=work->budget};
        float offset=work->risk[j]-ai_flag_risk(&child,work->side)-work->base[j];
        deepen_search(&child,work->depth,work->side,&ctx,work->flag_known,work->levels[i][j]);
        work->completed[i][j]=ctx.completed;
        for(int d=0;d<=ctx.completed;d=d<2?d+1:d+2)work->levels[i][j][d]+=offset;
    }
    free(table);
}
static void finish_search(RootSearch *work,int count){
    int common[SAMPLES];
    for(int j=0;j<SAMPLES;j++){
        common[j]=work->depth;
        for(int i=0;i<count;i++)if(work->completed[i][j]<common[j])common[j]=work->completed[i][j];
    }
    if(getenv("STRATEGO_AI_TRACE_DEPTH")){
        int low=work->depth,high=0,total=0;
        for(int j=0;j<SAMPLES;j++){if(common[j]<low)low=common[j];if(common[j]>high)high=common[j];total+=common[j];}
        fprintf(stderr,"Completed depth after candidate: %d..%d, mean %.2f (target %d, budget %d)\n",low,high,(float)total/SAMPLES,work->depth,work->budget);
    }
    for(int i=0;i<count;i++){
        float total=0,bad=0,losses=0,values[SAMPLES];
        for(int j=0;j<SAMPLES;j++){
            float v=work->levels[i][j][common[j]];
            values[j]=work->flag_known?v:fminf(200,v);total+=values[j];if(v<-1000)losses++;
        }
        if(work->depth==2){work->result[i]=total/SAMPLES+work->plan[i]-losses*50;continue;}
        for(int a=0;a<SAMPLES;a++)for(int b=a+1;b<SAMPLES;b++)if(values[b]<values[a]){float t=values[a];values[a]=values[b];values[b]=t;}
        for(int j=0;j<SAMPLES/4;j++)bad+=values[j];
        float caution=work->downside>0?work->downside:.2f;
        work->result[i]=(1-caution)*total/SAMPLES+caution*bad/(SAMPLES/4)+work->plan[i];
    }
}
Move ai_choose(const Game *g,int difficulty,uint32_t *rng) {
    if(g->winner>=0)return (Move){-1,-1};
    if(difficulty==2)return ai_learned(g,rng);
    if(!difficulty)return ai_basic(g,0,rng);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);if(!n)return (Move){-1,-1};
    Game view;int remaining[12];public_board(g,&view,remaining);
    for(int i=0;i<n;i++){Piece d=view.board[moves[i].to];if(d.side==1-g->turn&&d.revealed&&d.rank==FLAG)return moves[i];}
    float p[100][12],routes[12][100],defense[12][100];probabilities(&view,remaining,p);
    bool doomed[MAX_MOVES];int alternatives=0;
    for(int i=0;i<n;i++){doomed[i]=immediate_defeat(&view,moves[i]);if(!doomed[i])alternatives++;}
    if(alternatives){int kept=0;for(int i=0;i<n;i++)if(!doomed[i])moves[kept++]=moves[i];n=kept;}
    float terminal_risk[MAX_MOVES],least_risk=2;
    for(int i=0;i<n;i++){terminal_risk[i]=last_mobile_risk(&view,p,moves[i])+guard_tempo_risk(&view,p,moves[i]);least_risk=fminf(least_risk,terminal_risk[i]);}
    {int kept=0;for(int i=0;i<n;i++)if(terminal_risk[i]<=least_risk+.00001f)moves[kept++]=moves[i];n=kept;}
    /* Keep the terminal filters authoritative. Within equally viable moves,
       do not walk back into an avoided suspect when a quiet, materially safe
       alternative exists. This does not label that suspect as a known spy. */
    bool safe_wait=false;
    for(int i=0;i<n;i++)if(view.board[moves[i].to].side<0&&!marshal_returns_to_suspect(&view,p,moves[i])&&
        !marshal_known_spy_exposure(&view,moves[i])&&known_unanswered_loss(&view,moves[i])==0){safe_wait=true;break;}
    if(safe_wait){int kept=0;for(int i=0;i<n;i++)if(!marshal_returns_to_suspect(&view,p,moves[i]))moves[kept++]=moves[i];n=kept;}
    Move spy_capture=dangerous_spy_capture(&view,p,moves,n);if(spy_capture.from>=0)return spy_capture;
    Move cleanup=cleanup_capture(&view,moves,n);if(cleanup.from>=0)return cleanup;
    Move certain=continuity_capture(&view,p,moves,n);if(certain.from>=0)return certain;
    InterceptPlan intercept;intercept_plan(&view,p,&intercept);
    GuardRelief relief=guard_relief_plan(&view,&intercept);
    RaiderPlan raiders;raider_plan(&view,&raiders);
    AssaultPlan assault;assault_plan(&view,p,&assault);
    BreachPlan breach=breach_plan(&view,p);
    ContinuityPlan miner_mission=last_miner_plan(&view,p);
    ContinuityPlan support=reserve_support_plan(&view,p);
    /* A two-turn flag probe with a budgeted complete loss is an explicit
       conversion policy. Sampled hidden layouts otherwise drown this short
       winning window in speculative distant flag losses. The terminal-risk
       filters above retain priority over the mission. */
    for(int i=0;i<n;i++)if(breach_step(&breach,moves[i]))return moves[i];
    /* Finish a safe short reinforcement before buying another speculative
       search line. Candidate filtering still rules out immediate/forced loss. */
    for(int i=0;i<n;i++)if(miner_mission.step.from<0&&support.step.from==moves[i].from&&support.step.to==moves[i].to){
        return moves[i];
    }
    defense_maps(&view,p,defense);
    float preservation=ai_preservation_risk(&view,g->turn);
    float public_flag_pressure=ai_flag_risk(&view,g->turn);
    float approaching=approaching_officer_risk(&view,p);
    float exposure=army_exposure(&view,view.turn);
    for(int r=1;r<=10;r++)route_map(&view,p,r,routes[r]);
    Candidate candidates[ROOT_WIDTH];int count=0;float strategic[MAX_MOVES];
    StrategyHint hints[MAX_STRATEGY_HINTS];int hint_count=ai_strategy_hints(g,hints);
    float opportunity=0;
    for(int i=0;i<n;i++) {
        Move m=moves[i];Piece a=view.board[m.from],d=view.board[m.to];
        if(d.side>=0&&d.revealed&&combat_result(a.rank,d.rank)>0) {
            float net=exchange(a.rank,d.rank)-expected_threat(&view,p,m,true);
            if(net>opportunity)opportunity=net;
        }
    }
    bool reckless[MAX_MOVES];int safe_count=0,cheapest=MARSHAL;
    for(int i=0;i<n;i++){int rank=view.board[moves[i].from].rank;if(rank!=SPY&&rank<cheapest)cheapest=rank;}
    for(int i=0;i<n;i++){
        Piece a=view.board[moves[i].from],d=view.board[moves[i].to];float bomb=p[moves[i].to][BOMB];
        bool known_loss=d.side>=0&&d.revealed&&combat_result(a.rank,d.rank)<0;
        bool general_probe=a.rank==GENERAL&&a.revealed&&d.side>=0&&!d.revealed&&remaining[MARSHAL]>0;
        reckless[i]=known_loss||general_probe||(d.side>=0&&a.rank!=MINER&&
            (bomb>.999f||(a.rank>=CAPTAIN&&a.rank>cheapest&&bomb>(a.rank>=GENERAL?.04f:.16f))));
        if(!reckless[i])safe_count++;
    }
    for(int i=0;i<n;i++) {
        Move m=moves[i];Piece a=view.board[m.from],d=view.board[m.to];
        if(d.side>=0&&d.revealed&&d.rank==FLAG)return m;
        if(reckless[i]&&safe_count){strategic[i]=-1e6f;continue;}
        float gain=0;if(d.side>=0)for(int r=0;r<12;r++)gain+=p[m.to][r]*exchange(a.rank,r);
        float route=(routes[a.rank][m.from]-routes[a.rank][m.to])*.8f;
        if(fabsf(route)>30)route=0;
        strategic[i]=route-repetition_penalty(g,m)*4.0f;
        strategic[i]+=.25f*force_trade(&view,p,m);
        if(preservation<1&&approaching<1&&public_flag_pressure<1)
            strategic[i]+=recon_progress(&view,p,m,routes[SCOUT]);
        /* The root ordering used this danger only to shortlist moves. Keep
           the public threat in the final decision too: sampled continuations
           must not erase a known hanging piece for a speculative plan. */
        strategic[i]+=.85f*(expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true));
        strategic[i]-=known_unanswered_loss(&view,m);
        strategic[i]+=collective_retreat(&view,m,exposure);
        strategic[i]-=supported_capture_cost(&view,p,m);
        strategic[i]-=officer_trap_cost(&view,m);
        strategic[i]-=counter_capture_cost(&view,p,m);
        strategic[i]+=general_escort_bonus(&view,m);
        strategic[i]+=officer_clearance(&view,p,m);
        /* Do not use a valuable officer as a probe while a stronger rank is
           still unaccounted for. This uses remaining ranks, never identities. */
        if(a.rank>=COLONEL&&d.side>=0&&!d.revealed){
            float defeat=0;for(int r=0;r<12;r++)if(combat_result(a.rank,r)<0)defeat+=p[m.to][r];
            strategic[i]-=defeat*worth[a.rank]*4;
        }
        strategic[i]+=ai_coordination_bonus(&view,m);
        strategic[i]+=dominant_hunt(&view,m);
        strategic[i]+=pincer_bonus(&view,m);
        strategic[i]+=spy_clearance_bonus(&view,m)+spy_ambush_bonus(&view,m);
        strategic[i]-=spy_attack_cost(&view,p,m);
        /* Opening a long spy route must not postpone an officer's rescue. */
        if(a.rank==SPY||preservation<1)strategic[i]+=spy_hunt(&view,m);
        strategic[i]+=1.25f*escorted_defense(&view,m);
        /* A defensive mission must not subsidize a new marshal/spy ambush. */
        if(spy_exposure(&view,p,m,true)<=spy_exposure(&view,p,m,false))
            strategic[i]+=recall_officer(&view,m);
        strategic[i]+=1.25f*intercept_bonus(&view,p,&intercept,m);
        strategic[i]+=assault_bonus(&view,p,&assault,m);
        strategic[i]+=continuity_bonus(&miner_mission,m)+continuity_bonus(&support,m);
        strategic[i]-=continuity_guard_cost(&view,p,&support,m);
        if(miner_mission.step.from==m.from&&d.side==1-a.side&&m.to!=miner_mission.step.to){
            float failure=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(a.rank,r)<=0)failure+=p[m.to][r];
            if(p[m.to][FLAG]<p[miner_mission.goal][FLAG]*.5f)strategic[i]-=160*failure;
        }
        strategic[i]-=flag_race_cost(&view,p,m);
        strategic[i]-=last_guard_trade(&view,p,m);
        strategic[i]+=gate_interception(&view,p,m);
        if(a.rank==SPY&&d.side>=0&&d.revealed&&d.rank==MARSHAL)strategic[i]+=worth[MARSHAL];
        /* A reachable hidden flag is a chance to END the game now, not just
           another low-value observation. Pay analytically at the root, so
           rare sampled flag assignments cannot make the attacker wait.
           No equivalent bonus is given to hypothetical flags deeper in search. */
        if(a.rank==MINER&&d.side==1-g->turn&&!d.moved&&!d.revealed){
            float urgency=80;
            /* A known pursuer within two steps can close this window
               after a waiting move. Cash in the flag chance now. */
            for(int e=0;e<100;e++){
                Piece enemy=view.board[e];
                if(enemy.side!=1-a.side||!enemy.revealed||!movable(enemy)||combat_result(enemy.rank,a.rank)<0)continue;
                int distance=abs(e%10-m.from%10)+abs(e/10-m.from/10);
                bool reaches=distance==1;
                if(distance==2){
                    int steps[4],ns=neighbors(e,steps);
                    for(int j=0;j<ns;j++)if(!is_lake(steps[j])&&view.board[steps[j]].side<0&&
                        abs(steps[j]%10-m.from%10)+abs(steps[j]/10-m.from/10)==1)reaches=true;
                }
                if(reaches){urgency=320;break;}
            }
            strategic[i]+=p[m.to][FLAG]*urgency;
            /* Opening a bomb gate next to a plausible flag has value even
               before its identity is known. Only miners can exploit it. */
            int nb[4],nn=neighbors(m.to,nb);float behind=0;
            for(int k=0;k<nn;k++)behind+=p[nb[k]][FLAG];
            strategic[i]+=12.0f*p[m.to][BOMB]*fminf(1,behind);
        }
        /* Preserve the entire army: moving an unrelated scout must not erase
           a known pursuit or let the last local flag guard wander away. */
        Game planned=view;planned.board[m.to]=a;planned.board[m.from]=empty_piece();
        float safety=preservation-ai_preservation_risk(&planned,g->turn);
        if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))safety=fminf(0,safety);
        strategic[i]+=safety;
        Game cautious=optimistic_move(&view,m);
        if(public_flag_pressure>0){
            strategic[i]+=.5f*public_defense_relief(&view,p,m,public_flag_pressure);
        }
        if(d.side>=0&&cautious.board[m.to].side==g->turn)cautious.board[m.to].revealed=true;
        /* Pursuit intent belongs to the actual officer position, not a
           hypothetical retreat square that the enemy never approached. */
        float approach_safety=approaching-approaching_officer_risk_from(&cautious,&view,p);
        /* An uncertain capture cannot be credited as a successful rescue. */
        if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))approach_safety=fminf(0,approach_safety);
        strategic[i]+=approach_safety;
        strategic[i]+=raider_bonus(&view,&raiders,m);
        strategic[i]+=reserve_home_bonus(&view,p,m);
        strategic[i]+=guard_relief_bonus(&relief,m);
        strategic[i]-=stale_pursuit_cost(&view,m,public_flag_pressure);
        if(a.rank==MINER&&army_counts[MINER]-g->captured[g->turn][MINER]<=2&&army_counts[BOMB]>g->captured[1-g->turn][BOMB]){
            float risk=expected_threat(&view,p,m,true)-expected_threat(&view,p,m,false);
            strategic[i]-=fmaxf(0,risk)*.65f;
        }
        /* Activate a surviving dominant officer instead of endlessly sending
           weak units down the same corridor while the strongest stays home. */
        if(a.rank>=COLONEL){
            int stronger=0;for(int r=a.rank;r<=MARSHAL;r++)stronger+=army_counts[r]-g->captured[1-g->turn][r];
            if(stronger==0){
                strategic[i]+=route*2.5f;
                int advance=(m.to/10-m.from/10)*(g->turn==COMPUTER?1:-1);
                strategic[i]+=.5f*advance;
            }
        }
        strategic[i]+=spy_exposure(&view,p,m,false)-spy_exposure(&view,p,m,true);
        for(int h=0;h<hint_count;h++)if(hints[h].move.from==m.from&&hints[h].move.to==m.to)strategic[i]+=hints[h].bonus;
        float defend=defense[a.rank][m.to]-defense[a.rank][m.from];
        strategic[i]+=fmaxf(-15,fminf(15,defend));
        /* Cash in reliable tactical opportunities before buying more information. */
        strategic[i]+=information_gain(&view,p,m)/(1+opportunity/worth[SCOUT]);
        strategic[i]-=scout_disclosure_cost(&view,m);
        strategic[i]-=officer_disclosure_cost(&view,p,m);
        strategic[i]-=scarce_miner_probe(&view,p,m);
        strategic[i]-=miner_capability_cost(&view,p,m);
        /* Reward a sound immediate recapture, without paying for a known
           losing attack or overriding the search's assessment of defenders. */
        if(g->combat==1&&g->last_move.to==m.to&&d.side==1-g->turn&&gain>0)strategic[i]+=2.5f;
        float order=gain+strategic[i]+expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true);
        if(futile_raid_follow(&view,m)){
            strategic[i]=fminf(strategic[i],-known_unanswered_loss(&view,m));
            order=gain+strategic[i]+expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true);
        }
        insert(candidates,&count,ROOT_WIDTH,m,order);
    }
    Game worlds[SAMPLES];for(int j=0;j<SAMPLES;j++)sample_board(&view,remaining,&worlds[j],rng);
    Move best=moves[0];float best_score=-1e30f;
    Candidate finalists[6];int final_count=0;
    /* Use analytic immediate combat odds, rather than gambling on the small
       sample's accidental excess of weak enemies or absence of bombs. */
    for(int i=0;i<n;i++)if(view.board[moves[i].to].rank==-2){
        int a=view.board[moves[i].from].rank;float expected=0,sampled=0;
        for(int r=0;r<12;r++)expected+=p[moves[i].to][r]*(r==FLAG?35:exchange(a,r));
        for(int j=0;j<SAMPLES;j++){int r=worlds[j].board[moves[i].to].rank;sampled+=r==FLAG?35:exchange(a,r);}
        strategic[i]+=expected-sampled/SAMPLES;
    }
    RootSearch work={.worlds=worlds,.moves=candidates,.side=g->turn,.depth=2,.budget=600,.flag_known=remaining[FLAG]==0};
    work.downside=fmaxf(.2f,fminf(.35f,.2f+force_advantage(&view)*.5f));
    for(int j=0;j<SAMPLES;j++){
        work.base[j]=evaluate(&worlds[j],g->turn,work.flag_known);
        work.risk[j]=ai_flag_risk(&worlds[j],g->turn);
    }
    for(int i=0;i<count;i++)for(int k=0;k<n;k++)if(moves[k].from==candidates[i].move.from&&moves[k].to==candidates[i].move.to){work.plan[i]=strategic[k];break;}
    ai_parallel_for(count,evaluate_branch,&work);
    finish_search(&work,count);
    for(int i=0;i<count;i++) {
        Move m=candidates[i].move;float score=work.result[i];
        if(getenv("STRATEGO_AI_TRACE_ROOT"))fprintf(stderr,"Root %d -> %d : %.2f (plan %.2f)\n",m.from,m.to,score,work.plan[i]);
        score+=(float)(game_random(rng)%1000)*.00008f;
        if(score>best_score){best_score=score;best=m;}
        insert(finalists,&final_count,6,m,score);
    }
    /* Casualties give the exact number of surviving mobile ranks even when
       their squares are unknown. Hidden bombs must not hide a small ending. */
    int mobile_count=0;
    for(int side=0;side<2;side++)for(int r=SPY;r<=MARSHAL;r++)
        mobile_count+=army_counts[r]-view.captured[side][r];
    bool flag_crisis=false;int own_flag=-1;
    for(int s=0;s<100;s++)if(view.board[s].side==g->turn&&view.board[s].rank==FLAG)own_flag=s;
    if(own_flag>=0)for(int s=0;s<100;s++){
        Piece enemy=view.board[s];
        if(enemy.side!=1-g->turn||(!enemy.moved&&!enemy.revealed)||enemy.rank==BOMB||enemy.rank==FLAG)continue;
        if(abs(s%10-own_flag%10)+abs(s/10-own_flag/10)>4)continue;
        int distance[100],queue[100],head=0,tail=0;
        for(int t=0;t<100;t++)distance[t]=100;
        distance[s]=0;queue[tail++]=s;
        while(head<tail){
            int t=queue[head++];if(distance[t]>=4)continue;
            int nb[4],count=neighbors(t,nb);
            for(int k=0;k<count;k++){
                int next=nb[k];Piece blocker=view.board[next];
                if(is_lake(next)||distance[next]!=100||blocker.side==enemy.side)continue;
                if(blocker.side==g->turn&&blocker.rank==BOMB&&enemy.revealed&&enemy.rank!=MINER)continue;
                distance[next]=distance[t]+1;queue[tail++]=next;
            }
        }
        if(distance[own_flag]<=4)flag_crisis=true;
    }
    /* An escorted miner is a tactical finale even with many remote pieces
       still alive: budget by local danger as well as total material. */
    bool deepen=mobile_count<=10||flag_crisis;
    work.moves=finalists;work.depth=deepen?8:6;work.budget=deepen?6400:2400;
    for(int i=0;i<final_count;i++)for(int k=0;k<n;k++)if(moves[k].from==finalists[i].move.from&&moves[k].to==finalists[i].move.to){work.plan[i]=strategic[k];break;}
    ai_parallel_for(final_count,evaluate_branch,&work);
    finish_search(&work,final_count);
    best_score=-1e30f;
    for(int i=0;i<final_count;i++) {
        Move m=finalists[i].move;float score=work.result[i];
        if(getenv("STRATEGO_AI_TRACE"))fprintf(stderr,"AI %d -> %d : %.2f\n",m.from,m.to,score);
        if(score>best_score){best_score=score;best=m;}
    }
    return best;
}
