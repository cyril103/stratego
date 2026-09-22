/* Public tactical checks. Unknown enemy ranks never enter these routines. */
static bool officer_trapped(const Game *g,int side,int id,int depth,int *budget){
    if(--*budget<0)return false;
    int officer=-1;
    for(int s=0;s<100;s++)if(g->board[s].side==side&&g->board[s].id==id)officer=s;
    if(officer<0)return true;
    if(depth==0)return false;
    Move moves[MAX_MOVES];int n=game_moves(g,g->turn,moves),tested=0;
    for(int i=0;i<n;i++){
        Move m=moves[i];Piece a=g->board[m.from],d=g->board[m.to];
        if(!public_legal(g,m,g->turn))continue;
        if(g->turn!=side){
            if(!a.revealed||combat_result(a.rank,g->board[officer].rank)<=0)continue;
            int before=abs(m.from%10-officer%10)+abs(m.from/10-officer/10);
            int after=abs(m.to%10-officer%10)+abs(m.to/10-officer/10);
            if(after>=before||after>2)continue;
        }else if(d.side==1-side&&d.revealed&&d.rank==FLAG)return false;
        Game combat=*g;
        if(g->turn!=side&&d.side==side)combat.board[m.to].revealed=true;
        Game next=optimistic_move(&combat,m);next.turn=1-g->turn;
        bool loses;
        if(g->turn!=side&&m.to==officer){
            /* An adjacent spy can punish a marshal taking our general.
               Keep the exact legality, including the two-square rule. */
            bool recapture=false;
            for(int s=0;s<100;s++){
                Piece guard=next.board[s];
                if(guard.side==side&&movable(guard)&&combat_result(guard.rank,a.rank)>=0&&
                   public_legal(&next,(Move){s,m.to},side))recapture=true;
            }
            loses=!recapture;
        }else loses=officer_trapped(&next,side,id,depth-1,budget);
        tested++;
        if(g->turn==side&&!loses)return false;
        if(g->turn!=side&&loses)return true;
        if(*budget<0)return false;
    }
    return g->turn==side&&tested>0;
}
static float officer_trap_cost(const Game *view,Move m){
    Piece target=view->board[m.to];
    if(target.side>=0&&(!target.revealed||target.rank==FLAG))return 0;
    Game next=optimistic_move(view,m);next.turn=1-view->turn;float worst=0;
    for(int s=0;s<100;s++){
        Piece own=next.board[s];
        bool key_spy=own.rank==SPY&&view->captured[1-view->turn][MARSHAL]<army_counts[MARSHAL];
        if(own.side!=view->turn||(!key_spy&&(own.rank<COLONEL||own.rank>MARSHAL)))continue;
        bool near=false;
        for(int e=0;e<100;e++){
            Piece enemy=next.board[e];
            if(enemy.side==1-own.side&&enemy.revealed&&movable(enemy)&&
               combat_result(enemy.rank,own.rank)>0&&abs(s%10-e%10)+abs(s/10-e/10)<=3)near=true;
        }
        if(!near)continue;
        int budget=1600;
        if(officer_trapped(&next,own.side,own.id,7,&budget))worst=fmaxf(worst,4*worth[key_spy?MARSHAL:own.rank]);
    }
    return worst;
}
static bool spy_station_safe(const Game *g,int station,int side){
    for(int e=0;e<100;e++){
        Piece enemy=g->board[e];if(enemy.side!=1-side)continue;
        if(!enemy.revealed){if(abs(e%10-station%10)+abs(e/10-station/10)==1)return false;}
        else if(movable(enemy)&&public_legal(g,(Move){e,station},enemy.side))return false;
    }
    return true;
}
/* Distance to a safe square covering the general. A spy protects by attacking
   after the marshal captures; it must not be left where it is attacked first. */
