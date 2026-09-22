#ifndef STRATEGO_DEPLOYMENT_H
#define STRATEGO_DEPLOYMENT_H
#include "game.h"
typedef struct { Piece reserve[40]; } Deployment;
bool deployment_save(const Deployment *d,const Game *g,const char *path);
bool deployment_load(Deployment *d,Game *g,const char *path);
static inline void deployment_begin(Deployment *d,Game *g){
    for(int i=0;i<40;i++){d->reserve[i]=g->board[60+i];g->board[60+i]=empty_piece();}
}
static inline int deployment_count(const Deployment *d,int rank){
    int n=0;for(int i=0;i<40;i++)n+=d->reserve[i].side==HUMAN&&(rank<0||d->reserve[i].rank==rank);return n;
}
static inline bool deployment_remove(Deployment *d,Game *g,int s){
    if(s<60||s>=100||g->board[s].side!=HUMAN)return false;
    for(int i=0;i<40;i++)if(d->reserve[i].side<0){d->reserve[i]=g->board[s];g->board[s]=empty_piece();return true;}
    return false;
}
static inline bool deployment_place(Deployment *d,Game *g,int rank,int s){
    if(s<60||s>=100)return false;
    for(int i=0;i<40;i++)if(d->reserve[i].side==HUMAN&&d->reserve[i].rank==rank){Piece old=g->board[s];g->board[s]=d->reserve[i];d->reserve[i]=old;return true;}
    return false;
}
static inline void deployment_clear(Deployment *d,Game *g){for(int s=60;s<100;s++)deployment_remove(d,g,s);}
static inline bool deployment_complete(const Deployment *d,const Game *g){
    if(deployment_count(d,-1))return false;
    int counts[12]={0};bool ids[40]={0};
    for(int s=60;s<100;s++){Piece p=g->board[s];if(p.side!=HUMAN||p.rank<0||p.rank>11||p.id<0||p.id>=40||ids[p.id])return false;counts[p.rank]++;ids[p.id]=true;}
    for(int r=0;r<12;r++)if(counts[r]!=army_counts[r])return false;
    return true;
}
#endif
