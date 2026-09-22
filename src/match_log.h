#ifndef MATCH_LOG_H
#define MATCH_LOG_H
#include "game.h"
/* Diagnostic replay files never feed back into the AI or reveal ranks in UI. */
void match_log_begin(const Game *g,int difficulty);
void match_log_move(const Game *g,Move m);
void match_log_close(const Game *g);
#endif
