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
/* The final miner is an irreplaceable route through the enemy bomb screen.
   A recapture does not restore that capability. Only identified attackers
   establish this emergency; hidden ranks are never consulted. */
static bool known_miner_threat(const Game *v,int square){
    for(int e=0;e<100;e++){
        Piece a=v->board[e];
        if(a.side==1-v->turn&&a.revealed&&movable(a)&&combat_result(a.rank,MINER)>=0&&
            public_legal(v,(Move){e,square},a.side))return true;
    }
    return false;
}
static int threatened_last_miner(const Game *v){
    if(army_counts[MINER]-v->captured[v->turn][MINER]!=1||
        army_counts[BOMB]-v->captured[1-v->turn][BOMB]<=0)return -1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MINER&&known_miner_threat(v,s))return s;
    return -1;
}
static bool saves_last_miner(const Game *v,float p[100][12],Move m,int miner){
    if(!certain_survival(v,p,m))return false;
    Game next=optimistic_move(v,m);int square=m.from==miner?m.to:miner;
    Piece target=v->board[m.to];
    if(target.side==1-v->turn&&target.revealed)next.captured[target.side][target.rank]++;
    if(known_miner_threat(&next,square))return false;
    /* Do not rescue it by newly hanging an officer or the active spy. */
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&s!=square){
        int before=s==m.to?m.from:s;
        if(known_unanswered_loss_at(&next,v->turn,s)>known_unanswered_loss_at(v,v->turn,before))return false;
    }
    return true;
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
   weaker pieces or too few defenders against surviving enemies. Price allowing the
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
    int superior=0,enemy_mobile=0;
    for(int r=reserve+1;r<=MARSHAL;r++)superior+=army_counts[r]-v->captured[1-side][r];
    for(int r=SPY;r<=MARSHAL;r++)enemy_mobile+=army_counts[r]-v->captured[1-side][r];
    /* The equal opponent also disappears in the exchange. */
    /* A single reserve can outrank every enemy yet fail to cover several
       attackers approaching different flag gates. Count that loss of board
       coverage in an already losing army, not only numerical rank control. */
    bool lone_guard=others==1&&enemy_mobile>=4;
    if(superior<=1&&!lone_guard)return 0;
    Piece a=v->board[m.from],d=v->board[m.to];
    if(m.from==officer&&d.side==1-side&&d.revealed&&d.rank==highest)return 2*worth[highest];
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<=0))return 0;
    Game next=optimistic_move(v,m);int square=m.from==officer?m.to:officer;
    for(int e=0;e<100;e++)if(next.board[e].side==1-side&&next.board[e].revealed&&
        next.board[e].rank==highest&&public_legal(&next,(Move){e,square},1-side))return 2*worth[highest];
    return 0;
}
static bool initiates_last_officer_trade(const Game *v,Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    return d.side==1-v->turn&&d.revealed&&d.rank==a.rank&&last_officer_trade_cost(v,m)>0;
}
