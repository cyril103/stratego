/* Stable missions for scarce pieces. Plans use the masked public board and
   real traversable routes; proximity through our own bombs is not support. */
typedef struct {Move step;int guard,goal;float priority;} ContinuityPlan;
static ContinuityPlan last_miner_plan(const Game *v,float p[100][12]){
    ContinuityPlan plan={{-1,-1},-1,-1,0};int side=v->turn,miner=-1;
    if(army_counts[MINER]-v->captured[side][MINER]!=1||v->captured[1-side][BOMB]==army_counts[BOMB])return plan;
    float most=ai_flag_focus_threshold(v,p,1.5f);
    for(int s=0;s<100;s++){
        if(v->board[s].side==side&&v->board[s].rank==MINER)miner=s;
        if(!v->board[s].revealed&&p[s][FLAG]>most){most=p[s][FLAG];plan.goal=s;}
    }
    if(miner<0||plan.goal<0)return plan;
    /* This is a conversion mission inside the enemy camp, not permission to
       divert a home defender or replace an established escort in midfield. */
    if((side==COMPUTER?miner/10:9-miner/10)<6)return plan;
    if(ai_flag_risk(v,side)>0)return plan;
    float dist[100];bool done[100]={false};
    for(int s=0;s<100;s++)dist[s]=1000;
    dist[plan.goal]=0;
    for(int k=0;k<100;k++){
        int s=-1;for(int t=0;t<100;t++)if(!done[t]&&(s<0||dist[t]<dist[s]))s=t;
        if(s<0||dist[s]>=1000)break;
        done[s]=true;
        Piece occupant=v->board[s];float cost=1;
        if(occupant.side==side&&s!=miner)continue;
        if(occupant.side==1-side){
            float failure=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(MINER,r)<=0)failure+=p[s][r];
            if(failure>.999f)continue;
            cost+=12*failure;
        }
        int nb[4],nn=neighbors(s,nb);
        for(int j=0;j<nn;j++)if(!is_lake(nb[j]))dist[nb[j]]=fminf(dist[nb[j]],dist[s]+cost);
    }
    int nb[4],nn=neighbors(miner,nb);float best=0;
    for(int j=0;j<nn;j++){
        Move m={miner,nb[j]};if(!public_legal(v,m,side)||immediate_defeat(v,m))continue;
        Piece d=v->board[m.to];float failure=0;
        for(int r=SPY;r<=MARSHAL;r++)if(combat_result(MINER,r)<=0)failure+=p[m.to][r];
        if(d.side>=0&&failure>.05f&&p[m.to][FLAG]<most*.5f)continue;
        Game next=optimistic_move(v,m);
        if(known_unanswered_loss_at(&next,side,m.to)>0)continue;
        float progress=dist[miner]-dist[m.to];
        if(progress>best&&progress<100){
            best=progress;plan.step=m;plan.priority=100;
            /* Complete an opened gate instead of abandoning the last probe
               for a distant redeployment. The flag is still uncertain, so
               the extra conversion value is proportional to its probability.
               Immediate defence and tactical filters retain priority. */
            if(m.to==plan.goal&&v->cleared_bombs[1-side][m.from])plan.priority+=160*p[m.to][FLAG];
        }
    }
    return plan;
}
static ContinuityPlan reserve_support_plan(const Game *v,float p[100][12]){
    ContinuityPlan plan={{-1,-1},-1,-1,0};int side=v->turn,flag=-1,reserve=-1,mobile=0;
    for(int s=0;s<100;s++)if(v->board[s].side==side){
        if(v->board[s].rank==FLAG)flag=s;
        if(movable(v->board[s])){mobile++;if(reserve<0||v->board[s].rank>v->board[reserve].rank)reserve=s;}
    }
    if(flag<0||mobile<2||mobile>4||v->board[reserve].rank<CAPTAIN)return plan;
    for(int rank=v->board[reserve].rank;rank<=MARSHAL;rank++)
        if(army_counts[rank]>v->captured[1-side][rank])return plan;
    /* A reserve already under a known attack must be free to escape; never
       turn the formation's holding preference into a sacrifice order. */
    if(known_unanswered_loss_at(v,side,reserve)>0)return plan;
    int home[100];land_distances(flag,home);float best=-1e9f;
    for(int guard=0;guard<100;guard++){
        Piece ally=v->board[guard];
        if(guard==reserve||ally.side!=side||ally.rank<MINER||ally.rank>=v->board[reserve].rank||home[guard]>5)continue;
        if(known_unanswered_loss_at(v,side,guard)>0)continue;
        float threat=0;
        for(int e=0;e<100;e++){
            Piece enemy=v->board[e];if(enemy.side!=1-side||(!enemy.moved&&!enemy.revealed)||home[e]>8)continue;
            int distance=abs(e%10-guard%10)+abs(e/10-guard/10);
            if(distance>4)continue;
            float hunter=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(r,ally.rank)>=0)hunter+=p[e][r];
            threat=fmaxf(threat,hunter*(5-distance));
        }
        if(threat<.2f)continue;
        int stations[4],ns=neighbors(guard,stations);
        for(int j=0;j<ns;j++){
            int goal=stations[j];if(is_lake(goal)||(v->board[goal].side>=0&&goal!=reserve)||home[goal]>home[guard]+1)continue;
            int dist[100],parent[100],queue[100],head=0,tail=0;
            for(int s=0;s<100;s++)dist[s]=100;
            dist[reserve]=0;parent[reserve]=-1;queue[tail++]=reserve;
            while(head<tail){
                int s=queue[head++];if(s==goal)break;
                int nb[4],nn=neighbors(s,nb);
                for(int k=0;k<nn;k++){
                    int t=nb[k];if(is_lake(t)||v->board[t].side>=0||dist[t]!=100)continue;
                    Game next=*v;next.board[reserve]=empty_piece();next.board[t]=v->board[reserve];
                    if(known_unanswered_loss_at(&next,side,t)>0)continue;
                    if(v->board[reserve].rank==MARSHAL){
                        int near[4],count=neighbors(t,near);bool spy=false;
                        for(int z=0;z<count;z++)if(v->board[near[z]].side==1-side&&p[near[z]][SPY]>0){
                            /* A supported final station can accept a small
                               spy uncertainty during a flag emergency. The
                               guard can recapture; known spies stay forbidden. */
                            Game retaliation=next;retaliation.board[t]=(Piece){SPY,1-side,near[z],true,true};
                            bool supported=t==goal&&p[near[z]][SPY]<=.15f&&ai_flag_risk(v,side)>0&&
                                public_legal(&retaliation,(Move){guard,t},side);
                            if(!supported)spy=true;
                        }
                        if(spy)continue;
                    }
                    dist[t]=dist[s]+1;parent[t]=s;queue[tail++]=t;
                }
            }
            if(dist[goal]>6)continue;
            int to=goal;if(to!=reserve)while(parent[to]!=reserve)to=parent[to];
            Move m={reserve,to};
            if(to!=reserve&&(!public_legal(v,m,side)||immediate_defeat(v,m)))continue;
            float score=40*threat-4*dist[goal]-home[goal];
            if(score>best){best=score;plan=(ContinuityPlan){m,guard,goal,140};}
        }
    }
    return plan;
}
static float continuity_bonus(const ContinuityPlan *plan,Move m){
    if(plan->step.from==m.from&&plan->step.to==m.to)return plan->priority;
    if(plan->guard>=0&&plan->step.from==plan->step.to&&m.from==plan->step.from)return -plan->priority;
    return 0;
}
static float continuity_guard_cost(const Game *v,float p[100][12],const ContinuityPlan *plan,Move m){
    if(plan->guard!=m.from||v->board[m.to].side<0)return 0;
    int reserve=plan->step.from;
    Game next=optimistic_move(v,m);next.board[m.to]=v->board[m.to];
    if(public_legal(&next,(Move){reserve,m.to},v->turn))return 0;
    float failure=0;for(int r=0;r<12;r++)if(combat_result(v->board[m.from].rank,r)<=0)failure+=p[m.to][r];
    return 300*failure;
}
/* Cash in a certain interception from public rank counts. An unidentified
   moving piece can be safely capturable without pretending to know its rank. */
