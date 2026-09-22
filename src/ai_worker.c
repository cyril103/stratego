#include "ai_worker.h"
#include "ai_models.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static HANDLE worker;
#else
#include <pthread.h>
static pthread_t worker;
static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
static bool completed;
#endif
static struct {Game game;int difficulty;uint32_t rng;Move result;} job;
static bool active;
#ifdef _WIN32
static DWORD WINAPI run_search(LPVOID unused) {
    (void)unused;job.result=ai_model_choose(&job.game,job.difficulty,&job.rng);return 0;
}
#else
static void *run_search(void *unused) {
    (void)unused;job.result=ai_model_choose(&job.game,job.difficulty,&job.rng);
    pthread_mutex_lock(&lock);completed=true;pthread_mutex_unlock(&lock);return NULL;
}
#endif
bool ai_worker_busy(void) {return active;}
bool ai_worker_start(const Game *g,int difficulty,uint32_t rng) {
    if(active)return false;
    job.game=*g;job.difficulty=difficulty;job.rng=rng;job.result=(Move){-1,-1};
#ifdef _WIN32
    worker=CreateThread(NULL,0,run_search,NULL,0,NULL);active=worker!=NULL;
#else
    completed=false;active=pthread_create(&worker,NULL,run_search,NULL)==0;
#endif
    return active;
}
bool ai_worker_poll(Move *move,uint32_t *rng) {
    if(!active)return false;
#ifdef _WIN32
    if(WaitForSingleObject(worker,0)!=WAIT_OBJECT_0)return false;
    CloseHandle(worker);worker=NULL;
#else
    pthread_mutex_lock(&lock);bool done=completed;pthread_mutex_unlock(&lock);
    if(!done)return false;
    pthread_join(worker,NULL);
#endif
    *move=job.result;*rng=job.rng;active=false;return true;
}
void ai_worker_stop(void) {
    if(!active)return;
#ifdef _WIN32
    WaitForSingleObject(worker,INFINITE);CloseHandle(worker);worker=NULL;
#else
    pthread_join(worker,NULL);
#endif
    active=false;
}
