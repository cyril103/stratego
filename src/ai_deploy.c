#include "game.h"
/* Original constrained templates; see tests/DEPLOYMENT_RESEARCH.md. Local
   coordinates run from the rear (row 0) to the front (row 3). */
typedef struct {int flag,bombs[6],guard,marshal;} Formation;
static const Formation formations[AI_FORMATIONS]={
    {0,{1,10,7,8,18,27},11,24},
    {2,{1,3,12,7,8,18},13,21},
    {4,{3,5,14,7,8,18},15,28},
    {8,{7,9,18,1,2,12},17,25},
    {12,{2,11,13,22,8,18},23,20},
    {5,{0,10,1,8,18,9},15,24}
};
static void place(int ranks[40],int left[12],int cell,int rank){
    if(cell>=0&&cell<40&&ranks[cell]<0&&left[rank]>0){ranks[cell]=rank;left[rank]--;}
}
static void in_lane(int ranks[40],int left[12],int lane,int rank){
    static const int cells[3][4]={{20,21,10,11},{24,25,14,15},{28,29,18,19}};
    for(int i=0;i<4;i++)if(ranks[cells[lane][i]]<0){place(ranks,left,cells[lane][i],rank);return;}
}
void ai_deploy_template(Game *g,int side,int variant){
    if(variant<0||variant>=AI_FORMATIONS)variant=0;
    const Formation *f=&formations[variant];
    int ranks[40],left[12];for(int r=0;r<12;r++)left[r]=army_counts[r];
    for(int i=0;i<40;i++)ranks[i]=-1;
    place(ranks,left,f->flag,FLAG);
    for(int i=0;i<6;i++)place(ranks,left,f->bombs[i],BOMB);
    place(ranks,left,f->guard,CAPTAIN);
    if(variant==5){place(ranks,left,4,MAJOR);place(ranks,left,6,LIEUTENANT);}
    place(ranks,left,f->marshal,MARSHAL);
    int spy=f->marshal-10;
    if(ranks[spy]>=0)spy=f->marshal+(f->marshal%10<9?1:-1);
    place(ranks,left,spy,SPY);
    int front[10]={SCOUT,SERGEANT,SCOUT,LIEUTENANT,MINER,CAPTAIN,LIEUTENANT,SCOUT,SERGEANT,SCOUT};
    for(int lane=0;lane<3;lane++)if(game_random(&g->rng)%2){int x=lane==0?0:lane==1?4:8;int t=front[x];front[x]=front[x+1];front[x+1]=t;}
    for(int x=0;x<10;x++)place(ranks,left,30+x,front[x]);
    int main=f->marshal%10<=1?0:f->marshal%10>=8?2:1;
    in_lane(ranks,left,main==0?2:0,GENERAL);
    in_lane(ranks,left,0,MINER);in_lane(ranks,left,2,MINER);
    /* Each lane receives an officer capable of backing up its first probes. */
    for(int lane=0;lane<3;lane++){
        int col=lane==0?0:lane==1?4:8;bool officer=false;
        for(int row=2;row<=3;row++)for(int x=col;x<col+2;x++)if(ranks[row*10+x]>=CAPTAIN&&ranks[row*10+x]<=MARSHAL)officer=true;
        if(!officer)in_lane(ranks,left,lane,COLONEL);
    }
    int cells[10]={24,25,20,21,28,29,22,23,26,27};
    for(int i=0;i<10&&left[COLONEL];i++)place(ranks,left,cells[i],COLONEL);
    int pool[40],n=0;for(int r=0;r<12;r++)for(int k=0;k<left[r];k++)pool[n++]=r;
    for(int i=n-1;i>0;i--){int j=(int)(game_random(&g->rng)%(i+1));int t=pool[i];pool[i]=pool[j];pool[j]=t;}
    bool mirror=game_random(&g->rng)%2;
    for(int i=0;i<40;i++){
        if(ranks[i]<0)ranks[i]=pool[--n];
        int row=i/10,col=mirror?9-i%10:i%10;
        int s=(side==COMPUTER?row:9-row)*10+col;
        g->board[s]=(Piece){ranks[i],side,side*40+i,false,false};
    }
}
void ai_deploy(Game *g,int side){ai_deploy_template(g,side,(int)(game_random(&g->rng)%AI_FORMATIONS));}
