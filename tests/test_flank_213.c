#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Flank213 line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    {
        Game g,v;int rem[12];float p[100][12];
        CHECK(replay_load(STRATEGO_FLANK_FIXTURE,198,&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        FlankDefense plan=flank_defense_plan(&v,p);
        CHECK(plan.escort==31&&plan.distance==3);
        CHECK(flank_defense_bonus(&v,p,&plan,(Move){32,31})>0);
        v.captured[HUMAN][MARSHAL]=0;
        CHECK(flank_defense_bonus(&v,p,&plan,(Move){32,31})==0);
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        for(int s=0;s<100;s++)p[s][MINER]=0;
        CHECK(flank_defense_plan(&v,p).escort<0);
        probabilities(&v,rem,p);v.board[31].revealed=false;v.board[31].rank=-2;
        CHECK(flank_defense_plan(&v,p).escort<0);
    }
    for(uint32_t seed=1;seed<=8;seed++){
        Game g,v;int rem[12];float p[100][12];
        CHECK(replay_load(STRATEGO_FLANK_FIXTURE,162,&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        CHECK(marshal_needs_exit(&v,p)&&marshal_has_exit(&v,p));
        Move corridor[]={{40,41},{38,37}};
        CHECK(preserve_marshal_exit(&v,p,corridor,2)==1);
        CHECK(corridor[0].from==38);
        uint32_t opening=seed;Move start=ai_choose(&g,1,&opening);
        Game next=optimistic_move(&v,start);
        CHECK(!marshal_needs_exit(&next,p)||marshal_has_exit(&next,p));
        CHECK(game_apply(&g,start));
        if(game_legal(&g,(Move){62,61},HUMAN)){
            CHECK(game_apply(&g,(Move){62,61}));
            Move answer=ai_choose(&g,1,&opening);CHECK(game_apply(&g,answer));
            if(game_legal(&g,(Move){61,51},HUMAN))CHECK(game_apply(&g,(Move){61,51}));
            CHECK(g.captured[COMPUTER][MARSHAL]==0);
        }
        CHECK(replay_load(STRATEGO_FLANK_FIXTURE,164,&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        CHECK(marshal_contact_unsafe(&v,p));
        CHECK(certain_survival(&v,p,(Move){51,61}));
        /* If the nearby support has been publicly identified, the capture
           is a safe answer even when the attacking spy remains unknown. */
        g.board[71].revealed=true;
        uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);
        CHECK(m.from==51&&m.to==61);CHECK(game_apply(&g,m));
        CHECK(g.captured[HUMAN][SPY]==1&&g.board[61].rank==MARSHAL);
        /* This decision depends on movement and inventory, not the hidden spy. */
        CHECK(replay_load(STRATEGO_FLANK_FIXTURE,164,&g));
        g.board[71].revealed=true;
        int rank=g.board[44].rank;g.board[44].rank=g.board[61].rank;g.board[61].rank=rank;
        uint32_t same=seed;Move other=ai_choose(&g,1,&same);
        CHECK(other.from==m.from&&other.to==m.to&&rng==same);
        /* With an open escape, a safe retreat also answers the contact. */
        v.board[41]=empty_piece();Move options[]={{50,40},{51,41},{51,61}};
        CHECK(rescue_exposed_marshal(&v,p,options,3)==1);
        CHECK(options[0].from==51&&options[0].to==41);
    }
    for(uint32_t seed=1;seed<=4;seed++){
        Game g;CHECK(replay_load(STRATEGO_FLANK_FIXTURE,198,&g));uint32_t rng=seed;
        FILE *f=fopen(STRATEGO_FLANK_FIXTURE,"r");CHECK(f);char line[8192];
        while(fgets(line,sizeof(line),f)){
            int ply,side,from,to;
            if(sscanf(line,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d",&ply,&side,&from,&to)!=4||ply<198)continue;
            Move m=side==COMPUTER?ai_choose(&g,1,&rng):(Move){from,to};
            if(!game_legal(&g,m,side))break;
            printf("Flank213 seed%u ply%d %d>%d\n",seed,ply,m.from,m.to);fflush(stdout);
            CHECK(game_apply(&g,m));CHECK(g.winner!=HUMAN);
            if(g.winner>=0)break;
        }
        fclose(f);
        CHECK(g.board[12].side==COMPUTER&&g.board[12].rank==FLAG);
        CHECK(g.captured[HUMAN][GENERAL]==army_counts[GENERAL]);
        CHECK(g.captured[HUMAN][MINER]>0);
    }
    return 0;
}
