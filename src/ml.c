#include "ml.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
static MLModel active;
static bool ready;
bool ml_save(const char *path,const MLModel *m) {
    FILE *f=fopen(path,"w");if(!f)return false;
    fprintf(f,"STRATEGO_POLICY_V1 %d %u\n",ML_FEATURES,m->games);
    for(int k=0;k<ML_FEATURES;k++)fprintf(f,"%.9g\n",m->w[k]);
    bool ok=!ferror(f);return fclose(f)==0&&ok;
}
bool ml_load(const char *path,MLModel *m) {
    FILE *f=fopen(path,"r");if(!f)return false;MLModel next={0};char magic[64];int count;
    bool ok=fscanf(f,"%63s %d %u",magic,&count,&next.games)==3&&!strcmp(magic,"STRATEGO_POLICY_V1")&&count==ML_FEATURES;
    for(int k=0;k<ML_FEATURES&&ok;k++)ok=fscanf(f,"%f",&next.w[k])==1&&isfinite(next.w[k])&&fabsf(next.w[k])<=10;
    char extra;if(ok&&fscanf(f," %c",&extra)==1)ok=false;
    fclose(f);if(ok)*m=next;return ok;
}
bool ml_init(const char *path){ready=ml_load(path,&active);return ready;}
bool ml_ready(void){return ready;}
Move ml_choose(const Game *g,const MLModel *m,uint32_t *rng,float temperature,float gradient[ML_FEATURES]) {
    Move moves[MAX_MOVES];float scores[MAX_MOVES],features[MAX_MOVES][ML_FEATURES],prob[MAX_MOVES];
    int n=ai_policy_candidates(g,moves,scores,features),best=0;if(!n)return (Move){-1,-1};
    for(int i=0;i<n;i++){
        for(int k=0;k<ML_FEATURES;k++)scores[i]+=m->w[k]*features[i][k];
        if(scores[i]>scores[best])best=i;
    }
    if(gradient)memset(gradient,0,ML_FEATURES*sizeof(float));
    if(temperature<=0)return moves[best];
    float total=0;for(int i=0;i<n;i++){prob[i]=expf(fmaxf(-80,(scores[i]-scores[best])/temperature));total+=prob[i];}
    float pick=(game_random(rng)/(4294967296.0))*total;int chosen=n-1;
    for(int i=0;i<n;i++){pick-=prob[i];if(pick<=0){chosen=i;break;}}
    if(gradient)for(int k=0;k<ML_FEATURES;k++){
        float expected=0;for(int i=0;i<n;i++)expected+=prob[i]/total*features[i][k];
        gradient[k]=(features[chosen][k]-expected)/temperature;
    }
    return moves[chosen];
}
Move ai_learned(const Game *g,uint32_t *rng){return ready?ml_choose(g,&active,rng,.35f,NULL):ai_basic(g,1,rng);}
