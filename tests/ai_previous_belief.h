#ifndef STRATEGO_AI_BELIEF_H
#define STRATEGO_AI_BELIEF_H
#include "game.h"
/* Relative likelihood, never a revealed identity. Inspect only public motion
   and public neighboring ranks; stationary officers remain possible. */
static inline float ai_rank_prior(const Game *view,int s,int rank){
    Piece piece=view->board[s];
    if(piece.moved&&(rank==FLAG||rank==BOMB))return 0;
    int back=piece.side==COMPUTER?s/10:9-s/10;
    int neighbors[4]={s>=10?s-10:-1,s<90?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1};
    int stable=0,known_bombs=0;
    for(int i=0;i<4;i++)if(neighbors[i]>=0){
        Piece p=view->board[neighbors[i]];
        if(p.side!=piece.side)continue;
        if(p.revealed){if(p.rank==BOMB){known_bombs++;stable++;}}
        else if(!p.moved)stable++;
    }
    float evidence=(view->ply-40)/240.0f;
    if(evidence<0)evidence=0;
    if(evidence>1)evidence=1;
    if(piece.moved)evidence=0;
    if(rank==FLAG){
        float weight=back==0?5:back==1?2:1;
        for(int i=0;i<known_bombs;i++)weight*=2;
        return weight*(1+evidence*(1.0f+.8f*stable));
    }
    if(rank==BOMB)return (back<=1?2.0f:1.0f)*(1+evidence*(.65f+.25f*stable));
    return 1;
}
#endif
