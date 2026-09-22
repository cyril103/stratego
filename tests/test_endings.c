#include "game.h"
#include "replay.h"
#include "ai_strategy.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Endings line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,true,true};}
static void record(const char *path,const Game *initial,const Game *end){
    FILE *f=fopen(path,"w");if(!f)return;
    fprintf(f,"{\"version\":1,\"rules\":\"ISF-endings-v1\",\"turn\":%d,\"board\":[",initial->turn);
    for(int s=0;s<100;s++)fprintf(f,"%s[%d,%d,%d]",s?",":"",initial->board[s].side,initial->board[s].rank,initial->board[s].id);
    fprintf(f,"]}\n{\"end\":true,\"winner\":%d,\"ply\":%d,\"reason\":%d}\n",end->winner,end->ply,end->end_reason);fclose(f);
}
int main(void){
    Game g;game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    game_check_end(&g);CHECK(g.winner==GAME_DRAW&&g.end_reason==END_BOTH_IMMOBILE);
    CHECK(!game_apply(&g,(Move){99,98}));CHECK(!game_resign(&g,HUMAN));
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,60,GENERAL,HUMAN);put(&g,50,GENERAL,COMPUTER);
    CHECK(game_apply(&g,(Move){60,50}));CHECK(g.winner==GAME_DRAW);
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,60,SERGEANT,HUMAN);put(&g,50,BOMB,COMPUTER);put(&g,9,SCOUT,COMPUTER);
    CHECK(game_apply(&g,(Move){60,50}));CHECK(g.turn==COMPUTER&&g.winner==COMPUTER&&g.end_reason==END_IMMOBILE);
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);put(&g,9,SCOUT,COMPUTER);
    put(&g,90,SCOUT,HUMAN);put(&g,80,BOMB,HUMAN);put(&g,91,BOMB,HUMAN);
    game_check_end(&g);CHECK(g.winner==COMPUTER);
    Game start,loaded;game_init(&start,123);g=start;
    CHECK(!game_agree_draw(&g,true,false)&&g.winner==GAME_ONGOING);
    CHECK(game_agree_draw(&g,true,true)&&g.winner==GAME_DRAW&&g.end_reason==END_AGREEMENT);
    record("ending_test.jsonl",&start,&g);CHECK(replay_load("ending_test.jsonl",0,&loaded));CHECK(loaded.end_reason==END_AGREEMENT);
    g=start;CHECK(game_end_playing_period(&g)&&g.winner==GAME_DRAW);
    record("ending_test.jsonl",&start,&g);CHECK(replay_load("ending_test.jsonl",0,&loaded));CHECK(loaded.end_reason==END_PLAYING_PERIOD);
    g=start;CHECK(game_resign(&g,HUMAN)&&g.winner==COMPUTER);
    record("ending_test.jsonl",&start,&g);CHECK(replay_load("ending_test.jsonl",0,&loaded));CHECK(loaded.end_reason==END_RESIGNATION);remove("ending_test.jsonl");
    CHECK(!game_end_playing_period(&g)&&g.winner==COMPUTER);
    g=start;g.ply=700;game_check_end(&g);CHECK(g.winner==GAME_ONGOING);
    g=start;CHECK(!ai_accept_draw(&g,COMPUTER));g.ply=120;CHECK(ai_accept_draw(&g,COMPUTER));
    g.captured[HUMAN][MARSHAL]=1;CHECK(!ai_accept_draw(&g,COMPUTER));
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,90,LIEUTENANT,HUMAN);put(&g,9,LIEUTENANT,COMPUTER);
    for(int s=0;s<2;s++)for(int r=0;r<12;r++)g.captured[s][r]=army_counts[r];
    for(int s=0;s<2;s++){g.captured[s][FLAG]--;g.captured[s][LIEUTENANT]--;}
    g.ply=201;g.turn=COMPUTER;
    CHECK(ai_offer_draw(&g,COMPUTER,100,-100));
    CHECK(!ai_offer_draw(&g,COMPUTER,122,-100)); /* Recent combat. */
    CHECK(!ai_offer_draw(&g,COMPUTER,0,102)); /* Offer cooldown. */
    CHECK(ai_offer_draw(&g,COMPUTER,121,101)); /* Exact boundaries. */
    g.ply=199;CHECK(!ai_offer_draw(&g,COMPUTER,0,-100));g.ply=201;
    g.turn=HUMAN;CHECK(!ai_offer_draw(&g,COMPUTER,0,-100));g.turn=COMPUTER;
    put(&g,19,LIEUTENANT,HUMAN);g.board[90]=empty_piece();
    CHECK(!ai_offer_draw(&g,COMPUTER,0,-100)); /* Keep exploring attacks. */
    g.board[19].rank=MARSHAL;g.board[19].revealed=false;
    CHECK(!ai_offer_draw(&g,COMPUTER,0,-100)); /* Hidden rank doesn't matter. */
    g.board[19]=empty_piece();put(&g,90,LIEUTENANT,HUMAN);
    g.captured[HUMAN][MARSHAL]--;CHECK(!ai_offer_draw(&g,COMPUTER,0,-100));g.captured[HUMAN][MARSHAL]++;
    CHECK(game_agree_draw(&g,true,true));CHECK(!ai_offer_draw(&g,COMPUTER,0,-100));
    puts("ISF endings: mutual immobility, agreement, period, resignation and replays OK");return 0;
}
