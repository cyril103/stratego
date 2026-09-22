#include "match_log.h"
#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif
static FILE *record;
void match_log_close(const Game *g) {
    if(!record)return;
    fprintf(record,"{\"end\":true,\"winner\":%d,\"ply\":%d,\"reason\":%d}\n",g->winner,g->ply,g->end_reason);
    fclose(record);record=NULL;
}
void match_log_begin(const Game *g,int difficulty) {
    match_log_close(g);
#ifdef _WIN32
    _mkdir("reports");
#else
    mkdir("reports",0755);
#endif
    time_t now=time(NULL);struct tm *local=localtime(&now);char stamp[40],path[150];
    if(!local)return;
    strftime(stamp,sizeof(stamp),"%Y%m%d_%H%M%S",local);
    for(int i=0;i<100;i++){
        snprintf(path,sizeof(path),"reports/partie_%s_%02d.jsonl",stamp,i);
        FILE *existing=fopen(path,"r");if(existing){fclose(existing);continue;}
        record=fopen(path,"w");break;
    }
    if(!record)return;
    fprintf(record,"{\"version\":1,\"rules\":\"ISF-endings-v1\",\"engine\":\"%s\",\"difficulty\":%d,\"turn\":%d,\"board\":[",difficulty==2?"SelfPlayPolicyV1":difficulty==3?"ExpertPlusClassic":difficulty==1?"ExpertPlusImproved":"Decouverte",difficulty,g->turn);
    for(int s=0;s<100;s++)fprintf(record,"%s[%d,%d,%d]",s?",":"",g->board[s].side,g->board[s].rank,g->board[s].id);
    fputs("]}\n",record);fflush(record);
}
void match_log_move(const Game *g,Move m) {
    if(!record)return;
    fprintf(record,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d,\"combat\":%d,\"attacker\":%d,\"defender\":%d}\n",g->ply,1-g->turn,m.from,m.to,g->combat,g->attack_rank,g->defend_rank);
    fflush(record);if(g->winner>=0)match_log_close(g);
}
