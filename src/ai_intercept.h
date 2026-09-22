/* Public route estimates for reserve assignment. These are travel estimates,
   not forced lines: clearing a friendly mobile screen costs an extra tempo. */
static void intercept_distances(const Game *g,int goal,int side,int rank,int dist[100]){
    bool done[100]={false};
    bool unresolved_spy=rank==MARSHAL&&g->captured[1-side][SPY]<army_counts[SPY];
    if(unresolved_spy)for(int e=0;e<100;e++)if(g->board[e].side==1-side&&g->board[e].revealed&&g->board[e].rank==SPY)unresolved_spy=false;
    for(int s=0;s<100;s++)dist[s]=100;
    dist[goal]=0;
    for(int iteration=0;iteration<100;iteration++){
        int s=-1;
        for(int t=0;t<100;t++)if(!done[t]&&(s<0||dist[t]<dist[s]))s=t;
        if(s<0||dist[s]>=100)break;
        done[s]=true;
        Piece occupant=g->board[s];int cost=1;
        /* A reserve may leave a threatened origin, but must not be routed
           through a square where a known escort can take it. Distances are
           propagated backwards, so an unsafe square can still be an origin. */
        if(s!=goal&&side==g->turn){
            bool unsafe=false;
            if(unresolved_spy){
                int near[4],nn=neighbors(s,near);
                for(int j=0;j<nn;j++){
                    Piece suspect=g->board[near[j]];int id=suspect.id;
                    if(suspect.side==1-side&&!suspect.revealed&&id>=0&&id<80&&
                       (g->marshal_suspects[side][id/64]&(UINT64_C(1)<<(id%64))))unsafe=true;
                }
            }
            for(int e=0;e<100;e++){
                Piece enemy=g->board[e];
                if(enemy.side==1-side&&enemy.revealed&&movable(enemy)&&
                   combat_result(enemy.rank,rank)>0&&public_legal(g,(Move){e,s},enemy.side))unsafe=true;
            }
            if(unsafe)continue;
        }
        if(s!=goal&&occupant.side>=0){
            if(occupant.side==side){
                if(!(movable(occupant)||(occupant.rank==-2&&occupant.moved)))continue;
                cost=2;
            }else if((!occupant.revealed&&occupant.side!=g->turn)||combat_result(rank,occupant.rank)<=0)continue;
        }
        const int dx[4]={-1,1,0,0},dy[4]={0,0,-1,1};
        for(int direction=0;direction<4;direction++)for(int step=1;step<=(rank==SCOUT?9:1);step++){
            int x=s%10+dx[direction]*step,y=s/10+dy[direction]*step;
            if(x<0||x>9||y<0||y>9)break;
            int t=y*10+x;if(is_lake(t))break;
            if(dist[t]>dist[s]+cost)dist[t]=dist[s]+cost;
            /* Masked ranks still occupy squares and stop scout rays. */
            if(g->board[t].side>=0)break;
        }
    }
}
typedef struct {int enemy,arrival;float probability;int distance[12][100];} InterceptThreat;
typedef struct {InterceptThreat threats[40];int count;} InterceptPlan;
static void intercept_plan(const Game *view,float p[100][12],InterceptPlan *plan){
    plan->count=0;int flag=-1,side=view->turn;
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return;
    /* These maps depend on the hypothesized rank, not on which intruder is
       being evaluated. Compute them once rather than once per enemy piece. */
    int arrival_maps[12][100];
    for(int rank=SPY;rank<=MARSHAL;rank++)intercept_distances(view,flag,1-side,rank,arrival_maps[rank]);
    for(int e=0;e<100;e++){
        Piece enemy=view->board[e];
        if(enemy.side!=1-side||(!enemy.moved&&!enemy.revealed))continue;
        int fastest=100;float probability=0;
        /* An unknown mobile is not necessarily a miner. In particular, once
           miners are accounted for, captains and scouts still threaten an
           accessible flag. Weight only ranks with a public reachable route. */
        for(int rank=SPY;rank<=MARSHAL;rank++)if(p[e][rank]>0){
            if(arrival_maps[rank][e]<=8){
                if(arrival_maps[rank][e]<fastest)fastest=arrival_maps[rank][e];
                probability+=p[e][rank];
            }
        }
        if(fastest>8)continue;
        InterceptThreat *t=&plan->threats[plan->count++];
        t->enemy=e;t->arrival=fastest;t->probability=probability;
        for(int r=SPY;r<=MARSHAL;r++)intercept_distances(view,e,side,r,t->distance[r]);
    }
}
static float intercept_bonus(const Game *view,float p[100][12],const InterceptPlan *plan,Move m){
    Piece a=view->board[m.from],target=view->board[m.to];
    /* A speculative combat is not a successful redeployment. */
    if(target.side>=0&&(!target.revealed||combat_result(a.rank,target.rank)<=0))return 0;
    Game next=*view;next.board[m.from]=empty_piece();next.board[m.to]=a;
    for(int e=0;e<100;e++){
        Piece enemy=next.board[e];
        if(enemy.side==1-a.side&&enemy.revealed&&movable(enemy)&&combat_result(enemy.rank,a.rank)>=0&&
           public_legal(&next,(Move){e,m.to},enemy.side))return 0;
    }
    float bonus=0;
    for(int i=0;i<plan->count;i++){
        const InterceptThreat *t=&plan->threats[i];int best=100,after=100;
        for(int s=0;s<100;s++){
            Piece guard=view->board[s];if(guard.side!=view->turn||!movable(guard))continue;
            float stop=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(guard.rank,r)>=0)stop+=p[t->enemy][r];
            if(stop<.8f)continue;
            int d=t->distance[guard.rank][s];if(d<best)best=d;
            if(s==m.from)d=t->distance[guard.rank][m.to];
            if(d<after)after=d;
        }
        if(best>=100||after>=100)continue;
        /* The nearest capable reserve owns this task; unrelated troops do
           not all receive a reward for pursuing the same invader. */
        float urgency=(9-t->arrival)*sqrtf(t->probability);
        if(best>=t->arrival)urgency*=2;
        float value=3*urgency*(best-after);
        if(fabsf(value)>fabsf(bonus))bonus=value;
    }
    return fmaxf(-60,fminf(60,bonus));
}
/* A trade on the last local guard can open a race that distant reinforcements
   cannot win by chasing a moving miner. Keep the cost probabilistic for an
   unknown target, and never charge it for taking the miner itself. */
static float last_guard_trade(const Game *view,float p[100][12],Move m){
    Piece a=view->board[m.from],target=view->board[m.to];int flag=-1,side=view->turn;
    if(a.rank<MINER||target.side!=1-side)return 0;
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return 0;
    int home[100];land_distances(flag,home);if(home[m.from]>3)return 0;
    for(int s=0;s<100;s++){
        Piece guard=view->board[s];
        if(s!=m.from&&guard.side==side&&guard.rank>=MINER&&guard.rank<=MARSHAL&&home[s]<=3)return 0;
    }
    float threat=0;
    for(int s=0;s<100;s++){
        Piece intruder=view->board[s];
        if(s!=m.to&&intruder.side==1-side&&(intruder.moved||intruder.revealed)&&home[s]<=6)
            threat=fmaxf(threat,p[s][MINER]);
    }
    float lost=0;for(int r=0;r<12;r++)if(combat_result(a.rank,r)<=0)lost+=p[m.to][r];
    return 120*threat*lost;
}
