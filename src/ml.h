#ifndef STRATEGO_ML_H
#define STRATEGO_ML_H
#include "game.h"
#define ML_FEATURES 24
typedef struct {float w[ML_FEATURES];unsigned games;} MLModel;
int ai_policy_candidates(const Game *g,Move moves[MAX_MOVES],float scores[MAX_MOVES],float features[MAX_MOVES][ML_FEATURES]);
bool ml_save(const char *path,const MLModel *model);
bool ml_load(const char *path,MLModel *model);
bool ml_init(const char *path);
bool ml_ready(void);
Move ml_choose(const Game *g,const MLModel *model,uint32_t *rng,float temperature,float gradient[ML_FEATURES]);
Move ai_learned(const Game *g,uint32_t *rng);
#endif
