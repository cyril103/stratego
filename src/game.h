#ifndef STRATEGO_GAME_H
#define STRATEGO_GAME_H
#include <stdbool.h>
#include <stdint.h>
#define BOARD 100
#define MAX_MOVES 1600
enum { HUMAN, COMPUTER };
enum { GAME_ONGOING=-1, GAME_DRAW=2 };
enum { END_NONE, END_FLAG, END_IMMOBILE, END_BOTH_IMMOBILE, END_RESIGNATION, END_AGREEMENT, END_PLAYING_PERIOD };
enum { FLAG, SPY, SCOUT, MINER, SERGEANT, LIEUTENANT, CAPTAIN, MAJOR, COLONEL, GENERAL, MARSHAL, BOMB };
typedef struct { int rank, side, id; bool revealed, moved; } Piece;
typedef struct { int from, to; } Move;
typedef struct {
    Piece board[BOARD];
    int turn, winner, ply, end_reason;
    int captured[2][12];
    int last_id[2], last_from[2], last_to[2], repetitions[2];
    Move last_move;
    Move history[2][8];
    int history_id[2][8], history_count[2];
    int attack_rank, defend_rank, combat; /* -1 attacker lost, 0 mutual, 1 won, 2 no combat */
    uint32_t rng;
} Game;
extern const int army_counts[12];
extern const char *rank_names[12];
extern const char *rank_symbols[12];
uint32_t game_random(uint32_t *state);
Piece empty_piece(void);
bool is_lake(int square);
bool movable(Piece p);
void game_clear(Game *g);
void game_init(Game *g, uint32_t seed);
void game_deploy(Game *g, int side);
void ai_deploy(Game *g, int side);
#define AI_FORMATIONS 6
void ai_deploy_template(Game *g,int side,int variant);
bool game_legal(const Game *g, Move m, int side);
int game_moves(const Game *g, int side, Move out[MAX_MOVES]);
int combat_result(int attacker, int defender);
bool game_apply(Game *g, Move m);
void game_check_end(Game *g);
bool game_resign(Game *g,int side);
bool game_agree_draw(Game *g,bool human_agrees,bool computer_agrees);
bool game_end_playing_period(Game *g);
Move ai_choose(const Game *g, int difficulty, uint32_t *rng);
Move ai_basic(const Game *g, int difficulty, uint32_t *rng);
#endif
