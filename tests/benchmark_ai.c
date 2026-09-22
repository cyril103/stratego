#include "game.h"
#include "ai_models.h"
#include "ml.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/time.h>
#endif
static double now_seconds(void){
#ifdef _WIN32
    LARGE_INTEGER value,frequency;QueryPerformanceCounter(&value);QueryPerformanceFrequency(&frequency);return (double)value.QuadPart/frequency.QuadPart;
#else
    struct timeval t;gettimeofday(&t,NULL);return t.tv_sec+t.tv_usec/1000000.0;
#endif
}
int main(int argc,char **argv) {
    int pairs=argc>1?atoi(argv[1]):4,limit=argc>2?atoi(argv[2]):500;
    int first_seed=argc>3?atoi(argv[3]):1;
    int only_side=argc>4?atoi(argv[4]):-1;
    bool formation=argc>5&&atoi(argv[5])!=0;
    const char *output=argc>6?argv[6]:"build";
    double playing_period=argc>7?atof(argv[7]):0;
    int model_a=argc>8?atoi(argv[8]):AI_IMPROVED;
    int model_b=argc>9?atoi(argv[9]):AI_CLASSIC;
    const char *policy=argc>10?argv[10]:"assets/models/selfplay.policy";
    if(playing_period<0||pairs<1||limit<1||only_side < -1||only_side>1||model_a<0||model_a>3||model_b<0||model_b>3)return 1;
    if((model_a==AI_LEARNED||model_b==AI_LEARNED)&&!ml_init(policy)){
        fprintf(stderr,"Cannot load learned policy: %s\n",policy);return 2;
    }
    printf("MODELS A=%d (%s) B=%d (%s)\n",model_a,ai_model_name(model_a),model_b,ai_model_name(model_b));
    int wins=0,losses=0,draws=0,unfinished=0,total=0,old_total=0;double seconds=0,max_ms=0,old_seconds=0;
    for(int seed=first_seed;seed<first_seed+pairs;seed++)for(int strong=0;strong<2;strong++) {
        if(only_side>=0&&strong!=only_side)continue;
        Game g;game_init(&g,(uint32_t)(seed*7919));uint32_t rng[2]={123u+(unsigned)seed,987u+(unsigned)seed};
        if(formation){ai_deploy(&g,HUMAN);ai_deploy(&g,COMPUTER);}
        char path[1024];snprintf(path,sizeof(path),"%s/trace_%d_%d.txt",output,seed,strong);FILE *trace=fopen(path,"w");
        snprintf(path,sizeof(path),"%s/match_%d_%d.jsonl",output,seed,strong);FILE *replay=fopen(path,"w");
        if(!trace||!replay){perror("Tournament output");if(trace)fclose(trace);if(replay)fclose(replay);return 2;}
        fprintf(replay,"{\"version\":1,\"rules\":\"ISF-endings-v1\",\"engine\":\"ModelTournament\",\"difficulty\":%d,\"new_side\":%d,\"model_a\":%d,\"model_b\":%d,\"seed\":%d,\"turn\":%d,\"board\":[",model_a,strong,model_a,model_b,seed,g.turn);
        for(int s=0;s<100;s++)fprintf(replay,"%s[%d,%d,%d]",s?",":"",g.board[s].side,g.board[s].rank,g.board[s].id);
        fputs("]}\n",replay);fflush(replay);
        printf("START seed=%d expert_side=%d limit=%d\n",seed,strong,limit);fflush(stdout);
        double match_started=now_seconds();
        double decision_seconds[2]={0,0};int decisions[2]={0,0};
        game_check_end(&g);
        while(g.winner<0&&g.ply<limit) {
            if(playing_period>0&&now_seconds()-match_started>=playing_period){game_end_playing_period(&g);break;}
            bool expert=g.turn==strong;double start=now_seconds();
            Move m=ai_model_choose(&g,expert?model_a:model_b,&rng[g.turn]);
            double elapsed=now_seconds()-start;
            decision_seconds[expert?0:1]+=elapsed;decisions[expert?0:1]++;
            if(expert){seconds+=elapsed;total++;if(elapsed*1000>max_ms)max_ms=elapsed*1000;}
            else {old_seconds+=elapsed;old_total++;}
            if(playing_period>0&&now_seconds()-match_started>=playing_period){game_end_playing_period(&g);break;}
            if(!game_apply(&g,m)){fprintf(stderr,"Illegal move %d -> %d\n",m.from,m.to);return 1;}
            fprintf(replay,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d,\"combat\":%d,\"attacker\":%d,\"defender\":%d}\n",g.ply,1-g.turn,m.from,m.to,g.combat,g.attack_rank,g.defend_rank);fflush(replay);
            if(g.ply%100==0){printf("PROGRESS seed=%d expert_side=%d plies=%d\n",seed,strong,g.ply);fflush(stdout);}
            if(trace&&g.combat!=2)fprintf(trace,"ply %d %s: %s %d->%d attacks %s = %d\n",g.ply,expert?"NEW":"OLD",rank_names[g.attack_rank],m.from,m.to,rank_names[g.defend_rank],g.combat);
        }
        if(g.winner==GAME_DRAW)draws++;else if(g.winner==strong)wins++;else if(g.winner<0)unfinished++;else losses++;
        printf("seed=%d expert_side=%d plies=%d result=%s reason=%d\n",seed,strong,g.ply,g.winner<0?"unfinished":g.winner==GAME_DRAW?"DRAW":g.winner==strong?"WIN":"LOSS",g.end_reason);fflush(stdout);
        fprintf(replay,"{\"end\":true,\"winner\":%d,\"ply\":%d,\"reason\":%d,\"seconds_a\":%.6f,\"decisions_a\":%d,\"seconds_b\":%.6f,\"decisions_b\":%d}\n",g.winner,g.ply,g.end_reason,decision_seconds[0],decisions[0],decision_seconds[1],decisions[1]);fclose(replay);
        if(trace){for(int s=0;s<100;s++)if(g.board[s].side>=0)fprintf(trace,"alive %s %s at %d\n",g.board[s].side==strong?"NEW":"OLD",rank_names[g.board[s].rank],s);fclose(trace);}
    }
    printf("%s vs %s: %d wins / %d losses / %d draws / %d unfinished. A mean %.2f ms, max %.2f ms (%d decisions).\n",ai_model_name(model_a),ai_model_name(model_b),wins,losses,draws,unfinished,seconds*1000/(total?total:1),max_ms,total);
    printf("B mean %.2f ms (%d decisions).\n",old_seconds*1000/(old_total?old_total:1),old_total);
    return 0;
}
