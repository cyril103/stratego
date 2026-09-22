/* Read-only diagnostics of the public information used by Improved. */
#include "../src/ai.c"
#include "replay.h"
int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"Usage: strategy_probe replay.jsonl before_ply\n");return 1;}
    Game game,view;int remaining[12];float p[100][12];
    if(!replay_load(argv[1],atoi(argv[2]),&game))return 2;
    public_board(&game,&view,remaining);probabilities(&view,remaining,p);
    printf("Before %d, side %d, public flag pressure %.2f, force balance %.3f\n",
           game.ply+1,game.turn,ai_flag_risk(&view,game.turn),force_advantage(&view));
    for(int s=0;s<100;s++)if(view.board[s].side==1-game.turn&&view.board[s].moved)
        printf("Enemy %d: public rank %d, miner %.3f, spy %.3f, marshal %.3f\n",
               s,view.board[s].rank,p[s][MINER],p[s][SPY],p[s][MARSHAL]);
    Move moves[MAX_MOVES];int count=game_moves(&game,game.turn,moves);
    InterceptPlan intercept;intercept_plan(&view,p,&intercept);
    for(int i=0;i<count;i++){
        Move m=moves[i];
        Game next=optimistic_move(&view,m);next.turn=1-game.turn;
        printf("Post-move flag pressure %.2f; ",ai_flag_risk(&next,game.turn));
        printf("%d -> %d: rank %d, race %.2f, exposure %.2f, escort defense %.2f, intercept %.2f, recall %.2f\n",
               m.from,m.to,view.board[m.from].rank,flag_race_cost(&view,p,m),
               expected_threat(&view,p,m,true),escorted_defense(&view,m),
               intercept_bonus(&view,p,&intercept,m),recall_officer(&view,m));
    }
    return 0;
}
