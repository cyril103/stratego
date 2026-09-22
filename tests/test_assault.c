#include "../src/ai.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Assault line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,true,true};}
static void count_armies(Game *g){
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0)g->captured[g->board[s].side][g->board[s].rank]--;
}
static Game setup(void){
    Game g;game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,80,BOMB,HUMAN);put(&g,91,BOMB,HUMAN);
    put(&g,64,MINER,HUMAN);put(&g,65,GENERAL,HUMAN);
    put(&g,9,FLAG,COMPUTER);put(&g,8,BOMB,COMPUTER);put(&g,19,BOMB,COMPUTER);put(&g,29,SERGEANT,COMPUTER);
    count_armies(&g);return g;
}
int main(void){
    Game g=setup(),view;int left[12];float odds[100][12];AssaultPlan plan;
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);
    CHECK(plan.miner==64&&plan.escort==65&&plan.goal==9);
    CHECK(assault_bonus(&view,odds,&plan,(Move){64,54})>0);
    Game next=optimistic_move(&view,(Move){64,54});assault_plan(&next,odds,&plan);
    CHECK(assault_bonus(&next,odds,&plan,(Move){65,55})>0);
    g=setup();g.board[65]=empty_piece();put(&g,55,GENERAL,HUMAN);put(&g,44,SERGEANT,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);
    CHECK(assault_bonus(&view,odds,&plan,(Move){64,54})==0); /* Escort recapture would not save the miner. */
    g=setup();
    put(&g,29,GENERAL,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);CHECK(plan.miner<0);
    /* Trading an equal top rank is acceptable with a substantial army lead. */
    put(&g,74,CAPTAIN,HUMAN);put(&g,75,MAJOR,HUMAN);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);CHECK(plan.miner==64&&plan.escort==65);
    put(&g,29,MARSHAL,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);CHECK(plan.miner<0);
    g=setup();put(&g,70,MINER,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,odds);assault_plan(&view,odds,&plan);CHECK(plan.miner<0);
    g=setup();g.board[9].revealed=g.board[8].revealed=false;g.board[9].moved=g.board[8].moved=false;
    Game changed=g;changed.board[9].rank=BOMB;changed.board[8].rank=FLAG;
    uint32_t a=519,b=519;Move x=ai_choose(&g,1,&a),y=ai_choose(&changed,1,&b);
    CHECK(x.from==y.from&&x.to==y.to&&a==b);
    int flag_wins=0;
    for(uint32_t seed=1;seed<=4;seed++){
        g=setup();uint32_t rng=seed;int miner_moves=0,escort_moves=0;
        for(int ply=0;ply<100&&g.winner<0;ply++){
            Move m;
            if(g.turn==HUMAN){m=ai_choose(&g,1,&rng);miner_moves+=g.board[m.from].rank==MINER;escort_moves+=g.board[m.from].rank==GENERAL;}
            else {Move legal[MAX_MOVES];int n=game_moves(&g,COMPUTER,legal);CHECK(n>0);m=legal[game_random(&rng)%n];}
            CHECK(game_apply(&g,m));
        }
        CHECK(g.winner==HUMAN);CHECK(g.end_reason==END_FLAG||g.end_reason==END_IMMOBILE);
        CHECK(miner_moves>0&&escort_moves>0);
        if(g.end_reason==END_FLAG){flag_wins++;CHECK(g.captured[COMPUTER][BOMB]>4);}
        printf("Assault seed %u: win reason %d in %d plies, miner %d moves, escort %d moves\n",seed,g.end_reason,g.ply,miner_moves,escort_moves);
    }
    CHECK(flag_wins>0);
    puts("Escorted flag assault, emergency defense and hidden ranks OK");return 0;
}
