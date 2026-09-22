/* Paired diagnostic continuations, separate from complete tournament games. */
#include "game.h"
#include "ai_models.h"
#include "replay.h"
#include <stdio.h>
#include <stdlib.h>
int main(int argc,char **argv){
    if(argc!=6){fputs("Usage: continue_match replay before_ply improved_side additional_plies output\n",stderr);return 1;}
    Game g;int before=atoi(argv[2]),side=atoi(argv[3]),limit=atoi(argv[4]);
    if(before<1||side<0||side>1||limit<1||!replay_load(argv[1],before,&g))return 2;
    FILE *source=fopen(argv[1],"r"),*out=fopen(argv[5],"w");
    if(!source||!out){if(source)fclose(source);if(out)fclose(out);return 3;}
    char line[8192];for(int i=0;i<=g.ply;i++){if(!fgets(line,sizeof(line),source))return 4;fputs(line,out);}fclose(source);
    uint32_t rng[2]={20260917,20260918};int start=g.ply;
    while(g.winner<0&&g.ply-start<limit){
        Move m=ai_model_choose(&g,g.turn==side?AI_IMPROVED:AI_CLASSIC,&rng[g.turn]);
        if(!game_apply(&g,m)){fclose(out);return 5;}
        fprintf(out,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d,\"combat\":%d,\"attacker\":%d,\"defender\":%d}\n",g.ply,1-g.turn,m.from,m.to,g.combat,g.attack_rank,g.defend_rank);
        fflush(out);
    }
    fprintf(out,"{\"end\":true,\"winner\":%d,\"ply\":%d,\"reason\":%d}\n",g.winner,g.ply,g.end_reason);fclose(out);
    printf("Continuation: initial ply %d, final ply %d, winner %d, reason %d, improved side %d\n",start,g.ply,g.winner,g.end_reason,side);
    return 0;
}
