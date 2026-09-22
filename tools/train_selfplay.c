#include "ml.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LIMIT 800
static float trace[LIMIT][ML_FEATURES];
static int actors[LIMIT];
typedef struct {int wins,losses,draws,unfinished;} Results;
static void setup(Game *g,unsigned seed){game_init(g,seed*7919u);if(seed%2) {ai_deploy(g,HUMAN);ai_deploy(g,COMPUTER);}}
static Results evaluate(const MLModel *candidate,unsigned first,int pairs) {
    Results result={0};MLModel baseline={0};
    for(int p=0;p<pairs;p++)for(int side=0;side<2;side++) {
        Game g;setup(&g,first+(unsigned)p);uint32_t rng[2]={first+(unsigned)p,first+(unsigned)p+104729u};
        while(g.winner<0&&g.ply<LIMIT){Move m=ml_choose(&g,g.turn==side?candidate:&baseline,&rng[g.turn],.35f,NULL);if(!game_apply(&g,m)){fputs("Illegal evaluation move\n",stderr);exit(2);}}
        if(g.winner<0)result.unfinished++;else if(g.winner==GAME_DRAW)result.draws++;else if(g.winner==side)result.wins++;else result.losses++;
    }
    return result;
}
int main(int argc,char **argv) {
    const char *output=argc>1?argv[1]:"assets/models/selfplay.policy";
    int rounds=argc>2?atoi(argv[2]):8,per_round=argc>3?atoi(argv[3]):512;
    if(rounds<1||rounds>100||per_round<1||per_round>100000)return 1;
    MLModel model={0},best={0};int best_score=-100000,finished=0,unfinished=0;unsigned seed=1;
    puts("REINFORCE self-play, 24 public action features, terminal rewards only.");fflush(stdout);
    for(int round=0;round<rounds;round++) {
        for(int episode=0;episode<per_round;episode++,seed++) {
            Game g;setup(&g,seed);uint32_t rng=seed*104729u;
            while(g.winner<0&&g.ply<LIMIT) {
                int ply=g.ply;actors[ply]=g.turn;
                Move m=ml_choose(&g,&model,&rng,1.1f,trace[ply]);
                if(!game_apply(&g,m)){fputs("Illegal training move\n",stderr);return 2;}
            }
            model.games++;
            if(g.winner<0)unfinished++;
            else {
                finished++;float update[ML_FEATURES]={0};
                for(int t=0;t<g.ply;t++) {
                    float reward=g.winner==GAME_DRAW?0:actors[t]==g.winner?1:-1;
                    float credit=reward*powf(.997f,(float)(g.ply-1-t));
                    for(int k=0;k<ML_FEATURES;k++)update[k]+=credit*trace[t][k];
                }
                for(int k=0;k<ML_FEATURES;k++)model.w[k]=fmaxf(-4,fminf(4,model.w[k]*.9999f+.1f*update[k]/sqrtf((float)g.ply)));
            }
            if((episode+1)%128==0){printf("round %d/%d, games %u, finished %d, capped %d\n",round+1,rounds,model.games,finished,unfinished);fflush(stdout);}
        }
        Results validation=evaluate(&model,100000u,16);
        int score=validation.wins-validation.losses;
        printf("validation round %d: %d W / %d L / %d draws / %d capped\n",round+1,validation.wins,validation.losses,validation.draws,validation.unfinished);fflush(stdout);
        if(score>best_score){best_score=score;best=model;if(!ml_save(output,&best))return 3;}
    }
    Results test=evaluate(&best,300000u,128);
    printf("HELDOUT selected checkpoint %u games: %d W / %d L / %d draws / %d capped (256 games, zero-weight policy opponent)\n",best.games,test.wins,test.losses,test.draws,test.unfinished);
    char report[1024];if(snprintf(report,sizeof(report),"%s.json",output)>=(int)sizeof(report))return 3;
    FILE *f=fopen(report,"w");if(!f)return 3;
    fprintf(f,"{\n  \"algorithm\":\"REINFORCE linear residual policy\",\n  \"features\":%d,\n  \"training_games\":%u,\n  \"training_finished\":%d,\n  \"training_capped\":%d,\n  \"selected_checkpoint_games\":%u,\n  \"validation_pairs\":16,\n  \"validation_first_seed\":100000,\n  \"test_first_seed\":300000,\n  \"test_pairs\":128,\n  \"evaluation_temperature\":0.35,\n  \"ply_cap\":%d,\n  \"test_wins\":%d,\n  \"test_losses\":%d,\n  \"test_capped\":%d\n}\n",ML_FEATURES,model.games,finished,unfinished,best.games,LIMIT,test.wins,test.losses,test.unfinished);
    if(fclose(f))return 3;
    return 0;
}
