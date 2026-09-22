#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Secrecy line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];
    int plies[]={28,106};Move attacks[]={{35,34},{59,58}};
    for(int i=0;i<2;i++){
        CHECK(replay_load(STRATEGO_SECRECY_FIXTURE,plies[i],&g));
        public_board(&g,&v,remaining);probabilities(&v,remaining,p);
        float cost=officer_disclosure_cost(&v,p,attacks[i]);CHECK(cost>10);
        Game exposed=v;exposed.board[attacks[i].from].revealed=true;
        CHECK(officer_disclosure_cost(&exposed,p,attacks[i])==0);
        Game endgame=v;
        for(int s=0;s<100;s++)if(s!=attacks[i].from&&endgame.board[s].side==g.turn)endgame.board[s].revealed=true;
        CHECK(officer_disclosure_cost(&endgame,p,attacks[i])==0);
        for(uint32_t seed=1;seed<=4;seed++){
            Game other=swapped(g);uint32_t rng=seed,same=seed;
            Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
            CHECK(game_legal(&g,a,g.turn));CHECK(a.from==b.from&&a.to==b.to&&rng==same);
            if(i==0)CHECK(!(a.from==35&&a.to==34));
            printf("Before %d seed %u: %d>%d\n",plies[i],seed,a.from,a.to);fflush(stdout);
        }
    }
    /* Secrecy must never postpone taking a known flag. */
    g.board[58].rank=FLAG;g.board[58].revealed=true;
    uint32_t rng=519;Move win=ai_choose(&g,1,&rng);
    CHECK(win.from==59&&win.to==58);CHECK(game_apply(&g,win));CHECK(g.winner==COMPUTER);
    /* A forced defense overrides secrecy even with a large hidden army. */
    CHECK(replay_load(STRATEGO_SECRECY_FIXTURE,28,&g));
    g.board[44]=g.board[9];g.board[9]=empty_piece();
    g.board[34].revealed=true;g.board[33].rank=MINER;
    rng=519;Move defend=ai_choose(&g,1,&rng);
    CHECK(defend.from==35&&defend.to==34);CHECK(game_apply(&g,defend));
    CHECK(g.board[44].rank==FLAG&&g.board[34].rank==MARSHAL);
    puts("Officer secrecy: recorded opening, hidden identities and winning capture passed.");return 0;
}
