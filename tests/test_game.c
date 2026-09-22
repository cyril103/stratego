#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static void put(Game *g,int s,int rank,int side) {g->board[s]=(Piece){rank,side,s,false,false};}
static void known_position(Game *g) {
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g->captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g->board[s].side>=0){g->board[s].revealed=true;g->captured[g->board[s].side][g->board[s].rank]--;}
}
static Move expert(Game *g) {uint32_t rng=519;return ai_choose(g,1,&rng);}
/* Independent exhaustive mate oracle for small, fully revealed test positions. */
static bool forces_win(const Game *g,int side,int depth) {
    if(g->winner>=0)return g->winner==side;
    if(!depth)return false;
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves);
    if(!n)return g->turn!=side;
    bool our_turn=g->turn==side;
    for(int i=0;i<n;i++){Game next=*g;CHECK(game_apply(&next,moves[i]));bool wins=forces_win(&next,side,depth-1);
        if(our_turn&&wins)return true;
        if(!our_turn&&!wins)return false;
    }
    return !our_turn;
}
int main(void) {
    Game g;game_init(&g,42);
    for(int side=0;side<2;side++){int count[12]={0};for(int s=0;s<100;s++)if(g.board[s].side==side)count[g.board[s].rank]++;for(int r=0;r<12;r++)CHECK(count[r]==army_counts[r]);}
    CHECK(is_lake(42)&&is_lake(57)&&!is_lake(44));
    /* Deployment preserves the army and keeps a live miner at the front. */
    for(int seed=1;seed<=50;seed++)for(int side=0;side<2;side++){
        Game setup;game_init(&setup,(uint32_t)seed);ai_deploy(&setup,side);int count[12]={0},ids[80]={0};
        for(int s=0;s<100;s++)if(setup.board[s].side==side){Piece p=setup.board[s];CHECK(p.rank>=0&&p.rank<12);count[p.rank]++;CHECK(p.id>=0&&p.id<80);CHECK(++ids[p.id]==1);}
        for(int r=0;r<12;r++)CHECK(count[r]==army_counts[r]);
        CHECK(game_moves(&setup,side,NULL)>0);
    }
    /* The faster generator must enumerate exactly the authoritative legal set. */
    for(int side=0;side<2;side++) {
        Move generated[MAX_MOVES];int n=game_moves(&g,side,generated),reference=0;
        for(int a=0;a<100;a++)for(int b=0;b<100;b++)if(game_legal(&g,(Move){a,b},side)){
            reference++;int matches=0;for(int i=0;i<n;i++)matches+=generated[i].from==a&&generated[i].to==b;CHECK(matches==1);
        }
        CHECK(n==reference);
    }
    game_clear(&g);put(&g,64,SCOUT,HUMAN);put(&g,4,FLAG,COMPUTER);
    CHECK(game_legal(&g,(Move){64,4},HUMAN));CHECK(!game_legal(&g,(Move){64,53},HUMAN));
    put(&g,44,SERGEANT,HUMAN);CHECK(!game_legal(&g,(Move){64,4},HUMAN));
    put(&g,62,SCOUT,HUMAN);CHECK(!game_legal(&g,(Move){62,32},HUMAN));
    put(&g,60,BOMB,HUMAN);CHECK(!game_legal(&g,(Move){60,50},HUMAN));
    CHECK(combat_result(SPY,MARSHAL)==1);CHECK(combat_result(MARSHAL,SPY)==1);
    CHECK(combat_result(MINER,BOMB)==1);CHECK(combat_result(MARSHAL,BOMB)==-1);
    CHECK(combat_result(SERGEANT,SERGEANT)==0);
    game_clear(&g);put(&g,60,SCOUT,HUMAN);put(&g,0,FLAG,COMPUTER);
    CHECK(game_apply(&g,(Move){60,0}));CHECK(g.winner==HUMAN&&g.board[0].revealed);
    game_clear(&g);put(&g,60,MINER,HUMAN);put(&g,50,BOMB,COMPUTER);put(&g,1,SCOUT,COMPUTER);
    CHECK(game_apply(&g,(Move){60,50}));CHECK(g.board[50].rank==MINER&&g.captured[COMPUTER][BOMB]==1);
    game_clear(&g);put(&g,60,SERGEANT,HUMAN);put(&g,50,SERGEANT,COMPUTER);put(&g,1,SCOUT,COMPUTER);
    CHECK(game_apply(&g,(Move){60,50}));CHECK(g.board[50].side==-1&&g.captured[HUMAN][SERGEANT]==1);
    game_clear(&g);put(&g,60,SCOUT,HUMAN);put(&g,0,SCOUT,COMPUTER);
    for(int k=0;k<3;k++){CHECK(game_apply(&g,(Move){k%2?61:60,k%2?60:61}));CHECK(game_apply(&g,(Move){k%2?1:0,k%2?0:1}));}
    CHECK(!game_legal(&g,(Move){61,60},HUMAN));CHECK(game_legal(&g,(Move){61,71},HUMAN));
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,0,SCOUT,COMPUTER);game_check_end(&g);CHECK(g.winner==COMPUTER);
    /* Public-state invariance: swapping hidden enemy ranks must not affect AI. */
    game_init(&g,567);g.turn=COMPUTER;Game other=g;
    int t=other.board[60].rank;other.board[60].rank=other.board[97].rank;other.board[97].rank=t;
    uint32_t r1=44,r2=44;Move a=ai_choose(&g,1,&r1),b=ai_choose(&other,1,&r2);CHECK(a.from==b.from&&a.to==b.to);
    CHECK(r1==r2);
    /* Known tactical positions: immediate objectives and protected captures. */
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,4,FLAG,COMPUTER);put(&g,64,SCOUT,HUMAN);put(&g,0,MARSHAL,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(a.from==64&&a.to==4);
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);put(&g,64,MARSHAL,HUMAN);put(&g,54,SERGEANT,COMPUTER);put(&g,44,MARSHAL,COMPUTER);put(&g,80,SCOUT,HUMAN);known_position(&g);
    a=expert(&g);CHECK(!(a.from==64&&a.to==54));
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);put(&g,64,SPY,HUMAN);put(&g,54,MARSHAL,COMPUTER);put(&g,0,SCOUT,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(a.from==64&&a.to==54);
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);put(&g,64,MINER,HUMAN);put(&g,54,BOMB,COMPUTER);put(&g,0,SCOUT,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(a.from==64&&a.to==54);
    /* A five-ply flag attack justifies sacrificing the threatened general. */
    game_clear(&g);put(&g,98,FLAG,HUMAN);put(&g,2,FLAG,COMPUTER);put(&g,64,GENERAL,HUMAN);put(&g,54,MARSHAL,COMPUTER);put(&g,80,SCOUT,HUMAN);put(&g,70,SERGEANT,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(game_apply(&g,a));CHECK(forces_win(&g,HUMAN,4));
    /* With that flag route bombed, the general really must retreat. */
    game_clear(&g);put(&g,98,FLAG,HUMAN);put(&g,2,FLAG,COMPUTER);put(&g,1,BOMB,COMPUTER);put(&g,3,BOMB,COMPUTER);put(&g,12,BOMB,COMPUTER);put(&g,64,GENERAL,HUMAN);put(&g,54,MARSHAL,COMPUTER);put(&g,80,SCOUT,HUMAN);put(&g,70,SERGEANT,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(a.from==64&&a.to!=54);
    /* An unknown bomb/spy pair must be probed with disposable troops or miners,
       not with the marshal, regardless of the actual hidden permutation. */
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,2,FLAG,COMPUTER);put(&g,1,BOMB,COMPUTER);put(&g,3,BOMB,COMPUTER);put(&g,12,BOMB,COMPUTER);
    put(&g,67,MARSHAL,HUMAN);put(&g,76,SCOUT,HUMAN);put(&g,86,MINER,HUMAN);put(&g,77,BOMB,COMPUTER);put(&g,78,SPY,COMPUTER);known_position(&g);
    g.board[77].revealed=g.board[78].revealed=false;other=g;other.board[77].rank=SPY;other.board[78].rank=BOMB;
    for(int seed=1;seed<=8;seed++){r1=r2=(uint32_t)seed;a=ai_choose(&g,1,&r1);b=ai_choose(&other,1,&r2);
        CHECK(!(a.from==67&&a.to==77));CHECK(a.from==b.from&&a.to==b.to);CHECK(r1==r2);
    }
    /* Never uncover a scout's direct shot at our flag to take a quiet move. */
    game_clear(&g);put(&g,94,FLAG,HUMAN);put(&g,84,GENERAL,HUMAN);put(&g,80,SCOUT,HUMAN);put(&g,4,SCOUT,COMPUTER);put(&g,2,FLAG,COMPUTER);put(&g,1,BOMB,COMPUTER);put(&g,3,BOMB,COMPUTER);put(&g,12,BOMB,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(game_apply(&g,a));
    Move replies[MAX_MOVES];int reply_count=game_moves(&g,COMPUTER,replies);
    for(int i=0;i<reply_count;i++)CHECK(replies[i].to!=94);
    /* A tempting lieutenant does not justify losing a general to the marshal
       behind it. Fully known sparse positions also exercise deeper finales. */
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);
    put(&g,64,GENERAL,HUMAN);put(&g,80,MINER,HUMAN);put(&g,65,CAPTAIN,HUMAN);
    put(&g,54,LIEUTENANT,COMPUTER);put(&g,44,MARSHAL,COMPUTER);known_position(&g);
    a=expert(&g);CHECK(game_legal(&g,a,g.turn));CHECK(!(a.from==64&&a.to==54));
    /* Exchanging marshals opens a forced miner attack on the flag. The
       independent oracle proves that the apparently equal trade loses. */
    game_clear(&g);put(&g,94,FLAG,HUMAN);put(&g,84,MARSHAL,HUMAN);put(&g,90,SCOUT,HUMAN);
    put(&g,9,FLAG,COMPUTER);put(&g,83,MARSHAL,COMPUTER);put(&g,74,MINER,COMPUTER);known_position(&g);
    other=g;CHECK(game_apply(&other,(Move){84,83}));CHECK(forces_win(&other,COMPUTER,3));
    a=expert(&g);CHECK(!(a.from==84&&a.to==83));CHECK(game_legal(&g,a,g.turn));
    /* Exercise complete evolving positions, both sides, all AI move legality. */
    int plies=0;
    for(int seed=1;seed<=8;seed++){game_init(&g,(uint32_t)seed);uint32_t rng=(uint32_t)seed;
        int limit=seed<=2?80:350;
        for(int k=0;k<limit&&g.winner<0;k++){Move m=seed<=2?ai_choose(&g,1,&rng):ai_basic(&g,1,&rng);CHECK(game_legal(&g,m,g.turn));CHECK(game_apply(&g,m));plies++;
            for(int s=0;s<100;s++)CHECK(!is_lake(s)||g.board[s].side==-1);
            for(int side=0;side<2;side++)for(int r=0;r<12;r++){int alive=0;for(int s=0;s<100;s++)alive+=g.board[s].side==side&&g.board[s].rank==r;CHECK(alive+g.captured[side][r]==army_counts[r]);}
        }
    }
    printf("Rules, combat, deployment, hidden-information AI: OK (%d simulated plies)\n",plies);return 0;
}



