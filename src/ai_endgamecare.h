/* In a small losing army, trading its sole high officer can leave only
   weaker pieces against surviving superior enemies. Price allowing the
   exchange as well as initiating it; terminal flag filters still win. */
static float last_officer_trade_cost(const Game *v,Move m){
    int side=v->turn,officer=-1,highest=0,others=0,reserve=0;
    for(int s=0;s<100;s++)if(v->board[s].side==side&&movable(v->board[s])){
        if(v->board[s].rank>highest){highest=v->board[s].rank;officer=s;}
    }
    if(highest<CAPTAIN||force_advantage(v)>=0)return 0;
    for(int s=0;s<100;s++)if(s!=officer&&v->board[s].side==side&&movable(v->board[s])){
        others++;if(v->board[s].rank>reserve)reserve=v->board[s].rank;
    }
    if(others>3||reserve>=highest)return 0;
    int superior=0;
    for(int r=reserve+1;r<=MARSHAL;r++)superior+=army_counts[r]-v->captured[1-side][r];
    /* The equal opponent also disappears in the exchange. */
    if(superior<=1)return 0;
    Piece a=v->board[m.from],d=v->board[m.to];
    if(m.from==officer&&d.side==1-side&&d.revealed&&d.rank==highest)return 2*worth[highest];
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))return 0;
    Game next=optimistic_move(v,m);int square=m.from==officer?m.to:officer;
    for(int e=0;e<100;e++)if(next.board[e].side==1-side&&next.board[e].revealed&&
        next.board[e].rank==highest&&public_legal(&next,(Move){e,square},1-side))return 2*worth[highest];
    return 0;
}
