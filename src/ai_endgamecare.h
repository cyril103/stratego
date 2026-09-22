/* A rescue must work for every public identity of the root defender. */
static bool certain_survival(const Game *v,float p[100][12],Move m){
    if(v->board[m.to].side<0)return true;
    for(int r=FLAG;r<=BOMB;r++)if(p[m.to][r]>0&&combat_result(v->board[m.from].rank,r)<=0)return false;
    return true;
}
static bool active_spy_threat(const Game *v){
    if(v->captured[1-v->turn][MARSHAL]>=army_counts[MARSHAL])return false;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==SPY)
        return spy_square_unsafe(v,s,v->turn);
    return false;
}
static bool saves_active_spy(const Game *v,float p[100][12],Move m){
    Piece d=v->board[m.to],a=v->board[m.from];
    if(d.side>=0&&d.revealed&&d.rank==MARSHAL&&combat_result(a.rank,d.rank)>=0)return true;
    if(!certain_survival(v,p,m))return false;
    Game next=optimistic_move(v,m);
    /* Another unit may already be attacked too. Refuse a newly created loss,
       not an unrelated danger that this spy retreat cannot also repair. */
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank!=SPY){
        int before=s==m.to?m.from:s;
        if(known_unanswered_loss_at(&next,v->turn,s)>known_unanswered_loss_at(v,v->turn,before))return false;
    }
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank==SPY)
        return !spy_square_unsafe(&next,s,v->turn);
    return false;
}
/* Do not spend one of the final two mobile pieces on a mostly-bomb probe
   while a materially safe alternative survives the terminal filters. The
   last mobile piece and higher-confidence flag attempts retain their chance. */
static bool costly_bomb_probe(const Game *v,float p[100][12],Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(a.rank==MINER||d.side<0||d.revealed||p[m.to][BOMB]<.5f||p[m.to][FLAG]>=.5f)return false;
    int mobile=0;for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&movable(v->board[s]))mobile++;
    return mobile==2;
}
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
