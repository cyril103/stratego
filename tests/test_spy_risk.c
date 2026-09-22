#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Spy risk line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g,v;int remaining[12];float p[100][12];Move attack={68,67};
    CHECK(replay_load(STRATEGO_SPY_RISK_FIXTURE,522,&g));
    CHECK(game_legal(&g,attack,g.turn));
    public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    CHECK(v.board[67].rank==-2&&v.board[67].moved);
    CHECK(fabsf(p[67][MARSHAL]-.1f)<.0001f);
    CHECK(spy_comeback_chance(&v,p,attack)==1);
    CHECK(spy_attack_cost(&v,p,attack)<0);
    /* Restore three publicly lost scouts: a smaller deficit with a safe
       retreat still preserves part of the spy's option value. */
    Game moderate=g;bool used[80]={false};
    for(int s=0;s<100;s++)if(g.board[s].id>=0)used[g.board[s].id]=true;
    CHECK(moderate.captured[COMPUTER][SCOUT]>=3);
    int id=40;
    for(int s=0;s<3;s++){
        while(id<80&&used[id])id++;
        CHECK(id<80&&moderate.board[s].side<0);
        moderate.board[s]=(Piece){SCOUT,COMPUTER,id++,false,false};
        moderate.captured[COMPUTER][SCOUT]--;
    }
    Game modview;public_board(&moderate,&modview,remaining);
    float caution=spy_comeback_chance(&modview,p,attack);
    CHECK(caution>0&&caution<1);CHECK(spy_attack_cost(&modview,p,attack)>0);
    modview.board[69]=modview.board[0];modview.board[0]=empty_piece();
    CHECK(spy_comeback_chance(&modview,p,attack)>caution);
    /* No gamble on a stationary piece, known losing rank, or impossible marshal. */
    Game other=v;other.board[67].moved=false;
    CHECK(spy_comeback_chance(&other,p,attack)==0);
    other=v;other.board[67].revealed=true;other.board[67].rank=CAPTAIN;
    CHECK(spy_comeback_chance(&other,p,attack)==0);
    float odds=p[67][MARSHAL];p[67][MARSHAL]=0;
    CHECK(spy_comeback_chance(&v,p,attack)==0);p[67][MARSHAL]=odds;
    /* A balanced public army retains the old capability protection. */
    other=v;memset(other.captured,0,sizeof(other.captured));
    CHECK(spy_comeback_chance(&other,p,attack)==0);
    CHECK(spy_attack_cost(&other,p,attack)>100);
    for(uint32_t seed=1;seed<=8;seed++){
        Game swapped=g;int rank=swapped.board[67].rank;
        CHECK(!swapped.board[65].revealed&&swapped.board[65].moved);
        swapped.board[67].rank=swapped.board[65].rank;swapped.board[65].rank=rank;
        uint32_t rng=seed,same=seed;
        Move a=ai_choose(&g,1,&rng),b=ai_choose(&swapped,1,&same);
        printf("Spy seed %u: %d>%d\n",seed,a.from,a.to);fflush(stdout);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);
        CHECK(a.from==68&&a.to==67);Game next=g;
        CHECK(game_apply(&next,a));CHECK(next.board[67].rank==SPY);
        CHECK(next.captured[HUMAN][MARSHAL]==1);
    }
    g.board[67].revealed=true;uint32_t rng=519;
    public_board(&g,&v,remaining);
    CHECK(known_unanswered_loss(&v,attack)<4*worth[MARSHAL]);
    Move certain=ai_choose(&g,1,&rng);CHECK(certain.from==68&&certain.to==67);
    /* Even a comeback opportunity cannot ignore an immediate flag capture. */
    CHECK(replay_load(STRATEGO_SPY_RISK_FIXTURE,522,&g));
    id=0;while(id<40&&used[id])id++;
    CHECK(id<40&&g.captured[HUMAN][SCOUT]>0&&g.board[19].rank==BOMB);
    g.board[19]=(Piece){SCOUT,HUMAN,id,true,true};
    g.captured[HUMAN][SCOUT]--;g.captured[COMPUTER][BOMB]++;
    rng=519;Move defense=ai_choose(&g,1,&rng);CHECK(defense.from==18&&defense.to==19);
    puts("Spy comeback: public odds, retreat, deficit and hidden target permutation passed.");return 0;
}
