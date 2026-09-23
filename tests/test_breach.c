#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Breach line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static BreachPlan plan_for(Game *g){Game v;int r[12];float p[100][12];public_board(g,&v,r);probabilities(&v,r,p);return breach_plan(&v,p);}
int main(void){
    Game g,base;CHECK(replay_load(STRATEGO_BREACH_FIXTURE,418,&base));
    CHECK(base.cleared_bombs[HUMAN][83]);
    CHECK(base.captured[COMPUTER][MINER]==army_counts[MINER]);
    /* The old test required the marshal to gamble on this hidden flag,
       and even required his death after swapping it with a bomb. Superseded
       policy: a large reserve does not make that sacrifice a good conversion. */
    CHECK(plan_for(&base).goal<0);
    for(uint32_t seed=1;seed<=3;seed++){
        Game other=base;other.board[93].rank=BOMB;other.board[94].rank=FLAG;
        uint32_t a=seed,b=seed;Move x=ai_choose(&base,1,&a),y=ai_choose(&other,1,&b);
        CHECK(x.from==y.from&&x.to==y.to&&a==b);CHECK(game_legal(&base,x,base.turn));
        Game v;int left[12];float p[100][12];public_board(&base,&v,left);probabilities(&v,left,p);
        CHECK(!speculative_bomb_probe(&v,p,x));
    }
    /* A revealed flag remains an immediate win. */
    g=base;CHECK(game_apply(&g,(Move){84,83}));CHECK(game_apply(&g,(Move){96,66}));
    g.board[93].revealed=true;uint32_t rng=1;Move win=ai_choose(&g,1,&rng);
    CHECK(win.from==83&&win.to==93);CHECK(game_apply(&g,win));CHECK(g.winner==COMPUTER);
    /* A miner can exploit the same public breach without dying to a bomb. */
    g=base;g.board[84].rank=MINER;g.captured[COMPUTER][MARSHAL]++;g.captured[COMPUTER][MINER]--;
    CHECK(plan_for(&g).goal==93);Game miner=g;
    memset(g.cleared_bombs,0,sizeof(g.cleared_bombs));CHECK(plan_for(&g).goal<0);
    g=miner;g.board[93].rank=BOMB;g.board[93].revealed=true;CHECK(plan_for(&g).goal<0);
    g=miner;g.board[93].moved=true;CHECK(plan_for(&g).goal<0);
    g=miner;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&s!=84&&movable(g.board[s])){
        g.captured[COMPUTER][g.board[s].rank]++;g.board[s]=empty_piece();
    }CHECK(plan_for(&g).goal<0);
    g=miner;g.board[3]=(Piece){MARSHAL,HUMAN,39,true,true};CHECK(plan_for(&g).goal<0);
    CHECK(replay_load(STRATEGO_BREACH_FIXTURE,410,&g));CHECK(!g.cleared_bombs[HUMAN][83]);
    CHECK(game_apply(&g,(Move){73,83}));CHECK(g.cleared_bombs[HUMAN][83]);
    game_clear(&g);CHECK(!g.cleared_bombs[HUMAN][83]);
    puts("Breach conversion: no officer bomb gamble, miner opening and known flag victory passed.");return 0;
}
