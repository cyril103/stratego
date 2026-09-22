#ifndef AI_MODELS_H
#define AI_MODELS_H
#include "game.h"
/* Preserve existing difficulty IDs in saved match logs. */
enum { AI_DISCOVERY=0, AI_IMPROVED=1, AI_LEARNED=2, AI_CLASSIC=3 };
const char *ai_model_name(int model);
const char *ai_model_description(int model);
int ai_model_next(int model,bool learned_available);
Move ai_model_choose(const Game *g,int model,uint32_t *rng);
#endif
