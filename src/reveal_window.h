#ifndef REVEAL_WINDOW_H
#define REVEAL_WINDOW_H
#include "game.h"
/* Visual exposure is independent of permanently memorized game knowledge. */
typedef struct {int expires[80];} RevealWindow;
static inline void reveal_reset(RevealWindow *v){for(int i=0;i<80;i++)v->expires[i]=-1;}
static inline bool reveal_recent(const RevealWindow *v,const Game *g,Piece p){return p.side>=0&&p.id>=0&&p.id<80&&g->ply<v->expires[p.id];}
static inline void reveal_observe(RevealWindow *v,const Game *g){
    Move m=g->last_move;if(m.to<0||m.to>=100||m.from<0)return;
    Piece p=g->board[m.to];if(p.side<0||p.id<0||p.id>=80)return;
    int dx=m.to%10-m.from%10,dz=m.to/10-m.from/10;
    if(g->combat!=2||dx>1||dx< -1||dz>1||dz< -1)v->expires[p.id]=g->ply+1;
}
#endif
