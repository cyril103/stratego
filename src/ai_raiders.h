/* Give a known invading officer a specific, capable opponent even when our
   bomb enclosure prevents that invader from reaching the flag directly. */
typedef struct {int enemy,guard;float urgency;int distance[100];} RaiderTask;
typedef struct {RaiderTask tasks[5];int count;} RaiderPlan;
/* A stronger officer following the same weaker piece around a small circuit
   is not making progress. Stop funding repeated steps, while preserving
   captures, new squares, threatened-officer retreats and flag emergencies. */
static float stale_pursuit_cost(const Game *view,Move m,float flag_risk){
    Piece a=view->board[m.from];int side=view->turn;
    if(a.rank<SERGEANT||a.rank>MARSHAL||view->board[m.to].side>=0||flag_risk>0||
       view->history_count[side]<8||view->history_count[1-side]<8)return 0;
    bool visited[100]={false};int own_moves=0,own_squares=0;
    for(int i=0;i<8;i++)if(view->history_id[side][i]==a.id){
        int s=view->history[side][i].from;own_moves++;
        if(!visited[s]){visited[s]=true;own_squares++;}
    }
    if(own_moves<6||own_squares>4||!visited[m.to]||known_unanswered_loss_at(view,side,m.from)>0)return 0;
    for(int e=0;e<100;e++){
        Piece enemy=view->board[e];
        if(enemy.side!=1-side||!enemy.revealed||!movable(enemy)||
           combat_result(a.rank,enemy.rank)<=0||abs(e%10-m.from%10)+abs(e/10-m.from/10)>3)continue;
        bool circuit[100]={false};int moves=0,squares=0;
        for(int i=0;i<8;i++)if(view->history_id[1-side][i]==enemy.id){
            int s=view->history[1-side][i].from;moves++;
            if(!circuit[s]){circuit[s]=true;squares++;}
        }
        if(moves>=6&&squares<=4)return 128;
    }
    return 0;
}
static void raider_plan(const Game *view,RaiderPlan *plan){
    plan->count=0;int side=view->turn;bool assigned[100]={false};
    for(int e=0;e<100&&plan->count<5;e++){
        Piece enemy=view->board[e];
        if(enemy.side!=1-side||!enemy.revealed||enemy.rank<SERGEANT||enemy.rank>MARSHAL)continue;
        int depth=side==COMPUTER?9-e/10:e/10;
        if(depth<5)continue;
        RaiderTask task={.enemy=e,.guard=-1,.urgency=36};int best=100,best_score=100;
        bool vulnerable=false;int exposed=0;
        for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG&&
            abs(s%10-e%10)+abs(s/10-e/10)<=6)vulnerable=true;
        for(int s=0;s<100;s++){
            Piece own=view->board[s];
            if(own.side==side&&movable(own)&&combat_result(enemy.rank,own.rank)>0&&
               abs(s%10-e%10)+abs(s/10-e/10)<=3){vulnerable=true;exposed++;}
        }
        if(!vulnerable)continue;
        bool deep_officer=enemy.rank>=GENERAL&&(exposed>=2||depth>=6);
        task.urgency+=4*(depth-5);
        for(int rank=enemy.rank;rank<=MARSHAL;rank++){
            /* Do not seek a marshal trade while the spy lives. Intercepting
               a weaker raider is allowed, subject to local contact safety. */
            if(rank==MARSHAL&&enemy.rank==MARSHAL&&view->captured[side][SPY]<army_counts[SPY])continue;
            int distance[100];intercept_distances(view,e,side,rank,distance);
            for(int s=0;s<100;s++){
                Piece own=view->board[s];
                int score=distance[s]+(rank==enemy.rank?4:0)+(assigned[s]?4:0);
                if(own.side==side&&own.rank==rank&&distance[s]<=(deep_officer?16:8)&&score<best_score){
                    best=distance[s];best_score=score;task.guard=s;memcpy(task.distance,distance,sizeof(distance));
                }
            }
        }
        /* A remote expedition must not displace immediate local defense. */
        int reach=task.guard>=0&&view->board[task.guard].rank==MARSHAL&&enemy.rank>=GENERAL?8:6;
        if(deep_officer){reach=16;task.urgency+=12*exposed;}
        if(task.guard>=0&&best<=reach){plan->tasks[plan->count++]=task;assigned[task.guard]=true;}
    }
}
static bool futile_raid_follow(const Game *view,Move m){
    if(view->combat!=1||m.to!=view->last_move.from||view->board[m.to].side>=0||ai_flag_risk(view,view->turn)>0)return false;
    int raider=view->last_move.to;if(raider<0||raider>=100)return false;
    Piece enemy=view->board[raider],a=view->board[m.from];
    if(enemy.side!=1-a.side||!enemy.revealed||enemy.rank<SERGEANT||enemy.rank>MARSHAL||combat_result(a.rank,enemy.rank)<=0)return false;
    Game next=optimistic_move(view,m);
    for(int s=0;s<100;s++){
        Piece victim=next.board[s];
        if(victim.side!=a.side||!movable(victim)||combat_result(enemy.rank,victim.rank)<=0||
           !public_legal(&next,(Move){raider,s},enemy.side)||known_unanswered_loss_at(&next,a.side,s)<=0)continue;
        int nb[4],nn=neighbors(s,nb);
        for(int j=0;j<nn;j++){
            Move escape={s,nb[j]};
            if(view->board[nb[j]].side>=0||!public_legal(view,escape,a.side)||immediate_defeat(view,escape))continue;
            Game safer=optimistic_move(view,escape);
            if(known_unanswered_loss_at(&safer,a.side,escape.to)==0)return true;
        }
    }
    return false;
}
static float raider_bonus(const Game *view,const RaiderPlan *plan,Move m){
    Piece a=view->board[m.from],d=view->board[m.to];float best=0;
    if(d.side>=0&&(!d.revealed||combat_result(a.rank,d.rank)<0))return 0;
    Game next=optimistic_move(view,m);
    if(a.rank==MARSHAL){
        /* Allow a local interception while the spy lives, but never pay for
           entering contact with an unidentified unit that may be that spy. */
        int nb[4],nn=neighbors(m.to,nb);
        if(view->captured[1-a.side][SPY]<army_counts[SPY])for(int k=0;k<nn;k++)
            if(next.board[nb[k]].side==1-a.side&&!next.board[nb[k]].revealed)return 0;
    }
    for(int e=0;e<100;e++){
        Piece enemy=next.board[e];
        if(enemy.side==1-a.side&&enemy.revealed&&movable(enemy)&&
           combat_result(enemy.rank,a.rank)>0&&public_legal(&next,(Move){e,m.to},enemy.side))return 0;
    }
    for(int i=0;i<plan->count;i++){
        const RaiderTask *task=&plan->tasks[i];
        float gain=0;
        if(m.from==task->guard)gain=task->urgency*(task->distance[m.from]-task->distance[m.to]);
        else {
            /* A route through our own screen needs a real clearing move.
               Otherwise the assigned officer waits forever behind a teammate
               whose personal route leads in a different direction. */
            Piece guard=view->board[task->guard];
            if(abs(m.from%10-task->guard%10)+abs(m.from/10-task->guard/10)!=1||
               task->distance[m.from]>=task->distance[task->guard]||
               !public_legal(&next,(Move){task->guard,m.from},a.side))continue;
            Game advance=optimistic_move(&next,(Move){task->guard,m.from});
            if(known_unanswered_loss_at(&advance,a.side,m.from)>0)continue;
            if(guard.rank==MARSHAL&&view->captured[1-a.side][SPY]<army_counts[SPY]){
                int nb[4],nn=neighbors(m.from,nb);bool unknown=false;
                for(int k=0;k<nn;k++)if(advance.board[nb[k]].side==1-a.side&&!advance.board[nb[k]].revealed)unknown=true;
                if(unknown)continue;
            }
            gain=.8f*task->urgency;
        }
        /* The assigned officer must not abandon a more urgent flag defense. */
        if(gain>0&&ai_flag_risk(&next,a.side)>ai_flag_risk(view,a.side)+20)continue;
        if(gain>0&&task->urgency>80){
            bool hanging=false;
            for(int s=0;s<100;s++)if(next.board[s].side==a.side&&next.board[s].rank>=COLONEL&&
                next.board[s].rank<=MARSHAL&&known_unanswered_loss_at(&next,a.side,s)>0)hanging=true;
            if(hanging)continue;
        }
        if(fabsf(gain)>fabsf(best))best=gain;
    }
    return fmaxf(-80,fminf(80,best));
}
/* Keep the strongest remaining reserve in the defense area before a weak
   local guard is removed. A cheap remote capture must not postpone its return.
   The target is our flag area, so the mission persists when an intruder waits
   or switches lanes. Lake-aware distances avoid rewarding blocked directions. */
