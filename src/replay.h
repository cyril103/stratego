#ifndef STRATEGO_REPLAY_H
#define STRATEGO_REPLAY_H
#include "game.h"
/* Read the recorded initial position and validate every move up to before_ply.
   Pass 0 to validate the entire match. No logging data is used by the AI. */
bool replay_load(const char *path,int before_ply,Game *game);
#endif
