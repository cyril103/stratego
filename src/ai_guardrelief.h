/* A guard pinned to a scout ray cannot intercept a second attacker. Find a
   separate defender and an actually traversable route to cover that ray.
   These are public positions and ranks; no sampled identity is consulted. */
typedef struct {Move step;int guard,pinner,station,distance;float bonus;} GuardRelief;
static bool relief_station(const Game *view,int square,int pinner,int flag){
    if(square==pinner)return true; /* Capturing the scout also releases the guard. */
    if(view->board[square].side>=0)return false;
    if(pinner%10==flag%10)return square%10==flag%10&&
        square/10>fminf(pinner/10,flag/10)&&square/10<fmaxf(pinner/10,flag/10);
    return square/10==flag/10&&square%10>fminf(pinner%10,flag%10)&&square%10<fmaxf(pinner%10,flag%10);
}
static GuardRelief guard_relief_plan(const Game *view,const InterceptPlan *threats){
    GuardRelief best={.step={-1,-1},.guard=-1};int side=view->turn,flag=-1;float best_score=-1e9f;
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return best;
    for(int e=0;e<100;e++){
        Piece scout=view->board[e];
        if(scout.side!=1-side||!scout.revealed||scout.rank!=SCOUT||
           (e%10!=flag%10&&e/10!=flag/10))continue;
        int arrival=100;
        for(int i=0;i<threats->count;i++){
            const InterceptThreat *t=&threats->threats[i];
            if(t->enemy==e||t->probability<.5f||t->arrival>=arrival)continue;
            /* Once the relief is installed, taking the attacker comes first;
               do not start preparing another guard instead of cashing in. */
            bool intercepted=false;Piece intruder=view->board[t->enemy];
            if(intruder.revealed)for(int s=0;s<100;s++){
                Piece defender=view->board[s];Move capture={s,t->enemy};
                if(defender.side!=side||!movable(defender)||combat_result(defender.rank,intruder.rank)<=0||
                   !public_legal(view,capture,side)||immediate_defeat(view,capture))continue;
                Game next=optimistic_move(view,capture);
                if(known_unanswered_loss_at(&next,side,t->enemy)==0)intercepted=true;
            }
            if(!intercepted)arrival=t->arrival;
        }
        if(arrival>7)continue;
        for(int guard=0;guard<100;guard++){
            Piece pinned=view->board[guard];
            if(pinned.side!=side||!movable(pinned))continue;
            Game gap=*view;gap.board[guard]=empty_piece();
            if(!public_legal(&gap,(Move){e,flag},1-side)||public_legal(view,(Move){e,flag},1-side))continue;
            for(int reserve=0;reserve<100;reserve++){
                Piece ally=view->board[reserve];
                if(reserve==guard||ally.side!=side||ally.rank<MINER||ally.rank>MARSHAL)continue;
                int dist[100],parent[100],queue[100],head=0,tail=0;
                for(int s=0;s<100;s++){dist[s]=100;parent[s]=-1;}
                dist[reserve]=0;queue[tail++]=reserve;
                while(head<tail){
                    int s=queue[head++];
                    if(s!=reserve&&relief_station(view,s,e,flag)){
                        int first=s;while(parent[first]!=reserve)first=parent[first];
                        /* Prefer a nearby inexpensive replacement. Taking
                           the pinner wins an extra tempo over merely covering. */
                        float score=120-12*dist[s]-.2f*worth[ally.rank]+(s==e?8:0);
                        if(score>best_score&&!immediate_defeat(view,(Move){reserve,first})){
                            best_score=score;best=(GuardRelief){.step={reserve,first},.guard=guard,
                                .pinner=e,.station=s,.distance=dist[s],.bonus=80};
                        }
                        break;
                    }
                    if(dist[s]>=6||s==e)continue;
                    int nb[4],nn=neighbors(s,nb);
                    for(int k=0;k<nn;k++){
                        int t=nb[k];if(is_lake(t)||dist[t]!=100)continue;
                        if(view->board[t].side>=0&&t!=e)continue;
                        if(s==reserve&&!public_legal(view,(Move){s,t},side))continue;
                        Game probe=*view;probe.board[reserve]=empty_piece();probe.board[t]=ally;
                        bool unsafe=false;
                        for(int hunter=0;hunter<100;hunter++){
                            Piece enemy=probe.board[hunter];
                            if(enemy.side!=1-side)continue;
                            if(enemy.revealed&&movable(enemy)&&combat_result(enemy.rank,ally.rank)>=0&&
                               public_legal(&probe,(Move){hunter,t},1-side))unsafe=true;
                            if(ally.rank==MARSHAL&&!enemy.revealed&&
                               view->captured[1-side][SPY]<army_counts[SPY]&&
                               abs(hunter%10-t%10)+abs(hunter/10-t/10)==1)unsafe=true;
                        }
                        if(unsafe)continue;
                        dist[t]=dist[s]+1;parent[t]=s;queue[tail++]=t;
                    }
                }
            }
        }
    }
    return best;
}
static float guard_relief_bonus(const GuardRelief *plan,Move m){
    return m.from==plan->step.from&&m.to==plan->step.to?plan->bonus:0;
}
