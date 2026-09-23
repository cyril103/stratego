#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Scout flag line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int rotate(int s){return s<0?s:99-s;}
static Game mirrored(Game g){
    Game out=g;out.turn=1-g.turn;out.last_move=(Move){rotate(g.last_move.from),rotate(g.last_move.to)};
    for(int s=0;s<100;s++){Piece piece=g.board[s];if(piece.side>=0)piece.side=1-piece.side;out.board[99-s]=piece;}
    for(int side=0;side<2;side++){
        memcpy(out.captured[1-side],g.captured[side],sizeof(g.captured[side]));
        memcpy(out.marshal_suspects[1-side],g.marshal_suspects[side],sizeof(g.marshal_suspects[side]));
        out.last_id[1-side]=g.last_id[side];out.repetitions[1-side]=g.repetitions[side];
        out.last_from[1-side]=rotate(g.last_from[side]);out.last_to[1-side]=rotate(g.last_to[side]);
        out.history_count[1-side]=g.history_count[side];
        for(int j=0;j<8;j++){
            out.history_id[1-side][j]=g.history_id[side][j];
            out.history[1-side][j]=(Move){rotate(g.history[side][j].from),rotate(g.history[side][j].to)};
        }
        for(int s=0;s<100;s++)out.cleared_bombs[1-side][99-s]=g.cleared_bombs[side][s];
    }return out;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];
    CHECK(replay_load(STRATEGO_SCOUT_FLAG_FIXTURE,180,&g));
    public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    CHECK(v.board[64].rank==-2&&p[64][SCOUT]>.1f&&p[64][SCOUT]<.5f);
    CHECK(fabsf(scout_flag_risk(&v,p,(Move){45,55})-p[64][SCOUT])<.0001f);
    CHECK(scout_flag_risk(&v,p,(Move){45,55})>.29f);
    CHECK(scout_flag_risk(&v,p,(Move){25,24})==0);
    CHECK(scout_flag_risk(&v,p,(Move){45,44})==0);
    CHECK(scout_flag_risk(&v,p,(Move){13,14})==0);
    Game other=g;CHECK(!g.board[62].revealed&&g.board[62].moved);
    other.board[64].rank=g.board[62].rank;other.board[62].rank=g.board[64].rank;
    for(uint32_t seed=1;seed<=8;seed++){
        uint32_t rng=seed,same=seed;Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);
        Game next=g;CHECK(game_apply(&next,a));CHECK(!game_legal(&next,(Move){64,4},HUMAN));
        CHECK(next.winner<0);printf("Scout flag seed %u: %d>%d\n",seed,a.from,a.to);fflush(stdout);
    }
    Game reverse=mirrored(g);uint32_t rng=519;Move defense=ai_choose(&reverse,1,&rng);
    CHECK(game_apply(&reverse,defense));CHECK(!game_legal(&reverse,(Move){35,95},COMPUTER));
    g.board[64].revealed=true;rng=519;defense=ai_choose(&g,1,&rng);
    CHECK(game_apply(&g,defense));CHECK(!game_legal(&g,(Move){64,4},HUMAN));
    /* Blockers, lakes, row boundaries, no remaining scout and unmoved scouts. */
    Game blocked=v;blocked.board[14]=(Piece){BOMB,COMPUTER,79,false,false};
    CHECK(scout_flag_ray_risk(&blocked,p,COMPUTER)==0);
    blocked=v;blocked.board[14]=(Piece){-2,HUMAN,39,false,false};p[14][SCOUT]=0;
    CHECK(scout_flag_ray_risk(&blocked,p,COMPUTER)==0);
    blocked=v;blocked.board[64].moved=false;CHECK(scout_flag_ray_risk(&blocked,p,COMPUTER)>.29f);
    remaining[SCOUT]=0;probabilities(&v,remaining,p);CHECK(scout_flag_ray_risk(&v,p,COMPUTER)==0);
    game_clear(&blocked);memset(p,0,sizeof(p));blocked.turn=COMPUTER;
    blocked.board[2]=(Piece){FLAG,COMPUTER,40,false,false};
    blocked.board[62]=(Piece){-2,HUMAN,0,false,true};p[62][SCOUT]=1;
    CHECK(scout_flag_ray_risk(&blocked,p,COMPUTER)==0);
    blocked.board[2]=empty_piece();blocked.board[62]=empty_piece();
    blocked.board[0]=(Piece){FLAG,COMPUTER,40,false,false};
    blocked.board[19]=(Piece){-2,HUMAN,0,false,true};p[19][SCOUT]=1;
    CHECK(scout_flag_ray_risk(&blocked,p,COMPUTER)==0);
    /* Attacking an unknown screen must account for losing to its scout. */
    game_clear(&blocked);memset(p,0,sizeof(p));blocked.turn=COMPUTER;
    blocked.board[4]=(Piece){FLAG,COMPUTER,40,false,false};
    blocked.board[25]=(Piece){SPY,COMPUTER,41,false,true};
    blocked.board[24]=(Piece){-2,HUMAN,0,false,true};p[24][SCOUT]=.3f;p[24][MINER]=.7f;
    CHECK(fabsf(scout_flag_risk(&blocked,p,(Move){25,24})-.3f)<.0001f);
    blocked.board[25].rank=GENERAL;CHECK(scout_flag_risk(&blocked,p,(Move){25,24})==0);
    /* Taking the enemy flag wins now, even while our own ray is open. */
    CHECK(replay_load(STRATEGO_SCOUT_FLAG_FIXTURE,180,&g));
    g.board[55]=(Piece){FLAG,HUMAN,33,true,false};rng=519;
    Move win=ai_choose(&g,1,&rng);CHECK(win.from==45&&win.to==55);
    CHECK(game_apply(&g,win));CHECK(g.winner==COMPUTER);
    puts("Scout flag defense: public uncertainty, legal rays, combat outcomes and replay passed.");return 0;
}
