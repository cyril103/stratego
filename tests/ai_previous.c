/* Frozen Expert+ baseline before search improvements, 2026-09-08. */
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
    return entropy*rate;
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
    if(g->winner==GAME_DRAW)return 0; /* Terminal-rule compatibility only. */
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
        float v=p.rank==FLAG?0:worth[p.rank];
        if(movable(p)) {
            mobile[p.side]++;
            int stronger=0;for(int r=p.rank+1;r<=10;r++)stronger+=alive[1-p.side][r];
            if(p.rank>=SERGEANT)v+=12.0f/(1+stronger);
            if(p.rank==SPY)v=alive[1-p.side][MARSHAL]?13:3;
            if(p.rank==MINER)v=alive[1-p.side][BOMB]?16.0f+12.0f/(alive[p.side][MINER]+1):5;
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
static float search(const Game *g,int depth,int side,float alpha,float beta,int *budget,bool flag_known) {
    if((*budget)--<=0)return evaluate(g,side,flag_known);
    if(g->winner>=0)return evaluate(g,side,flag_known);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);
    if(!n)return g->turn==side?-WIN:WIN;
    if(depth==0)return tactical_search(g,3,side,alpha,beta,budget,flag_known);
    Candidate best_moves[BEAM];int count=0;
    for(int i=0;i<n;i++)insert(best_moves,&count,BEAM,moves[i],move_order(g,moves[i]));
    float best=g->turn==side?-1e20f:1e20f;
    for(int i=0;i<count;i++) {
        Game child=*g;game_apply(&child,best_moves[i].move);
        float v=search(&child,depth-1,side,alpha,beta,budget,flag_known);
        if(g->turn==side){if(v>best)best=v;if(best>alpha)alpha=best;}
        else {if(v<best)best=v;if(best<beta)beta=best;}
        if(beta<=alpha)break;
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
static void land_distances(int source,int distance[100]) {
    int queue[100],head=0,tail=0;for(int s=0;s<100;s++)distance[s]=100;
    distance[source]=0;queue[tail++]=source;
    while(head<tail){int s=queue[head++],nb[4],n=neighbors(s,nb);
        for(int k=0;k<n;k++){int t=nb[k];if(is_lake(t)||distance[t]!=100)continue;distance[t]=distance[s]+1;queue[tail++]=t;}
    }
}
static float known_unanswered_loss(const Game *view,Move m){
    if(view->board[m.to].side>=0)return 0;
    Game next=*view;Piece a=next.board[m.from];next.board[m.to]=a;next.board[m.from]=empty_piece();
    for(int s=0;s<100;s++){
        Piece e=next.board[s];if(e.side!=1-a.side||!e.revealed||!movable(e)||combat_result(e.rank,a.rank)<=0)continue;
        if(!game_legal(&next,(Move){s,m.to},e.side))continue;
        Game reply=next;reply.board[s]=empty_piece();reply.board[m.to]=e;
        bool recapture=false;
        for(int t=0;t<100;t++){Piece guard=reply.board[t];if(guard.side==a.side&&movable(guard)&&combat_result(guard.rank,e.rank)>=0&&game_legal(&reply,(Move){t,m.to},a.side)){recapture=true;break;}}
        if(!recapture)return worth[a.rank]*4;
    }
    return 0;
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
typedef struct {
    const Game *worlds;const Candidate *moves;int side,depth,budget;
    bool flag_known;
    float base[SAMPLES],risk[SAMPLES],result[ROOT_WIDTH],plan[ROOT_WIDTH];
} RootSearch;
static void evaluate_branch(int i,void *context) {
    RootSearch *work=context;float total=0,bad=0,losses=0,values[SAMPLES];
    for(int j=0;j<SAMPLES;j++) {
        Game child=work->worlds[j];game_apply(&child,work->moves[i].move);int budget=work->budget;
        float v=search(&child,work->depth,work->side,-1e20f,1e20f,&budget,work->flag_known)-work->base[j];
        v+=work->risk[j]-ai_flag_risk(&child,work->side);
        values[j]=work->flag_known?v:fminf(200,v);total+=values[j];if(v<-1000)losses++;
    }
    if(work->depth==2)work->result[i]=total/SAMPLES+work->plan[i]-losses*50;
    else {
        for(int a=0;a<SAMPLES;a++)for(int b=a+1;b<SAMPLES;b++)if(values[b]<values[a]){float t=values[a];values[a]=values[b];values[b]=t;}
        for(int j=0;j<SAMPLES/4;j++)bad+=values[j];
        work->result[i]=.8f*total/SAMPLES+.2f*bad/(SAMPLES/4)+work->plan[i];
    }
}
Move ai_previous(const Game *g,int difficulty,uint32_t *rng) {
    if(difficulty==2)return ai_learned(g,rng);
    if(!difficulty)return ai_basic(g,0,rng);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);if(!n)return (Move){-1,-1};
    Game view;int remaining[12];public_board(g,&view,remaining);
    float p[100][12],routes[12][100],defense[12][100];probabilities(&view,remaining,p);
    defense_maps(&view,p,defense);
    float preservation=ai_preservation_risk(&view,g->turn);
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
        reckless[i]=known_loss||(d.side>=0&&a.rank!=MINER&&
            (bomb>.999f||(a.rank>=MAJOR&&a.rank>cheapest&&bomb>(a.rank>=GENERAL?.04f:.16f))));
        if(!reckless[i])safe_count++;
    }
    for(int i=0;i<n;i++) {
        Move m=moves[i];Piece a=view.board[m.from],d=view.board[m.to];
        if(d.side>=0&&d.revealed&&d.rank==FLAG)return m;
        if(reckless[i]&&safe_count){strategic[i]=-1e6f;continue;}
        float gain=0;if(d.side>=0)for(int r=0;r<12;r++)gain+=p[m.to][r]*exchange(a.rank,r);
        float route=(routes[a.rank][m.from]-routes[a.rank][m.to])*.8f;
        if(fabsf(route)>30)route=0;
        strategic[i]=route-repetition_penalty(g,m)*1.5f;
        /* The root ordering used this danger only to shortlist moves. Keep
           the public threat in the final decision too: sampled continuations
           must not erase a known hanging piece for a speculative plan. */
        strategic[i]+=.85f*(expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true));
        strategic[i]-=known_unanswered_loss(&view,m);
        strategic[i]+=ai_coordination_bonus(&view,m);
        /* Preserve the entire army: moving an unrelated scout must not erase
           a known pursuit or let the last local flag guard wander away. */
        Game planned=view;planned.board[m.to]=a;planned.board[m.from]=empty_piece();
        float safety=preservation-ai_preservation_risk(&planned,g->turn);
        if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))safety=fminf(0,safety);
        strategic[i]+=safety;
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
        /* Reward a sound immediate recapture, without paying for a known
           losing attack or overriding the search's assessment of defenders. */
        if(g->combat==1&&g->last_move.to==m.to&&d.side==1-g->turn&&gain>0)strategic[i]+=2.5f;
        float order=gain+strategic[i]+expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true);
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
    RootSearch work={.worlds=worlds,.moves=candidates,.side=g->turn,.depth=2,.budget=220,.flag_known=remaining[FLAG]==0};
    for(int j=0;j<SAMPLES;j++){
        work.base[j]=evaluate(&worlds[j],g->turn,work.flag_known);
        work.risk[j]=ai_flag_risk(&worlds[j],g->turn);
    }
    for(int i=0;i<count;i++)for(int k=0;k<n;k++)if(moves[k].from==candidates[i].move.from&&moves[k].to==candidates[i].move.to){work.plan[i]=strategic[k];break;}
    ai_parallel_for(count,evaluate_branch,&work);
    for(int i=0;i<count;i++) {
        Move m=candidates[i].move;float score=work.result[i];
        if(getenv("STRATEGO_AI_TRACE_ROOT"))fprintf(stderr,"Root %d -> %d : %.2f (plan %.2f)\n",m.from,m.to,score,work.plan[i]);
        score+=(float)(game_random(rng)%1000)*.00008f;
        if(score>best_score){best_score=score;best=m;}
        insert(finalists,&final_count,6,m,score);
    }
    int mobile_count=0;for(int s=0;s<100;s++)if(view.board[s].side>=0&&(view.board[s].rank==-2||view.board[s].moved||movable(view.board[s])))mobile_count++;
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
    work.moves=finalists;work.depth=deepen?6:4;work.budget=deepen?3200:1200;
    for(int i=0;i<final_count;i++)for(int k=0;k<n;k++)if(moves[k].from==finalists[i].move.from&&moves[k].to==finalists[i].move.to){work.plan[i]=strategic[k];break;}
    ai_parallel_for(final_count,evaluate_branch,&work);
    best_score=-1e30f;
    for(int i=0;i<final_count;i++) {
        Move m=finalists[i].move;float score=work.result[i];
        if(getenv("STRATEGO_AI_TRACE"))fprintf(stderr,"AI %d -> %d : %.2f\n",m.from,m.to,score);
        if(score>best_score){best_score=score;best=m;}
    }
    return best;
}
