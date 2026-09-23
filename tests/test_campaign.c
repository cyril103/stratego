#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Campaign line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,true,true};}
static void count(Game *g){
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0)g->captured[g->board[s].side][g->board[s].rank]--;
}
int main(void){
    Game g,v;game_clear(&g);int left[12];float p[100][12];
    put(&g,11,GENERAL,HUMAN);put(&g,1,BOMB,HUMAN);put(&g,10,BOMB,HUMAN);
    put(&g,22,SERGEANT,HUMAN);put(&g,31,MARSHAL,COMPUTER);
    put(&g,99,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);count(&g);
    public_board(&g,&v,left);probabilities(&v,left,p);
    CHECK(officer_exits(&v,p,11)==1);
    float before=officer_escape_reserve(&v,p);
    CHECK(before==0);
    CHECK(retreat_reserve_bonus(&v,p,(Move){22,12},before)<-10);
    CHECK(retreat_reserve_bonus(&v,p,(Move){22,23},before)==0);
    /* Removing the hunter, or blocking its approach, removes the penalty. */
    v.board[31]=empty_piece();CHECK(retreat_reserve_bonus(&v,p,(Move){22,12},0)==0);
    public_board(&g,&v,left);probabilities(&v,left,p);put(&v,21,BOMB,COMPUTER);
    CHECK(retreat_reserve_bonus(&v,p,(Move){22,12},0)==0);
    game_init(&g,42);
    CHECK(raid_loss_budget(&g,MAJOR)>raid_loss_budget(&g,MARSHAL));
    for(int r=MAJOR;r<=MARSHAL;r++)g.captured[HUMAN][r]=army_counts[r];
    g.captured[HUMAN][MAJOR]--;
    CHECK(raid_loss_budget(&g,MAJOR)<=.06f);
    game_clear(&g);put(&g,54,MINER,HUMAN);
    float distance[100];for(int s=0;s<100;s++)distance[s]=1000;
    distance[64]=5;distance[54]=4;g.history_count[HUMAN]=1;
    g.history[HUMAN][0]=(Move){64,54};g.history_id[HUMAN][0]=54;
    CHECK(assault_commitment(&g,54,distance)>0);
    distance[64]=3;CHECK(assault_commitment(&g,54,distance)==0);
    distance[64]=5;g.history_id[HUMAN][0]=12;CHECK(assault_commitment(&g,54,distance)==0);
    /* An attractive flag behind a bomb does not justify losing the last
       miner to its known guard. Bring the officer in before opening it. */
    game_clear(&g);g.turn=COMPUTER;
    put(&g,0,FLAG,COMPUTER);put(&g,1,BOMB,COMPUTER);put(&g,10,BOMB,COMPUTER);
    put(&g,20,MARSHAL,COMPUTER);put(&g,29,SCOUT,COMPUTER);put(&g,63,MINER,COMPUTER);
    put(&g,83,FLAG,HUMAN);put(&g,73,BOMB,HUMAN);put(&g,82,BOMB,HUMAN);
    put(&g,84,BOMB,HUMAN);put(&g,93,BOMB,HUMAN);put(&g,74,GENERAL,HUMAN);
    put(&g,80,SCOUT,HUMAN);put(&g,90,SCOUT,HUMAN);count(&g);
    for(uint32_t seed=1;seed<=3;seed++){
        Game next=g;uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);
        CHECK(game_apply(&next,m));int miner=-1;
        for(int s=0;s<100;s++)if(next.board[s].side==COMPUTER&&next.board[s].rank==MINER)miner=s;
        CHECK(miner>=0&&!game_legal(&next,(Move){74,miner},HUMAN));
    }
    CHECK(replay_load(STRATEGO_CAMPAIGN_FIXTURE,468,&g));
    public_board(&g,&v,left);probabilities(&v,left,p);
    CHECK(v.board[25].rank==-2&&p[25][SPY]>0);
    Game pocket=optimistic_move(&v,(Move){14,4});
    CHECK(marshal_approach_trap(&pocket,p));
    Game open=optimistic_move(&v,(Move){14,13});
    CHECK(!marshal_approach_trap(&open,p));
    /* A certainly capturable possible spy is not an automatic retreat order. */
    Game contact=optimistic_move(&v,(Move){14,24});
    CHECK(!marshal_approach_trap(&contact,p));
    float none[100][12];memcpy(none,p,sizeof(none));
    for(int s=0;s<100;s++)none[s][SPY]=0;
    CHECK(!marshal_approach_trap(&pocket,none));
    Game other=g;int rank=other.board[25].rank;
    other.board[25].rank=other.board[54].rank;other.board[54].rank=rank;
    for(uint32_t seed=1;seed<=3;seed++){
        uint32_t rng=seed,same=seed;
        Move m=ai_choose(&g,1,&rng),hidden=ai_choose(&other,1,&same);
        CHECK(m.from==hidden.from&&m.to==hidden.to&&rng==same);
        CHECK(game_legal(&g,m,g.turn));
        CHECK(!(m.from==14&&m.to==4));
        Game next=optimistic_move(&v,m);
        CHECK(!marshal_contact_unsafe(&next,p)&&!marshal_approach_trap(&next,p));
    }
    /* A marshal need not retreat when a safe waiting move exists elsewhere.
       The possible spy still has to spend a turn approaching into contact. */
    CHECK(replay_load(STRATEGO_CAMPAIGN_WAIT_FIXTURE,403,&g));
    public_board(&g,&v,left);probabilities(&v,left,p);
    Game standoff=optimistic_move(&v,(Move){68,67});
    CHECK(!marshal_approach_trap(&standoff,p));
    Move keep[]={{68,67},{85,75}};bool retained=false;
    int kept=preserve_marshal_exit(&v,p,keep,2);
    for(int i=0;i<kept;i++)retained|=keep[i].from==68&&keep[i].to==67;
    CHECK(retained);
    puts("Campaign: collective exits, public marshal traps, raid risk and mission continuity OK");return 0;
}