static Move continuity_capture(const Game *v,float p[100][12],Move *moves,int n){
    Move best={-1,-1};int flag=-1,mobile=0;float value=-1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn){if(v->board[s].rank==FLAG)flag=s;if(movable(v->board[s]))mobile++;}
    if(flag<0||mobile<2||mobile>4)return best;
    for(int i=0;i<n;i++){
        Move m=moves[i];Piece a=v->board[m.from],d=v->board[m.to];
        if(d.side!=1-v->turn||!d.moved||abs(m.to%10-flag%10)+abs(m.to/10-flag/10)>5)continue;
        /* A local capture does not settle an ending while a stronger enemy
           survives elsewhere. Keep those decisions in the main search. */
        bool dominant=true;
        for(int r=a.rank;r<=MARSHAL;r++)if(army_counts[r]>v->captured[1-v->turn][r])dominant=false;
        if(!dominant)continue;
        float win=0,gain=0;for(int r=0;r<12;r++){if(combat_result(a.rank,r)>0)win+=p[m.to][r];gain+=p[m.to][r]*worth[r];}
        if(win<.999f)continue;
        if(known_unanswered_loss(v,m)>0)continue;
        Game next=optimistic_move(v,m);bool safe=true;
        for(int e=0;e<100;e++)if(e!=m.to&&v->board[e].side==1-v->turn){
            for(int r=SPY;r<=MARSHAL;r++)if(p[e][r]>0&&combat_result(r,a.rank)>=0){
                Game attack=next;attack.board[e].rank=r;
                if(public_legal(&attack,(Move){e,m.to},1-v->turn))safe=false;
            }
        }
        if(safe&&gain>value){value=gain;best=m;}
    }
    return best;
}
