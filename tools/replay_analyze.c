#include "replay.h"
#include "ai_strategy.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv) {
    if(argc<3){fprintf(stderr,"Usage: replay_analyze match.jsonl before_ply [seed]\n");return 1;}
    Game g;if(!replay_load(argv[1],atoi(argv[2]),&g)){fputs("Invalid replay\n",stderr);return 2;}
    uint32_t rng=argc>3?(uint32_t)strtoul(argv[3],NULL,10):519;
    if(getenv("STRATEGO_AI_TRACE_SAFETY")){
        float base=ai_preservation_risk(&g,g.turn);Move moves[MAX_MOVES];int n=game_moves(&g,g.turn,moves);
        fprintf(stderr,"Safety risk %.2f\n",base);
        for(int i=0;i<n;i++){Game next=g;Move m=moves[i];Piece p=g.board[m.from];next.board[m.from]=empty_piece();next.board[m.to]=p;
            fprintf(stderr,"Safety %s %d -> %d : %.2f\n",rank_names[p.rank],m.from,m.to,base-ai_preservation_risk(&next,g.turn));}
    }
    Move m=ai_choose(&g,1,&rng);if(m.from<0){puts("No legal move");return 0;}
    Piece a=g.board[m.from],d=g.board[m.to];
    printf("Before ply %d: %s %d -> %d; target %s (%s)\n",g.ply+1,rank_names[a.rank],m.from,m.to,d.side<0?"empty":d.revealed?rank_names[d.rank]:"unknown",d.revealed?"revealed":"hidden");
    return !game_legal(&g,m,g.turn);
}
