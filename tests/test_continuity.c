#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Continuity line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;}
    }return g;
}
int main(void){
    uint32_t seeds[]={1,2,3,519};
    for(int i=0;i<4;i++){
        Game g;CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,424,&g));
        Game other=swapped(g);uint32_t rng=seeds[i],same=rng;
        Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);CHECK(a.from==88&&a.to!=89);
        CHECK(game_apply(&g,a));
        while(g.winner<0&&g.ply<434){
            Move m;if(g.turn==COMPUTER)m=ai_choose(&g,1,&rng);
            else{Game original;CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,g.ply+2,&original));m=original.last_move;}
            CHECK(game_apply(&g,m));
        }
        CHECK(g.winner==COMPUTER&&g.end_reason==END_FLAG);
        CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,532,&g));other=swapped(g);rng=seeds[i];same=rng;
        a=ai_choose(&g,1,&rng);b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);CHECK(a.from==24&&a.to==23);
        CHECK(game_apply(&g,a));
        Move replies[]={{55,45},{30,20},{31,21}};
        for(int j=0;j<3;j++){CHECK(game_apply(&g,replies[j]));a=ai_choose(&g,1,&rng);CHECK(game_apply(&g,a));}
        CHECK(g.board[20].side==COMPUTER&&g.board[20].rank==MARSHAL);
        CHECK(g.board[11].side==COMPUTER&&g.board[11].rank==LIEUTENANT);
        CHECK(g.board[2].rank==FLAG&&g.winner<0);
        printf("Continuity seed %u: miner flag win and coordinated interception passed\n",seeds[i]);fflush(stdout);
    }
    Game g,v;int r[12];float p[100][12];
    CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,482,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    ContinuityPlan c=reserve_support_plan(&v,p);CHECK(c.guard==31&&c.step.from==34&&c.step.to==33);
    uint32_t rng=519;Move m=ai_choose(&g,1,&rng);CHECK(m.from==34&&m.to==33);
    CHECK(game_apply(&g,m));CHECK(game_apply(&g,(Move){8,7}));m=ai_choose(&g,1,&rng);CHECK(m.from==33&&m.to==32);
    /* A known spy beside the reinforcement station must never be treated as
       the small, supported uncertainty allowed during a flag emergency. */
    CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,532,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    v.board[31].rank=SPY;v.board[31].revealed=true;memset(p[31],0,sizeof(p[31]));p[31][SPY]=1;
    c=reserve_support_plan(&v,p);CHECK(c.goal!=21);
    v.board[23]=(Piece){SPY,HUMAN,35,true,true};memset(p[23],0,sizeof(p[23]));p[23][SPY]=1;
    c=reserve_support_plan(&v,p);CHECK(c.guard<0);
    /* With no alternate path, preserving a miner must not prohibit its only
       flag attempt. The mission is advisory and requires a safe first step. */
    CHECK(replay_load(STRATEGO_CONTINUITY_FIXTURE,424,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    v.board[87]=(Piece){MARSHAL,HUMAN,30,true,true};memset(p[87],0,sizeof(p[87]));p[87][MARSHAL]=1;
    c=last_miner_plan(&v,p);CHECK(c.step.to!=87);
    CHECK(replay_load(STRATEGO_CONTINUITY_REGRESSION,768,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    Move moves[MAX_MOVES];int n=game_moves(&g,g.turn,moves);
    CHECK(continuity_capture(&v,p,moves,n).from<0);
    puts("Scarce-piece continuity and public-information checks passed.");return 0;
}
