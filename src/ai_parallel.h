#ifndef AI_PARALLEL_H
#define AI_PARALLEL_H
/* Independent root branches; deterministic per-branch accumulation. */
void ai_parallel_for(int count,void (*function)(int,void *),void *context);
#endif
