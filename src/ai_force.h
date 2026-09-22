/* Army roles depend on what is still alive. Only public casualty counts are
   used at the root; search calls use the current hypothetical army. */
static float force_value(int alive[2][12],int side,int rank){
    int enemy=1-side;
    if(rank==FLAG)return 0;
    /* Do not lower our bomb score when the last enemy miner is captured:
       that would perversely discount the value of removing that miner. */
    if(rank==BOMB)return worth[BOMB];
    if(rank==SPY)return alive[enemy][MARSHAL]?15:3;
    if(rank==MINER)return alive[enemy][BOMB]?16+24.0f/(alive[side][MINER]+1):6;
    float value=worth[rank];
    if(rank>=SERGEANT){
        int stronger=0,equal=alive[enemy][rank],lower=0,reserve=0;
        for(int r=SPY;r<rank;r++)lower+=alive[enemy][r];
        for(int r=rank+1;r<=MARSHAL;r++){stronger+=alive[enemy][r];reserve+=alive[side][r];}
        /* A sole superior controls the surviving lower ranks. Exchanging it
           can surrender that control even when nominal material is equal. */
        value+=12.0f/(1+stronger);
        value+=fminf(24,3.0f*lower)/((1+stronger+equal)*(1+reserve));
    }
    return value;
}
static float force_advantage(const Game *view){
    int alive[2][12];float total[2]={0,0};
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)alive[side][r]=army_counts[r]-view->captured[side][r];
    for(int side=0;side<2;side++)for(int r=SPY;r<=MARSHAL;r++)total[side]+=alive[side][r]*force_value(alive,side,r);
    return (total[view->turn]-total[1-view->turn])/fmaxf(80,total[0]+total[1]);
}
static float force_total(int alive[2][12],int side){
    float score=0;
    for(int r=SPY;r<=MARSHAL;r++)score+=alive[side][r]*force_value(alive,side,r)-alive[1-side][r]*force_value(alive,1-side,r);
    return score;
}
static float force_trade(const Game *view,float p[100][12],Move m){
    Piece a=view->board[m.from],d=view->board[m.to];
    if(d.side<0)return 0;
    int alive[2][12];
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)alive[side][r]=army_counts[r]-view->captured[side][r];
    float before=force_total(alive,a.side),expected=0;
    for(int r=SPY;r<=MARSHAL;r++)if(p[m.to][r]>0){
        int result=combat_result(a.rank,r);
        if(result<=0)alive[a.side][a.rank]--;
        if(result>=0)alive[d.side][r]--;
        float nominal=result>0?worth[r]:result<0?-worth[a.rank]:worth[r]-worth[a.rank];
        expected+=p[m.to][r]*(force_total(alive,a.side)-before-nominal);
        if(result<=0)alive[a.side][a.rank]++;
        if(result>=0)alive[d.side][r]++;
    }
    /* A bounded root adjustment: do not replace tactical search or defensive
       emergencies with a large speculative army-composition bonus. */
    return fmaxf(-12,fminf(12,expected*.5f));
}