static float reserve_home_bonus(const Game *view,float p[100][12],Move m){
    int side=view->turn,flag=-1,mobile=0,strongest=0;
    for(int s=0;s<100;s++)if(view->board[s].side==side){
        Piece own=view->board[s];if(own.rank==FLAG)flag=s;
        if(movable(own)){mobile++;if(own.rank>strongest)strongest=own.rank;}
    }
    Piece a=view->board[m.from],d=view->board[m.to];
    if(flag<0||mobile>6||strongest<SERGEANT||a.rank!=strongest)return 0;
    int home[100];land_distances(flag,home);int threat=100;
    for(int e=0;e<100;e++){
        Piece enemy=view->board[e];if(enemy.side!=1-side)continue;
        bool raider=enemy.revealed&&movable(enemy);
        bool miner=enemy.moved&&p[e][MINER]>0;
        if((raider&&home[e]<=8)||(miner&&home[e]<=6))if(home[e]<threat)threat=home[e];
    }
    if(threat==100||home[m.from]<=3)return 0;
    if(d.side>=0&&d.revealed&&combat_result(a.rank,d.rank)>=0&&home[m.to]<=6)return 0;
    Game next=optimistic_move(view,m);
    if(known_unanswered_loss_at(&next,side,m.to)>0)return 0;
    float urgency=fminf(48,4.0f*fmaxf(2,home[m.from]-threat+2));
    return urgency*(home[m.from]-home[m.to]);
}
