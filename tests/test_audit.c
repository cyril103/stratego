#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Audit line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,false,rank>FLAG&&rank<BOMB};}
static void count_armies(Game *g){
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0)g->captured[g->board[s].side][g->board[s].rank]--;
}
int main(void){
    Game g,view;int left[12];float p[100][12];
    game_clear(&g);put(&g,95,FLAG,HUMAN);put(&g,84,LIEUTENANT,HUMAN);
    put(&g,4,FLAG,COMPUTER);put(&g,75,SPY,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,p);
    CHECK(flag_race_cost(&view,p,(Move){84,74})>500);
    CHECK(flag_race_cost(&view,p,(Move){84,85})==0);
    for(uint32_t seed=1;seed<=4;seed++){
        uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);CHECK(m.from==84&&m.to==85);
    }
    /* Concealed own bombs still stop every non-miner, including scouts. */
    put(&g,85,BOMB,HUMAN);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,p);
    CHECK(flag_race_cost(&view,p,(Move){84,74})==0);
    put(&g,75,MINER,COMPUTER);count_armies(&g);
    public_board(&g,&view,left);probabilities(&view,left,p);
    CHECK(flag_race_cost(&view,p,(Move){84,74})>500);
    CHECK(replay_load(STRATEGO_AUDIT_FIXTURES "/audit_flagrace_2917.jsonl",623,&g));
    public_board(&g,&view,left);probabilities(&view,left,p);
    CHECK(flag_race_cost(&view,p,(Move){85,84})>500);
    CHECK(flag_race_cost(&view,p,(Move){85,86})>500);
    /* The recorded lone defender is already lost: no claim of a rescue. */
    Game changed=g;int a=-1,b=-1;
    for(int s=0;s<100;s++)if(g.board[s].side==1-g.turn&&!g.board[s].revealed&&g.board[s].moved){if(a<0)a=s;else if(g.board[s].rank!=g.board[a].rank)b=s;}
    CHECK(a>=0&&b>=0);int rank=changed.board[a].rank;changed.board[a].rank=changed.board[b].rank;changed.board[b].rank=rank;
    for(uint32_t seed=1;seed<=4;seed++){
        uint32_t rng=seed,other=seed;Move m=ai_choose(&g,1,&rng),n=ai_choose(&changed,1,&other);
        CHECK(game_legal(&g,m,g.turn));
        CHECK(m.from==n.from&&m.to==n.to&&rng==other);
    }
    CHECK(replay_load(STRATEGO_AUDIT_FIXTURES "/audit_assault_2918.jsonl",561,&g));
    public_board(&g,&view,left);probabilities(&view,left,p);AssaultPlan plan;assault_plan(&view,p,&plan);
    CHECK(plan.miner>=0&&plan.escort>=0);
    CHECK(view.board[plan.miner].rank==MINER&&view.board[plan.escort].rank==GENERAL);
    CHECK(assault_miner_steps(&view,p,&plan)==0);
    CHECK(assault_bonus(&view,p,&plan,(Move){45,35})>0);
    uint32_t rng=519;Move m=ai_choose(&g,1,&rng);
    printf("Equal-general assault: miner %d escort %d, move %d -> %d\n",plan.miner,plan.escort,m.from,m.to);
    CHECK(game_legal(&g,m,g.turn));
    CHECK(assault_bonus(&view,p,&plan,m)>0);
    CHECK(replay_load(STRATEGO_AUDIT_FIXTURES "/audit_hidden_officer_2918.jsonl",29,&g));
    public_board(&g,&view,left);probabilities(&view,left,p);
    Game exposed=optimistic_move(&view,(Move){55,45});
    CHECK(!view.board[55].revealed&&approaching_officer_risk(&exposed,p)>approaching_officer_risk(&view,p));
    for(uint32_t seed=1;seed<=4;seed++){
        rng=seed;m=ai_choose(&g,1,&rng);CHECK(!(m.from==55&&m.to==45));
    }
    CHECK(replay_load(STRATEGO_AUDIT_FIXTURES "/audit_regression_2921.jsonl",159,&g));
    public_board(&g,&view,left);probabilities(&view,left,p);assault_plan(&view,p,&plan);
    CHECK(plan.miner<0); /* No constructive first step for the pair or its support. */
    puts("Flag race, bomb knowledge, hidden invariance and equal-rank assault OK");return 0;
}
