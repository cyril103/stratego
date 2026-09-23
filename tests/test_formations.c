#include "game.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Formation %d seed %d side %d line %d: %s\n",variant,seed,side,__LINE__,#x);return 1;}}while(0)
int main(void){
    int flag_positions[100]={0},evolved_positions[100]={0},checked=0,unique=0;
    uint64_t layouts[200]={0};int front_hist[3]={0};
    for(int variant=0;variant<=AI_FORMATIONS;variant++)for(int seed=1;seed<=200;seed++)for(int side=0;side<2;side++){
        Game g;game_init(&g,seed);
        if(variant==AI_FORMATIONS)ai_deploy(&g,side);else ai_deploy_template(&g,side,variant);
        int count[12]={0},ids[80]={0},marshal=-1,spy=-1;
        for(int s=0;s<100;s++)if(g.board[s].side==side){
            Piece p=g.board[s];CHECK(p.rank>=0&&p.rank<12);count[p.rank]++;
            CHECK(p.id>=side*40&&p.id<side*40+40);CHECK(++ids[p.id]==1);
            CHECK(side==COMPUTER?s<40:s>=60);
            if(p.rank==MARSHAL)marshal=s;
            if(p.rank==SPY)spy=s;
            if(p.rank==FLAG&&side==COMPUTER&&variant<AI_FORMATIONS)flag_positions[s]++;
        }
        for(int r=0;r<12;r++)CHECK(count[r]==army_counts[r]);
        if(variant==AI_FORMATIONS&&side==COMPUTER){
            uint64_t layout=0;int flag=-1;
            for(int s=0;s<40;s++){if(g.board[s].rank==BOMB)layout|=UINT64_C(1)<<s;if(g.board[s].rank==FLAG)flag=s;}
            CHECK(flag>=0&&flag<20);evolved_positions[flag]++;
            layout|=(uint64_t)flag<<40;
            bool found=false;for(int i=0;i<unique;i++)if(layouts[i]==layout)found=true;
            if(!found)layouts[unique++]=layout;
            bool decoy=false;int probes=0,front_bombs=0;
            for(int s=0;s<20;s++)if(movable(g.board[s])){
                int adj[4]={s>=10?s-10:-1,s+10,s%10?s-1:-1,s%10<9?s+1:-1},bombs=0;
                for(int k=0;k<4;k++)if(adj[k]>=0&&g.board[adj[k]].rank==BOMB)bombs++;
                if(bombs>=2)decoy=true;
            }
            for(int s=30;s<40;s++){
                CHECK(g.board[s].rank!=SPY&&(g.board[s].rank<MAJOR||g.board[s].rank==BOMB));
                probes+=g.board[s].rank==SCOUT;front_bombs+=g.board[s].rank==BOMB;
            }
            CHECK(decoy&&probes>=3&&front_bombs<=2);front_hist[front_bombs]++;
        }
        CHECK((marshal%10-spy%10)*(marshal%10-spy%10)+(marshal/10-spy/10)*(marshal/10-spy/10)==1);
        /* Ignoring other mobile troops, every mobile piece can reach an open
           front lane without ever requiring a friendly bomb to disappear. */
        bool reachable[100]={false};int queue[100],head=0,tail=0;
        for(int s=0;s<100;s++)if(g.board[s].side==side&&movable(g.board[s])&&s/10==(side==COMPUTER?3:6)&&!is_lake(s+(side==COMPUTER?10:-10))){reachable[s]=true;queue[tail++]=s;}
        while(head<tail){int s=queue[head++],nb[4]={s>=10?s-10:-1,s<90?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1};for(int k=0;k<4;k++){int t=nb[k];if(t>=0&&!reachable[t]&&g.board[t].side==side&&movable(g.board[t])){reachable[t]=true;queue[tail++]=t;}}}
        for(int s=0;s<100;s++)if(g.board[s].side==side&&movable(g.board[s]))CHECK(reachable[s]);
        for(int lane=0;lane<3;lane++){
            int col=lane==0?0:lane==1?4:8;bool miner=false,officer=false;
            for(int row=0;row<4;row++)for(int x=col;x<col+2;x++){
                Piece p=g.board[(side==COMPUTER?row:9-row)*10+x];
                if(p.rank==MINER)miner=true;
                if(row>=2&&p.rank>=CAPTAIN&&p.rank<=MARSHAL)officer=true;
            }
            CHECK(miner&&officer);
        }
        Game same;game_init(&same,seed);
        if(variant==AI_FORMATIONS)ai_deploy(&same,side);else ai_deploy_template(&same,side,variant);
        for(int s=0;s<100;s++)CHECK(g.board[s].rank==same.board[s].rank&&g.board[s].id==same.board[s].id);
        checked++;
    }
    int positions=0;for(int s=0;s<100;s++)positions+=flag_positions[s]>0;
    int evolved=0;for(int s=0;s<100;s++)evolved+=evolved_positions[s]>0;
    printf("%d formations checked, %d template flag positions, %d evolved flag positions, %d bomb/flag topologies: inventory, lanes, escape routes, spy and reproducibility OK\n",checked,positions,evolved,unique);
    printf("Front bomb counts: zero %d, one %d, two %d\n",front_hist[0],front_hist[1],front_hist[2]);
    return positions<8||evolved<12||unique<100||!front_hist[0]||!front_hist[1]||!front_hist[2];
}
