#include "ml.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#define LIMIT 1600
static float gradients[LIMIT][ML_FEATURES];
static int plies[LIMIT];
static int checkpoint(const char *path,const MLModel *model){
    char temp[1024];if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp))return 0;
    if(!ml_save(temp,model))return 0;
#ifdef _WIN32
    return MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
    return rename(temp,path)==0;
#endif
}
int main(int argc,char **argv){
    if(argc<3){fprintf(stderr,"Usage: train_expert output.policy total_games [ply_cap=1600]\n");return 1;}
    const char *path=argv[1];unsigned target=(unsigned)strtoul(argv[2],NULL,10);
    int cap=argc>3?atoi(argv[3]):LIMIT;
    if(!target||target>1000000||cap<2||cap>LIMIT)return 1;
    MLModel model={0};
    FILE *existing=fopen(path,"r");
    if(existing){fclose(existing);if(!ml_load(path,&model)){fputs("Invalid checkpoint; refusing to overwrite\n",stderr);return 2;}}
    else {if(ml_load("assets/models/selfplay.policy",&model))model.games=0;if(!checkpoint(path,&model))return 3;}
    unsigned wins=0,losses=0,draws=0,capped=0,start=model.games;time_t began=time(NULL);
    printf("Opponent: actual Expert+ (ai_choose difficulty=1), unchanged search.\nREINFORCE, alternating sides, terminal rewards, cap=%d plies.\nResume=%u target=%u; session statistics start here.\n",cap,start,target);fflush(stdout);
    while(model.games<target){
        unsigned episode=model.games;uint32_t seed=700001u+episode;
        Game g;game_init(&g,seed*7919u);ai_deploy(&g,HUMAN);ai_deploy(&g,COMPUTER);
        int learner=(int)(episode%2),steps=0;uint32_t rng[2]={seed*104729u,seed*15485863u};
        while(g.winner<0&&g.ply<cap){
            Move move;
            if(g.turn==learner){plies[steps]=g.ply;move=ml_choose(&g,&model,&rng[g.turn],1.1f,gradients[steps++]);}
            else move=ai_choose(&g,1,&rng[g.turn]);
            if(move.from<0){game_check_end(&g);if(g.winner<0)return 4;break;}
            if(!game_apply(&g,move)){fputs("Illegal training move\n",stderr);return 4;}
            if(g.ply%20==0){printf("game %u/%u | learner %s | ply %d | elapsed %.0fs\n",episode+1,target,learner==HUMAN?"blue":"red",g.ply,difftime(time(NULL),began));fflush(stdout);}
        }
        if(g.winner<0)capped++;
        else {
            float reward=g.winner==GAME_DRAW?0:g.winner==learner?1:-1,update[ML_FEATURES]={0};
            if(g.winner==GAME_DRAW)draws++;else if(reward>0)wins++;else losses++;
            for(int t=0;t<steps;t++)for(int k=0;k<ML_FEATURES;k++)update[k]+=reward*powf(.997f,(float)(g.ply-1-plies[t]))*gradients[t][k];
            if(steps)for(int k=0;k<ML_FEATURES;k++)model.w[k]=fmaxf(-4,fminf(4,model.w[k]*.9999f+.1f*update[k]/sqrtf((float)steps)));
        }
        model.games++;
        if(!checkpoint(path,&model)){fputs("Checkpoint write failed\n",stderr);return 3;}
        printf("SAVED %u/%u | session W=%u L=%u draws=%u capped=%u | last plies=%d | elapsed %.0fs\n",model.games,target,wins,losses,draws,capped,g.ply,difftime(time(NULL),began));fflush(stdout);
        if(model.games%1000==0){char archive[1024];snprintf(archive,sizeof(archive),"%s.%06u",path,model.games);if(!checkpoint(archive,&model))return 3;}
    }
    printf("COMPLETE %u training games against Expert+. Evaluate independently before deployment.\n",model.games);return 0;
}
