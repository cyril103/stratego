#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Conversion line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side,bool known){g->board[s]=(Piece){rank,side,s,known,known&&rank>FLAG&&rank<BOMB};}
static void recount(Game *g){
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0)g->captured[g->board[s].side][g->board[s].rank]--;
}
static Game hidden_swap(Game g){
    for(int a=0;a<100;a++)for(int b=a+1;b<100;b++){
        Piece x=g.board[a],y=g.board[b];
        if(x.side==1-g.turn&&y.side==x.side&&!x.revealed&&!y.revealed&&x.moved==y.moved&&x.rank!=y.rank){
            g.board[a].rank=y.rank;g.board[b].rank=x.rank;return g;
        }
    }
    return g;
}
int main(void){
    Game g,v;int left[12];float p[100][12];
    const char *files[]={STRATEGO_CONVERSION_474,STRATEGO_CONVERSION_474,STRATEGO_CONVERSION_505};
    int plies[]={362,372,424};Move rejected[]={{70,80},{82,83},{69,79}};
    for(int k=0;k<3;k++){
        CHECK(replay_load(files[k],plies[k],&g));
        public_board(&g,&v,left);probabilities(&v,left,p);
        CHECK(p[rejected[k].to][BOMB]>.25f&&p[rejected[k].to][FLAG]<.05f);
        CHECK(speculative_bomb_probe(&v,p,rejected[k]));
        Move legal[MAX_MOVES];int n=game_moves(&g,g.turn,legal);bool only_flag_hope=true;
        for(int i=0;i<n;i++){
            Piece target=v.board[legal[i].to];
            bool flag_guess=target.side==1-v.turn&&!target.revealed&&!target.moved&&p[legal[i].to][FLAG]>0;
            if(!flag_guess&&!immediate_defeat(&v,legal[i]))only_flag_hope=false;
        }
        /* At 372 a known captain in 17 can already take our flag in 7.
           There is no saving reply: keep the immediate flag gamble then. */
        CHECK(only_flag_hope==(k==1));
        /* A sole mandatory move remains available after terminal filtering. */
        Move only[]={rejected[k]};CHECK(preserve_conversion_army(&v,p,only,1)==1);
        for(uint32_t seed=1;seed<=3;seed++){
            uint32_t rng=seed,same=seed;Game swapped=hidden_swap(g);
            Move m=ai_choose(&g,1,&rng),other=ai_choose(&swapped,1,&same);
            CHECK(game_legal(&g,m,g.turn));
            CHECK(m.from==other.from&&m.to==other.to&&rng==same);
            if(!only_flag_hope)CHECK(!speculative_bomb_probe(&v,p,m));
            printf("Before %d seed %u: %d>%d\n",plies[k],seed,m.from,m.to);fflush(stdout);
        }
        /* Known flag and miner probes retain their tactical meaning. */
        Game known=g;known.board[rejected[k].to].rank=FLAG;known.board[rejected[k].to].revealed=true;
        uint32_t rng=1;Move win=ai_choose(&known,1,&rng);
        CHECK(win.to==rejected[k].to);CHECK(game_apply(&known,win));CHECK(known.winner==COMPUTER);
        v.board[rejected[k].from].rank=MINER;CHECK(!speculative_bomb_probe(&v,p,rejected[k]));
    }
    /* Material superiority should be converted by capturing mobile prey,
       even with a tempting hidden immobile target beside the same officer. */
    game_clear(&g);g.turn=COMPUTER;
    put(&g,0,FLAG,COMPUTER,true);put(&g,1,BOMB,COMPUTER,true);put(&g,10,BOMB,COMPUTER,true);
    put(&g,20,MARSHAL,COMPUTER,true);put(&g,64,COLONEL,COMPUTER,true);
    put(&g,30,SCOUT,COMPUTER,true);put(&g,19,MINER,COMPUTER,true);
    put(&g,99,FLAG,HUMAN,false);put(&g,89,BOMB,HUMAN,false);put(&g,98,BOMB,HUMAN,false);
    put(&g,65,BOMB,HUMAN,false);put(&g,74,CAPTAIN,HUMAN,true);put(&g,86,SERGEANT,HUMAN,true);recount(&g);
    public_board(&g,&v,left);probabilities(&v,left,p);RaiderPlan none={0};
    CHECK(force_advantage(&v)>.1f);
    CHECK(mobile_conversion_bonus(&v,p,&none,(Move){64,74})>0);
    CHECK(mobile_conversion_bonus(&v,p,&none,(Move){64,65})==0);
    for(uint32_t seed=1;seed<=3;seed++){
        uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);
        CHECK(m.from==64&&m.to==74);
    }
    v.board[54]=v.board[64];v.board[64]=empty_piece();
    CHECK(mobile_conversion_bonus(&v,p,&none,(Move){54,64})>0);
    CHECK(mobile_conversion_bonus(&v,p,&none,(Move){54,44})==0);
    /* Public inference is sufficient: a moved unknown weaker rank is prey. */
    v.board[74].revealed=false;v.board[74].rank=-2;
    CHECK(mobile_conversion_bonus(&v,p,&none,(Move){54,64})>0);
    puts("Conversion: actual bomb losses avoided, safe mobile captures and hidden invariance passed.");return 0;
}
