#include "ai_strategy.h"
#include "ai_belief.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Strategy line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,true,true};}
static bool hinted(Game *g,int from,int to){StrategyHint hints[MAX_STRATEGY_HINTS];int n=ai_strategy_hints(g,hints);for(int i=0;i<n;i++)if(hints[i].move.from==from&&hints[i].move.to==to)return true;return false;}
int main(void){
    Game g;game_clear(&g);g.turn=COMPUTER;
    put(&g,4,FLAG,COMPUTER);put(&g,14,CAPTAIN,COMPUTER);
    put(&g,23,MINER,HUMAN);put(&g,25,MINER,HUMAN);
    float two=ai_flag_risk(&g,COMPUTER);
    Game left=g;left.board[25]=empty_piece();Game right=g;right.board[23]=empty_piece();
    float separate=ai_flag_risk(&left,COMPUTER)+ai_flag_risk(&right,COMPUTER);
    printf("Two intruders %.2f, isolated risks %.2f\n",two,separate);
    CHECK(two>separate);
    put(&g,15,CAPTAIN,COMPUTER);CHECK(ai_flag_risk(&g,COMPUTER)<two);
    g.board[23].revealed=false;g.board[25].revealed=false;Game hidden=g;
    hidden.board[23].rank=MARSHAL;hidden.board[25].rank=SPY;
    CHECK(ai_flag_risk(&g,COMPUTER)==ai_flag_risk(&hidden,COMPUTER));
    game_clear(&g);g.turn=COMPUTER;put(&g,65,MINER,COMPUTER);put(&g,45,CAPTAIN,COMPUTER);
    CHECK(ai_coordination_bonus(&g,(Move){45,55})>ai_coordination_bonus(&g,(Move){45,35}));
    float fresh=ai_coordination_bonus(&g,(Move){45,55});
    g.history_count[COMPUTER]=3;g.history_id[COMPUTER][0]=45;
    g.history_id[COMPUTER][1]=65;g.history_id[COMPUTER][2]=65;
    CHECK(ai_coordination_bonus(&g,(Move){45,55})>fresh);
    put(&g,54,MARSHAL,HUMAN);CHECK(ai_coordination_bonus(&g,(Move){45,55})==0);
    game_clear(&g);g.turn=COMPUTER;
    put(&g,7,FLAG,COMPUTER);put(&g,6,BOMB,COMPUTER);put(&g,8,BOMB,COMPUTER);put(&g,17,BOMB,COMPUTER);
    put(&g,26,MAJOR,COMPUTER);put(&g,27,LIEUTENANT,COMPUTER);put(&g,18,CAPTAIN,HUMAN);put(&g,28,LIEUTENANT,HUMAN);
    CHECK(hinted(&g,27,37));
    Game mirror=g;for(int s=0;s<100;s++)mirror.board[s/10*10+9-s%10]=g.board[s];
    CHECK(hinted(&mirror,22,32));
    game_clear(&g);
    int cluster[]={7,6,8,17};
    for(int i=0;i<4;i++)g.board[cluster[i]]=(Piece){CAPTAIN,COMPUTER,cluster[i],false,false};
    float early=ai_rank_prior(&g,7,FLAG);g.ply=280;
    CHECK(ai_rank_prior(&g,7,FLAG)>early);
    float grouped=ai_rank_prior(&g,7,FLAG);
    Game unknown=g;unknown.board[6].rank=BOMB;unknown.board[8].rank=FLAG;
    CHECK(ai_rank_prior(&unknown,7,FLAG)==grouped);
    g.board[6].moved=g.board[8].moved=g.board[17].moved=true;
    CHECK(ai_rank_prior(&g,7,FLAG)<grouped);
    CHECK(ai_rank_prior(&g,7,CAPTAIN)>0);
    g.board[7].moved=true;CHECK(ai_rank_prior(&g,7,FLAG)==0&&ai_rank_prior(&g,7,BOMB)==0);
    game_clear(&g);g.turn=COMPUTER;
    put(&g,8,FLAG,COMPUTER);put(&g,7,BOMB,COMPUTER);put(&g,9,BOMB,COMPUTER);put(&g,18,BOMB,COMPUTER);
    put(&g,27,CAPTAIN,COMPUTER);put(&g,29,SERGEANT,COMPUTER);put(&g,35,MINER,HUMAN);
    float reserve=ai_preservation_risk(&g,COMPUTER);
    Game departure=g;departure.board[28]=departure.board[27];departure.board[27]=empty_piece();
    CHECK(ai_preservation_risk(&departure,COMPUTER)>reserve);
    put(&departure,17,LIEUTENANT,COMPUTER);CHECK(ai_preservation_risk(&departure,COMPUTER)<=reserve);
    g.board[35].revealed=false;hidden=g;hidden.board[35].rank=SPY;
    CHECK(ai_preservation_risk(&g,COMPUTER)==ai_preservation_risk(&hidden,COMPUTER));
    mirror=g;for(int s=0;s<100;s++)mirror.board[s/10*10+9-s%10]=g.board[s];
    CHECK(ai_preservation_risk(&g,COMPUTER)==ai_preservation_risk(&mirror,COMPUTER));
    game_clear(&g);g.turn=COMPUTER;put(&g,70,GENERAL,COMPUTER);put(&g,63,MARSHAL,HUMAN);
    float pursuit=ai_preservation_risk(&g,COMPUTER);CHECK(pursuit>0);
    Game retreat=g;retreat.board[60]=retreat.board[70];retreat.board[70]=empty_piece();
    CHECK(ai_preservation_risk(&retreat,COMPUTER)<pursuit);
    g.board[63].revealed=false;CHECK(ai_preservation_risk(&g,COMPUTER)==0);
    game_clear(&g);g.turn=COMPUTER;
    put(&g,8,FLAG,COMPUTER);put(&g,7,BOMB,COMPUTER);put(&g,9,BOMB,COMPUTER);put(&g,18,BOMB,COMPUTER);
    put(&g,17,MINER,HUMAN);put(&g,16,MARSHAL,HUMAN);
    float unguarded=ai_flag_risk(&g,COMPUTER);
    put(&g,27,CAPTAIN,COMPUTER);
    CHECK(ai_flag_risk(&g,COMPUTER)<unguarded);
    puts("Strategy, pursuit, reserves and public beliefs: OK");return 0;
}
