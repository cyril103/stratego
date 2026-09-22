#include "../src/ai.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Army care line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g;game_clear(&g);g.turn=COMPUTER;
    g.board[30]=(Piece){GENERAL,HUMAN,0,true,true};
    g.board[31]=(Piece){LIEUTENANT,COMPUTER,40,true,true};
    g.board[21]=(Piece){CAPTAIN,COMPUTER,41,true,true};
    float before=army_exposure(&g,COMPUTER);
    CHECK(before>0);
    CHECK(collective_retreat(&g,(Move){31,32},before)>0);
    Game hidden=g;hidden.board[30].revealed=false;hidden.board[30].rank=-2;
    CHECK(army_exposure(&hidden,COMPUTER)==0);
    Game single=g;single.board[21]=empty_piece();
    CHECK(army_exposure(&single,COMPUTER)<before);
    g.board[20]=(Piece){MARSHAL,COMPUTER,42,true,true};
    CHECK(army_exposure(&g,COMPUTER)<before);
    game_clear(&g);g.turn=COMPUTER;float p[100][12]={{0}};
    g.board[31]=(Piece){MINER,COMPUTER,40,true,true};
    g.captured[COMPUTER][MINER]=army_counts[MINER]-1;
    g.board[32]=(Piece){BOMB,HUMAN,0,false,false};p[32][BOMB]=1;
    CHECK(miner_capability_cost(&g,p,(Move){31,32})==0);
    p[32][BOMB]=0;p[32][CAPTAIN]=1;
    float last=miner_capability_cost(&g,p,(Move){31,32});CHECK(last>worth[MINER]);
    g.captured[COMPUTER][MINER]--;CHECK(miner_capability_cost(&g,p,(Move){31,32})<last);
    g.board[32]=empty_piece();memset(p,0,sizeof(p));
    g.board[33]=(Piece){GENERAL,HUMAN,1,true,true};p[33][GENERAL]=1;
    CHECK(miner_capability_cost(&g,p,(Move){31,32})>0);
    g.board[32]=(Piece){FLAG,HUMAN,0,true,false};p[32][FLAG]=1;
    CHECK(miner_capability_cost(&g,p,(Move){31,32})==0);
    g.captured[HUMAN][BOMB]=army_counts[BOMB];p[32][FLAG]=0;p[32][GENERAL]=1;
    CHECK(miner_capability_cost(&g,p,(Move){31,32})==0);
    game_clear(&g);g.turn=COMPUTER;
    g.board[21]=(Piece){LIEUTENANT,HUMAN,0,true,true};
    g.board[20]=(Piece){GENERAL,COMPUTER,40,true,true};
    g.board[13]=(Piece){COLONEL,COMPUTER,41,true,true};
    CHECK(pincer_bonus(&g,(Move){13,12})>0);
    CHECK(pincer_bonus(&g,(Move){13,14})==0);
    Game masked=g;masked.board[21].revealed=false;masked.board[21].rank=-2;
    CHECK(pincer_bonus(&masked,(Move){13,12})==0);
    g.board[20]=empty_piece();CHECK(pincer_bonus(&g,(Move){13,12})==0);
    g.board[20]=(Piece){GENERAL,COMPUTER,40,true,true};
    g.board[2]=(Piece){MARSHAL,HUMAN,1,true,true};
    CHECK(pincer_bonus(&g,(Move){13,12})==0);
    puts("Army care: retreat, miners and coordinated legal escape control passed.");return 0;
}
