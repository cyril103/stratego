#include "deployment.h"
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* Store only the player's ranks, never an opponent or an in-progress game. */
bool deployment_save(const Deployment *d,const Game *g,const char *path){
    if(g->ply!=0||!deployment_complete(d,g))return false;
    char temp[1024];
    if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp))return false;
    FILE *f=fopen(temp,"w");if(!f)return false;
    bool ok=fprintf(f,"STRATEGO_DEPLOYMENT_V1\n")>0;
    for(int i=0;i<40;i++)if(fprintf(f,"%d%c",g->board[60+i].rank,i%10==9?'\n':' ')<0)ok=false;
    if(fclose(f)!=0)ok=false;
    if(ok){
#ifdef _WIN32
        ok=MoveFileExA(temp,path,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
#else
        ok=rename(temp,path)==0;
#endif
    }
    if(!ok)remove(temp);
    return ok;
}

bool deployment_load(Deployment *d,Game *g,const char *path){
    if(g->ply!=0)return false;
    FILE *f=fopen(path,"r");if(!f)return false;
    char token[64];int ranks[40],counts[12]={0};
    bool ok=fscanf(f,"%63s",token)==1&&!strcmp(token,"STRATEGO_DEPLOYMENT_V1");
    /* Bounded tokens avoid integer overflow even for damaged files. */
    for(int i=0;ok&&i<40;i++){
        ok=fscanf(f,"%63s",token)==1;
        int r=-1;
        if(ok)for(int k=0;k<12;k++){char expected[4];snprintf(expected,sizeof(expected),"%d",k);if(!strcmp(token,expected))r=k;}
        if(r<0)ok=false;else{ranks[i]=r;counts[r]++;}
    }
    if(ok&&fscanf(f,"%63s",token)!=EOF)ok=false;
    if(ferror(f))ok=false;
    if(fclose(f)!=0)ok=false;
    for(int r=0;r<12;r++)if(counts[r]!=army_counts[r])ok=false;
    if(!ok)return false;
    /* Commit only after full validation; opponent and match state stay intact. */
    for(int i=0;i<40;i++){
        g->board[60+i]=(Piece){ranks[i],HUMAN,i,false,false};
        d->reserve[i]=empty_piece();
    }
    return true;
}
