#include "ml.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"ML check %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static Move moves[MAX_MOVES],other_moves[MAX_MOVES];
static float scores[MAX_MOVES],other_scores[MAX_MOVES],features[MAX_MOVES][ML_FEATURES],other_features[MAX_MOVES][ML_FEATURES];
static double log_probability(const MLModel *m,int n,int chosen) {
    double logits[MAX_MOVES],max=-1e30,total=0;
    for(int i=0;i<n;i++){logits[i]=scores[i];for(int k=0;k<ML_FEATURES;k++)logits[i]+=m->w[k]*features[i][k];if(logits[i]>max)max=logits[i];}
    for(int i=0;i<n;i++)total+=exp(logits[i]-max);
    return logits[chosen]-max-log(total);
}
int main(void) {
    Game g,other;game_init(&g,57);g.turn=COMPUTER;other=g;
    int rank=other.board[60].rank;other.board[60].rank=other.board[97].rank;other.board[97].rank=rank;
    int n=ai_policy_candidates(&g,moves,scores,features);
    CHECK(n==ai_policy_candidates(&other,other_moves,other_scores,other_features));
    CHECK(!memcmp(moves,other_moves,n*sizeof(Move))&&!memcmp(scores,other_scores,n*sizeof(float))&&!memcmp(features,other_features,n*sizeof(features[0])));
    MLModel model={0},loaded={0};for(int k=0;k<ML_FEATURES;k++)model.w[k]=(k-12)*.025f;model.games=32;
    float gradient[ML_FEATURES];uint32_t rng=19,other_rng=19;
    Move a=ml_choose(&g,&model,&rng,1,gradient),b=ml_choose(&other,&model,&other_rng,1,NULL);
    CHECK(a.from==b.from&&a.to==b.to&&rng==other_rng&&game_legal(&g,a,g.turn));
    int chosen=-1;for(int i=0;i<n;i++)if(moves[i].from==a.from&&moves[i].to==a.to)chosen=i;CHECK(chosen>=0);
    for(int k=0;k<ML_FEATURES;k++){
        MLModel plus=model,minus=model;plus.w[k]+=.002f;minus.w[k]-=.002f;
        double numeric=(log_probability(&plus,n,chosen)-log_probability(&minus,n,chosen))/.004;
        CHECK(fabs(numeric-gradient[k])<.002);
    }
    CHECK(ml_save("ml_roundtrip_test.policy",&model));CHECK(ml_load("ml_roundtrip_test.policy",&loaded));
    CHECK(loaded.games==model.games);for(int k=0;k<ML_FEATURES;k++)CHECK(loaded.w[k]==model.w[k]);
    FILE *f=fopen("ml_roundtrip_test.policy","w");CHECK(f);fputs("STRATEGO_POLICY_V1 24 1\nnan\n",f);fclose(f);
    CHECK(!ml_load("ml_roundtrip_test.policy",&loaded));CHECK(loaded.games==model.games);remove("ml_roundtrip_test.policy");
    puts("ML: public-state invariance, policy gradient finite differences, roundtrip and malformed-model rejection OK");return 0;
}
