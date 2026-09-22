#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Persistent tactics line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swapped(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
int main(void){
    Game g,v;int rem[12];float p[100][12];
    CHECK(replay_load(STRATEGO_PERSISTENT_FIXTURE,402,&g));
    public_board(&g,&v,rem);probabilities(&v,rem,p);
    CHECK(marshal_avoided(&v,v.board[44].id,33));
    v.history_count[0]=v.history_count[1]=0;
    CHECK(marshal_avoided(&v,v.board[44].id,33));
    CHECK(marshal_returns_to_suspect(&v,p,(Move){44,34}));
    v.board[33].revealed=true;v.board[33].rank=SERGEANT;
    CHECK(!marshal_avoided(&v,v.board[44].id,33));
    for(uint32_t seed=1;seed<=4;seed++){
        int plies[]={114,282,310,342,402};
        for(int i=0;i<5;i++){
            CHECK(replay_load(STRATEGO_PERSISTENT_FIXTURE,plies[i],&g));
            Game hidden=swapped(g);uint32_t rng=seed,same=seed;
            Move a=ai_choose(&g,1,&rng),b=ai_choose(&hidden,1,&same);
            CHECK(a.from==b.from&&a.to==b.to&&rng==same);CHECK(game_legal(&g,a,g.turn));
            printf("Before %d seed%u: %d>%d\n",plies[i],seed,a.from,a.to);fflush(stdout);
            if(plies[i]==282){CHECK(!(a.from==25&&a.to==35));}
            if(plies[i]==114){
                CHECK(a.from==36&&a.to==26);CHECK(game_apply(&g,a));
                CHECK(game_apply(&g,(Move){37,36}));CHECK(g.combat==2);
                CHECK(g.board[26].side==COMPUTER&&g.board[26].rank==LIEUTENANT);
            }
            if(plies[i]==342){CHECK(a.from==44&&a.to==45);}
            if(plies[i]==310){
                CHECK(a.from==24);CHECK(game_apply(&g,a));CHECK(game_apply(&g,(Move){27,26}));
                Move escape=ai_choose(&g,1,&rng);CHECK(escape.from==25&&escape.to==24);
                CHECK(game_apply(&g,escape));CHECK(!spy_square_unsafe(&g,24,COMPUTER));
            }
            if(plies[i]==402){CHECK(!(a.from==44&&a.to==34));}
        }
    }
    CHECK(replay_load(STRATEGO_PERSISTENT_FIXTURE,282,&g));public_board(&g,&v,rem);
    CHECK(spy_hunt(&v,(Move){25,35})==0);
    Game exposed=optimistic_move(&v,(Move){25,35});CHECK(known_unanswered_loss_at(&exposed,COMPUTER,35)>0);
    CHECK(replay_load(STRATEGO_PERSISTENT_FIXTURE,310,&g));public_board(&g,&v,rem);
    CHECK(spy_clearance_bonus(&v,(Move){24,23})>0);
    game_clear(&g);g.turn=COMPUTER;
    g.board[23]=(Piece){SPY,COMPUTER,40,false,true};
    g.board[25]=(Piece){SERGEANT,COMPUTER,41,false,true};
    g.board[35]=(Piece){MARSHAL,HUMAN,0,true,true};
    CHECK(spy_ambush_bonus(&g,(Move){23,24})>0);
    Game attacker=g;attacker.board[25]=attacker.board[23];attacker.board[23]=empty_piece();
    CHECK(public_legal(&attacker,(Move){25,35},COMPUTER));
    memset(p,0,sizeof(p));p[35][MARSHAL]=1;
    CHECK(spy_attack_cost(&attacker,p,(Move){25,35})==0);
    p[35][MARSHAL]=0;p[35][SPY]=1;
    CHECK(spy_attack_cost(&attacker,p,(Move){25,35})>worth[SPY]);
    g.board[14]=(Piece){SCOUT,HUMAN,1,true,true};
    CHECK(spy_ambush_bonus(&g,(Move){23,24})<=0);
    CHECK(replay_load(STRATEGO_PERSISTENT_FIXTURE,402,&g));
    int suspect=g.board[33].id;
    CHECK(game_apply(&g,(Move){44,34}));CHECK(game_apply(&g,(Move){33,34}));
    CHECK(!(g.marshal_suspects[COMPUTER][suspect/64]&(UINT64_C(1)<<(suspect%64))));
    /* Opposite camp and high identity bits; observation and new-game reset. */
    game_clear(&g);g.turn=HUMAN;
    g.board[65]=(Piece){MARSHAL,HUMAN,0,true,true};
    g.board[64]=(Piece){SCOUT,COMPUTER,70,false,true};
    g.board[90]=(Piece){SERGEANT,HUMAN,1,true,true};
    CHECK(game_apply(&g,(Move){65,75}));
    CHECK(g.marshal_suspects[HUMAN][1]&(UINT64_C(1)<<6));
    CHECK(game_apply(&g,(Move){64,63}));g.history_count[0]=g.history_count[1]=0;
    CHECK(marshal_avoided(&g,0,63));
    CHECK(game_apply(&g,(Move){90,91}));
    CHECK(game_apply(&g,(Move){63,60})); /* Long scout ray reveals identity. */
    CHECK(!(g.marshal_suspects[HUMAN][1]&(UINT64_C(1)<<6)));
    game_clear(&g);CHECK(g.marshal_suspects[0][0]==0&&g.marshal_suspects[1][1]==0);
    puts("Persistent memory, spy safety, clearance, bait control and hidden swaps passed.");return 0;
}
