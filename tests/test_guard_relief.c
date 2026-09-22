#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Guard relief line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game permuted(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==1-g.turn&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }
    return g;
}
static int rotated(int s){return s<0?s:99-s;}
static Game reverse_sides(Game g){
    Game result=g;result.turn=1-g.turn;
    for(int s=0;s<100;s++){Piece p=g.board[s];if(p.side>=0)p.side=1-p.side;result.board[99-s]=p;}
    result.last_move=(Move){rotated(g.last_move.from),rotated(g.last_move.to)};
    for(int side=0;side<2;side++){
        memcpy(result.captured[1-side],g.captured[side],sizeof(g.captured[side]));
        result.last_id[1-side]=g.last_id[side];result.last_from[1-side]=rotated(g.last_from[side]);
        result.last_to[1-side]=rotated(g.last_to[side]);result.repetitions[1-side]=g.repetitions[side];
        result.history_count[1-side]=g.history_count[side];
        for(int i=0;i<8;i++){
            result.history_id[1-side][i]=g.history_id[side][i];
            result.history[1-side][i]=(Move){rotated(g.history[side][i].from),rotated(g.history[side][i].to)};
        }
    }
    return result;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];InterceptPlan threats;
    CHECK(replay_load(STRATEGO_RELIEF_FIXTURE,0,&g));CHECK(g.winner==HUMAN&&g.ply==577);
    CHECK(replay_load(STRATEGO_RELIEF_FIXTURE,568,&g));
    public_board(&g,&v,remaining);probabilities(&v,remaining,p);intercept_plan(&v,p,&threats);
    CHECK(v.board[41].revealed&&v.board[41].rank==MINER); /* Public deduction. */
    GuardRelief plan=guard_relief_plan(&v,&threats);
    CHECK(plan.guard==12&&plan.pinner==32&&plan.step.from==3&&plan.step.to==13);
    /* An installed replacement frees this sergeant, even if another scout
       ray elsewhere still calls for a different guard's relief. */
    Game covered=g;covered.board[22]=covered.board[3];covered.board[3]=empty_piece();
    public_board(&covered,&v,remaining);probabilities(&v,remaining,p);intercept_plan(&v,p,&threats);
    CHECK(guard_relief_plan(&v,&threats).guard!=12);
    /* Mirroring the camps preserves the same tactical assignment. */
    Game reversed=reverse_sides(g);public_board(&reversed,&v,remaining);
    probabilities(&v,remaining,p);intercept_plan(&v,p,&threats);plan=guard_relief_plan(&v,&threats);
    CHECK(plan.guard==87&&plan.pinner==67&&plan.step.from==96&&plan.step.to==86);
    uint32_t seeds[]={1,2,3,519};
    for(int i=0;i<4;i++){
        CHECK(replay_load(STRATEGO_RELIEF_FIXTURE,568,&g));Game other=permuted(g);
        uint32_t rng=seeds[i],same_rng=rng;Move m=ai_choose(&g,1,&rng),same=ai_choose(&other,1,&same_rng);
        CHECK(m.from==same.from&&m.to==same.to&&rng==same_rng);
        CHECK(m.from==3&&m.to==13);CHECK(game_apply(&g,m));
        Move attack[]={{41,31},{31,21},{21,11}};
        for(int j=0;j<3;j++){
            CHECK(game_apply(&g,attack[j]));m=ai_choose(&g,1,&rng);
            printf("Relief seed %u ply %d: %d -> %d\n",seeds[i],g.ply+1,m.from,m.to);fflush(stdout);
            CHECK(game_apply(&g,m));CHECK(g.winner!=HUMAN);
        }
        CHECK(g.captured[HUMAN][MINER]==army_counts[MINER]);
        CHECK(g.board[2].side==COMPUTER&&g.board[2].rank==FLAG);
        CHECK(!game_legal(&g,(Move){32,2},HUMAN));
    }
    /* Remote survivors must not disable the local proof. Restore captured
       scouts legally to make an army larger than any former small-army cap. */
    CHECK(replay_load(STRATEGO_RELIEF_FIXTURE,574,&g));
    int squares[]={60,62,64,66,68,70,72,74};bool used[80]={false};
    for(int s=0;s<100;s++)if(g.board[s].side>=0)used[g.board[s].id]=true;
    for(int i=0;i<8;i++){
        CHECK(g.board[squares[i]].side<0&&g.captured[COMPUTER][SCOUT]>0);
        int id=40;while(id<80&&used[id])id++;CHECK(id<80);used[id]=true;
        g.board[squares[i]]=(Piece){SCOUT,COMPUTER,id,true,true};g.captured[COMPUTER][SCOUT]--;
    }
    public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    CHECK(guard_tempo_risk(&v,p,(Move){12,13})>0);
    /* A broader small-ending trigger exhausted the proof budget on a losing
       sacrifice and preferred it to retreats. Preserve the tested small-army
       behavior while extending the formerly disabled large-army cases. */
    CHECK(replay_load(STRATEGO_RELIEF_REGRESSION,762,&g));
    uint32_t rng=519;Move m=ai_choose(&g,1,&rng);
    printf("Small ending before 762: %d -> %d\n",m.from,m.to);fflush(stdout);
    CHECK(m.from==21&&m.to==31);
    CHECK(replay_load(STRATEGO_RELIEF_REGRESSION,764,&g));rng=519;m=ai_choose(&g,1,&rng);
    printf("Small ending before 764: %d -> %d\n",m.from,m.to);fflush(stdout);
    CHECK(!(m.from==22&&m.to==12));
    puts("Pinned guard relief stops the recorded attack on four seeds; mirrored, hidden-rank and small-ending checks pass.");return 0;
}
