/* A rescue must work for every public identity of the root defender. */
static bool certain_survival(const Game *v,float p[100][12],Move m){
    if(v->board[m.to].side<0)return true;
    for(int r=FLAG;r<=BOMB;r++)if(p[m.to][r]>0&&combat_result(v->board[m.from].rank,r)<=0)return false;
    return true;
}
/* Keep possible long-range fire separate from certain contact danger: when
   no immediate rescue exists, opening an escape corridor remains useful. */
static bool spy_survival_unsafe(const Game *v,int square,int side){
    if(spy_square_unsafe(v,square,side))return true;
    int hidden_scouts=army_counts[SCOUT]-v->captured[1-side][SCOUT];
    for(int s=0;s<100;s++)if(v->board[s].side==1-side&&v->board[s].revealed&&v->board[s].rank==SCOUT)hidden_scouts--;
    if(hidden_scouts<=0)return false;
    for(int e=0;e<100;e++){
        Piece enemy=v->board[e];
        if(enemy.side!=1-side||enemy.revealed||!enemy.moved)continue;
        Game probe=*v;probe.board[e].rank=SCOUT;
        if(public_legal(&probe,(Move){e,square},1-side))return true;
    }
    return false;
}
static bool active_spy_threat(const Game *v){
    if(v->captured[1-v->turn][MARSHAL]>=army_counts[MARSHAL])return false;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==SPY)
        return spy_survival_unsafe(v,s,v->turn);
    return false;
}
static bool saves_active_spy(const Game *v,float p[100][12],Move m){
    Piece d=v->board[m.to],a=v->board[m.from];
    if(d.side>=0&&d.revealed&&d.rank==MARSHAL&&combat_result(a.rank,d.rank)>=0)return true;
    if(!certain_survival(v,p,m))return false;
    if(a.rank==SPY&&spy_box_cost(v,m)>0)return false;
    Game next=optimistic_move(v,m);
    /* Another unit may already be attacked too. Refuse a newly created loss,
       not an unrelated danger that this spy retreat cannot also repair. */
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank!=SPY){
        int before=s==m.to?m.from:s;
        if(known_unanswered_loss_at(&next,v->turn,s)>known_unanswered_loss_at(v,v->turn,before))return false;
    }
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank==SPY)
        return !spy_survival_unsafe(&next,s,v->turn);
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
/* Look one enemy approach ahead: an allied blocker can make a last miner's
   future rescue impossible. Only known surviving attackers establish a trap. */
static bool miner_trapped_after_approach(const Game *v,int miner){
    for(int e=0;e<100;e++){
        Piece enemy=v->board[e];
        if(enemy.side!=1-v->turn||!enemy.revealed||!movable(enemy)||combat_result(enemy.rank,MINER)<0)continue;
        for(int t=0;t<100;t++){
            if(t==miner||!public_legal(v,(Move){e,t},enemy.side))continue;
            Piece target=v->board[t];
            if(target.side==v->turn&&combat_result(enemy.rank,target.rank)<=0)continue;
            Game probe=*v;
            if(target.side==v->turn)probe.board[t].revealed=true;
            Game next=optimistic_move(&probe,(Move){e,t});
            if(!known_miner_threat(&next,miner))continue;
            bool escape=false;int nb[4],nn=neighbors(miner,nb);
            for(int j=0;j<nn;j++){
                Move m={miner,nb[j]};Piece d=next.board[m.to];
                if(!public_legal(&next,m,v->turn)||
                   (d.side>=0&&(!d.revealed||combat_result(MINER,d.rank)<=0)))continue;
                Game out=optimistic_move(&next,m);
                if(!known_miner_threat(&out,m.to)){escape=true;break;}
            }
            /* A guard may remove the approaching attacker before it takes
               the miner. An equal exchange also eliminates that threat. */
            for(int s=0;s<100&&!escape;s++){
                Piece guard=next.board[s];
                if(guard.side!=v->turn||!movable(guard)||combat_result(guard.rank,enemy.rank)<0||
                   !public_legal(&next,(Move){s,t},v->turn))continue;
                Game out=optimistic_move(&next,(Move){s,t});
                if(!known_miner_threat(&out,s==miner?t:miner))escape=true;
            }
            if(!escape)return true;
        }
    }
    return false;
}
static int boxed_last_miner(const Game *v){
    if(army_counts[MINER]-v->captured[v->turn][MINER]!=1||
       v->captured[1-v->turn][BOMB]>=army_counts[BOMB])return -1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MINER&&
        !known_miner_threat(v,s)&&miner_trapped_after_approach(v,s))return s;
    return -1;
}
static bool opens_miner_escape(const Game *v,float p[100][12],Move m,int miner){
    if(!saves_last_miner(v,p,m,miner))return false;
    Game next=optimistic_move(v,m);
    return !miner_trapped_after_approach(&next,m.from==miner?m.to:miner);
}
/* Do not spend one of the final two mobile pieces on a mostly-bomb probe
   while a materially safe alternative survives the terminal filters. The
   last mobile piece and higher-confidence flag attempts retain their chance. */
