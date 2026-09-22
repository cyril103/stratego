#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Recall line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g;CHECK(replay_load(STRATEGO_RECALL_GAME,0,&g));CHECK(g.ply==259&&g.winner==HUMAN);
    CHECK(replay_load(STRATEGO_RECALL_GAME,174,&g));
    Game view;int left[12];public_board(&g,&view,left);
    CHECK(recall_officer(&view,(Move){85,75})>0);
    Game returning=optimistic_move(&view,(Move){85,75});
    CHECK(recall_officer(&returning,(Move){75,65})>0); /* Continue beyond the old row cutoff. */
    Game mirrored=view;
    for(int s=0;s<100;s++)mirrored.board[99-s]=view.board[s];
    CHECK(recall_officer(&mirrored,(Move){14,24})==recall_officer(&view,(Move){85,75}));
    CHECK(replay_load(STRATEGO_RECALL_GAME,188,&g));Game hidden=g;
    int first=-1,second=-1;
    for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&!g.board[s].revealed){
        if(first<0)first=s;
        else if(g.board[s].moved==g.board[first].moved&&g.board[s].rank!=g.board[first].rank){second=s;break;}
    }
    CHECK(second>=0);int rank=hidden.board[first].rank;hidden.board[first].rank=hidden.board[second].rank;hidden.board[second].rank=rank;
    for(uint32_t seed=1;seed<=4;seed++){
        uint32_t a=seed,b=seed;Move x=ai_choose(&g,1,&a),y=ai_choose(&hidden,1,&b);
        CHECK(x.from==y.from&&x.to==y.to&&a==b);
    }
    Game hunt;game_clear(&hunt);hunt.turn=HUMAN;
    hunt.board[60]=(Piece){GENERAL,HUMAN,0,true,true};
    hunt.board[30]=(Piece){LIEUTENANT,COMPUTER,40,true,true};
    hunt.captured[COMPUTER][GENERAL]=1;hunt.captured[COMPUTER][MARSHAL]=1;
    CHECK(dominant_hunt(&hunt,(Move){60,50})>0);
    CHECK(dominant_hunt(&hunt,(Move){60,70})<0);
    hunt.board[30].revealed=false;hunt.board[30].moved=false;
    CHECK(dominant_hunt(&hunt,(Move){60,50})==0); /* Do not hunt possible bombs. */
    for(uint32_t seed=1;seed<=4;seed++){
        CHECK(replay_load(STRATEGO_RECALL_GAME,174,&g));
        int raider=g.board[55].id;uint32_t rng=seed;bool captured=false;
        for(int ply=174;ply<=206;ply+=2){
            Move m=ai_choose(&g,1,&rng);CHECK(game_apply(&g,m));
            printf("Recall seed %u ply %d: %d -> %d\n",seed,ply,m.from,m.to);fflush(stdout);
            bool alive=false;for(int s=0;s<100;s++)if(g.board[s].side==HUMAN&&g.board[s].id==raider)alive=true;
            if(!alive){captured=true;break;}
            Game recorded;CHECK(replay_load(STRATEGO_RECALL_GAME,ply+2,&recorded));
            CHECK(game_apply(&g,recorded.last_move));CHECK(g.winner!=HUMAN);
        }
        CHECK(captured);CHECK(g.board[8].side==COMPUTER&&g.board[8].rank==FLAG);
    }
    puts("Dominant officer intercepts recorded invasion");return 0;
}
