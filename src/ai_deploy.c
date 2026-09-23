#include "game.h"
#include <stdlib.h>
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
/* Randomize the topology as well as the mobile ranks. A recognisable bomb
   template must not identify the flag. Rejection preserves deployment roles
   and escape routes; every accepted swap keeps the exact army inventory. */
static bool deploy_viable(const int ranks[40]){
    int flag=-1,marshal=-1,spy=-1,front_scouts=0,front_miners=0,front_bombs=0;
    for(int s=0;s<40;s++){
        if(ranks[s]==FLAG)flag=s;
        if(ranks[s]==MARSHAL)marshal=s;
        if(ranks[s]==SPY)spy=s;
        if(s>=30){
            if(ranks[s]==SPY||(ranks[s]>=MAJOR&&ranks[s]<=MARSHAL))return false;
            front_scouts+=ranks[s]==SCOUT;front_miners+=ranks[s]==MINER;front_bombs+=ranks[s]==BOMB;
        }
    }
    if(flag<0||flag>=20||marshal<0||spy<0)return false;
    /* A permanently bomb-free front would advertise safe officer raids.
       Keep some uncertainty without sealing the army behind its own bombs. */
    if(front_scouts<3||front_miners>2||front_bombs>2)return false;
    int dx=marshal%10-spy%10,dy=marshal/10-spy/10;
    if(dx*dx+dy*dy!=1)return false;
    int nb[4]={flag>=10?flag-10:-1,flag+10,flag%10?flag-1:-1,flag%10<9?flag+1:-1};
    int closed=0;bool guard=false;
    for(int k=0;k<4;k++)closed+=nb[k]<0||ranks[nb[k]]==BOMB;
    if(closed<2)return false;
    for(int s=0;s<30;s++)if(ranks[s]>=CAPTAIN&&ranks[s]<=MARSHAL){
        int distance=abs(s%10-flag%10)+abs(s/10-flag/10);
        if(distance<=2)guard=true;
    }
    if(!guard)return false;
    bool decoy=false;
    for(int s=0;s<20;s++)if(ranks[s]>FLAG&&ranks[s]<BOMB){
        int adj[4]={s>=10?s-10:-1,s+10,s%10?s-1:-1,s%10<9?s+1:-1},bombs=0;
        for(int k=0;k<4;k++)if(adj[k]>=0&&ranks[adj[k]]==BOMB)bombs++;
        if(bombs>=2)decoy=true;
    }
    if(!decoy)return false;
    for(int lane=0;lane<3;lane++){
        int col=lane==0?0:lane==1?4:8;bool miner=false,officer=false;
        for(int row=0;row<4;row++)for(int x=col;x<col+2;x++){
            int rank=ranks[row*10+x];
            if(rank==MINER)miner=true;
            if(row>=2&&rank>=CAPTAIN&&rank<=MARSHAL)officer=true;
        }
        if(!miner||!officer)return false;
    }
    bool seen[40]={false};int queue[40],head=0,tail=0;
    for(int x=0;x<10;x++)if(x<2||(x>=4&&x<6)||x>=8){
        int s=30+x;if(ranks[s]>FLAG&&ranks[s]<BOMB){seen[s]=true;queue[tail++]=s;}
    }
    while(head<tail){
        int s=queue[head++],adj[4]={s>=10?s-10:-1,s<30?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1};
        for(int k=0;k<4;k++){int t=adj[k];if(t>=0&&!seen[t]&&ranks[t]>FLAG&&ranks[t]<BOMB){seen[t]=true;queue[tail++]=t;}}
    }
    for(int s=0;s<40;s++)if(ranks[s]>FLAG&&ranks[s]<BOMB&&!seen[s])return false;
    return true;
}
void ai_deploy(Game *g,int side){
    ai_deploy_template(g,side,(int)(game_random(&g->rng)%AI_FORMATIONS));
    int ranks[40];
    for(int s=0;s<40;s++)ranks[s]=g->board[(side==COMPUTER?s/10:9-s/10)*10+s%10].rank;
    for(int attempt=0;attempt<240;attempt++){
        int a=(int)(game_random(&g->rng)%40),b=(int)(game_random(&g->rng)%40);
        if(ranks[a]==ranks[b])continue;
        int t=ranks[a];ranks[a]=ranks[b];ranks[b]=t;
        if(!deploy_viable(ranks)){t=ranks[a];ranks[a]=ranks[b];ranks[b]=t;}
    }
    for(int s=0;s<40;s++)g->board[(side==COMPUTER?s/10:9-s/10)*10+s%10]=(Piece){ranks[s],side,side*40+s,false,false};
}
