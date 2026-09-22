#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Marshal guard line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game permuted(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;}
    }return g;
}
static bool alive(const Game *g,int id){for(int s=0;s<100;s++)if(g->board[s].id==id)return true;return false;}
static int rotated(int s){return s<0?s:99-s;}
static Game mirror(Game g){
    Game out=g;out.turn=1-g.turn;
    for(int s=0;s<100;s++){Piece p=g.board[s];if(p.side>=0)p.side=1-p.side;out.board[99-s]=p;}
    for(int side=0;side<2;side++){
        out.history_count[1-side]=g.history_count[side];
        for(int j=0;j<8;j++){
            out.history_id[1-side][j]=g.history_id[side][j];
            out.history[1-side][j]=(Move){rotated(g.history[side][j].from),rotated(g.history[side][j].to)};
        }
    }return out;
}
int main(void){
    Game g,v;int r[12];float p[100][12];
    CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,86,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    CHECK(marshal_avoided(&v,v.board[39].id,48));CHECK(!marshal_avoided(&v,v.board[39].id,49));
    CHECK(marshal_returns_to_suspect(&v,p,(Move){39,38}));CHECK(marshal_returns_to_suspect(&v,p,(Move){39,49}));
    CHECK(!marshal_returns_to_suspect(&v,p,(Move){26,27}));
    Game reversed=mirror(v);CHECK(marshal_avoided(&reversed,v.board[39].id,99-48));
    CHECK(!marshal_avoided(&reversed,v.board[39].id,99-49));
    Game forgotten=v;forgotten.history_count[0]=forgotten.history_count[1]=0;CHECK(marshal_avoided(&forgotten,v.board[39].id,48));
    p[48][SPY]=0;CHECK(!marshal_returns_to_suspect(&v,p,(Move){39,38}));
    CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,84,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    CHECK(marshal_returns_to_suspect(&v,p,(Move){26,27}));CHECK(!marshal_returns_to_suspect(&v,p,(Move){38,39}));
    for(uint32_t seed=1;seed<=16;seed++){
        CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,86,&g));Game other=permuted(g);
        int marshal=g.board[39].id,spy=g.board[28].id;uint32_t rng=seed,same=seed;
        Move a=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(a.from==b.from&&a.to==b.to&&rng==same);
        CHECK(!(a.from==39&&a.to==38));CHECK(game_apply(&g,a));
        CHECK(game_apply(&g,(Move){48,38}));CHECK(alive(&g,marshal));
        a=ai_choose(&g,1,&rng);CHECK(game_apply(&g,a));
        if(game_legal(&g,(Move){38,28},g.turn))CHECK(game_apply(&g,(Move){38,28}));
        CHECK(alive(&g,marshal)&&alive(&g,spy));
        CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,88,&g));other=permuted(g);rng=seed;same=seed;
        a=ai_choose(&g,1,&rng);b=ai_choose(&other,1,&same);
        CHECK(a.from==37&&a.to==38);CHECK(a.from==b.from&&a.to==b.to&&rng==same);
        CHECK(game_apply(&g,a));CHECK(g.board[28].rank==SPY&&g.board[38].rank==LIEUTENANT);
    }
    CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,88,&g));public_board(&g,&v,r);probabilities(&v,r,p);
    CHECK(known_unanswered_loss_at(&v,COMPUTER,28)>0);
    v.captured[HUMAN][MARSHAL]=army_counts[MARSHAL];CHECK(known_unanswered_loss_at(&v,COMPUTER,28)==0);
    /* Refuse the automatic recapture when it hangs its attacker. */
    public_board(&g,&v,r);probabilities(&v,r,p);v.board[48]=(Piece){MAJOR,HUMAN,39,true,true};
    memset(p[48],0,sizeof(p[48]));p[48][MAJOR]=1;
    Move moves[MAX_MOVES];int n=game_moves(&v,v.turn,moves);CHECK(dangerous_spy_capture(&v,p,moves,n).from<0);
    /* Saving the flag still outranks saving the spy. Restore a captured
       enemy scout on the opened gate; the sergeant must take it immediately. */
    CHECK(replay_load(STRATEGO_MARSHAL_FIXTURE,88,&g));
    bool used[80]={false};for(int s=0;s<100;s++)if(g.board[s].id>=0)used[g.board[s].id]=true;
    int id=0;while(id<40&&used[id])id++;CHECK(id<40&&g.captured[HUMAN][SCOUT]>0);
    g.captured[HUMAN][SCOUT]--;g.captured[COMPUTER][BOMB]++;
    g.board[8]=(Piece){SCOUT,HUMAN,id,true,true};uint32_t rng=519;
    Move save=ai_choose(&g,1,&rng);CHECK(save.from==9&&save.to==8);
    /* A pawn's recapture does not make trading marshal for spy harmless. */
    game_clear(&g);g.turn=COMPUTER;
    g.board[39]=(Piece){MARSHAL,COMPUTER,40,true,true};g.board[38]=(Piece){SPY,HUMAN,0,true,true};
    g.board[49]=(Piece){LIEUTENANT,COMPUTER,41,true,true};
    CHECK(known_unanswered_loss_at(&g,COMPUTER,39)>0);
    puts("Marshal and spy survive the recorded pursuit on 16 seeds; public history, hidden swaps and safe recaptures passed.");return 0;
}
