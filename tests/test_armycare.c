#include "../src/ai.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Army care line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g;game_clear(&g);g.turn=COMPUTER;
    g.board[30]=(Piece){GENERAL,HUMAN,0,true,true};
    g.board[31]=(Piece){LIEUTENANT,COMPUTER,40,true,true};
    g.board[21]=(Piece){CAPTAIN,COMPUTER,41,true,true};
    float before=army_exposure(&g,COMPUTER);
    CHECK(before>worth[LIEUTENANT]);
    CHECK(collective_retreat(&g,(Move){31,32},before)>0);
    Game hidden=g;hidden.board[30].revealed=false;hidden.board[30].rank=-2;
    CHECK(army_exposure(&hidden,COMPUTER)==0);
    Game single=g;single.board[21]=empty_piece();
    CHECK(army_exposure(&single,COMPUTER)<before);
    g.board[20]=(Piece){MARSHAL,COMPUTER,42,true,true};
    CHECK(army_exposure(&g,COMPUTER)<before);
    puts("Collective retreat: multiple victims, hidden ranks and support passed.");return 0;
}
