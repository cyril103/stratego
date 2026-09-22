#include "replay.h"
#include <stdio.h>
#include <string.h>
bool replay_load(const char *path,int before_ply,Game *g) {
    FILE *f=fopen(path,"r");if(!f)return false;
    char line[8192];bool ok=false;
    if(!fgets(line,sizeof(line),f))goto done;
    bool legacy_endings=strstr(line,"\"rules\":\"ISF-endings-v1\"")==NULL;
    char *p=strstr(line,"\"board\":[");if(!p)goto done;p+=9;
    game_clear(g);int ids[80]={0},counts[2][12]={{0}};
    for(int s=0;s<100;s++) {
        int side,rank,id,used=0;
        if(sscanf(p,"[%d,%d,%d]%n",&side,&rank,&id,&used)!=3||!used)goto done;
        if(side<0){if(side!=-1||rank!=-1||id!=-1)goto done;}
        else {
            if(side>1||rank<0||rank>11||id<0||id>=80||++ids[id]>1||is_lake(s))goto done;
            counts[side][rank]++;
        }
        g->board[s]=(Piece){rank,side,id,false,false};p+=used;if(s<99){if(*p!=',')goto done;p++;}
    }
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)if(counts[side][r]!=army_counts[r])goto done;
    p=strstr(line,"\"turn\":");if(!p||sscanf(p+7,"%d",&g->turn)!=1||g->turn<0||g->turn>1)goto done;
    while(fgets(line,sizeof(line),f)) {
        if(strstr(line,"\"end\":true")){
            int winner,ply,reason=END_NONE;
            if(sscanf(line,"{\"end\":true,\"winner\":%d,\"ply\":%d}",&winner,&ply)!=2||ply!=g->ply)goto done;
            char *ending=strstr(line,"\"reason\":");if(ending&&sscanf(ending+9,"%d",&reason)!=1)goto done;
            if(g->winner<0){
                if(winner==GAME_DRAW&&reason==END_AGREEMENT)game_agree_draw(g,true,true);
                else if(winner==GAME_DRAW&&reason==END_PLAYING_PERIOD)game_end_playing_period(g);
                else if((winner==HUMAN||winner==COMPUTER)&&reason==END_RESIGNATION)game_resign(g,1-winner);
            }
            if(winner!=g->winner||(ending&&reason!=g->end_reason))goto done;
            ok=before_ply==0||before_ply==g->ply+1;goto done;
        }
        int ply,side,a,d,combat,from,to;
        if(sscanf(line,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d,\"combat\":%d,\"attacker\":%d,\"defender\":%d}",&ply,&side,&from,&to,&combat,&a,&d)!=7)goto done;
        if(ply!=g->ply+1||side!=g->turn)goto done;
        if(before_ply==ply){ok=true;goto done;}
        if(!game_apply(g,(Move){from,to})||g->combat!=combat||g->attack_rank!=a||g->defend_rank!=d)goto done;
        /* Keep historical results reproducible under their original ending
           rule. Newly recorded games explicitly identify the corrected rule. */
        if(legacy_endings&&(g->end_reason==END_IMMOBILE||g->end_reason==END_BOTH_IMMOBILE)){
            if(game_moves(g,g->turn,NULL)){g->winner=GAME_ONGOING;g->end_reason=END_NONE;}
            else {g->winner=1-g->turn;g->end_reason=END_IMMOBILE;}
        }
    }
    ok=before_ply==0||before_ply==g->ply+1;
done:
    fclose(f);return ok;
}
