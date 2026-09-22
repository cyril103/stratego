#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Officer team line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==HUMAN&&b.side==HUMAN&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }
    return g;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];uint32_t seeds[]={1,2,3,519};
    CHECK(replay_load(STRATEGO_TEAM_FIXTURE,0,&g));CHECK(g.winner==HUMAN&&g.ply==379);
    CHECK(replay_load(STRATEGO_TEAM_FIXTURE,108,&g));public_board(&g,&v,remaining);
    printf("General cage %.1f escape %.1f\n",officer_trap_cost(&v,(Move){38,28}),officer_trap_cost(&v,(Move){38,39}));fflush(stdout);
    CHECK(officer_trap_cost(&v,(Move){38,28})>officer_trap_cost(&v,(Move){38,39}));
    const int positions[]={108,226,312};
    for(int pos=0;pos<3;pos++){
        CHECK(replay_load(STRATEGO_TEAM_FIXTURE,positions[pos],&g));Game other=swapped(g);
        for(int i=0;i<4;i++){
            uint32_t a=seeds[i],b=a;Move m=ai_choose(&g,1,&a),same=ai_choose(&other,1,&b);
            printf("Team before %d seed %u: %d -> %d\n",g.ply+1,seeds[i],m.from,m.to);fflush(stdout);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==same.from&&m.to==same.to&&a==b);
            if(pos==2)CHECK(!(m.from==24&&m.to==25));
            else if(pos==1)CHECK(m.from==18&&m.to==17);
            else CHECK(m.from==38&&m.to==39);
        }
    }
    for(int seed=0;seed<2;seed++){
        CHECK(replay_load(STRATEGO_TEAM_FIXTURE,220,&g));int colonel=g.board[28].id;
        uint32_t rng=seed?519:1;
        Move attack[]={{58,48},{48,38},{38,28},{28,29},{29,19},{19,9},{9,8}};
        for(int i=0;i<7;i++){
            Move m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
            CHECK(game_apply(&g,attack[i]));
        }
        bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&g.board[s].id==colonel)alive=true;
        CHECK(alive);
    }
    CHECK(replay_load(STRATEGO_TEAM_FIXTURE,254,&g));public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    RaiderPlan plan;raider_plan(&v,&plan);CHECK(plan.count>0);
    bool marshal=false;for(int i=0;i<plan.count;i++)if(v.board[plan.tasks[i].guard].rank==MARSHAL)marshal=true;
    CHECK(marshal);
    game_clear(&g);g.turn=COMPUTER;
    g.board[0]=(Piece){FLAG,COMPUTER,40,false,false};
    g.board[44]=(Piece){GENERAL,COMPUTER,41,true,true};
    g.board[24]=(Piece){SPY,COMPUTER,42,false,true};
    g.board[64]=(Piece){MARSHAL,HUMAN,0,true,true};
    g.board[99]=(Piece){FLAG,HUMAN,1,false,false};
    public_board(&g,&v,remaining);
    CHECK(general_escort_bonus(&v,(Move){24,34})>0);
    uint32_t rng=519;Move escort=ai_choose(&g,1,&rng);
    printf("Escort choice %d -> %d\n",escort.from,escort.to);fflush(stdout);
    CHECK(general_escort_bonus(&v,escort)>0);
    CHECK(game_apply(&g,(Move){24,34}));CHECK(game_apply(&g,(Move){64,54}));
    public_board(&g,&v,remaining);int budget=1600;
    v.turn=HUMAN;CHECK(!officer_trapped(&v,COMPUTER,41,3,&budget));
    /* Inspect the capture/recapture tactic independently of an intervening
       defender move: the dedicated search above checks legal turn sequences. */
    g.turn=HUMAN;CHECK(game_apply(&g,(Move){54,44}));
    CHECK(game_apply(&g,(Move){34,44}));CHECK(g.board[44].side==COMPUTER&&g.board[44].rank==SPY);
    /* A known scout can attack the escort station along its clear file. */
    game_clear(&g);g.turn=COMPUTER;
    g.board[44]=(Piece){GENERAL,COMPUTER,41,true,true};g.board[24]=(Piece){SPY,COMPUTER,42,false,true};
    g.board[64]=(Piece){MARSHAL,HUMAN,0,true,true};g.board[39]=(Piece){SCOUT,HUMAN,2,true,true};
    public_board(&g,&v,remaining);CHECK(general_escort_bonus(&v,(Move){24,34})==0);
    puts("Officer retreat, interception and spy escort OK");return 0;
}
