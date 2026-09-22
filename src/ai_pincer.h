/* Count legal escapes that cannot immediately be captured at a known profit.
   Uses identified prey only; hidden ranks never supply an imaginary target. */
static int prey_exits(const Game *v,int prey,int side){
    Move replies[MAX_MOVES];int n=game_moves(v,1-side,replies),safe=0;
    for(int i=0;i<n;i++){
        Move m=replies[i];if(m.from!=prey)continue;
        Piece a=v->board[prey],d=v->board[m.to];
        if(d.side==side&&combat_result(a.rank,d.rank)<=0)continue;
        Game next=*v;next.board[prey]=empty_piece();next.board[m.to]=a;
        bool caught=false;
        for(int s=0;s<100;s++){
            Piece hunter=next.board[s];
            if(hunter.side!=side||!movable(hunter)||combat_result(hunter.rank,a.rank)<=0)continue;
            if(!public_legal(&next,(Move){s,m.to},side))continue;
            Game capture=next;capture.board[s]=empty_piece();capture.board[m.to]=hunter;
            if(known_unanswered_loss_at(&capture,side,m.to)==0){caught=true;break;}
        }
        if(!caught)safe++;
    }
    return safe;
}
static float pincer_bonus(const Game *v,Move m){
    if(v->board[m.to].side>=0||v->board[m.from].rank<MINER)return 0;
    int side=v->turn,mobile=0;
    for(int s=0;s<100;s++)if(v->board[s].side==side&&movable(v->board[s]))mobile++;
    if(mobile<2||mobile>8)return 0;
    Game next=optimistic_move(v,m);
    if(known_unanswered_loss_at(&next,side,m.to)>0||ai_flag_risk(&next,side)>ai_flag_risk(v,side))return 0;
    float best=0;
    for(int e=0;e<100;e++){
        Piece prey=v->board[e];
        if(prey.side!=1-side||!prey.revealed||!movable(prey)||combat_result(v->board[m.from].rank,prey.rank)<=0)continue;
        if(abs(m.to%10-e%10)+abs(m.to/10-e/10)>3)continue;
        bool partner=false;
        for(int s=0;s<100;s++){
            Piece a=v->board[s];
            if(s!=m.from&&a.side==side&&movable(a)&&combat_result(a.rank,prey.rank)>0&&
               public_legal(v,(Move){s,e},side)&&public_legal(&next,(Move){s,e},side)){partner=true;break;}
        }
        if(!partner)continue;
        int closed=prey_exits(v,e,side)-prey_exits(&next,e,side);
        best=fmaxf(best,fminf(12,4*closed));
    }
    return best;
}
