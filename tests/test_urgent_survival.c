#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Urgent survival line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
int main(void){
    Game g,v;int rem[12];float p[100][12];
    CHECK(replay_load(STRATEGO_URGENT_FIXTURE,412,&g));public_board(&g,&v,rem);probabilities(&v,rem,p);
    CHECK(active_spy_threat(&v));CHECK(saves_active_spy(&v,p,(Move){29,28}));
    CHECK(!saves_active_spy(&v,p,(Move){59,58}));
    v.captured[HUMAN][MARSHAL]=1;CHECK(!active_spy_threat(&v));
    CHECK(replay_load(STRATEGO_URGENT_FIXTURE,438,&g));public_board(&g,&v,rem);probabilities(&v,rem,p);
    CHECK(v.board[79].side==HUMAN&&v.board[79].revealed&&v.board[79].rank==BOMB);
    CHECK(!v.board[88].revealed);
    CHECK(costly_bomb_probe(&v,p,(Move){78,88}));
    Game lone=v;for(int s=0;s<100;s++)if(s!=78&&lone.board[s].side==COMPUTER&&movable(lone.board[s]))lone.board[s]=empty_piece();
    CHECK(!costly_bomb_probe(&lone,p,(Move){78,88}));
    v.board[78].rank=MINER;CHECK(!costly_bomb_probe(&v,p,(Move){78,88}));
    int plies[]={412,438};
    for(int i=0;i<2;i++)for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_URGENT_FIXTURE,plies[i],&g));Game other=swapped(g);
        uint32_t rng=seed,same=seed;Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);CHECK(game_legal(&g,a,g.turn));
        printf("Before %d seed %u: %d>%d\n",plies[i],seed,a.from,a.to);fflush(stdout);
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        if(i==0){CHECK(saves_active_spy(&v,p,a));CHECK(game_apply(&g,a));
            CHECK(g.board[28].side==COMPUTER&&g.board[28].rank==SPY);
            CHECK(!game_legal(&g,(Move){39,28},HUMAN));}
        if(i==1){CHECK(!costly_bomb_probe(&v,p,a));
            CHECK(!(v.board[a.to].side==HUMAN&&v.board[a.to].revealed&&v.board[a.to].rank==BOMB));}
    }
    /* A known flag capture still takes priority over the spy emergency. */
    CHECK(replay_load(STRATEGO_URGENT_FIXTURE,412,&g));g.board[58]=(Piece){FLAG,HUMAN,33,true,false};
    uint32_t rng=1;Move win=ai_choose(&g,1,&rng);CHECK(win.from==59&&win.to==58);
    puts("Urgent survival: spy rescue, bomb restraint and hidden swaps passed.");return 0;
}
