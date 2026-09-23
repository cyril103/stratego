/* Opportunistic raids use public odds, not sampled weak targets. Front-row
   immobile targets are eligible only when their actual bomb odds are low;
   moved targets are preferable because bombs cannot move. */
static float raid_loss_budget(const Game *v,int rank){
    int reserves=0;
    for(int r=rank;r<=MARSHAL;r++)reserves+=army_counts[r]-v->captured[v->turn][r];
    /* A replaceable major can take a measured chance; the last strong
       defender must not buy the same raid at the same probability of loss. */
    float limit=rank==MARSHAL?.06f:rank==GENERAL?.08f:rank==COLONEL?.12f:.16f;
    if(reserves<=1)limit=fminf(limit,.06f);
    float lead=force_advantage(v);
    if(lead>.2f)limit*=.75f;
    return limit;
}
static float officer_raid_bonus(const Game *v,float p[100][12],const RaiderPlan *defense,Move m,float flag_risk){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(a.rank<MAJOR||a.rank>MARSHAL)return 0;
    /* Keep concealed top officers' existing disclosure policy. */
    if(a.rank>=GENERAL&&!a.revealed)return 0;
    for(int i=0;i<defense->count;i++)if(defense->tasks[i].guard==m.from)return 0;
    /* An officer already in uncertain contact needs its escape evaluation,
       not a pursuit premium for stepping beside another possible superior. */
    if(d.side<0&&approaching_officer_risk(v,p)>0)return 0;
    if(officer_trap_cost(v,m)>0)return 0;
    if(known_unanswered_loss(v,m)>0||spy_exposure(v,p,m,true)>spy_exposure(v,p,m,false))return 0;
    Game next=optimistic_move(v,m);next.turn=1-v->turn;
    if(ai_flag_risk(&next,v->turn)>flag_risk+1)return 0;
    float best=0;
    for(int e=0;e<100;e++){
        Piece prey=v->board[e];if(prey.side!=1-v->turn)continue;
        int depth=a.side==COMPUTER?e/10-6:3-e/10;
        if(!prey.moved&&(depth<0||depth>1))continue;
        float win=0,loss=0,value=0;
        for(int r=SPY;r<=BOMB;r++){
            if(combat_result(a.rank,r)>0){win+=p[e][r];value+=p[e][r]*worth[r];}
            else loss+=p[e][r];
        }
        float budget=raid_loss_budget(v,a.rank);
        if(p[e][BOMB]>.08f||loss>budget||win<1-budget||value<=0)continue;
        if(e==m.to){
            if(supported_capture_cost(v,p,m)>0||counter_capture_cost(v,p,m)>0)continue;
            float margin=value-loss*worth[a.rank];
            if(margin>0)best=fmaxf(best,fminf(18,margin*.8f));
        }else if(d.side<0){
            int before=abs(m.from%10-e%10)+abs(m.from/10-e/10);
            int after=abs(m.to%10-e%10)+abs(m.to/10-e/10);
            if(before<=3&&after<before&&p[e][BOMB]==0&&expected_threat(v,p,m,true)<=expected_threat(v,p,m,false))
                best=fmaxf(best,3*win);
        }
    }
    return best;
}