static float general_escort(const Game *view){
    int side=view->turn,spy=-1,general=-1,marshal=-1;
    for(int s=0;s<100;s++){
        Piece p=view->board[s];
        if(p.side==side&&p.rank==SPY)spy=s;
        if(p.side==side&&p.rank==GENERAL)general=s;
        if(p.side==1-side&&p.revealed&&p.rank==MARSHAL)marshal=s;
    }
    if(spy<0||general<0||marshal<0)return 0;
    int threat=abs(marshal%10-general%10)+abs(marshal/10-general/10);
    if(threat>6)return 0;
    int dist[100],queue[100],head=0,tail=0;
    for(int s=0;s<100;s++)dist[s]=100;
    dist[spy]=0;queue[tail++]=spy;
    while(head<tail){
        int s=queue[head++];Game probe=*view;probe.board[spy]=empty_piece();probe.board[s]=view->board[spy];
        bool safe=spy_station_safe(&probe,s,side);
        if(safe&&abs(s%10-general%10)+abs(s/10-general/10)==1){
            Game reply=probe;reply.board[marshal]=empty_piece();reply.board[general]=view->board[marshal];
            if(public_legal(&reply,(Move){s,general},side))return fmaxf(0,80-12*dist[s]);
        }
        if(dist[s]>=6||(!safe&&s!=spy))continue;
        int nb[4],nn=neighbors(s,nb);
        for(int k=0;k<nn;k++)if(!is_lake(nb[k])&&view->board[nb[k]].side<0&&dist[nb[k]]==100){
            if(s==spy&&!public_legal(view,(Move){s,nb[k]},side))continue;
            dist[nb[k]]=dist[s]+1;queue[tail++]=nb[k];
        }
    }
    return 0;
}
static float general_escort_bonus(const Game *view,Move m){
    Piece a=view->board[m.from];if(a.rank!=SPY&&a.rank!=GENERAL)return 0;
    if(view->board[m.to].side>=0)return 0;
    Game next=optimistic_move(view,m);
    if(a.rank==SPY&&!spy_station_safe(&next,m.to,a.side))return 0;
    return general_escort(&next)-general_escort(view);
}
/* A favorable material exchange may still surrender our last high officer
   to an equal-rank recapture, leaving every remaining enemy officer dominant. */
static float lost_counter_cost(const Game *view,Move m,int recapturer){
    Piece a=view->board[m.from],d=view->board[m.to];
    int highest=0,own_mobile=0,enemy_mobile=0;
    for(int s=0;s<100;s++)if(s!=m.from&&view->board[s].side==a.side&&movable(view->board[s])){
        int rank=view->board[s].rank;
        if(rank>highest)highest=rank;
        own_mobile++;
    }
    /* Losing an officer is not losing our last counter if an equally strong
       reserve remains. Count enemy ranks from public casualties only. */
    if(highest>=a.rank)return 0;
    int superior=0,equal=0;
    for(int r=SPY;r<=MARSHAL;r++){
        int remaining=army_counts[r]-view->captured[1-a.side][r]-(d.rank==r)-(recapturer==r);
        if(remaining<0)remaining=0;
        enemy_mobile+=remaining;
        if(r>highest&&r>=SERGEANT)superior+=remaining;
        if(r==highest)equal=remaining;
    }
    if(enemy_mobile<=own_mobile||(!superior&&!equal))return 0;
    return 4*worth[a.rank];
}
static float last_counter_cost(const Game *view,Move m){
    Piece a=view->board[m.from],d=view->board[m.to];
    if(a.rank<SERGEANT||a.rank>MARSHAL||d.side<0||!d.revealed||combat_result(a.rank,d.rank)<0)return 0;
    if(combat_result(a.rank,d.rank)==0)return lost_counter_cost(view,m,-1);
    Game next=optimistic_move(view,m);
    float cost=0;
    for(int e=0;e<100;e++){
        Piece enemy=next.board[e];
        if(enemy.side!=1-a.side||!enemy.revealed||enemy.rank!=a.rank||!public_legal(&next,(Move){e,m.to},enemy.side))continue;
        cost=fmaxf(cost,lost_counter_cost(view,m,enemy.rank));
    }
    return cost;
}
static float counter_capture_cost(const Game *view,float p[100][12],Move m){
    Piece d=view->board[m.to];
    if(d.side<0||view->board[m.from].rank<SERGEANT)return 0;
    if(d.revealed)return last_counter_cost(view,m);
    float cost=0;
    for(int r=0;r<12;r++)if(p[m.to][r]>0&&r!=FLAG){
        Game hypothesis=*view;hypothesis.board[m.to].rank=r;hypothesis.board[m.to].revealed=true;
        cost+=p[m.to][r]*last_counter_cost(&hypothesis,m);
    }
    return cost;
}
