#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H
#include "game.h"
#define MAX_STRATEGY_HINTS 64
typedef struct {Move move;float bonus;} StrategyHint;
int ai_strategy_hints(const Game *g,StrategyHint hints[MAX_STRATEGY_HINTS]);
float ai_flag_risk(const Game *g,int side);
float ai_coordination_bonus(const Game *g,Move move);
float ai_preservation_risk(const Game *g,int side);
bool ai_accept_draw(const Game *g,int side);
bool ai_offer_draw(const Game *g,int side,int last_combat_ply,int last_offer_ply);
#endif