static bool costly_bomb_probe(const Game *v,float p[100][12],Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(a.rank==MINER||d.side<0||d.revealed||p[m.to][BOMB]<.25f||p[m.to][FLAG]>=.5f)return false;
    int mobile=0;for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&movable(v->board[s]))mobile++;
    return mobile==2;
}
/* In a small losing army, trading its sole high officer can leave only
   weaker pieces or too few defenders against surviving enemies. Price allowing the
   exchange as well as initiating it; terminal flag filters still win. */
/* Compare both positions with the opponent to move. A defensive exchange
   may release the remaining guard from a stronger escort's control. */
static float flag_exchange_relief(const Game *v,Move m){
    Piece a=v->board[m.from],d=v->board[m.to];
    if(d.side!=1-v->turn||!d.revealed||d.rank!=a.rank||a.rank<CAPTAIN)return 0;
    int flag=-1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==FLAG)flag=s;
    if(flag<0||abs(m.to%10-flag%10)+abs(m.to/10-flag/10)>3)return 0;
    Game wait=*v,after=optimistic_move(v,m);wait.turn=after.turn=1-v->turn;
    return fmaxf(0,ai_flag_risk(&wait,v->turn)-ai_flag_risk(&after,v->turn));
}
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
    return d.side==1-v->turn&&d.revealed&&d.rank==a.rank&&last_officer_trade_cost(v,m)>0&&flag_exchange_relief(v,m)<40;
}
/* In a small defense, preserve an exchange that is substantially safer for
   the flag than EVERY available retreat. Sampled armies must not reinstate
   an exposed escort merely because retaining the officer scores well. */
static int urgent_flag_exchanges(const Game *v,float p[100][12],Move moves[MAX_MOVES],int n){
    int mobile=0;for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&movable(v->board[s]))mobile++;
    if(mobile>3)return n;
    bool exchange[MAX_MOVES];bool found=false;
    for(int i=0;i<n;i++){
        if(v->board[moves[i].to].side==1-v->turn&&p[moves[i].to][FLAG]>.5f)return n;
        exchange[i]=flag_exchange_relief(v,moves[i])>=40;found|=exchange[i];
    }
    if(!found)return n;
    float risk[MAX_MOVES],best=1e9f,other=1e9f;
    for(int i=0;i<n;i++){
        Game next=optimistic_move(v,moves[i]);next.turn=1-v->turn;
        risk[i]=ai_flag_risk(&next,v->turn);
        if(exchange[i])best=fminf(best,risk[i]);else other=fminf(other,risk[i]);
    }
    if(other<best+40)return n;
    int kept=0;for(int i=0;i<n;i++)if(exchange[i]&&risk[i]<=best+.00001f)moves[kept++]=moves[i];
    return kept?kept:n;
}
/* An already exposed marshal needs an answer even when another piece moves.
   A moved, unidentified neighbour is a possible spy, never a known spy. */
static bool marshal_contact_unsafe(const Game *v,float p[100][12]){
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MARSHAL&&v->board[s].revealed){
        int nb[4],nn=neighbors(s,nb);
        for(int i=0;i<nn;i++){
            int e=nb[i];Piece enemy=v->board[e];
            if(enemy.side==1-v->turn&&p[e][SPY]>0)return true;
        }
    }
    return false;
}
static bool marshal_needs_exit(const Game *v,float p[100][12]){
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MARSHAL&&v->board[s].revealed)
        for(int e=0;e<100;e++)if(v->board[e].side==1-v->turn&&v->board[e].moved&&p[e][SPY]>0&&
            abs(e%10-s%10)+abs(e/10-s/10)<=2)return true;
    return false;
}
static bool marshal_has_exit(const Game *v,float p[100][12]){
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MARSHAL){
        int nb[4],nn=neighbors(s,nb);
        for(int i=0;i<nn;i++){
            Move retreat={s,nb[i]};
            if(v->board[retreat.to].side>=0||!public_legal(v,retreat,v->turn))continue;
            Game next=optimistic_move(v,retreat);
            if(!marshal_contact_unsafe(&next,p)&&known_unanswered_loss(v,retreat)==0)return true;
        }
    }
    return false;
}
/* A retreat can be safe now but leave both exits covered by one quiet enemy
   approach. Test that geometry with public identities, including captures by
   the marshal: an unprotected possible spy is prey when WE initiate combat. */
