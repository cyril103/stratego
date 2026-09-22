#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Breach line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static BreachPlan plan_for(Game *g){Game v;int r[12];float p[100][12];public_board(g,&v,r);probabilities(&v,r,p);return breach_plan(&v,p);}
int main(void){
    Game g,base;CHECK(replay_load(STRATEGO_BREACH_FIXTURE,418,&base));
    CHECK(base.cleared_bombs[HUMAN][83]);
    CHECK(base.captured[COMPUTER][MINER]==army_counts[MINER]);
    uint32_t seeds[]={1,2,3,519};
    for(int i=0;i<4;i++){
        g=base;uint32_t rng=seeds[i];Move m=ai_choose(&g,1,&rng);
        CHECK(m.from==84&&m.to==83);CHECK(game_apply(&g,m));
        /* Every legal reply on the recorded board leaves the capture open. */
        Move replies[MAX_MOVES];int n=game_moves(&g,HUMAN,replies);
        for(int j=0;j<n;j++){
            Game next=g;CHECK(game_apply(&next,replies[j]));CHECK(next.winner!=HUMAN);
            if(next.winner==COMPUTER)continue;
            CHECK(game_apply(&next,(Move){83,93}));
            CHECK(next.winner==COMPUTER&&next.end_reason==END_FLAG);
        }
        CHECK(game_apply(&g,(Move){96,66}));m=ai_choose(&g,1,&rng);
        CHECK(m.from==83&&m.to==93);CHECK(game_apply(&g,m));CHECK(g.winner==COMPUTER);
        /* Same public evidence, different hidden flag: same policy and RNG.
           Here the marshal really dies, with a dominant general left. */
        Game other=base;other.board[93].rank=BOMB;other.board[94].rank=FLAG;
        uint32_t a=seeds[i],b=a;Move x=ai_choose(&base,1,&a),y=ai_choose(&other,1,&b);
        CHECK(x.from==y.from&&x.to==y.to&&a==b);CHECK(game_apply(&other,y));
        CHECK(game_apply(&other,(Move){96,66}));y=ai_choose(&other,1,&b);
        CHECK(y.from==83&&y.to==93);CHECK(game_apply(&other,y));
        CHECK(other.combat==-1&&other.board[62].rank==GENERAL&&other.winner<0);
    }
    g=base;memset(g.cleared_bombs,0,sizeof(g.cleared_bombs));CHECK(plan_for(&g).goal<0);
    g=base;g.board[93].rank=BOMB;g.board[93].revealed=true;CHECK(plan_for(&g).goal<0);
    g=base;g.board[93].moved=true;CHECK(plan_for(&g).goal<0);
    g=base;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&s!=84&&movable(g.board[s])){
        g.captured[COMPUTER][g.board[s].rank]++;g.board[s]=empty_piece();
    }CHECK(plan_for(&g).goal<0);
    g=base;g.board[3]=(Piece){MARSHAL,HUMAN,39,true,true};CHECK(plan_for(&g).goal<0);
    CHECK(replay_load(STRATEGO_BREACH_FIXTURE,410,&g));CHECK(!g.cleared_bombs[HUMAN][83]);
    CHECK(game_apply(&g,(Move){73,83}));CHECK(g.cleared_bombs[HUMAN][83]);
    game_clear(&g);CHECK(!g.cleared_bombs[HUMAN][83]);
    puts("Breach exploitation: recorded victory, hidden swaps and safety checks passed.");return 0;
}
