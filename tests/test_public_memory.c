#include "game.h"
#include "ai_belief.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game position(int rank,bool known){
    Game g;game_clear(&g);g.turn=HUMAN;
    g.board[60]=(Piece){rank,HUMAN,0,false,true};
    g.board[62]=(Piece){MARSHAL,COMPUTER,40,known,true};
    g.board[90]=(Piece){SCOUT,HUMAN,1,false,true};
    g.board[99]=(Piece){FLAG,HUMAN,2,false,false};
    g.board[9]=(Piece){FLAG,COMPUTER,41,false,false};
    return g;
}
int main(void){
    Game g=position(SPY,true),other=position(CAPTAIN,true),sim=g;
    CHECK(game_apply(&g,(Move){60,61}));
    CHECK(game_apply(&other,(Move){60,61}));
    CHECK(!memcmp(g.evidence,other.evidence,sizeof(g.evidence)));
    CHECK(g.evidence[0].pursued==(1u<<MARSHAL)&&g.evidence[0].approaches==1);
    CHECK(ai_behavior_likelihood(&g,61,SPY)==1); /* One approach is not a tell. */
    CHECK(game_apply_search(&sim,(Move){60,61}));
    CHECK(sim.evidence[0].approaches==0);
    memcpy(sim.evidence,g.evidence,sizeof(g.evidence));
    CHECK(!memcmp(&sim,&g,sizeof(g))); /* Rules, histories and terminal state identical. */
    Game hidden=position(SPY,false);CHECK(game_apply(&hidden,(Move){60,61}));
    CHECK(hidden.evidence[0].approaches==0);
    g.turn=HUMAN;CHECK(game_apply(&g,(Move){61,60}));
    CHECK(g.evidence[0].avoided==(1u<<MARSHAL)&&g.evidence[0].retreats==1);
    g.turn=HUMAN;CHECK(game_apply(&g,(Move){90,80})); /* Break repetition. */
    g.turn=HUMAN;CHECK(game_apply(&g,(Move){60,61}));
    CHECK(g.evidence[0].approaches==2);
    CHECK(ai_behavior_likelihood(&g,61,SPY)>ai_behavior_likelihood(&g,61,CAPTAIN));
    for(int r=SPY;r<=MARSHAL;r++)CHECK(ai_behavior_likelihood(&g,61,r)>.8f&&ai_behavior_likelihood(&g,61,r)<1.4f);
    int observed=g.evidence[0].last_ply;
    g.turn=HUMAN;CHECK(game_apply(&g,(Move){80,90}));
    CHECK(g.evidence[0].declined==(1u<<MARSHAL)&&g.evidence[0].last_ply==observed);
    g.ply+=20;memset(g.history,0,sizeof(g.history));
    CHECK(ai_behavior_likelihood(&g,61,SPY)>1); /* Survives short move ring. */
    g.ply+=121;
    CHECK(fabsf(ai_behavior_likelihood(&g,61,SPY)-1)<.00001f);
    g.evidence[0].retreats=2;g.evidence[0].retreat_ply=g.ply;
    CHECK(ai_behavior_likelihood(&g,61,SPY)<1);
    g.board[61].revealed=true;CHECK(ai_behavior_likelihood(&g,61,SPY)==1);
    game_clear(&g);CHECK(g.evidence[0].pursued==0);
    for(int seed=1;seed<=100;seed++){
        game_init(&g,(uint32_t)seed);float p[100][12]={{0}};int remaining[12];
        for(int r=0;r<12;r++)remaining[r]=army_counts[r];
        for(int s=0;s<40;s++){
            /* Only legal public motion; identity information is then erased. */
            g.board[s].moved=movable(g.board[s])&&(seed==100||((s+seed)%3==0));
            g.board[s].rank=-2;float sum=0;
            for(int r=0;r<12;r++){p[s][r]=remaining[r]*ai_rank_prior(&g,s,r);sum+=p[s][r];}
            for(int r=0;r<12;r++)p[s][r]/=sum;
        }
        ai_balance_beliefs(&g,remaining,p);float counts[12]={0};
        for(int s=0;s<40;s++){
            float sum=0;for(int r=0;r<12;r++){CHECK(p[s][r]>=0&&p[s][r]<=1.00001f);sum+=p[s][r];counts[r]+=p[s][r];}
            CHECK(fabsf(sum-1)<.0001f);
            if(g.board[s].moved)CHECK(p[s][FLAG]==0&&p[s][BOMB]==0);
        }
        for(int r=0;r<12;r++)CHECK(fabsf(counts[r]-remaining[r])<.0002f);
    }
    puts("Public memory: hidden-rank invariance, persistence, fading, bounded bluff beliefs, search parity OK");
    return 0;
}
