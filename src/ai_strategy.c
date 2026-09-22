#include "ai_strategy.h"
#include <stdlib.h>
#include <math.h>
/* Agreement is an AI decision, not an automatic draw rule. Only public
   casualties and revealed capture opportunities influence the response. */
bool ai_accept_draw(const Game *g,int side){
    if(g->winner>=0||g->ply<120)return false;
    static const int values[12]={0,7,4,12,7,9,12,17,24,35,50,0};
    int material[2]={0};
    for(int s=0;s<2;s++)for(int r=1;r<=MARSHAL;r++)material[s]+=(army_counts[r]-g->captured[s][r])*values[r];
    if(abs(material[side]-material[1-side])>8)return false;
    Move moves[MAX_MOVES];int count=game_moves(g,side,moves);
    for(int i=0;i<count;i++){
        Piece a=g->board[moves[i].from],d=g->board[moves[i].to];
        if(d.side==1-side&&d.revealed&&combat_result(a.rank,d.rank)>0&&(d.rank==FLAG||d.rank>=COLONEL))return false;
    }
    return true;
}
/* Quiet, balanced positions justify an offer, never an automatic result.
   Any available attack warrants continued exploration instead. */
bool ai_offer_draw(const Game *g,int side,int last_combat_ply,int last_offer_ply){
    if(g->turn!=side||g->ply<200||g->ply-last_combat_ply<80||g->ply-last_offer_ply<100)return false;
    if(!ai_accept_draw(g,side))return false;
    Move moves[MAX_MOVES];int n=game_moves(g,side,moves);
    if(!n)return false;
    for(int i=0;i<n;i++)if(g->board[moves[i].to].side==1-side)return false;
    return true;
}
static int adjacent(int s,int out[4]) {
    int n=0;if(s>=10)out[n++]=s-10;if(s<90)out[n++]=s+10;if(s%10)out[n++]=s-1;if(s%10<9)out[n++]=s+1;return n;
}
/* Use only revealed enemy ranks. Hidden ranks are never consulted, even when
   checking whether a future corridor would be safe for a commander. */
static bool controlled(const Game *g,int side,int rank,int square,int vacated) {
    for(int s=0;s<100;s++){
        Piece enemy=g->board[s];if(enemy.side!=1-side||!enemy.revealed||!movable(enemy))continue;
        int dx=s%10-square%10,dz=s/10-square/10;if(dx&&dz)continue;
        int distance=abs(dx)+abs(dz);if(!distance)continue;
        if(distance>1&&enemy.rank!=SCOUT)continue;
        int step=dx?(dx>0?1:-1):(dz>0?10:-10);bool blocked=false;
        for(int t=square+step;t!=s;t+=step)if(is_lake(t)||(t!=vacated&&g->board[t].side>=0)){blocked=true;break;}
        if(!blocked&&combat_result(enemy.rank,rank)>=0)return true;
    }
    return false;
}
/* Conservative public pursuit distance: known mobile screens can move aside;
   lakes, our bombs and unconfirmed enemy blockers stop the projected route. */
