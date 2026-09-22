/* A small public-information proof search for bomb-gate zugzwang. The defender
   gets every legal move and the BEST result of an unidentified combat. Only
   identified enemy pieces may demonstrate a forced loss. Budget exhaustion
   means unknown, never a proof. This is separate from sampled army searches. */
static bool public_flag_forced(const Game *g,int side,int depth,int *budget){
    if(--*budget<0)return false;
    int flag=-1;
    for(int s=0;s<100;s++)if(g->board[s].side==side&&g->board[s].rank==FLAG)flag=s;
    if(flag<0)return true;
    if(depth==0)return false;
    Move moves[MAX_MOVES];int count=game_moves(g,g->turn,moves),tested=0;
    if(g->turn!=side){
        Candidate ordered[MAX_MOVES];int n=0;
        for(int i=0;i<count;i++){
            Move m=moves[i];Piece a=g->board[m.from],d=g->board[m.to];
            if(!a.revealed||!public_legal(g,m,g->turn))continue;
            float score=0;
            if(d.side==side)score=d.rank==FLAG?10000:combat_result(a.rank,d.rank)>=0?1000:-1000;
            for(int s=0;s<100;s++)if(g->board[s].side==side&&movable(g->board[s])&&combat_result(a.rank,g->board[s].rank)>=0){
                int before=abs(m.from%10-s%10)+abs(m.from/10-s/10),after=abs(m.to%10-s%10)+abs(m.to/10-s/10);
                score=fmaxf(score,100.0f*(before-after)+80.0f/(after+1));
            }
            insert(ordered,&n,MAX_MOVES,m,score);
        }
        count=n;for(int i=0;i<n;i++)moves[i]=ordered[i].move;
    }
    for(int i=0;i<count;i++){
        Move m=moves[i];Piece a=g->board[m.from],d=g->board[m.to];
        if(!public_legal(g,m,g->turn))continue;
        if(g->turn!=side&&!a.revealed)continue;
        if(g->turn==side&&d.side==1-side&&
           ((d.revealed&&d.rank==FLAG)||(!d.revealed&&!d.moved)))return false;
        Game combat=*g;
        /* Our ranks are known to us even when the opponent has not seen them.
           Its attacks must not magically defeat an unrevealed stronger guard. */
        if(g->turn!=side&&d.side==side)combat.board[m.to].revealed=true;
        Game next=optimistic_move(&combat,m);next.turn=1-g->turn;
        bool loses=public_flag_forced(&next,side,depth-1,budget);
        tested++;
        if(g->turn==side&&!loses)return false;
        if(g->turn!=side&&loses)return true;
        if(*budget<0)return false;
    }
    return g->turn==side&&tested>0;
}
static float guard_tempo_risk(const Game *view,float p[100][12],Move m){
    int side=view->turn,flag=-1,mobile=0;
    for(int s=0;s<100;s++)if(view->board[s].side==side){
        if(view->board[s].rank==FLAG)flag=s;
        if(movable(view->board[s]))mobile++;
    }
    int attackers=0;
    for(int r=SPY;r<=MARSHAL;r++)attackers+=army_counts[r]-view->captured[1-side][r];
    /* A local ending can have many surviving defenders. In particular a
       scout can pin one guard while a miner approaches another bomb gate. */
    /* Extend the formerly disabled large-army case. Keep the established
       small-army horizon: farther attacks in those positions can exhaust
       the proof budget and falsely favor an unproven sacrificial line over
       a known losing but materially sound retreat. */
    bool local_ending=attackers<=3&&mobile>4;
    if(flag<0||(mobile>4&&!local_ending))return 0;
    bool gate_threat=false;
    for(int s=0;s<100;s++){
        Piece enemy=view->board[s];
        if(enemy.side==1-side&&enemy.revealed&&movable(enemy)&&
           abs(s%10-flag%10)+abs(s/10-flag/10)<=(local_ending?5:2)){
            /* Once a gate is open, any mobile rank can finish the attack.
               Keep the proof search dormant behind an intact bomb ring
               unless the identified threat is a miner. */
            int nb[4],nn=neighbors(flag,nb);
            for(int k=0;k<nn;k++)if(enemy.rank==MINER||
                view->board[nb[k]].side!=side||view->board[nb[k]].rank!=BOMB)gate_threat=true;
        }
    }
    if(!gate_threat)return 0;
    Piece target=view->board[m.to];
    if(target.side==1-side&&p[m.to][FLAG]>0)return 0;
    float risk=0;
    if(target.side==1-side&&!target.revealed){
        /* Losing the penultimate mobile piece can be fatal even when the
           remaining colonel is strong: it may be tied to its bomb gate. */
        for(int rank=SPY;rank<=BOMB;rank++)if(p[m.to][rank]>0){
            Game hypothesis=*view;hypothesis.board[m.to].rank=rank;hypothesis.board[m.to].revealed=true;
            Game next=optimistic_move(&hypothesis,m);next.turn=1-side;
            int budget=20000;
            if(public_flag_forced(&next,side,9,&budget))risk+=p[m.to][rank];
        }
    }else{
        Game next=optimistic_move(view,m);next.turn=1-side;int budget=20000;
        if(public_flag_forced(&next,side,9,&budget))risk=1;
        /* A moved but unidentified hunter may close the last spare officer's
           exits next turn. Enumerate only public plausible ranks in contact
           range; do not wait until the equal-rank trap has been revealed. */
        if(risk==0)for(int e=0;e<100;e++){
            Piece hunter=next.board[e];
            if(hunter.side!=1-side||hunter.revealed||!hunter.moved||
               abs(e%10-m.to%10)+abs(e/10-m.to/10)>2)continue;
            float danger=0;
            for(int rank=SPY;rank<=MARSHAL;rank++)if(p[e][rank]>0&&combat_result(rank,view->board[m.from].rank)>=0){
                Game hypothesis=next;hypothesis.board[e].rank=rank;hypothesis.board[e].revealed=true;
                budget=20000;
                if(public_flag_forced(&hypothesis,side,9,&budget))danger+=p[e][rank];
            }
            risk=fmaxf(risk,danger);
        }
    }
    return risk;
}
