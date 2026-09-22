#include "game.h"
#include "ml.h"
#include <math.h>
#include <stdlib.h>
static const float value[12]={260,5,3,7,5,6,8,11,15,21,28,8};
/* Unknown enemy ranks are NEVER read here. Beliefs use public casualties,
   revealed pieces and whether an enemy has moved. No sampled hidden board. */
static void belief(const Game *g,Piece enemy,float p[12]) {
    for(int r=0;r<12;r++)p[r]=0;
    if(enemy.revealed) {p[enemy.rank]=1;return;}
    int remaining[12];
    for(int r=0;r<12;r++)remaining[r]=army_counts[r]-g->captured[enemy.side][r];
    for(int s=0;s<100;s++){Piece e=g->board[s];if(e.side==enemy.side&&e.revealed)remaining[e.rank]--;}
    float total=0;
    for(int r=0;r<12;r++) {p[r]=(enemy.moved&&(r==FLAG||r==BOMB))?0:(float)(remaining[r]>0?remaining[r]:0);total+=p[r];}
    if(total>0)for(int r=0;r<12;r++)p[r]/=total;
}
static float attack_value(int rank,const float p[12]) {
    float score=0;for(int r=0;r<12;r++){int c=combat_result(rank,r);score+=p[r]*(c>0?value[r]:(c<0?-value[rank]:value[r]-value[rank]));}return score;
}
static bool clear_line(const Game *g,int from,int to,int vacated) {
    int dx=to%10-from%10,dz=to/10-from/10;if(dx&&dz)return false;
    int step=dx?(dx>0?1:-1):(dz>0?10:-10);
    for(int s=from+step;s!=to;s+=step)if(is_lake(s)||(s!=vacated&&g->board[s].side>=0))return false;
    return true;
}
static Move policy(const Game *g,int difficulty,uint32_t *rng,Move *out_moves,float *out_scores,float out_features[][ML_FEATURES]) {
    if(g->winner>=0)return (Move){-1,-1};
    Move moves[MAX_MOVES];int count=game_moves(g,g->turn,moves);Move best={-1,-1};float best_score=-1e30f;
    float beliefs[100][12];for(int s=0;s<100;s++)if(g->board[s].side==1-g->turn)belief(g,g->board[s],beliefs[s]);
    for(int i=0;i<count;i++) {
        Move m=moves[i];Piece a=g->board[m.from],d=g->board[m.to];
        float noise=rng?(float)(game_random(rng)%10000)/10000.0f:0;
        float score=noise*(difficulty==0?6.0f:0.65f);
        int advance=(m.to/10-m.from/10)*(a.side==COMPUTER?1:-1);
        score+=advance*0.22f;
        score+=0.025f*(4.5f-fabsf(m.to%10-4.5f));
        if(g->last_id[a.side]==a.id&&g->last_from[a.side]==m.to)score-=1.3f;
        float gain=d.side==1-a.side?attack_value(a.rank,beliefs[m.to]):0;
        if(d.side==1-a.side)score+=gain+(!d.revealed?1.1f:0);
        /* Route toward the nearest enemy through actual land and open squares. */
        int dist[100],queue[100],head=0,tail=0;for(int s=0;s<100;s++)dist[s]=-1;
        queue[tail++]=m.to;dist[m.to]=0;int nearest=20;
        while(head<tail){int s=queue[head++];if(dist[s]>=nearest)continue;
            int nb[4]={s-10,s+10,s%10?s-1:-1,s%10<9?s+1:-1};
            for(int k=0;k<4;k++){int t=nb[k];if(t<0||t>=100||is_lake(t)||dist[t]>=0)continue;
                if(t!=m.from&&g->board[t].side==a.side)continue;
                dist[t]=dist[s]+1;if(g->board[t].side==1-a.side){nearest=dist[t];continue;}queue[tail++]=t;
            }
        }
        score-=0.16f*nearest;
        float risk=0,defense=0;
        if(difficulty>0){
            for(int e=0;e<100;e++)if(e!=m.to&&g->board[e].side==1-a.side){
                int distance=abs(e%10-m.to%10)+abs(e/10-m.to/10);
                if(!clear_line(g,e,m.to,m.from))continue;
                float threat=0;for(int r=1;r<=10;r++)if(distance==1||r==SCOUT){int c=combat_result(r,a.rank);if(c>=0)threat+=beliefs[e][r]*value[a.rank];}
                if(threat>risk)risk=threat;
            }
            score-=risk*0.85f;
            /* Preserve miners and spy until their special targets are useful. */
            if(a.rank==MINER&&d.side>=0&&!d.moved&&!d.revealed)score+=1.7f;
            for(int f=0;f<100;f++)if(g->board[f].side==a.side&&g->board[f].rank==FLAG){
                int before=abs(f%10-m.from%10)+abs(f/10-m.from/10),after=abs(f%10-m.to%10)+abs(f/10-m.to/10);
                for(int e=0;e<100;e++)if(g->board[e].side==1-a.side&&abs(e%10-f%10)+abs(e/10-f/10)<4)defense+=(before-after)*0.45f;
            }
            score+=defense;
        }
        if(out_moves){
            out_moves[i]=m;out_scores[i]=score;
            float *f=out_features[i];for(int k=0;k<ML_FEATURES;k++)f[k]=0;
            f[0]=fmaxf(-2,fminf(2,gain/10));f[1]=risk/30;
            f[2]=advance/9.0f;f[3]=nearest/20.0f;f[4]=fmaxf(-2,fminf(2,defense/5));
            f[5]=g->last_id[a.side]==a.id&&g->last_from[a.side]==m.to;
            f[6]=d.side>=0&&!d.revealed;f[7]=d.side>=0&&d.revealed;
            f[8]=a.rank==SCOUT&&d.side>=0&&!d.revealed;
            f[9]=a.rank==MINER&&d.side>=0&&!d.moved;
            f[10]=a.rank>=COLONEL?risk/30:0;
            f[11]=(4.5f-fabsf(m.to%10-4.5f))/4.5f;
            f[12+a.rank-1]=1; /* own rank only, ranks 1..10 */
            f[22]=a.rank==MINER?advance/9.0f:0;
            f[23]=a.rank==SCOUT?advance/9.0f:0;
        }
        if(score>best_score){best_score=score;best=m;}
    }
    return best;
}
Move ai_basic(const Game *g,int difficulty,uint32_t *rng) {return policy(g,difficulty,rng,NULL,NULL,NULL);}
int ai_policy_candidates(const Game *g,Move moves[MAX_MOVES],float scores[MAX_MOVES],float features[MAX_MOVES][ML_FEATURES]) {
    if(g->winner>=0)return 0;
    policy(g,1,NULL,moves,scores,features);return game_moves(g,g->turn,NULL);
}

