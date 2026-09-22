#ifndef AI_WORKER_H
#define AI_WORKER_H
#include "game.h"
/* Single background search; all interface functions are called by the UI. */
bool ai_worker_start(const Game *game,int difficulty,uint32_t rng);
bool ai_worker_busy(void);
bool ai_worker_poll(Move *move,uint32_t *rng);
void ai_worker_stop(void);
#endif
