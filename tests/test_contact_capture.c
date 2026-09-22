#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Contact capture line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g,v;int remaining[12];float p[100][12];Move attack={35,45},retreat={35,34};
    CHECK(replay_load(STRATEGO_CONTACT_FIXTURE,88,&g));
    public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    CHECK(v.board[45].rank==-2&&v.board[45].moved&&p[45][BOMB]==0);
    float before=approaching_officer_risk(&v,p);
    float credit=contact_safety_bonus(&v,p,attack,before);
    CHECK(credit>0&&credit<contact_safety_bonus(&v,p,retreat,before));
    CHECK(fabsf(credit-before*(1-p[45][MARSHAL]))<.001f);
    Game concealed=v;concealed.board[35].revealed=false;
    CHECK(contact_safety_bonus(&concealed,p,attack,approaching_officer_risk(&concealed,p))==0);
    Game supported=v;supported.board[55]=(Piece){-2,HUMAN,39,false,true};
    p[55][SPY]=1;
    CHECK(contact_safety_bonus(&supported,p,attack,before)<=0);
    memset(p[55],0,sizeof(p[55]));
    for(uint32_t seed=1;seed<=8;seed++){
        Game swapped=g;CHECK(!g.board[68].revealed&&g.board[68].moved);
        swapped.board[45].rank=g.board[68].rank;swapped.board[68].rank=g.board[45].rank;
        uint32_t rng=seed,same=seed;Move a=ai_choose(&g,1,&rng),b=ai_choose(&swapped,1,&same);
        printf("Contact seed %u: %d>%d\n",seed,a.from,a.to);fflush(stdout);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);
        CHECK(a.from==35&&a.to==45);Game next=g;CHECK(game_apply(&next,a));
        CHECK(next.board[45].rank==MARSHAL&&next.captured[HUMAN][SPY]==1);
        Move replies[MAX_MOVES];int n=game_moves(&next,HUMAN,replies);
        for(int i=0;i<n;i++)CHECK(replies[i].to!=45);
    }
    /* A losing attack or equal trade must never count as a rescued officer. */
    memset(p[45],0,sizeof(p[45]));p[45][BOMB]=1;
    CHECK(contact_safety_bonus(&v,p,attack,before)==0);
    p[45][BOMB]=0;p[45][MARSHAL]=1;CHECK(contact_safety_bonus(&v,p,attack,before)==0);
    v.board[45].revealed=true;v.board[45].rank=MARSHAL;
    CHECK(contact_safety_bonus(&v,p,attack,before)==0);
    v.board[45].rank=SPY;CHECK(contact_safety_bonus(&v,p,attack,before)>0);
    /* New unknown support is still penalized at full strength, including
       when the nominal capture itself is unlikely to succeed. */
    public_board(&g,&v,remaining);memset(p,0,sizeof(p));p[45][SERGEANT]=1;
    v.board[55]=(Piece){-2,HUMAN,39,false,true};p[55][SPY]=1;
    before=approaching_officer_risk(&v,p);CHECK(before==0);
    float danger=contact_safety_bonus(&v,p,attack,before);CHECK(danger<0);
    p[45][SERGEANT]=.1f;p[45][BOMB]=.9f;
    CHECK(fabsf(contact_safety_bonus(&v,p,attack,before)-danger)<.001f);
    /* Preserve the spy's winning recapture in the same recorded game. */
    CHECK(replay_load(STRATEGO_CONTACT_FIXTURE,216,&g));uint32_t rng=519;
    Move win=ai_choose(&g,1,&rng);CHECK(win.from==38&&win.to==48);
    puts("Contact capture: public survival odds, hidden identity swaps and support danger passed.");return 0;
}