static bool marshal_approach_trap(const Game *v,float p[100][12]){
    int marshal=-1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MARSHAL&&v->board[s].revealed)marshal=s;
    if(marshal<0)return false;
    for(int e=0;e<100;e++){
        Piece enemy=v->board[e];
        if(enemy.side!=1-v->turn||!enemy.moved||p[e][SPY]<=0||
           abs(e%10-marshal%10)+abs(e/10-marshal/10)>3)continue;
        int nb[4],nn=neighbors(e,nb);
        for(int j=0;j<nn;j++){
            int t=nb[j];
            if(v->board[t].side>=0||abs(t%10-marshal%10)+abs(t/10-marshal/10)>2)continue;
            Game legal=*v;legal.board[e].rank=SPY;
            if(!public_legal(&legal,(Move){e,t},enemy.side))continue;
            Game next=optimistic_move(v,(Move){e,t});
            float moved[100][12];memcpy(moved,p,sizeof(moved));
            memcpy(moved[t],moved[e],sizeof(moved[t]));memset(moved[e],0,sizeof(moved[e]));
            Move answers[MAX_MOVES];int count=game_moves(&next,v->turn,answers);bool escape=false;
            for(int k=0;k<count;k++){
                Move m=answers[k];
                if(!public_legal(&next,m,v->turn)||!certain_survival(&next,moved,m)||known_unanswered_loss(&next,m)>0)continue;
                Game out=optimistic_move(&next,m);
                if(marshal_contact_unsafe(&out,moved))continue;
                /* A guard may remove the hunter or clear a safe retreat. */
                if(m.from==marshal||m.to==t||marshal_has_exit(&out,moved)){escape=true;break;}
                /* At distance two the suspect cannot attack the marshal yet.
                   Another safe move can hold this standoff: approaching into
                   contact hands initiative back to the marshal. Lack of an
                   immediate retreat alone is not a forced loss. */
                if(!immediate_defeat(&next,m)&&scout_flag_risk(&next,moved,m)==0){escape=true;break;}
            }
            if(!escape)return true;
        }
    }
    return false;
}
static int preserve_marshal_exit(const Game *v,float p[100][12],Move *moves,int n){
    if(marshal_contact_unsafe(v,p)||!marshal_needs_exit(v,p)||!marshal_has_exit(v,p))return n;
    Move safe[MAX_MOVES];int kept=0;
    for(int i=0;i<n;i++){
        Game next=optimistic_move(v,moves[i]);
        bool blocks=v->board[moves[i].from].rank!=MARSHAL&&v->board[moves[i].to].side<0&&
            marshal_needs_exit(&next,p)&&!marshal_has_exit(&next,p);
        if(v->board[moves[i].to].side<0&&marshal_approach_trap(&next,p))blocks=true;
        if(!blocks)safe[kept++]=moves[i];
    }
    if(kept){memcpy(moves,safe,(size_t)kept*sizeof(Move));return kept;}
    return n;
}
static int rescue_exposed_marshal(const Game *v,float p[100][12],Move *moves,int n){
    if(!marshal_contact_unsafe(v,p))return n;
    Move safe[MAX_MOVES];int kept=0;
    for(int i=0;i<n;i++){
        Move m=moves[i];
        float failure=0;
        if(v->board[m.to].side==1-v->turn)for(int r=FLAG;r<=BOMB;r++)
            if(combat_result(v->board[m.from].rank,r)<=0)failure+=p[m.to][r];
        if(failure>.1f||known_unanswered_loss(v,m)>0||last_officer_trade_cost(v,m)>0)continue;
        Game next=optimistic_move(v,m);
        if(marshal_contact_unsafe(&next,p))continue;
        safe[kept++]=m;
    }
    if(kept){memcpy(moves,safe,(size_t)kept*sizeof(Move));return kept;}
    return n;
}
/* A sole guard must finish a reachable interception instead of resuming a
   speculative flag hunt while an identified miner crosses our back ranks. */
static int sole_guard_miner_route(const Game *v,int dist[100]){
    int guard=-1,flag=-1;
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn){
        if(v->board[s].rank==FLAG)flag=s;
        if(movable(v->board[s])){if(guard>=0)return -1;guard=s;}
    }
    if(guard<0||flag<0||v->board[guard].rank<=MINER)return -1;
    int home[100];intercept_distances(v,flag,1-v->turn,MINER,home);
    int hidden_miners=army_counts[MINER]-v->captured[1-v->turn][MINER];
    bool dominant=true;
    for(int r=v->board[guard].rank;r<=MARSHAL;r++)if(army_counts[r]>v->captured[1-v->turn][r])dominant=false;
    for(int s=0;s<100;s++)if(v->board[s].side==1-v->turn&&v->board[s].revealed&&v->board[s].rank==MINER)hidden_miners--;
    int enemy=-1,arrival=7;
    for(int s=0;s<100;s++)if(v->board[s].side==1-v->turn&&
        ((v->board[s].revealed&&v->board[s].rank==MINER)||
         (!v->board[s].revealed&&v->board[s].moved&&hidden_miners>0&&dominant))&&home[s]<arrival){enemy=s;arrival=home[s];}
    if(enemy<0)return -1;
    intercept_distances(v,enemy,v->turn,v->board[guard].rank,dist);
    return dist[guard]<100?guard:-1;
}
