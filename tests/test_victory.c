#include "game.h"
#include "replay.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Victory line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g,other;CHECK(replay_load(STRATEGO_VICTORY_FIXTURE,0,&g));
    CHECK(g.winner==COMPUTER&&g.end_reason==END_FLAG&&g.ply==374);
    const int plies[]={372,370,368},from[]={98,88,78},to[]={97,98,88};
    for(int k=0;k<3;k++)for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_VICTORY_FIXTURE,plies[k],&g));
        uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);
        printf("Race before %d seed %u: %d -> %d\n",plies[k],seed,m.from,m.to);
        CHECK(m.from==from[k]&&m.to==to[k]);
        /* The same public position must yield the same choice even if the
           real hidden flag is elsewhere in the remaining bomb enclosure. */
        if(k==0){
            other=g;CHECK(!g.board[97].revealed&&!g.board[96].revealed);
            int rank=other.board[97].rank;other.board[97].rank=other.board[96].rank;other.board[96].rank=rank;
            uint32_t changed_rng=seed;Move changed=ai_choose(&other,1,&changed_rng);
            CHECK(changed.from==m.from&&changed.to==m.to&&changed_rng==rng);
        }
    }
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_LOSS_FIXTURE,206,&g));uint32_t rng=seed;
        Move m=ai_choose(&g,1,&rng);printf("General rescue %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==45);CHECK(game_apply(&g,m));
        CHECK(!game_legal(&g,(Move){35,m.to},HUMAN));
    }
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_LATEST_FIXTURE,44,&g));uint32_t rng=seed;
        Move m=ai_choose(&g,1,&rng);CHECK(!(m.from==62&&m.to==63));
        CHECK(game_legal(&g,m,g.turn));
        CHECK(replay_load(STRATEGO_LATEST_FIXTURE,328,&g));rng=seed;
        m=ai_choose(&g,1,&rng);printf("Base defense %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from!=78); /* Marshal's remote raid must not consume this tempo. */
    }
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_MOBILITY_FIXTURE,118,&g));uint32_t rng=seed;
        Move m=ai_choose(&g,1,&rng);CHECK(!(m.from==66&&m.to==76));CHECK(game_legal(&g,m,g.turn));
        other=g;int swap=-1;
        for(int s=0;s<100;s++)if(s!=76&&g.board[s].side==HUMAN&&!g.board[s].revealed&&!g.board[s].moved&&g.board[s].rank!=g.board[76].rank){swap=s;break;}
        CHECK(swap>=0&&!g.board[76].revealed&&!g.board[76].moved);
        int rank=other.board[76].rank;other.board[76].rank=other.board[swap].rank;other.board[swap].rank=rank;
        uint32_t other_rng=seed;Move same=ai_choose(&other,1,&other_rng);
        CHECK(same.from==m.from&&same.to==m.to&&other_rng==rng);
        CHECK(replay_load(STRATEGO_MOBILITY_FIXTURE,300,&g));rng=seed;
        m=ai_choose(&g,1,&rng);printf("Mobility finish %u: %d -> %d\n",seed,m.from,m.to);CHECK(m.from==83&&m.to==73);CHECK(game_apply(&g,m));
        Move forced[MAX_MOVES];CHECK(game_moves(&g,HUMAN,forced)==1);
        CHECK(forced[0].from==95&&forced[0].to==96);CHECK(game_apply(&g,forced[0]));
        CHECK(g.winner==COMPUTER&&g.end_reason==END_IMMOBILE);
    }
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_SACRIFICE_FIXTURE,448,&g));uint32_t rng=seed;
        Move m=ai_choose(&g,1,&rng);printf("Gate interception %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==19&&m.to==18);
        other=g;int swap=-1;
        for(int s=0;s<100;s++)if(s!=18&&g.board[s].side==HUMAN&&!g.board[s].revealed&&g.board[s].moved&&g.board[s].rank!=g.board[18].rank){swap=s;break;}
        CHECK(swap>=0&&!g.board[18].revealed);
        int rank=other.board[18].rank;other.board[18].rank=other.board[swap].rank;other.board[swap].rank=rank;
        uint32_t other_rng=seed;Move same=ai_choose(&other,1,&other_rng);
        CHECK(same.from==m.from&&same.to==m.to&&other_rng==rng);
        CHECK(game_apply(&g,m));CHECK(game_apply(&g,(Move){8,18}));
        CHECK(g.board[18].rank==MARSHAL&&g.board[18].side==HUMAN);
        CHECK(g.board[4].rank==BOMB&&g.board[6].rank==BOMB&&g.board[15].rank==BOMB);
        CHECK(g.board[5].rank==FLAG&&g.board[5].side==COMPUTER&&g.winner==GAME_ONGOING);
    }
    for(uint32_t seed=1;seed<=8;seed++){
        CHECK(replay_load(STRATEGO_RECALL_FIXTURE,200,&g));uint32_t rng=seed;
        Move m=ai_choose(&g,1,&rng);printf("Marshal recall %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==94&&m.to==84);CHECK(game_apply(&g,m));
        CHECK(game_apply(&g,(Move){32,31}));
        m=ai_choose(&g,1,&rng);printf("Recall follow-through %u: %d -> %d\n",seed,m.from,m.to);
        CHECK(m.from==84&&m.to!=94);CHECK(game_legal(&g,m,g.turn));
    }
    puts("Recorded games: defense, hidden-rank independence, mobility, sacrifice and recall OK");return 0;
}
