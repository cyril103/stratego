#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Defensive tempo line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swap_unknowns(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==HUMAN&&b.side==HUMAN&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }
    return g;
}
int main(void){
    Game g,view;int remaining[12];float p[100][12];
    CHECK(replay_load(STRATEGO_TEMPO_FIXTURE,0,&g));CHECK(g.winner==HUMAN&&g.ply==383);
    CHECK(replay_load(STRATEGO_TEMPO_FIXTURE,64,&g));
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    Game retreat=optimistic_move(&view,(Move){44,45});
    CHECK(approaching_officer_risk_from(&retreat,&view,p)<approaching_officer_risk(&view,p));
    /* Never infer pursuit towards a square the marshal has not occupied. */
    CHECK(approaching_officer_risk_from(&retreat,&view,p)>0);
    const uint32_t seeds[]={1,2,3,519};
    for(int position=0;position<3;position++){
        int ply=position==0?64:position==1?374:112;
        CHECK(replay_load(STRATEGO_TEMPO_FIXTURE,ply,&g));Game changed=swap_unknowns(g);
        public_board(&g,&view,remaining);probabilities(&view,remaining,p);
        RaiderPlan plan;raider_plan(&view,&plan);
        if(position==1){
            Game probe=optimistic_move(&view,(Move){23,33});probe.turn=HUMAN;probe.board[44].rank=MAJOR;probe.board[44].revealed=true;
            int left=20000;bool forced=public_flag_forced(&probe,COMPUTER,9,&left);
            CHECK(forced&&left>=0);
            printf("Tempo risk: D7 %.4f E8 %.4f\n",guard_tempo_risk(&view,p,(Move){23,33}),guard_tempo_risk(&view,p,(Move){23,24}));fflush(stdout);
            CHECK(guard_tempo_risk(&view,p,(Move){23,33})>0);
            CHECK(guard_tempo_risk(&view,p,(Move){23,24})==0);
        }
        for(int seed=0;seed<4;seed++){
            uint32_t a=seeds[seed],b=a;Move m=ai_choose(&g,1,&a),same=ai_choose(&changed,1,&b);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==same.from&&m.to==same.to&&a==b);
            if(position==0)CHECK(m.from==44&&m.to==45);
            if(position==1)CHECK(m.from==23&&m.to==24);
            if(position==2)CHECK(raider_bonus(&view,&plan,m)>0);
            printf("Defense before %d seed %u: %d -> %d\n",ply,seeds[seed],m.from,m.to);fflush(stdout);
        }
    }
    /* Already-lost positions must still return a legal move, not an empty list. */
    CHECK(replay_load(STRATEGO_TEMPO_FIXTURE,380,&g));
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    int budget=20000;CHECK(public_flag_forced(&view,COMPUTER,4,&budget));
    budget=0;CHECK(!public_flag_forced(&view,COMPUTER,4,&budget));
    uint32_t rng=519;Move m=ai_choose(&g,1,&rng);CHECK(game_legal(&g,m,g.turn));
    /* An extra free mobile unit restores the ability to wait. */
    view.board[69]=(Piece){SCOUT,COMPUTER,79,true,true};budget=20000;
    CHECK(!public_flag_forced(&view,COMPUTER,4,&budget));
    /* Our unrevealed general still defeats the known miner. The proof must
       not apply the defender's optimistic unknown-combat rule to its enemy. */
    game_clear(&view);view.turn=HUMAN;
    view.board[0]=(Piece){FLAG,COMPUTER,40,false,false};
    view.board[10]=(Piece){GENERAL,COMPUTER,41,false,false};
    view.board[99]=(Piece){SERGEANT,COMPUTER,42,true,true};
    view.board[20]=(Piece){MINER,HUMAN,0,true,true};
    view.board[90]=(Piece){-2,HUMAN,1,false,false};
    budget=20000;CHECK(!public_flag_forced(&view,COMPUTER,3,&budget));
    puts("Defensive tempo, marshal escape and raider assignment OK");return 0;
}
