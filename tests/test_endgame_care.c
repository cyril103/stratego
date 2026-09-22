#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Endgame care line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
static int safe_spy_exits(const Game *g,int spy){
    Game v;int rem[12];public_board(g,&v,rem);int nb[4],nn=neighbors(spy,nb),safe=0;
    for(int j=0;j<nn;j++)if(v.board[nb[j]].side<0&&public_legal(&v,(Move){spy,nb[j]},v.turn)){
        Game next=optimistic_move(&v,(Move){spy,nb[j]});
        if(!spy_square_unsafe(&next,nb[j],v.turn))safe++;
    }return safe;
}
int main(void){
    Game g,v;int rem[12];float p[100][12];int plies[]={174,334,350};
    CHECK(replay_load(STRATEGO_ENDGAME_CARE_FIXTURE,174,&g));public_board(&g,&v,rem);probabilities(&v,rem,p);
    CHECK(contact_safety_bonus(&v,p,(Move){25,35},approaching_officer_risk(&v,p))<=.5f*worth[COLONEL]);
    CHECK(replay_load(STRATEGO_ENDGAME_CARE_FIXTURE,334,&g));public_board(&g,&v,rem);
    CHECK(spy_box_cost(&v,(Move){24,34})>0);CHECK(spy_box_cost(&v,(Move){24,23})==0);
    Game trapped=g;CHECK(game_apply(&trapped,(Move){24,34}));CHECK(game_apply(&trapped,(Move){45,44}));
    CHECK(safe_spy_exits(&trapped,34)==0);
    v.captured[HUMAN][MARSHAL]=1;CHECK(spy_box_cost(&v,(Move){24,34})==0);
    CHECK(replay_load(STRATEGO_ENDGAME_CARE_FIXTURE,350,&g));public_board(&g,&v,rem);
    CHECK(last_officer_trade_cost(&v,(Move){13,23})>0);
    CHECK(last_officer_trade_cost(&v,(Move){15,14})>0);
    CHECK(last_officer_trade_cost(&v,(Move){15,25})==0);
    Game equal=v;
    for(int r=SPY;r<MARSHAL;r++)equal.captured[HUMAN][r]=army_counts[r];
    CHECK(last_officer_trade_cost(&equal,(Move){15,14})==0);
    for(int i=0;i<3;i++)for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_ENDGAME_CARE_FIXTURE,plies[i],&g));
        Game other=swapped(g);uint32_t rng=seed,same=seed;
        Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);CHECK(game_legal(&g,a,g.turn));
        printf("Before %d seed %u: %d>%d\n",plies[i],seed,a.from,a.to);fflush(stdout);
        if(i==0){CHECK(!(a.from==25&&a.to==35));}
        if(i==1){
            CHECK(a.from==24&&a.to==23);CHECK(game_apply(&g,a));
            CHECK(game_apply(&g,(Move){45,44}));CHECK(safe_spy_exits(&g,23)>0);
            Move follow=ai_choose(&g,1,&rng);CHECK(game_apply(&g,follow));
            CHECK(game_apply(&g,(Move){44,34}));
            bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&g.board[s].rank==SPY)alive=true;
            CHECK(alive);
        }
        if(i==2){
            CHECK(a.from==15&&g.board[a.to].side<0);CHECK(game_apply(&g,a));
            CHECK(!game_legal(&g,(Move){14,a.to},HUMAN));
        }
    }
    /* Taking a known flag still overrides all conservation heuristics. */
    CHECK(replay_load(STRATEGO_ENDGAME_CARE_FIXTURE,350,&g));g.board[23]=(Piece){FLAG,HUMAN,33,true,false};
    uint32_t rng=519;Move win=ai_choose(&g,1,&rng);CHECK(win.from==13&&win.to==23);
    CHECK(game_apply(&g,win));CHECK(g.winner==COMPUTER);
    puts("Endgame care: bounded probes, spy exits and last-officer survival passed.");return 0;
}
