#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Audit miner line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
int main(void){
    const char *files[]={STRATEGO_AUDIT_MINER_FIXTURES "/audit_miner_9231.jsonl",STRATEGO_AUDIT_MINER_FIXTURES "/audit_miner_9232.jsonl"};
    int plies[]={612,420},miners[]={13,37};Move failures[]={{21,22},{26,27}},escapes[]={{13,12},{37,36}};
    for(int i=0;i<2;i++){
        Game g,v;int rem[12];float p[100][12];CHECK(replay_load(files[i],plies[i],&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        CHECK(threatened_last_miner(&v)==miners[i]);
        CHECK(!saves_last_miner(&v,p,failures[i],miners[i]));
        CHECK(saves_last_miner(&v,p,escapes[i],miners[i]));
        if(i==1)CHECK(known_unanswered_loss(&v,failures[i])==0); /* Recapture hid the capability loss. */
        Game no_bombs=v;no_bombs.captured[HUMAN][BOMB]=army_counts[BOMB];CHECK(threatened_last_miner(&no_bombs)<0);
        Game reserve=v;reserve.captured[COMPUTER][MINER]--;CHECK(threatened_last_miner(&reserve)<0);
        Game concealed=v;int attacker=i?38:23;concealed.board[attacker].revealed=false;concealed.board[attacker].rank=-2;
        CHECK(!known_miner_threat(&concealed,miners[i]));
        for(uint32_t seed=1;seed<=8;seed++){
            uint32_t rng=seed,other_rng=seed;Game other=swapped(g);
            Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&other_rng);
            CHECK(a.from==b.from&&a.to==b.to&&rng==other_rng);
            CHECK(game_legal(&g,a,g.turn)&&saves_last_miner(&v,p,a,miners[i]));
            CHECK(a.from==miners[i]);Game next=g;CHECK(game_apply(&next,a));
            CHECK(next.board[a.to].rank==MINER&&next.board[a.to].side==COMPUTER);
            CHECK(!game_legal(&next,(Move){attacker,a.to},HUMAN));
            printf("Miner rescue before %d seed%u: %d>%d\n",plies[i],seed,a.from,a.to);fflush(stdout);
        }
    }
    /* Winning now still outranks the capability rescue. */
    Game win;CHECK(replay_load(files[1],420,&win));win.board[32]=(Piece){FLAG,HUMAN,33,true,false};
    uint32_t rng=1;Move m=ai_choose(&win,1,&rng);CHECK(m.from==31&&m.to==32);
    /* A lone captain cannot cover all flag gates against a larger army just
       because every surviving enemy has a lower rank. */
    Game g,v;int rem[12];CHECK(replay_load(STRATEGO_AUDIT_MINER_FIXTURES "/audit_guard_9234.jsonl",342,&g));
    public_board(&g,&v,rem);
    CHECK(last_officer_trade_cost(&v,(Move){21,31})>0);
    CHECK(last_officer_trade_cost(&v,(Move){21,20})==0);
    for(uint32_t seed=1;seed<=8;seed++){
        rng=seed;uint32_t other_rng=seed;Game other=swapped(g);
        Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&other_rng);
        CHECK(a.from==b.from&&a.to==b.to&&rng==other_rng);
        CHECK(a.from==21&&a.to==20);
        printf("Guard coverage before 342 seed%u: %d>%d\n",seed,a.from,a.to);fflush(stdout);
    }
    puts("Audit miner: both avoidable last-miner losses, public knowledge and flag precedence passed.");return 0;
}
