/* Diagnostic executable: compile with game, replay, strategy, basic AI, ML,
   and parallel support. ai.c is included to inspect its public-only maps. */
#include "../src/ai.c"
#include "replay.h"
int main(int argc,char **argv){
    if(argc<3){fprintf(stderr,"Usage: intercept_analyze replay.jsonl before_ply\n");return 1;}
    Game g,view;int remaining[12];float p[100][12];
    if(!replay_load(argv[1],atoi(argv[2]),&g))return 2;
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    InterceptPlan plan;intercept_plan(&view,p,&plan);
    for(int i=0;i<plan.count;i++){
        InterceptThreat *t=&plan.threats[i];Piece enemy=view.board[t->enemy];
        printf("Threat %d %s: flag arrival %d, probability %.3f\n",t->enemy,
               enemy.revealed?rank_names[enemy.rank]:"unknown / miner hypothesis",t->arrival,t->probability);
    }
    Move moves[MAX_MOVES];int count=game_moves(&g,g.turn,moves);
    for(int i=0;i<count;i++){
        float bonus=intercept_bonus(&view,p,&plan,moves[i]);
        if(bonus)printf("%s %d -> %d: reserve %.2f\n",rank_names[g.board[moves[i].from].rank],moves[i].from,moves[i].to,bonus);
    }
    return 0;
}
