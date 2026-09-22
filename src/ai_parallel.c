#include "ai_parallel.h"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <pthread.h>
#endif
typedef struct {int lane,count;void (*function)(int,void *);void *context;} Work;
static void process(Work *w){for(int i=w->lane;i<w->count;i+=4)w->function(i,w->context);}
#ifdef _WIN32
static DWORD WINAPI run(LPVOID p){process((Work *)p);return 0;}
#else
static void *run(void *p){process((Work *)p);return 0;}
#endif
void ai_parallel_for(int count,void (*function)(int,void *),void *context) {
    if(count<4){for(int i=0;i<count;i++)function(i,context);return;}
    Work jobs[4];
#ifdef _WIN32
    HANDLE handles[3]={0};
#else
    pthread_t handles[3];int created[3]={0};
#endif
    for(int i=0;i<4;i++)jobs[i]=(Work){i,count,function,context};
    for(int i=0;i<3;i++){
#ifdef _WIN32
        handles[i]=CreateThread(NULL,0,run,&jobs[i],0,NULL);
        if(!handles[i])process(&jobs[i]);
#else
        created[i]=pthread_create(&handles[i],NULL,run,&jobs[i])==0;
        if(!created[i])process(&jobs[i]);
#endif
    }
    process(&jobs[3]);
    for(int i=0;i<3;i++){
#ifdef _WIN32
        if(handles[i]){WaitForSingleObject(handles[i],INFINITE);CloseHandle(handles[i]);}
#else
        if(created[i])pthread_join(handles[i],NULL);
#endif
    }
}
