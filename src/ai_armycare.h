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
    return largest+.35f*(total-largest);
}
static float collective_retreat(const Game *v,Move m,float before){
    Piece d=v->board[m.to],a=v->board[m.from];
    /* Unknown combats cannot manufacture a rescued formation. */
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))return 0;
    Game next=optimistic_move(v,m);
    return 2*(before-army_exposure(&next,v->turn));
}
