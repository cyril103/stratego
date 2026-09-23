#include "../src/ai.c"
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
    puts("Campaign: collective exits, role-sensitive raid risk and actual-move mission continuity OK");return 0;
}
