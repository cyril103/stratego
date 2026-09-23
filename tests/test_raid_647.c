#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Raid 647 line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game swap_hidden(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==1-g.turn&&b.side==a.side&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }return g;
}
int main(void){
    int plies[]={98,102,468,634};
    for(int k=0;k<4;k++)for(uint32_t seed=1;seed<=8;seed++){
        Game g,v;int rem[12];float p[100][12];CHECK(replay_load(STRATEGO_RAID_FIXTURE,plies[k],&g));
        public_board(&g,&v,rem);probabilities(&v,rem,p);
        Game other=swap_hidden(g);uint32_t rng=seed,same=seed;
        Move m=ai_choose(&g,1,&rng),b=ai_choose(&other,1,&same);
        CHECK(m.from==b.from&&m.to==b.to&&rng==same);CHECK(game_legal(&g,m,g.turn));
        printf("Raid647 ply%d seed%u %d>%d\n",plies[k],seed,m.from,m.to);fflush(stdout);
        if(k<2){Game next=optimistic_move(&v,m);CHECK(!active_spy_threat(&next));}
        if(k==2){CHECK(costly_bomb_probe(&v,p,(Move){91,92}));CHECK(!costly_bomb_probe(&v,p,m));}
        if(k==3){
            CHECK(game_apply(&g,m));
            Move attack[]={{14,4},{4,3},{40,41},{41,31},{30,20},{3,2},{2,1}};
            for(int i=0;i<7;i++){
                if(!game_legal(&g,attack[i],HUMAN))break;
                CHECK(game_apply(&g,attack[i]));CHECK(g.winner!=HUMAN);
                if(g.winner>=0)break;
                Move reply=ai_choose(&g,1,&rng);CHECK(game_apply(&g,reply));CHECK(g.winner!=HUMAN);
                if(g.winner>=0)break;
            }
            CHECK(g.board[1].side==COMPUTER&&g.board[1].rank==FLAG);
            CHECK(g.captured[HUMAN][MINER]>v.captured[HUMAN][MINER]);
        }
    }
    /* Bring the equal officer back while the invasion can still be contained. */
    for(uint32_t seed=1;seed<=4;seed++){
        Game g;CHECK(replay_load(STRATEGO_RAID_FIXTURE,164,&g));uint32_t rng=seed;
        Move first=ai_choose(&g,1,&rng);CHECK(first.from==78&&first.to==77);
        int losses=0,before=0;for(int r=SPY;r<=MARSHAL;r++)before+=g.captured[COMPUTER][r];
        FILE *f=fopen(STRATEGO_RAID_FIXTURE,"r");CHECK(f);char line[8192];
        while(fgets(line,sizeof(line),f)){
            int ply,side,from,to;
            if(sscanf(line,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d",&ply,&side,&from,&to)!=4||ply<164)continue;
            if(ply>260)break;
            Move m=side==COMPUTER?ai_choose(&g,1,&rng):(Move){from,to};
            if(side==HUMAN&&!game_legal(&g,m,side))break;
            CHECK(game_apply(&g,m));CHECK(g.winner!=HUMAN);if(g.winner>=0)break;
        }
        fclose(f);for(int r=SPY;r<=MARSHAL;r++)losses+=g.captured[COMPUTER][r];
        printf("General containment seed%u ply%d additional losses%d\n",seed,g.ply,losses-before);fflush(stdout);
        CHECK(g.captured[HUMAN][GENERAL]==army_counts[GENERAL]);
        CHECK(g.ply<=200&&losses-before<=8);
    }
    /* Safe raids target mobile prey. Hidden identities must not change the
       decision, and a possible bomb or stronger officer cancels the bonus. */
    Game g;game_clear(&g);g.turn=COMPUTER;
    for(int side=0;side<2;side++)for(int r=SPY;r<=MARSHAL;r++)g.captured[side][r]=army_counts[r];
    g.captured[COMPUTER][COLONEL]--;g.captured[HUMAN][CAPTAIN]--;g.captured[HUMAN][SERGEANT]--;
    g.board[4]=(Piece){FLAG,COMPUTER,40,false,false};g.board[44]=(Piece){COLONEL,COMPUTER,41,true,true};
    g.board[54]=(Piece){CAPTAIN,HUMAN,0,false,true};g.board[56]=(Piece){SERGEANT,HUMAN,1,false,true};
    g.board[95]=(Piece){FLAG,HUMAN,2,false,false};
    Game v;int rem[12];float p[100][12];public_board(&g,&v,rem);probabilities(&v,rem,p);RaiderPlan no_defense={0};
    CHECK(officer_raid_bonus(&v,p,&no_defense,(Move){44,54},ai_flag_risk(&v,COMPUTER))>0);
    RaiderPlan assigned={0};assigned.count=1;assigned.tasks[0].guard=44;
    CHECK(officer_raid_bonus(&v,p,&assigned,(Move){44,54},ai_flag_risk(&v,COMPUTER))==0);
    float original[12];memcpy(original,p[54],sizeof(original));memset(p[54],0,sizeof(p[54]));p[54][BOMB]=.3f;p[54][CAPTAIN]=.7f;
    CHECK(officer_raid_bonus(&v,p,&no_defense,(Move){44,54},0)==0);
    memset(p[54],0,sizeof(p[54]));p[54][GENERAL]=.3f;p[54][CAPTAIN]=.7f;
    CHECK(officer_raid_bonus(&v,p,&no_defense,(Move){44,54},0)==0);memcpy(p[54],original,sizeof(original));
    Game front=v;front.board[54]=front.board[44];front.board[44]=empty_piece();front.board[64]=v.board[54];front.board[64].moved=false;
    memset(p[64],0,sizeof(p[64]));p[64][BOMB]=.04f;p[64][CAPTAIN]=.96f;
    CHECK(officer_raid_bonus(&front,p,&no_defense,(Move){54,64},ai_flag_risk(&front,COMPUTER))>0);
    Game back=front;back.board[74]=back.board[54];back.board[54]=empty_piece();back.board[84]=back.board[64];back.board[64]=empty_piece();memcpy(p[84],p[64],sizeof(p[84]));
    CHECK(officer_raid_bonus(&back,p,&no_defense,(Move){74,84},ai_flag_risk(&back,COMPUTER))==0);
    for(uint32_t seed=1;seed<=8;seed++){
        Game raid=g;uint32_t rng=seed;Move m=ai_choose(&raid,1,&rng);CHECK(m.from==44&&m.to==54);CHECK(game_apply(&raid,m));
        CHECK(game_apply(&raid,(Move){56,55}));m=ai_choose(&raid,1,&rng);CHECK(m.from==54&&m.to==55);CHECK(game_apply(&raid,m));
        CHECK(raid.winner==COMPUTER);
    }
    puts("Raid647: persistent shield, reserve, hidden miner interception and opportunistic officer raid passed.");return 0;
}