static void pursuit_distances(const Game *g,int start,int dist[100]) {
    int queue[100],head=0,tail=0;
    for(int s=0;s<100;s++)dist[s]=100;
    dist[start]=0;queue[tail++]=start;
    while(head<tail){
        int s=queue[head++];if(dist[s]>=5)continue;
        int nb[4],n=adjacent(s,nb);
        for(int k=0;k<n;k++){
            int t=nb[k];Piece p=g->board[t];
            if(is_lake(t)||dist[t]!=100)continue;
            if(p.side==g->board[start].side&&(!p.revealed||!movable(p)))continue;
            if(p.side==1-g->board[start].side&&p.rank==BOMB)continue;
            dist[t]=dist[s]+1;queue[tail++]=t;
        }
    }
}
float ai_preservation_risk(const Game *g,int side) {
    static const float value[12]={0,7,4,12,7,9,12,17,24,35,50,0};
    static const float danger[6]={0,2.0f,1.25f,.75f,.35f,.1f};
    float exposure[100]={0},risk=0;
    for(int e=0;e<100;e++){
        Piece hunter=g->board[e];
        if(hunter.side!=1-side||!hunter.revealed||!movable(hunter))continue;
        int dist[100];pursuit_distances(g,e,dist);
        for(int s=0;s<100;s++){
            Piece officer=g->board[s];
            if(officer.side!=side||officer.rank<COLONEL||officer.rank>MARSHAL||dist[s]>5||combat_result(hunter.rank,officer.rank)<=0)continue;
            /* A friendly superior can cover the threatened officer. */
            bool covered=false;int nb[4],n=adjacent(s,nb);
            for(int k=0;k<n;k++){Piece p=g->board[nb[k]];if(p.side==side&&movable(p)&&combat_result(p.rank,hunter.rank)>=0)covered=true;}
            float loss=value[officer.rank]*danger[dist[s]]*(covered?.2f:1);
            if(!covered){
                /* A chase near a dead end needs action before contact. Count
                   safe exits using public occupants, including known captures. */
                int exits=0;
                for(int k=0;k<n;k++){
                    int t=nb[k];Piece p=g->board[t];
                    if(is_lake(t)||p.side==side||controlled(g,side,officer.rank,t,s))continue;
                    if(p.side>=0&&(!p.revealed||combat_result(officer.rank,p.rank)<=0))continue;
                    exits++;
                }
                loss+=value[officer.rank]*(exits==0?1.0f:exits==1?.7f:0);
                int advance=side==COMPUTER?s/10:9-s/10;
                loss+=value[officer.rank]*1.5f*fmaxf(0,advance-4);
            }
            if(loss>exposure[s])exposure[s]=loss;
        }
    }
    for(int s=0;s<100;s++)risk+=exposure[s];
    /* Keep a replaceable reserve near the bomb screen even before an invasion
       is visible. Miners cannot substitute for an officer against enemy miners.
       Count route distance, not proximity through an impassable bomb wall. */
    int flag=-1,bombs=0;
    for(int s=0;s<100;s++)if(g->board[s].side==side){if(g->board[s].rank==FLAG)flag=s;if(g->board[s].rank==BOMB)bombs++;}
    if(flag>=0&&bombs&&g->captured[1-side][MINER]<army_counts[MINER]){
        int gates[4],gate_count=adjacent(flag,gates);
        for(int gate_index=0;gate_index<gate_count;gate_index++){
            int gate=gates[gate_index];Piece screen=g->board[gate];
            if(is_lake(gate))continue;
            int dist[100],queue[100],head=0,tail=0;
            for(int s=0;s<100;s++)dist[s]=100;
            int starts[4],count=1;starts[0]=gate;
            if(screen.side==side&&screen.rank==BOMB)count=adjacent(gate,starts);
            for(int k=0;k<count;k++){int t=starts[k];Piece p=g->board[t];if(is_lake(t)||dist[t]!=100||(p.side==side&&!movable(p)))continue;dist[t]=0;queue[tail++]=t;}
            while(head<tail){int s=queue[head++],nb[4],n=adjacent(s,nb);if(dist[s]>=6)continue;
                for(int k=0;k<n;k++){int t=nb[k];Piece p=g->board[t];if(is_lake(t)||dist[t]!=100||(p.side>=0&&(p.side!=side||!movable(p))))continue;dist[t]=dist[s]+1;queue[tail++]=t;}
            }
            int nearest=6;
            for(int s=0;s<100;s++){Piece p=g->board[s];if(p.side==side&&p.rank>=SERGEANT&&p.rank<=MARSHAL&&dist[s]<nearest)nearest=dist[s];}
            float pressure=1;
            for(int e=0;e<100;e++){
                Piece p=g->board[e];if(p.side!=1-side||(!p.moved&&!p.revealed)||(p.revealed&&!movable(p)))continue;
                int distance=abs(e%10-gate%10)+abs(e/10-gate/10);
                pressure=fmaxf(pressure,1+fmaxf(0,6-distance));
            }
            risk+=5*pressure*fmaxf(0,nearest-1);
        }
    }
    return risk;
}
static int clear_chain(const Game *g,int square,int depth,bool visited[100],Move *first) {
    Piece blocker=g->board[square];
    if(blocker.side!=g->turn||!movable(blocker)||!depth||visited[square])return 99;
    visited[square]=true;int nb[4],n=adjacent(square,nb),best=99;
    for(int k=0;k<n;k++) {
        int t=nb[k];if(is_lake(t)||visited[t])continue;
        if(g->board[t].side<0) {
            Move m={square,t};
            if(game_legal(g,m,g->turn)&&!controlled(g,g->turn,blocker.rank,t,square)){best=1;*first=m;break;}
        } else if(g->board[t].side==g->turn) {
            Move move;int cost=clear_chain(g,t,depth-1,visited,&move);
            if(cost+1<best){best=cost+1;*first=move;}
        }
    }
    visited[square]=false;return best;
}
static void add_hint(StrategyHint out[MAX_STRATEGY_HINTS],int *count,Move m,float bonus) {
    for(int i=0;i<*count;i++)if(out[i].move.from==m.from&&out[i].move.to==m.to){if(bonus>out[i].bonus)out[i].bonus=bonus;return;}
    if(*count<MAX_STRATEGY_HINTS)out[(*count)++]=(StrategyHint){m,bonus};
}
/* Plan an officer's route through the friendly screen. A friendly blocker
   costs both its clearance and the officer's next step, rather than being
   treated as either an impassable wall or an already-open square. */
