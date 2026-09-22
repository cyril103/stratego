#include "game.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Hidden ranks are erased before evaluating or sampling any search board. */
#define SAMPLES 6
#define ROOT_WIDTH 16
#define BEAM 6
#define WIN 10000.0f
static const float worth[12]={1000,7,4,12,7,9,12,17,24,35,50,9};
typedef struct {Move move;float score;} Candidate;
static int neighbors(int s,int n[4]) {
    int k=0;if(s%10)n[k++]=s-1;if(s%10<9)n[k++]=s+1;if(s>=10)n[k++]=s-10;if(s<90)n[k++]=s+10;return k;
}
static float exchange(int a,int d) {int c=combat_result(a,d);return c>0?worth[d]:(c<0?-worth[a]:worth[d]-worth[a]);}
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
    Piece p=view->board[s];if(p.moved&&(rank==FLAG||rank==BOMB))return 0;
    int back=p.side==COMPUTER?s/10:9-s/10;
    if(rank==FLAG) {
        float weight=back==0?5:(back==1?2:1);
        int nb[4],n=neighbors(s,nb);
        for(int i=0;i<n;i++)if(view->board[nb[i]].side==p.side&&view->board[nb[i]].revealed&&view->board[nb[i]].rank==BOMB)weight*=2;
        return weight;
    }
    if(rank==BOMB)return back<=1?2.0f:1.0f;
    return 1;
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
static float evaluate(const Game *g,int side) {
    if(g->winner>=0)return g->winner==side?WIN:-WIN;
    float score=0;int mobile[2]={0};
    for(int s=0;s<100;s++){Piece p=g->board[s];if(p.side<0)continue;
        float v=worth[p.rank];
        if(movable(p)) {
            mobile[p.side]++;
            int advance=p.side==COMPUTER?s/10:9-s/10;
            v+=advance*.12f;
            int nb[4],n=neighbors(s,nb),freedom=0;
            for(int k=0;k<n;k++)if(!is_lake(nb[k])&&g->board[nb[k]].side!=p.side)freedom++;
            v+=freedom*.16f;
        }
        score+=p.side==side?v:-v;
    }
    if(!mobile[side])return -WIN;
    if(!mobile[1-side])return WIN;
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
static float search(const Game *g,int depth,int side,float alpha,float beta) {
    if(g->winner>=0)return evaluate(g,side);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);
    if(!n)return g->turn==side?-WIN:WIN;
    if(depth==0) {
        /* Extend one profitable tactical reply at the horizon. */
        float base=evaluate(g,side),best=base;
        for(int i=0;i<n;i++)if(g->board[moves[i].to].side>=0&&exchange(g->board[moves[i].from].rank,g->board[moves[i].to].rank)>0){
            Game child=*g;game_apply(&child,moves[i]);float v=evaluate(&child,side);
            if(g->turn==side){if(v>best)best=v;}else if(v<best)best=v;
        }
        return best;
    }
    Candidate best_moves[BEAM];int count=0;
    for(int i=0;i<n;i++)insert(best_moves,&count,BEAM,moves[i],move_order(g,moves[i]));
    float best=g->turn==side?-1e20f:1e20f;
    for(int i=0;i<count;i++) {
        Game child=*g;game_apply(&child,best_moves[i].move);
        float v=search(&child,depth-1,side,alpha,beta);
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
            for(int r=0;r<12;r++){gain+=p[s][r]*exchange(rank,r);if(combat_result(rank,r)>0)win+=p[s][r];}
            if(gain>0||win>.55f)dist[s]=-fminf(10,gain*.35f)-win*2;
            else if(!view->board[s].revealed&&rank<=MINER)dist[s]=1;
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
Move ai_previous(const Game *g,int difficulty,uint32_t *rng) {
    if(!difficulty)return ai_basic(g,0,rng);
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);if(!n)return (Move){-1,-1};
    Game view;int remaining[12];public_board(g,&view,remaining);
    float p[100][12],routes[12][100];probabilities(&view,remaining,p);
    for(int r=1;r<=10;r++)route_map(&view,p,r,routes[r]);
    Candidate candidates[ROOT_WIDTH];int count=0;float strategic[MAX_MOVES];
    for(int i=0;i<n;i++) {
        Move m=moves[i];Piece a=view.board[m.from],d=view.board[m.to];
        if(d.side>=0&&d.revealed&&d.rank==FLAG)return m;
        float gain=0;if(d.side>=0)for(int r=0;r<12;r++)gain+=p[m.to][r]*exchange(a.rank,r);
        float route=(routes[a.rank][m.from]-routes[a.rank][m.to])*.8f;
        if(fabsf(route)>30)route=0;
        strategic[i]=route-repetition_penalty(g,m);
        if(d.side>=0&&!d.revealed)strategic[i]+=(a.rank==SCOUT?.8f:.25f);
        float order=gain+strategic[i]+expected_threat(&view,p,m,false)-expected_threat(&view,p,m,true);
        insert(candidates,&count,ROOT_WIDTH,m,order);
    }
    Game worlds[SAMPLES];for(int j=0;j<SAMPLES;j++)sample_board(&view,remaining,&worlds[j],rng);
    Move best=moves[0];float best_score=-1e30f;
    for(int i=0;i<count;i++) {
        Move m=candidates[i].move;float sum=0,losses=0,plan=0;
        for(int k=0;k<n;k++)if(moves[k].from==m.from&&moves[k].to==m.to){plan=strategic[k];break;}
        for(int j=0;j<SAMPLES;j++) {
            Game child=worlds[j];float base=evaluate(&child,g->turn);game_apply(&child,m);
            float result=search(&child,2,g->turn,-1e20f,1e20f)-base;
            sum+=result;if(result<-1000)losses++;
        }
        float score=sum/SAMPLES+plan-losses*50;
        score+=(float)(game_random(rng)%1000)*.00008f;
        if(score>best_score){best_score=score;best=m;}
    }
    return best;
}

