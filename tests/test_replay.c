#include "replay.h"
#include "ai_strategy.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Replay check failed at %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static bool has_hint(const Game *g,int from,int to) {
    StrategyHint hints[MAX_STRATEGY_HINTS];int n=ai_strategy_hints(g,hints);
    for(int i=0;i<n;i++)if(hints[i].move.from==from&&hints[i].move.to==to)return true;
    return false;
}
int main(void) {
    Game g;CHECK(replay_load(STRATEGO_FIXTURE,0,&g));CHECK(g.ply==175&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_FIXTURE,120,&g));
    CHECK(has_hint(&g,34,24));
    uint32_t rng=519;Move m=ai_choose(&g,1,&rng);
    /* A deeper search may first rescue the miner attacked by the revealed
       colonel in 31; both this retreat and opening the marshal's route help. */
    bool clearance=m.from==34&&m.to==24;
    bool miner_escape=m.from==21&&g.board[m.to].side<0&&
        abs(m.to%10-31%10)+abs(m.to/10-31/10)>1;
    CHECK(clearance||miner_escape);CHECK(game_apply(&g,m));
    /* Unit-test the clearance plan, without claiming these are opponent turns. */
    CHECK(replay_load(STRATEGO_FIXTURE,120,&g));CHECK(game_apply(&g,(Move){34,24}));
    g.turn=COMPUTER;CHECK(has_hint(&g,44,34));CHECK(game_apply(&g,(Move){44,34}));
    g.turn=COMPUTER;CHECK(has_hint(&g,54,44));
    CHECK(replay_load(STRATEGO_FIXTURE,162,&g));
    for(uint32_t seed=1;seed<=8;seed++) {
        rng=seed;m=ai_choose(&g,1,&rng);CHECK(game_legal(&g,m,g.turn));
        Piece a=g.board[m.from],d=g.board[m.to];
        CHECK(!(d.side==HUMAN&&d.revealed&&combat_result(a.rank,d.rank)<0));
    }
    /* Same observation with different hidden identities must produce the same decision. */
    Game other=g;int a=-1,b=-1;
    for(int s=0;s<BOARD;s++)if(g.board[s].side==HUMAN&&!g.board[s].revealed) {
        if(a<0)a=s;
        else if(g.board[s].moved==g.board[a].moved&&g.board[s].rank!=g.board[a].rank){b=s;break;}
    }
    CHECK(a>=0&&b>=0);int rank=other.board[a].rank;other.board[a].rank=other.board[b].rank;other.board[b].rank=rank;
    uint32_t r1=519,r2=519;Move x=ai_choose(&g,1,&r1),y=ai_choose(&other,1,&r2);
    CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    CHECK(replay_load(STRATEGO_SECOND_FIXTURE,0,&g));CHECK(g.ply==349&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_SECOND_FIXTURE,340,&g));other=g;
    CHECK(!g.board[38].revealed&&!g.board[39].revealed&&g.board[38].moved&&g.board[39].moved);
    rank=other.board[38].rank;other.board[38].rank=other.board[39].rank;other.board[39].rank=rank;
    CHECK(ai_flag_risk(&g,COMPUTER)==ai_flag_risk(&other,COMPUTER));
    r1=r2=519;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);
    CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    for(uint32_t seed=1;seed<=4;seed++) {
        CHECK(replay_load(STRATEGO_SECOND_FIXTURE,340,&g));
        int intruder=g.board[39].id;int route[]={39,29,19,9,8,7};rng=seed;
        for(int step=0;step<5;step++) {
            m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
            printf("Defense seed %u: %d -> %d\n",seed,m.from,m.to);
            if(g.board[route[step]].side!=HUMAN||g.board[route[step]].id!=intruder)break;
            CHECK(game_apply(&g,(Move){route[step],route[step+1]}));
            CHECK(g.winner!=HUMAN);
            if(g.board[route[step+1]].side!=HUMAN||g.board[route[step+1]].id!=intruder)break;
        }
        CHECK(g.board[7].side==COMPUTER&&g.board[7].rank==FLAG);
    }
    CHECK(replay_load(STRATEGO_THIRD_FIXTURE,0,&g));CHECK(g.ply==511&&g.winner==HUMAN);
    {
        uint32_t seed=519;
        CHECK(replay_load(STRATEGO_THIRD_FIXTURE,112,&g));rng=seed;m=ai_choose(&g,1,&rng);
        CHECK(g.board[m.from].rank==SCOUT&&g.board[m.to].side==HUMAN&&!g.board[m.to].revealed);
        printf("Reconnaissance seed %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(replay_load(STRATEGO_THIRD_FIXTURE,450,&g));rng=seed;
        m=ai_choose(&g,1,&rng);CHECK(m.from==58&&m.to==59);CHECK(game_apply(&g,m));
        CHECK(game_apply(&g,(Move){31,21}));
        m=ai_choose(&g,1,&rng);CHECK(m.from==59&&m.to==69);CHECK(game_apply(&g,m));
        CHECK(g.board[69].side==COMPUTER&&g.board[69].rank==GENERAL);
    }
    CHECK(replay_load(STRATEGO_FOURTH_FIXTURE,0,&g));CHECK(g.ply==265&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_FOURTH_FIXTURE,262,&g));other=g;
    CHECK(game_apply(&other,(Move){17,18}));
    CHECK(ai_flag_risk(&other,COMPUTER)>ai_flag_risk(&g,COMPUTER)+20);
    /* The danger depends on public movement, not the intruder's hidden rank. */
    other=g;CHECK(!g.board[8].revealed&&!g.board[58].revealed&&g.board[8].moved&&g.board[58].moved);
    rank=other.board[8].rank;other.board[8].rank=other.board[58].rank;other.board[58].rank=rank;
    CHECK(ai_flag_risk(&g,COMPUTER)==ai_flag_risk(&other,COMPUTER));
    r1=r2=519;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);
    CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    for(uint32_t seed=1;seed<=4;seed++) {
        CHECK(replay_load(STRATEGO_FOURTH_FIXTURE,262,&g));rng=seed;
        m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
        CHECK(g.board[17].side==COMPUTER&&g.board[17].rank==GENERAL);
        CHECK(game_apply(&g,(Move){8,7}));
        m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
        CHECK(g.board[7].side==COMPUTER&&g.board[7].rank==GENERAL);
        CHECK(g.board[6].side==COMPUTER&&g.board[6].rank==FLAG);
        printf("Bomb-screen recapture seed %u: %d -> %d\n",seed,m.from,m.to);
    }
    CHECK(replay_load(STRATEGO_FIFTH_FIXTURE,0,&g));CHECK(g.ply==239&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_FIFTH_FIXTURE,26,&g));other=g;
    CHECK(!g.board[66].revealed&&!g.board[78].revealed&&g.board[66].moved&&g.board[78].moved);
    rank=other.board[66].rank;other.board[66].rank=other.board[78].rank;other.board[78].rank=rank;
    for(uint32_t seed=1;seed<=8;seed++) {
        r1=r2=seed;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);
        CHECK(!(x.from==55&&x.to==65));
        CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    }
    CHECK(replay_load(STRATEGO_SIXTH_FIXTURE,0,&g));CHECK(g.ply==161&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_SIXTH_FIXTURE,80,&g));rng=519;m=ai_choose(&g,1,&rng);
    CHECK(g.board[m.from].rank==GENERAL&&m.to/10>m.from/10);CHECK(game_legal(&g,m,g.turn));
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_SIXTH_FIXTURE,142,&g));rng=seed;m=ai_choose(&g,1,&rng);
        CHECK(!(m.from==27&&m.to==28));CHECK(game_legal(&g,m,g.turn));
    }
    /* The added tactical protection must still ignore unrevealed identities. */
    CHECK(replay_load(STRATEGO_SIXTH_FIXTURE,142,&g));other=g;a=b=-1;
    for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&!g.board[s].revealed){if(a<0)a=s;else if(g.board[s].moved==g.board[a].moved){b=s;break;}}
    CHECK(a>=0&&b>=0);rank=other.board[a].rank;other.board[a].rank=other.board[b].rank;other.board[b].rank=rank;
    r1=r2=519;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    CHECK(replay_load(STRATEGO_SEVENTH_FIXTURE,0,&g));CHECK(g.ply==251&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_SEVENTH_FIXTURE,238,&g));other=g;a=b=-1;
    for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&!g.board[s].revealed){if(a<0)a=s;else if(g.board[s].moved==g.board[a].moved&&g.board[s].rank!=g.board[a].rank){b=s;break;}}
    CHECK(a>=0&&b>=0);rank=other.board[a].rank;other.board[a].rank=other.board[b].rank;other.board[b].rank=rank;
    r1=r2=519;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    for(uint32_t seed=1;seed<=4;seed++){
        CHECK(replay_load(STRATEGO_SEVENTH_FIXTURE,238,&g));rng=seed;
        Move attack[]={{19,29},{29,28},{39,29},{29,19},{19,9},{9,8},{8,7}};
        int miner=g.board[39].id;
        for(int t=0;t<7;t++){
            m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
            printf("Clearance seed %u step %d: %d -> %d\n",seed,t,m.from,m.to);
            bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&g.board[s].id==miner)alive=true;
            if(!alive)break;
            CHECK(game_apply(&g,attack[t]));CHECK(g.winner!=HUMAN);
        }
        CHECK(g.board[7].side==COMPUTER&&g.board[7].rank==FLAG);
        bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&g.board[s].id==miner)alive=true;
        CHECK(!alive);
    }
    CHECK(replay_load(STRATEGO_EIGHTH_FIXTURE,0,&g));CHECK(g.ply==335&&g.winner==HUMAN);
    for(uint32_t seed=1;seed<=4;seed++){
        CHECK(replay_load(STRATEGO_EIGHTH_FIXTURE,134,&g));rng=seed;
        int general=g.board[21].id;m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
        printf("Raider interception seed %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(game_apply(&g,(Move){21,22}));
        m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
        bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&g.board[s].id==general)alive=true;
        CHECK(!alive);CHECK(g.board[22].side==COMPUTER&&g.board[22].rank==MARSHAL);
    }
    CHECK(replay_load(STRATEGO_NINTH_FIXTURE,0,&g));CHECK(g.ply==227&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_NINTH_FIXTURE,24,&g));other=g;a=66;b=-1;
    CHECK(!g.board[a].revealed&&g.board[a].moved);
    for(int s=0;s<100;s++)if(s!=a&&g.board[s].side==HUMAN&&!g.board[s].revealed&&g.board[s].moved){b=s;break;}
    CHECK(b>=0);rank=other.board[a].rank;other.board[a].rank=other.board[b].rank;other.board[b].rank=rank;
    r1=r2=519;x=ai_choose(&g,1,&r1);y=ai_choose(&other,1,&r2);
    CHECK(x.from==y.from&&x.to==y.to&&r1==r2);
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_NINTH_FIXTURE,24,&g));rng=seed;
        m=ai_choose(&g,1,&rng);CHECK(game_legal(&g,m,g.turn));
        CHECK(!(m.from==55&&m.to==65));
        CHECK(replay_load(STRATEGO_NINTH_FIXTURE,60,&g));rng=seed;
        m=ai_choose(&g,1,&rng);printf("General escape %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==70&&m.to==60);CHECK(game_apply(&g,m));
        CHECK(game_apply(&g,(Move){63,62}));
        m=ai_choose(&g,1,&rng);printf("General follow-through %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==60&&m.to==50);CHECK(game_apply(&g,m));
        CHECK(replay_load(STRATEGO_NINTH_FIXTURE,148,&g));rng=seed;
        float before=ai_preservation_risk(&g,COMPUTER);
        other=g;CHECK(game_apply(&other,(Move){27,28}));
        CHECK(ai_preservation_risk(&other,COMPUTER)>before);
        m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
        CHECK(ai_preservation_risk(&g,COMPUTER)<=before);
    }
    puts("Recorded defeats: defense, public information and officer interception OK");return 0;
}
