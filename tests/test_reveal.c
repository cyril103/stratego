#include "reveal_window.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Reveal check %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(Game *g,int s,int rank,int side,int id){g->board[s]=(Piece){rank,side,id,false,false};}
int main(void){
    Game g;RevealWindow v;game_clear(&g);reveal_reset(&v);
    put(&g,99,FLAG,HUMAN,0);put(&g,0,FLAG,COMPUTER,40);
    put(&g,64,SCOUT,HUMAN,1);put(&g,65,SCOUT,HUMAN,2);put(&g,80,SCOUT,HUMAN,3);put(&g,54,CAPTAIN,COMPUTER,41);
    CHECK(!reveal_recent(&v,&g,g.board[54]));
    CHECK(game_apply(&g,(Move){64,54}));reveal_observe(&v,&g);
    CHECK(reveal_recent(&v,&g,g.board[54])&&g.board[54].revealed);
    /* Identity follows a moving survivor; the next completed half-turn hides it. */
    CHECK(game_apply(&g,(Move){54,55}));reveal_observe(&v,&g);
    CHECK(!reveal_recent(&v,&g,g.board[55])&&g.board[55].revealed);
    CHECK(game_apply(&g,(Move){65,55}));reveal_observe(&v,&g);
    CHECK(reveal_recent(&v,&g,g.board[55]));
    CHECK(game_apply(&g,(Move){55,54}));reveal_observe(&v,&g);
    CHECK(!reveal_recent(&v,&g,g.board[54])&&g.board[54].revealed);
    game_clear(&g);reveal_reset(&v);g.turn=COMPUTER;
    put(&g,99,FLAG,HUMAN,0);put(&g,0,FLAG,COMPUTER,40);put(&g,30,SCOUT,COMPUTER,41);put(&g,80,SCOUT,HUMAN,1);
    CHECK(game_apply(&g,(Move){30,60}));reveal_observe(&v,&g);CHECK(reveal_recent(&v,&g,g.board[60]));
    CHECK(game_apply(&g,(Move){80,81}));reveal_observe(&v,&g);CHECK(!reveal_recent(&v,&g,g.board[60]));
    CHECK(g.board[60].revealed);reveal_reset(&v);CHECK(!reveal_recent(&v,&g,g.board[60]));
    CHECK(!reveal_recent(&v,&g,empty_piece()));
    puts("Temporary reveal: combat, next half-turn, identity tracking, renewed combat, scout and permanent memory OK");return 0;
}
