/* An evaluation-only league. Opponent styles use public policy features;
   neither their decisions nor the candidate may inspect hidden enemy ranks. */
#include "game.h"
#include "ml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif
static double now_seconds(void){
#ifdef _WIN32
    LARGE_INTEGER t,f;QueryPerformanceCounter(&t);QueryPerformanceFrequency(&f);return (double)t.QuadPart/f.QuadPart;
#else
    struct timeval t;gettimeofday(&t,NULL);return t.tv_sec+t.tv_usec/1000000.0;
#endif
}
Move ai_reference(const Game *,int,uint32_t *);
Move ai_previous(const Game *,int,uint32_t *);
void ai_deploy_reference(Game *,int);
int ai_policy_reference(const Game *,Move[MAX_MOVES],float[MAX_MOVES],float[MAX_MOVES][ML_FEATURES]);
static Move opponent(const Game *g,const char *name,uint32_t *rng){
    if(!strcmp(name,"reference"))return ai_reference(g,1,rng);
    if(!strcmp(name,"classic"))return ai_previous(g,1,rng);
    Move moves[MAX_MOVES];float scores[MAX_MOVES],features[MAX_MOVES][ML_FEATURES];
    int n=ai_policy_reference(g,moves,scores,features),best=-1;float value=-1e30f;
    for(int i=0;i<n;i++){
        float *f=features[i],score=scores[i];Piece a=g->board[moves[i].from],d=g->board[moves[i].to];
        if(!strcmp(name,"raider"))score+=2*f[0]+(a.rank>=MAJOR?5*f[2]:0)-2*f[3]+1.5f*f[6];
        if(!strcmp(name,"miner"))score+=5*f[9]+18*f[22]-2*f[3]+2*f[8];
        if(!strcmp(name,"cautious"))score-=8*f[1]+5*f[10]+2*f[6];
        if(d.side==1-g->turn&&d.revealed&&d.rank==FLAG)return moves[i];
        score+=(game_random(rng)%1000)*.0005f;
        if(score>value){value=score;best=i;}
    }
    return best>=0?moves[best]:(Move){-1,-1};
}
int main(int argc,char **argv){
    if(argc!=8){fprintf(stderr,"league seed candidate_side plies opponent formation baseline replay\n");return 2;}
    int seed=atoi(argv[1]),side=atoi(argv[2]),limit=atoi(argv[3]),baseline=atoi(argv[6]);
    const char *name=argv[4],*formation=argv[5];
    if(seed<1||side<0||side>1||limit<1||baseline<0||baseline>1)return 2;
    if(strcmp(name,"reference")&&strcmp(name,"classic")&&strcmp(name,"raider")&&strcmp(name,"miner")&&strcmp(name,"cautious"))return 2;
    if(strcmp(formation,"stable")&&strcmp(formation,"random")&&strcmp(formation,"evolved"))return 2;
    FILE *f=fopen(argv[7],"w");if(!f){perror(argv[7]);return 2;}
    Game g;game_init(&g,(uint32_t)seed*7919u);uint32_t rng[2]={123u+(unsigned)seed,987u+(unsigned)seed};
    if(!strcmp(formation,"stable")){ai_deploy_reference(&g,HUMAN);ai_deploy_reference(&g,COMPUTER);}
    if(!strcmp(formation,"evolved")){ai_deploy(&g,HUMAN);ai_deploy(&g,COMPUTER);}
    fprintf(f,"{\"version\":1,\"rules\":\"ISF-endings-v1\",\"engine\":\"CampaignLeague\",\"difficulty\":1,\"turn\":%d,\"board\":[",g.turn);
    for(int s=0;s<100;s++)fprintf(f,"%s[%d,%d,%d]",s?",":"",g.board[s].side,g.board[s].rank,g.board[s].id);
    fputs("]}\n",f);fflush(f);
    int decisions[2]={0,0};double seconds[2]={0,0};
    while(g.winner<0&&g.ply<limit){
        bool candidate=g.turn==side;double started=now_seconds();
        Move m=candidate?(baseline?ai_reference(&g,1,&rng[g.turn]):ai_choose(&g,1,&rng[g.turn])):opponent(&g,name,&rng[g.turn]);
        seconds[candidate?0:1]+=now_seconds()-started;
        decisions[candidate?0:1]++;
        if(!game_apply(&g,m)){fprintf(stderr,"Illegal move %d>%d ply%d\n",m.from,m.to,g.ply);fclose(f);return 3;}
        fprintf(f,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d,\"combat\":%d,\"attacker\":%d,\"defender\":%d}\n",g.ply,1-g.turn,m.from,m.to,g.combat,g.attack_rank,g.defend_rank);
        if(g.ply%20==0)fflush(f);
    }
    fprintf(f,"{\"end\":true,\"winner\":%d,\"ply\":%d,\"reason\":%d}\n",g.winner,g.ply,g.end_reason);fclose(f);
    printf("{\"winner\":%d,\"ply\":%d,\"reason\":%d,\"decisions\":[%d,%d],\"decision_seconds\":[%.6f,%.6f]}\n",g.winner,g.ply,g.end_reason,decisions[0],decisions[1],seconds[0],seconds[1]);
    return 0;
}