static void defensive_clearance(const Game *g,StrategyHint out[MAX_STRATEGY_HINTS],int *count){
    int flag=-1;
    for(int s=0;s<100;s++)if(g->board[s].side==g->turn&&g->board[s].rank==FLAG)flag=s;
    if(flag<0)return;
    bool invasion=false;
    for(int s=0;s<100;s++)if(g->board[s].side==1-g->turn&&(g->board[s].moved||g->board[s].revealed)&&abs(s%10-flag%10)+abs(s/10-flag/10)<=6)invasion=true;
    if(!invasion)return;
    for(int start=0;start<100;start++){
        Piece officer=g->board[start];if(officer.side!=g->turn||officer.rank<CAPTAIN||officer.rank>MARSHAL)continue;
        int dist[100];bool done[100]={false},needs_clearance[100]={false};Move first[100];
        for(int s=0;s<100;s++){dist[s]=100;first[s]=(Move){-1,-1};}dist[start]=0;
        for(int iteration=0;iteration<100;iteration++){
            int s=-1;for(int t=0;t<100;t++)if(!done[t]&&(s<0||dist[t]<dist[s]))s=t;
            if(s<0||dist[s]>7)break;
            done[s]=true;
            Piece target=g->board[s];
            if(target.side==1-g->turn){
                int home=abs(s%10-flag%10)+abs(s/10-flag/10);
                bool raider=target.revealed&&target.rank>=MAJOR&&target.rank<=MARSHAL&&
                    (g->turn==COMPUTER?s/10<=5:s/10>=4);
                if(home<=6&&(target.moved||target.revealed)&&first[s].from>=0&&(needs_clearance[s]||raider)){
                    float urgency=(7-home)*(target.revealed&&target.rank==MINER?1.4f:1);
                    if(raider)urgency+=(target.rank-5)*2;
                    add_hint(out,count,first[s],28*urgency/(dist[s]+2));
                }
                /* Equal ranks can neutralize an invader, but neither piece
                   survives to continue along this route. */
                if(target.revealed&&combat_result(officer.rank,target.rank)==0)continue;
            }
            int nb[4],n=adjacent(s,nb);
            for(int k=0;k<n;k++){
                int t=nb[k];Piece p=g->board[t];if(done[t]||is_lake(t))continue;
                Move step={start,t};int cost=1;
                if(p.side==g->turn){
                    if(!movable(p))continue;
                    bool visited[100]={false};visited[start]=true;
                    int clearance=clear_chain(g,t,3,visited,&step);if(clearance>=99)continue;
                    cost+=clearance;
                }else if(p.side>=0){
                    if(!p.revealed){if(!p.moved)continue;cost+=2;}
                    else if(combat_result(officer.rank,p.rank)<0)continue;
                }
                if(controlled(g,officer.side,officer.rank,t,start))continue;
                if(dist[t]>dist[s]+cost){
                    Move candidate=s==start?step:first[s];
                    if(candidate.from<0||!game_legal(g,candidate,g->turn))continue;
                    dist[t]=dist[s]+cost;first[t]=candidate;
                    needs_clearance[t]=needs_clearance[s]||p.side==g->turn||
                        (p.side<0&&g->last_from[g->turn]==t&&g->last_id[g->turn]!=officer.id);
                }
            }
        }
    }
}
int ai_strategy_hints(const Game *g,StrategyHint out[MAX_STRATEGY_HINTS]) {
    int count=0;
    defensive_clearance(g,out,&count);
    for(int s=0;s<100;s++) {
        Piece key=g->board[s];if(key.side!=g->turn||key.rank<COLONEL||key.rank>MARSHAL)continue;
        int nb[4],n=adjacent(s,nb),safe_count=0;Move escape={-1,-1};
        for(int k=0;k<n;k++) {
            int t=nb[k];if(g->board[t].side<0&&game_legal(g,(Move){s,t},g->turn)&&!controlled(g,key.side,key.rank,t,s)){
                safe_count++;escape=(Move){s,t};
            }
        }
        if(safe_count>1)continue;
        /* Follow through once a useful corridor has actually been opened. */
        if(safe_count==1){add_hint(out,&count,escape,(float)(key.rank-5)*.2f);continue;}
        for(int k=0;k<n;k++) {
            int t=nb[k];if(g->board[t].side!=key.side||!movable(g->board[t]))continue;
            if(controlled(g,key.side,key.rank,t,s))continue;
            bool visited[100]={false};visited[s]=true;Move first;
            int cost=clear_chain(g,t,3,visited,&first);
            if(cost<99)add_hint(out,&count,first,(float)(key.rank-5)*3.5f/(cost+safe_count));
        }
    }
    return count;
}

