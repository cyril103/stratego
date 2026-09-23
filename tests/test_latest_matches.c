#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Latest matches line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
static bool spy_survives_reply(Game g,Move m){
    if(!game_apply(&g,m))return false;
    int spy=-1;for(int s=0;s<100;s++)if(g.board[s].side==COMPUTER&&g.board[s].rank==SPY)spy=s;
    if(spy<0)return false;
    for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&game_legal(&g,(Move){s,spy},HUMAN))return false;
    return true;
}
int main(void){
    int plies[]={168,238,260,262};
    for(int k=0;k<4;k++){
        Game g,v;int rem[12];float p[100][12];
        CHECK(replay_load(STRATEGO_LATEST_FIXTURES "/human_win_285_20260923.jsonl",plies[k],&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        if(k==0){
            CHECK(boxed_last_miner(&v)==33);
            CHECK(!opens_miner_escape(&v,p,(Move){36,35},33));
            CHECK(opens_miner_escape(&v,p,(Move){23,24},33));
            Game reserve=v;reserve.captured[COMPUTER][MINER]--;CHECK(boxed_last_miner(&reserve)<0);
            Game no_bombs=v;no_bombs.captured[HUMAN][BOMB]=army_counts[BOMB];CHECK(boxed_last_miner(&no_bombs)<0);
        }
        if(k==1){
            CHECK(active_spy_threat(&v));
            CHECK(!saves_active_spy(&v,p,(Move){25,35}));
            CHECK(saves_active_spy(&v,p,(Move){25,24}));
            CHECK(saves_active_spy(&v,p,(Move){34,35}));
            Game no_scout=v;no_scout.captured[HUMAN][SCOUT]=army_counts[SCOUT];CHECK(!active_spy_threat(&no_scout));
            Game identified=v;identified.captured[HUMAN][SCOUT]=army_counts[SCOUT]-1;
            identified.board[98]=(Piece){SCOUT,HUMAN,38,true,false};CHECK(!active_spy_threat(&identified));
            Game no_marshal=v;no_marshal.captured[HUMAN][MARSHAL]=1;CHECK(!active_spy_threat(&no_marshal));
        }
        if(k==2){
            CHECK(known_unanswered_loss(&v,(Move){15,14})>0);
            CHECK(known_unanswered_loss(&v,(Move){15,16})==0);
        }
        if(k==3){
            CHECK(last_officer_trade_cost(&v,(Move){24,14})>0);
            CHECK(flag_exchange_relief(&v,(Move){24,14})>=40);
            CHECK(!initiates_last_officer_trade(&v,(Move){24,14}));
        }
        for(uint32_t seed=1;seed<=8;seed++){
            uint32_t rng=seed,other_rng=seed;Game other=swapped(g);
            Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&other_rng);
            CHECK(a.from==b.from&&a.to==b.to&&rng==other_rng);CHECK(game_legal(&g,a,g.turn));
            if(k==0){
                CHECK(opens_miner_escape(&v,p,a,33));
                Game next=g;CHECK(game_apply(&next,a));CHECK(game_apply(&next,(Move){31,32}));
                Move escape=ai_choose(&next,1,&rng);CHECK(escape.from==33);
                CHECK(game_apply(&next,escape));CHECK(!game_legal(&next,(Move){32,escape.to},HUMAN));
            }
            if(k==1)CHECK(spy_survives_reply(g,a));
            if(k==2)CHECK(known_unanswered_loss(&v,a)==0);
            if(k==3){
                printf("Defensive exchange seed%u: %d>%d\n",seed,a.from,a.to);fflush(stdout);
                CHECK(a.from==24&&a.to==14);
                Game next=g;CHECK(game_apply(&next,a));
                /* Replay the same miner incursion, with the new defensive
                   replies, until interception makes the recorded move illegal. */
                Move raid[]={{61,51},{51,41},{41,31},{31,21},{21,11},{11,1},{1,0},{0,1},{1,2},{2,3},{3,4},{4,5}};
                for(int j=0;j<12;j++){
                    if(!game_legal(&next,raid[j],HUMAN))break;
                    if(seed==1){printf("Raid %d>%d at %d\n",raid[j].from,raid[j].to,next.ply+1);fflush(stdout);}
                    CHECK(game_apply(&next,raid[j]));CHECK(next.winner!=HUMAN);
                    if(next.winner>=0)break;
                    Move defense=ai_choose(&next,1,&rng);
                    if(seed==1){printf("Defense %d>%d\n",defense.from,defense.to);fflush(stdout);}
                    CHECK(game_apply(&next,defense));CHECK(next.winner!=HUMAN);
                    if(next.winner>=0)break;
                }
                CHECK(next.board[5].side==COMPUTER&&next.board[5].rank==FLAG);
                CHECK(next.captured[HUMAN][MINER]>g.captured[HUMAN][MINER]);
            }
            printf("Latest match ply%d seed%u: %d>%d\n",plies[k],seed,a.from,a.to);fflush(stdout);
        }
    }
    /* A guard's certain interception is also a valid rescue from a trap. */
    Game guarded;game_clear(&guarded);guarded.turn=COMPUTER;
    guarded.board[33]=(Piece){MINER,COMPUTER,40,false,true};
    guarded.board[23]=(Piece){CAPTAIN,COMPUTER,41,false,true};
    guarded.board[32]=(Piece){LIEUTENANT,COMPUTER,42,false,true};
    guarded.board[34]=(Piece){BOMB,COMPUTER,43,false,false};
    guarded.board[31]=(Piece){MAJOR,HUMAN,0,true,true};
    CHECK(miner_trapped_after_approach(&guarded,33));
    guarded.board[22]=(Piece){GENERAL,COMPUTER,44,false,true};
    CHECK(!miner_trapped_after_approach(&guarded,33));
    /* Preserve the successful marshal attack on a still-hidden spy. */
    Game win;CHECK(replay_load(STRATEGO_LATEST_FIXTURES "/ai_win_284_20260923.jsonl",82,&win));
    for(uint32_t seed=1;seed<=8;seed++){
        uint32_t rng=seed;Move m=ai_choose(&win,1,&rng);CHECK(m.from==54&&m.to==55);
    }
    puts("Latest matches: miner escape, hidden scout, recapture consistency, defensive exchange and winning contact passed.");
    return 0;
}
