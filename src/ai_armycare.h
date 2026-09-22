/* Multiple victims are alternatives, not simultaneous captures. */
static float army_exposure(const Game *v,int side){
    float largest=0,total=0;
    for(int s=0;s<100;s++){
        Piece victim=v->board[s];
        if(victim.side!=side||!movable(victim))continue;
        float danger=known_unanswered_loss_at(v,side,s)*.25f;
        /* Anticipate a revealed invader approaching through an empty square. */
        if(danger==0)for(int e=0;e<100;e++){
            Piece enemy=v->board[e];
            if(enemy.side!=1-side||!enemy.revealed||!movable(enemy)||
               combat_result(enemy.rank,victim.rank)<=0)continue;
            if(enemy.rank!=SCOUT&&abs(e%10-s%10)+abs(e/10-s/10)>2)continue;
            int nb[4],n=neighbors(e,nb);
            for(int j=0;j<n;j++){
                Move step={e,nb[j]};
                if(v->board[step.to].side>=0||!public_legal(v,step,1-side))continue;
                Game next=*v;next.board[e]=empty_piece();next.board[step.to]=enemy;
                danger=fmaxf(danger,.07f*known_unanswered_loss_at(&next,side,s));
            }
        }
        largest=fmaxf(largest,danger);total+=danger;
    }
    /* The main evaluator already prices the largest tactical loss. This
       term only adds the rest of the formation, avoiding a second rescue
       premium that could postpone an urgent attack for one exposed piece. */
    return .35f*(total-largest);
}
static float collective_retreat(const Game *v,Move m,float before){
    Piece d=v->board[m.to],a=v->board[m.from];
    /* Unknown combats cannot manufacture a rescued formation. */
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))return 0;
    Game next=optimistic_move(v,m);
    return 2*(before-army_exposure(&next,v->turn));
}
/* Charge the loss of a scarce capability in addition to ordinary material.
   Integrate public probabilities analytically, including rare counterattacks. */
static float miner_capability_cost(const Game *v,float p[100][12],Move m){
    Piece a=v->board[m.from],d=v->board[m.to];int side=v->turn;
    int left=army_counts[MINER]-v->captured[side][MINER];
    if(a.rank!=MINER||left<1||left>3||v->captured[1-side][BOMB]>=army_counts[BOMB])return 0;
    float failure=0,flag=0;
    if(d.side==1-side){
        flag=p[m.to][FLAG];
        for(int r=0;r<12;r++)if(combat_result(MINER,r)<=0)failure+=p[m.to][r];
    }
    Game next=optimistic_move(v,m);float counter=0;
    for(int e=0;e<100;e++)if(e!=m.to&&v->board[e].side==1-side){
        float attack=0;
        for(int r=SPY;r<=MARSHAL;r++)if(p[e][r]>0&&combat_result(r,MINER)>=0){
            Game probe=next;probe.board[e].rank=r;
            if(public_legal(&probe,(Move){e,m.to},1-side))attack+=p[e][r];
        }
        counter=fmaxf(counter,attack);
    }
    /* A flag capture ends the game; no hypothetical reply is then charged. */
    float risk=failure+fmaxf(0,1-failure-flag)*counter;
    /* Bomb-screen probes are the capability's purpose. Preserve their
       existing route evaluation instead of charging the full reserve price. */
    float mission=d.side==1-side&&!d.moved?p[m.to][BOMB]+flag:0;
    int advance=side==COMPUTER?m.to/10:9-m.to/10;
    if(advance>=6&&mission>=.5f)return 0;
    return 3*worth[MINER]*risk*(1-.8f*mission)/left;
}