/* Continue a useful approach with the same identity, and bring an officer
   alongside an advanced miner. All opposing ranks used here are public. */
float ai_coordination_bonus(const Game *g,Move m){
    Piece p=g->board[m.from];float bonus=0;
    int advance=(m.to/10-m.from/10)*(p.side==COMPUTER?1:-1);
    if(advance>0&&!controlled(g,p.side,p.rank,m.to,m.from)){
        int history=g->history_count[p.side];
        for(int age=0;age<4&&age<history;age++){
            int slot=(history-1-age)%8;
            if(g->history_id[p.side][slot]==p.id){bonus+=.6f/(1+age*.25f);break;}
        }
    }
    if(p.rank>=CAPTAIN&&p.rank<=GENERAL){
        int best=-1,best_advance=-1;
        for(int s=0;s<100;s++)if(g->board[s].side==p.side&&g->board[s].rank==MINER){
            int depth=p.side==COMPUTER?s/10:9-s/10;
            if(depth>=4&&depth>best_advance){best=s;best_advance=depth;}
        }
        if(best>=0){
            int before=abs(m.from%10-best%10)+abs(m.from/10-best/10);
            int after=abs(m.to%10-best%10)+abs(m.to/10-best/10);
            /* Stop at escort distance instead of crowding the miner's square. */
            if(before>1&&after>=1&&after<before&&!controlled(g,p.side,p.rank,m.to,m.from))bonus+=1.2f;
        }
    }
    return bonus;
}
static int interception_margin(const int home[100],int e,int flag,const int guard[100],int tempo){
    int slack[100];for(int s=0;s<100;s++)slack[s]=-100;
    slack[e]=guard[e]-tempo;
    for(int layer=home[e];layer>0;layer--)for(int s=0;s<100;s++)if(home[s]==layer&&slack[s]>-100){
        int nb[4],n=adjacent(s,nb);
        for(int j=0;j<n;j++){
            int t=nb[j];if(home[t]!=layer-1)continue;
            int margin=t==flag?slack[s]:guard[t]-(home[e]-home[t])-tempo;
            int value=margin<slack[s]?margin:slack[s];if(value>slack[t])slack[t]=value;
        }
    }
    return slack[flag];
}
static bool assign_guard(int threat,bool candidates[100][100],int owner[100],bool seen[100]){
    for(int guard=0;guard<100;guard++)if(candidates[threat][guard]&&!seen[guard]){
        seen[guard]=true;
        if(owner[guard]<0||assign_guard(owner[guard],candidates,owner,seen)){owner[guard]=threat;return true;}
    }
    return false;
}
/* Compare invasion and interception arrival times, including the bomb screen.
   An unidentified moving intruder may be a miner: never trust a bomb screen
   simply because sixteen sampled armies happened to assign another rank. */
