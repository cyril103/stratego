#include "ai_worker.h"
#include "ai_models.h"
Move ai_previous(const Game *g,int difficulty,uint32_t *rng);
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static void pause_thread(void){Sleep(1);}
#else
#include <time.h>
static void pause_thread(void){struct timespec delay={0,1000000};nanosleep(&delay,NULL);}
#endif
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Worker check failed: %s\n",#x);exit(1);}}while(0)
int main(void) {
    Game g;game_init(&g,24);uint32_t expected_rng=91;
    Move expected=ai_choose(&g,0,&expected_rng),actual;uint32_t actual_rng=0;
    CHECK(ai_worker_start(&g,0,91));CHECK(!ai_worker_start(&g,0,12));
    /* The worker owns a snapshot, independent of UI/new-game mutations. */
    game_init(&g,88);
    while(!ai_worker_poll(&actual,&actual_rng))pause_thread();
    CHECK(!ai_worker_busy());CHECK(actual.from==expected.from&&actual.to==expected.to);CHECK(actual_rng==expected_rng);
    CHECK(ai_worker_start(&g,1,91));ai_worker_stop();CHECK(!ai_worker_busy());CHECK(!ai_worker_poll(&actual,&actual_rng));
    expected_rng=10;expected=ai_choose(&g,0,&expected_rng);
    CHECK(ai_worker_start(&g,0,10));while(!ai_worker_poll(&actual,&actual_rng))pause_thread();
    CHECK(actual.from==expected.from&&actual.to==expected.to);CHECK(actual_rng==expected_rng);
    for(int model=AI_IMPROVED;model<=AI_CLASSIC;model+=2){
        expected_rng=73;expected=model==AI_CLASSIC?ai_previous(&g,1,&expected_rng):ai_choose(&g,1,&expected_rng);
        CHECK(ai_worker_start(&g,model,73));while(!ai_worker_poll(&actual,&actual_rng))pause_thread();
        CHECK(actual.from==expected.from&&actual.to==expected.to);CHECK(actual_rng==expected_rng);
    }
    puts("Worker snapshots, model selection, stop, restart and result synchronization: OK");return 0;
}
