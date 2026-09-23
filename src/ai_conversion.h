/* Conversion uses public rank counts and movement evidence only. Being ahead
   is a reason to preserve useful pieces, not to buy expensive flag guesses. */
static bool speculative_bomb_probe(const Game *v,float p[100][12],Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(d.side!=1-v->turn||d.revealed||d.moved||a.rank==MINER||p[m.to][FLAG]>.999f)return false;
    int own=0,enemy=0;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&movable(v->board[s]))own++;
    for(int r=SPY;r<=MARSHAL;r++)enemy+=army_counts[r]-v->captured[1-v->turn][r];
    /* Keep last-piece desperation and the final search among immobile
       survivors available. Scouts remain the expendable reconnaissance role. */
    if(own<=1||enemy==0||a.rank==SCOUT)return false;
    float limit=a.rank>=CAPTAIN?.08f:.25f;
    if(force_advantage(v)>.1f)limit*=.5f;
    return p[m.to][BOMB]>limit;
}
static int preserve_conversion_army(const Game *v,float p[100][12],Move *moves,int n){
    bool alternative=false;
    for(int i=0;i<n;i++)if(!speculative_bomb_probe(v,p,moves[i])&&
        certain_survival(v,p,moves[i])&&known_unanswered_loss(v,moves[i])==0&&
        spy_exposure(v,p,moves[i],true)<=spy_exposure(v,p,moves[i],false)){
        alternative=true;break;
    }
    if(!alternative)return n;
    int kept=0;
    for(int i=0;i<n;i++)if(!speculative_bomb_probe(v,p,moves[i]))moves[kept++]=moves[i];
    return kept;
}
static float mobile_conversion_bonus(const Game *v,float p[100][12],const RaiderPlan *defense,Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(a.rank<SERGEANT||a.rank>MARSHAL)return 0;
    if(a.rank>=GENERAL&&!a.revealed)return 0;
    int strongest=SPY;
    for(int r=SPY;r<=MARSHAL;r++)if(army_counts[r]>v->captured[1-v->turn][r])strongest=r;
    if(force_advantage(v)<.1f&&a.rank<strongest)return 0;
    for(int i=0;i<defense->count;i++)if(defense->tasks[i].guard==m.from)return 0;
    if(known_unanswered_loss(v,m)>0||counter_capture_cost(v,p,m)>0||
        supported_capture_cost(v,p,m)>0||officer_trap_cost(v,m)>0||
        spy_exposure(v,p,m,true)>spy_exposure(v,p,m,false))return 0;
    Game next=optimistic_move(v,m);
    if(ai_flag_risk(&next,v->turn)>ai_flag_risk(v,v->turn)+.01f)return 0;
    float best=0;
    for(int e=0;e<100;e++){
        Piece target=v->board[e];
        if(target.side!=1-v->turn||(!target.moved&&(!target.revealed||!movable(target))))continue;
        float win=0,value=0;
        for(int r=SPY;r<=MARSHAL;r++)if(combat_result(a.rank,r)>0){win+=p[e][r];value+=p[e][r]*worth[r];}
        if(win<.98f)continue;
        if(m.to==e){best=fmaxf(best,fminf(22,8+value*.5f));continue;}
        if(d.side>=0||expected_threat(v,p,m,true)>expected_threat(v,p,m,false))continue;
        /* Actual open routes, not Manhattan distance through a lake, bomb
           or friendly blocker. Stop at six steps; defense remains primary. */
        int dist[100],queue[100],head=0,tail=0;
        for(int s=0;s<100;s++)dist[s]=100;
        dist[e]=0;queue[tail++]=e;
        while(head<tail){
            int s=queue[head++];if(dist[s]>=6)continue;
            int nb[4],nn=neighbors(s,nb);
            for(int j=0;j<nn;j++){
                int t=nb[j];if(is_lake(t)||dist[t]<100||(t!=m.from&&v->board[t].side>=0))continue;
                dist[t]=dist[s]+1;queue[tail++]=t;
            }
        }
        if(dist[m.from]<100&&dist[m.to]<dist[m.from])best=fmaxf(best,6*win);
    }
    return best;
}
