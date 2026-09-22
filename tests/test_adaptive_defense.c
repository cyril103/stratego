#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Adaptive defense line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game permuted(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==1-g.turn&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }
    return g;
}
static void casualties(Game *g){
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0)g->captured[g->board[s].side][g->board[s].rank]--;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];
    CHECK(replay_load(STRATEGO_ADAPTIVE_FIXTURE,0,&g));CHECK(g.winner==HUMAN&&g.ply==479);
    CHECK(replay_load(STRATEGO_ADAPTIVE_FIXTURE,164,&g));public_board(&g,&v,remaining);
    CHECK(officer_trap_cost(&v,(Move){21,20})>officer_trap_cost(&v,(Move){22,32}));
    CHECK(replay_load(STRATEGO_ADAPTIVE_FIXTURE,172,&g));public_board(&g,&v,remaining);
    RaiderPlan plan;raider_plan(&v,&plan);
    CHECK(raider_bonus(&v,&plan,(Move){34,33})>0);
    int positions[]={164,166,406,434,438,460};uint32_t seeds[]={1,519};
    for(int k=0;k<6;k++){
        CHECK(replay_load(STRATEGO_ADAPTIVE_FIXTURE,positions[k],&g));Game other=permuted(g);
        public_board(&g,&v,remaining);probabilities(&v,remaining,p);raider_plan(&v,&plan);
        if(k==2)CHECK(counter_capture_cost(&v,p,(Move){39,38})>0);
        if(k==3)CHECK(counter_capture_cost(&v,p,(Move){17,18})>0);
        for(int i=0;i<2;i++){
            uint32_t a=seeds[i],b=a;Move m=ai_choose(&g,1,&a),same=ai_choose(&other,1,&b);
            printf("Adaptive before %d seed %u: %d -> %d\n",positions[k],seeds[i],m.from,m.to);fflush(stdout);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==same.from&&m.to==same.to&&a==b);
            if(k<2)CHECK(officer_trap_cost(&v,m)==0);
            if(k==2)CHECK(!(m.from==39&&m.to==38));
            if(k==3)CHECK(!(m.from==17&&m.to==18));
            if(k==4)CHECK(reserve_home_bonus(&v,p,m)>0);
            if(k==5)CHECK(raider_bonus(&v,&plan,m)>0);
        }
    }
    /* Follow the recorded encirclement from before the first blocking error.
       Stop only when an original human move ceases to be legal, and keep the
       actual stopping ply visible: this is a replay, not an optimal opponent. */
    CHECK(replay_load(STRATEGO_ADAPTIVE_FIXTURE,164,&g));
    int spy=g.board[21].id;uint32_t continuation=519;
    FILE *file=fopen(STRATEGO_ADAPTIVE_FIXTURE,"r");CHECK(file);char line[8192];
    while(fgets(line,sizeof(line),file)){
        int ply,side,from,to;
        if(sscanf(line,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d",&ply,&side,&from,&to)!=4||ply<164)continue;
        if(ply>181)break;
        Move m=side==COMPUTER?ai_choose(&g,1,&continuation):(Move){from,to};
        if(side==HUMAN&&!game_legal(&g,m,side))break;
        CHECK(game_apply(&g,m));
        bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&g.board[s].id==spy)alive=true;
        CHECK(alive);
        if(g.winner>=0)break;
    }
    fclose(file);printf("Recorded spy encirclement replayed through ply %d\n",g.ply);fflush(stdout);
    /* Do not discourage eliminating the last enemy threat, or a forced draw. */
    game_clear(&g);g.turn=COMPUTER;
    g.board[0]=(Piece){FLAG,COMPUTER,40,false,false};
    g.board[99]=(Piece){FLAG,HUMAN,0,false,false};
    g.board[34]=(Piece){CAPTAIN,COMPUTER,41,true,true};
    g.board[35]=(Piece){CAPTAIN,HUMAN,1,true,true};
    casualties(&g);public_board(&g,&v,remaining);
    CHECK(last_counter_cost(&v,(Move){34,35})==0);
    g.board[14]=(Piece){SERGEANT,COMPUTER,42,true,true};
    g.board[65]=(Piece){LIEUTENANT,HUMAN,2,false,false};
    g.board[75]=(Piece){MINER,HUMAN,3,false,false};
    casualties(&g);public_board(&g,&v,remaining);
    CHECK(last_counter_cost(&v,(Move){34,35})>0);
    /* A replacement captain makes the direct trade materially defendable. */
    g.board[24]=(Piece){CAPTAIN,COMPUTER,43,true,true};
    casualties(&g);public_board(&g,&v,remaining);
    CHECK(last_counter_cost(&v,(Move){34,35})==0);
    /* A captain, not just a miner, threatens an opened flag corridor.
       Moving the covering captain away loses; retaining its recapture wins. */
    game_clear(&g);g.turn=COMPUTER;
    g.board[0]=(Piece){FLAG,COMPUTER,40,false,false};
    g.board[10]=(Piece){LIEUTENANT,COMPUTER,41,false,false};
    g.board[11]=(Piece){CAPTAIN,COMPUTER,42,false,false};
    g.board[99]=(Piece){MINER,COMPUTER,43,false,false};
    g.board[20]=(Piece){CAPTAIN,HUMAN,0,true,true};
    g.board[90]=(Piece){FLAG,HUMAN,1,false,false};
    casualties(&g);public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    CHECK(guard_tempo_risk(&v,p,(Move){11,12})>0);
    CHECK(guard_tempo_risk(&v,p,(Move){99,98})==0);
    CHECK(replay_load(STRATEGO_PURSUIT_FIXTURE,1001,&g));public_board(&g,&v,remaining);
    CHECK(stale_pursuit_cost(&v,(Move){41,40},0)>0);
    CHECK(stale_pursuit_cost(&v,(Move){41,40},100)==0);
    for(int i=0;i<2;i++){
        Game other=permuted(g);uint32_t rng=seeds[i],same_rng=rng;Move m=ai_choose(&g,1,&rng),same=ai_choose(&other,1,&same_rng);
        printf("Break pursuit seed %u: %d -> %d\n",seeds[i],m.from,m.to);fflush(stdout);
        CHECK(game_legal(&g,m,g.turn));CHECK(stale_pursuit_cost(&v,m,0)==0);
        CHECK(m.from==same.from&&m.to==same.to&&rng==same_rng);
    }
    puts("Adaptive ranks, blocked reserves, spy escape, direct trades and pursuit progress OK");return 0;
}
