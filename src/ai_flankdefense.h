/* A known officer can screen a hidden miner from an entire wing. Measure
   access to that officer again after each move: moving an allied screen can
   matter more than moving the reserve itself. Only public ranks are used. */
typedef struct {int escort,distance;} FlankDefense;
static int flank_guard_distance(const Game *v,int escort){
    int best=100;
    for(int rank=v->board[escort].rank;rank<=MARSHAL;rank++){
        int dist[100];intercept_distances(v,escort,v->turn,rank,dist);
        for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==rank&&
            last_officer_trade_cost(v,(Move){s,escort})==0)
            best=best<dist[s]?best:dist[s];
    }
    return best;
}
static FlankDefense flank_defense_plan(const Game *v,float p[100][12]){
    FlankDefense plan={-1,100};int flag=-1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==FLAG)flag=s;
    if(flag<0)return plan;
    int home[100];intercept_distances(v,flag,1-v->turn,MINER,home);
    for(int e=0;e<100;e++){
        Piece escort=v->board[e];
        if(escort.side!=1-v->turn||!escort.revealed||escort.rank<GENERAL||escort.rank>MARSHAL||
           abs(e%10-flag%10)+abs(e/10-flag/10)>4)continue;
        bool miner=false;
        for(int s=0;s<100;s++)if(s!=e&&v->board[s].side==1-v->turn&&v->board[s].moved&&p[s][MINER]>0&&home[s]<=8&&
            abs(s%10-e%10)+abs(s/10-e/10)<=4)miner=true;
        if(!miner)continue;
        int distance=flank_guard_distance(v,e);
        if(distance<plan.distance)plan=(FlankDefense){e,distance};
    }
    return plan;
}
static float flank_defense_bonus(const Game *v,float p[100][12],const FlankDefense *plan,Move m){
    if(plan->escort<0)return 0;
    Piece a=v->board[m.from],d=v->board[m.to];
    /* A spent spy can clear a blocked passage. Never spend an active spy or
       a valuable officer merely to make a travel estimate look better. */
    bool expendable=a.rank==SPY&&v->captured[1-v->turn][MARSHAL]==army_counts[MARSHAL]&&
        m.to==plan->escort;
    bool exchange=m.to==plan->escort&&a.rank==d.rank;
    if(last_officer_trade_cost(v,m)>0)return 0;
    if(!expendable&&!exchange&&(!certain_survival(v,p,m)||known_unanswered_loss(v,m)>0))return 0;
    Game next=optimistic_move(v,m);
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank>=COLONEL&&next.board[s].rank<=MARSHAL&&
        known_unanswered_loss_at(&next,v->turn,s)>known_unanswered_loss_at(v,v->turn,s))return 0;
    if(m.to==plan->escort&&combat_result(a.rank,d.rank)>=0)return 100;
    int after=flank_guard_distance(&next,plan->escort);
    return fmaxf(-100,fminf(100,80.0f*(plan->distance-after)));
}