float ai_flag_risk(const Game *g,int side) {
    int flag=-1;float total=0;
    bool candidates[100][100]={{false}};int threats[100],threat_count=0;
    float urgency[100]={0};
    for(int s=0;s<100;s++)if(g->board[s].side==side&&g->board[s].rank==FLAG)flag=s;
    if(flag<0)return 0;
    for(int e=0;e<100;e++) {
        Piece enemy=g->board[e];
        if(enemy.side!=1-side||(!enemy.revealed&&!enemy.moved))continue;
        int rank=enemy.revealed?enemy.rank:MINER;
        if(rank==FLAG||rank==BOMB)continue;
        int home[100],queue[100],head=0,tail=0;
        for(int s=0;s<100;s++)home[s]=100;
        home[flag]=0;queue[tail++]=flag;
        while(head<tail) {
            int s=queue[head++],nb[4],n=adjacent(s,nb);
            for(int j=0;j<n;j++) {
                int t=nb[j];Piece p=g->board[t];
                if(is_lake(t)||home[t]!=100||(p.side==1-side&&t!=e))continue;
                if(p.side==side&&p.rank==BOMB&&rank!=MINER)continue;
                home[t]=home[s]+1;queue[tail++]=t;
            }
        }
        if(home[e]>8)continue;
        if(home[e]<=6){threats[threat_count++]=e;urgency[e]=35.0f*(7-home[e])/6;}
        int guard[100];for(int s=0;s<100;s++)guard[s]=100;
        for(int a=0;a<100;a++) {
            Piece defender=g->board[a];
            if(defender.side!=side||!movable(defender)||combat_result(defender.rank,rank)<0)continue;
            if(g->turn!=side&&controlled(g,side,defender.rank,a,a))continue;
            int dist[100];for(int s=0;s<100;s++)dist[s]=100;
            head=tail=0;dist[a]=0;queue[tail++]=a;
            while(head<tail) {
                int s=queue[head++];if(dist[s]<guard[s])guard[s]=dist[s];
                if(dist[s]>=8||(s!=a&&g->board[s].side>=0))continue;
                int nb[4],n=adjacent(s,nb);
                for(int j=0;j<n;j++) {
                    int t=nb[j];if(is_lake(t)||dist[t]!=100)continue;
                    /* A miner removes our bomb when entering its square. A
                       guard can then recapture there, but cannot walk THROUGH
                       an intact bomb to reach some other interception square. */
                    if(g->board[t].side==side&&g->board[t].rank!=BOMB)continue;
                    /* Removing the invading miner can save the flag even if
                       its escort recaptures our guard afterwards. */
                    if(controlled(g,side,defender.rank,t,a)&&t!=e&&!(g->board[t].side==side&&g->board[t].rank==BOMB&&rank==MINER))continue;
                    dist[t]=dist[s]+1;queue[tail++]=t;
                }
            }
            if(home[e]<=6&&interception_margin(home,e,flag,dist,g->turn==side?1:0)<=0)candidates[e][a]=true;
        }
        /* Intruder selects the least protected among all shortest routes.
           A single guard must arrive before the intruder leaves a route node. */
        int margin=interception_margin(home,e,flag,guard,g->turn==side?1:0);
        float exposure=margin<=-2?0:margin==-1?8:margin==0?25:margin==1?110:180;
        total+=exposure*(9-home[e])/8.0f;
    }
    if(threat_count>1){
        /* Most urgent routes claim a defender first; augmenting paths reassign
           existing guards, so iteration order cannot needlessly waste one. */
        for(int i=0;i<threat_count;i++)for(int j=i+1;j<threat_count;j++)if(urgency[threats[j]]>urgency[threats[i]]){int t=threats[i];threats[i]=threats[j];threats[j]=t;}
        int owner[100];for(int a=0;a<100;a++)owner[a]=-1;
        for(int i=0;i<threat_count;i++){bool seen[100]={false};if(!assign_guard(threats[i],candidates,owner,seen))total+=urgency[threats[i]];}
    }
    return total;
}
