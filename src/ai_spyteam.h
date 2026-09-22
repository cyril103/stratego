/* A losing army needs chances to recover, not an indefinitely preserved spy.
   Public casualties establish the deficit; only a moved, unidentified target
   with nonzero marshal odds qualifies. Safe retreat delays risk at moderate
   deficits, but cannot veto it when outnumbered by more than roughly 3:1. */
static float spy_comeback_chance(const Game *v,float p[100][12],Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(a.rank!=SPY||d.side!=1-v->turn||d.revealed||!d.moved||p[m.to][MARSHAL]<=0||
       v->captured[1-v->turn][MARSHAL]>=army_counts[MARSHAL]||force_advantage(v)>=0)return 0;
    int own=0,enemy=0;
    for(int r=SPY;r<=MARSHAL;r++){
        own+=army_counts[r]-v->captured[v->turn][r];
        enemy+=army_counts[r]-v->captured[1-v->turn][r];
    }
    if(enemy<=0)return 0;
    float deficit=1-(float)own/enemy;
    float urgency=fmaxf(0,fminf(1,(deficit-.25f)/.4f));
    if(urgency==0)return 0;
    int nb[4],nn=neighbors(m.from,nb);bool escape=false;
    for(int j=0;j<nn;j++){
        Move retreat={m.from,nb[j]};
        if(v->board[retreat.to].side>=0||!public_legal(v,retreat,v->turn))continue;
        Game next=optimistic_move(v,retreat);
        if(!spy_square_unsafe(&next,retreat.to,v->turn)){escape=true;break;}
    }
    return escape?urgency: fminf(1,urgency*1.5f);
}
/* Retain the capability cost in viable positions. In a severe deficit, an
   analytic marshal upside keeps a rare winning identity from disappearing
   in the small sample. Ordinary combat losses and flag safety still apply. */
static float spy_attack_cost(const Game *v,float p[100][12],Move m){
    if(v->board[m.from].rank!=SPY||v->board[m.to].side!=1-v->turn||
       v->captured[1-v->turn][MARSHAL]>=army_counts[MARSHAL])return 0;
    float loss=0;for(int r=0;r<12;r++)if(combat_result(SPY,r)<=0)loss+=p[m.to][r];
    float comeback=spy_comeback_chance(v,p,m);
    return 4*worth[MARSHAL]*loss*(1-comeback)-2*worth[MARSHAL]*p[m.to][MARSHAL]*comeback;
}
/* Open a safe exit before the enemy can trap the spy behind its own army. */
static float spy_clearance_bonus(const Game *v,Move m){
    int side=v->turn,spy=-1;
    if(v->captured[1-side][MARSHAL]>=army_counts[MARSHAL]||v->board[m.to].side>=0)return 0;
    for(int s=0;s<100;s++)if(v->board[s].side==side&&v->board[s].rank==SPY)spy=s;
    if(spy<0||spy==m.from||spy_square_unsafe(v,spy,side))return 0;
    int nb[4],nn=neighbors(spy,nb);bool boxed=true,approaching=false;
    for(int j=0;j<nn;j++){
        Move escape={spy,nb[j]};
        if(v->board[nb[j]].side<0&&public_legal(v,escape,side)){
            Game next=optimistic_move(v,escape);
            if(!spy_square_unsafe(&next,escape.to,side))boxed=false;
        }
        for(int e=0;e<100;e++){
            Piece enemy=v->board[e],target=v->board[nb[j]];
            if(enemy.side!=1-side||!enemy.revealed||!movable(enemy))continue;
            if(target.side==side&&combat_result(enemy.rank,target.rank)<=0)continue;
            if(public_legal(v,(Move){e,nb[j]},1-side))approaching=true;
        }
    }
    if(!boxed||!approaching)return 0;
    Game next=optimistic_move(v,m);
    if(!public_legal(&next,(Move){spy,m.from},side)||known_unanswered_loss_at(&next,side,m.to)>0)return 0;
    /* Preparing tomorrow's escape must not abandon a miner or officer that
       can already be captured today. A scout screen may still buy the tempo. */
    for(int s=0;s<100;s++)if(next.board[s].side==side&&next.board[s].rank>=MINER&&
        next.board[s].rank<=MARSHAL&&known_unanswered_loss_at(&next,side,s)>0)return 0;
    Game escape=optimistic_move(&next,(Move){spy,m.from});
    if(spy_square_unsafe(&escape,m.from,side)||ai_flag_risk(&next,side)>ai_flag_risk(v,side))return 0;
    return 96;
}
/* Value control of a bait square, without assuming the opponent takes it. */
static float spy_ambush_value(const Game *v){
    int side=v->turn,spy=-1,marshal=-1;
    for(int s=0;s<100;s++){
        Piece a=v->board[s];
        if(a.side==side&&a.rank==SPY)spy=s;
        if(a.side==1-side&&a.revealed&&a.rank==MARSHAL)marshal=s;
    }
    if(spy<0||marshal<0||spy_square_unsafe(v,spy,side))return 0;
    int nb[4],nn=neighbors(spy,nb);float best=0;
    for(int j=0;j<nn;j++){
        int bait=nb[j];Piece ally=v->board[bait];
        if(ally.side!=side||ally.rank<SERGEANT||ally.rank>CAPTAIN)continue;
        if(!public_legal(v,(Move){marshal,bait},1-side))continue;
        Game take=*v;take.board[marshal]=empty_piece();take.board[bait]=v->board[marshal];
        if(!public_legal(&take,(Move){spy,bait},side))continue;
        best=fmaxf(best,16-.5f*worth[ally.rank]);
    }
    return best;
}
static float spy_ambush_bonus(const Game *v,Move m){
    if(v->board[m.to].side>=0)return 0;
    Game next=optimistic_move(v,m);
    if(ai_flag_risk(&next,v->turn)>ai_flag_risk(v,v->turn))return 0;
    return spy_ambush_value(&next)-spy_ambush_value(v);
}
