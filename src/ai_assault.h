/* One public-information miner/escort mission. Recomputed at the root so
   captures or a defensive emergency can cancel it without stale state. */
typedef struct {int miner,escort,goal;float distance[100];} AssaultPlan;
static float assault_bonus(const Game *view,float p[100][12],const AssaultPlan *plan,Move m);
/* Give an already progressing miner a small head start when assigning the
   next escort. Reconstruct intent from actual moves so replay/undo/restart
   stays deterministic. A better route or an emergency still cancels it. */
static float assault_commitment(const Game *v,int miner,const float distance[100]){
    int side=v->turn,count=v->history_count[side];if(count>8)count=8;
    for(int age=0;age<count;age++){
        int h=(v->history_count[side]-1-age)%8;
        if(v->history_id[side][h]!=v->board[miner].id)continue;
        Move past=v->history[side][h];
        if(past.to!=miner||distance[past.from]>=1000||distance[miner]>=distance[past.from])return 0;
        return 2.0f*(8-age)/8;
    }
    return 0;
}
static void assault_plan(const Game *view,float p[100][12],AssaultPlan *plan){
    plan->miner=plan->escort=plan->goal=-1;
    if(ai_flag_risk(view,view->turn)>0)return;
    float probable=0;
    for(int s=0;s<100;s++)if(p[s][FLAG]>probable){probable=p[s][FLAG];plan->goal=s;}
    if(plan->goal<0)return;
    bool done[100]={false};for(int s=0;s<100;s++)plan->distance[s]=1000;
    plan->distance[plan->goal]=0;
    for(int step=0;step<100;step++){
        int s=-1;for(int t=0;t<100;t++)if(!done[t]&&(s<0||plan->distance[t]<plan->distance[s]))s=t;
        if(s<0||plan->distance[s]>=1000)break;
        done[s]=true;
        Piece blocker=view->board[s];float cost=1;
        if(s!=plan->goal&&blocker.side==view->turn){
            if(!movable(blocker))continue;
            cost=3;
        }else if(blocker.side==1-view->turn){
            float danger=0;for(int r=SPY;r<=MARSHAL;r++)if(combat_result(MINER,r)<=0)danger+=p[s][r];
            cost+=3*danger;
        }
        int nb[4],n=neighbors(s,nb);
        for(int k=0;k<n;k++)if(!is_lake(nb[k]))plan->distance[nb[k]]=fminf(plan->distance[nb[k]],plan->distance[s]+cost);
    }
    float best=1e9f;
    float material[2]={0,0};int mobile[2]={0,0};
    for(int side=0;side<2;side++)for(int r=SPY;r<=MARSHAL;r++){
        int alive=army_counts[r]-view->captured[side][r];
        material[side]+=alive*worth[r];mobile[side]+=alive;
    }
    for(int e=0;e<100;e++){
        Piece officer=view->board[e];if(officer.side!=view->turn||officer.rank<CAPTAIN||officer.rank>MARSHAL)continue;
        bool dominant=true;
        for(int r=officer.rank+1;r<=MARSHAL;r++)if(army_counts[r]>view->captured[1-view->turn][r])dominant=false;
        /* An equal opposing officer need not freeze an otherwise winning
           army. Public casualties must establish a substantial mobile
           advantage; bombs are not counted as offensive material. */
        if(army_counts[officer.rank]>view->captured[1-view->turn][officer.rank]&&
           (material[view->turn]-material[1-view->turn]<worth[officer.rank]||
            mobile[view->turn]<mobile[1-view->turn]+2))dominant=false;
        if(!dominant)continue;
        bool recalled=false;int exits[4],count=neighbors(e,exits);
        for(int k=0;k<count;k++){
            Move m={e,exits[k]};
            if(public_legal(view,m,view->turn)&&recall_officer(view,m)>0)recalled=true;
        }
        if(recalled)continue;
        for(int m=0;m<100;m++)if(view->board[m].side==view->turn&&view->board[m].rank==MINER&&plan->distance[m]<1000){
            int join[100];intercept_distances(view,m,view->turn,officer.rank,join);
            if(join[e]>=100)continue;
            float score=plan->distance[m]+.7f*join[e]-assault_commitment(view,m,plan->distance);
            if(score<best){best=score;plan->miner=m;plan->escort=e;}
        }
    }
    /* A mission must offer a legal constructive first step. Otherwise its
       separation penalties can freeze BOTH partners (including when a
       two-square restriction temporarily prevents the escort from joining).
       Let ordinary play unblock the position, then reconsider next turn. */
    if(plan->miner>=0){
        Move moves[MAX_MOVES];int n=game_moves(view,view->turn,moves);bool useful=false;
        for(int i=0;i<n;i++)if(public_legal(view,moves[i],view->turn)&&assault_bonus(view,p,plan,moves[i])>0){useful=true;break;}
        if(!useful)plan->miner=plan->escort=-1;
    }
}
static int assault_miner_steps(const Game *view,float p[100][12],const AssaultPlan *plan){
    int nb[4],n=neighbors(plan->miner,nb),usable=0,escort_distance[100];
    land_distances(plan->escort,escort_distance);
    for(int k=0;k<n;k++){
        Move m={plan->miner,nb[k]};
        if(plan->distance[m.to]>=plan->distance[m.from]||escort_distance[m.to]>2||
           !public_legal(view,m,view->turn)||expected_threat(view,p,m,true)>.5f)continue;
        if(view->board[m.to].side>=0){
            float failure=0;for(int r=0;r<12;r++)if(combat_result(MINER,r)<=0)failure+=p[m.to][r];
            if(failure>.001f)continue;
        }
        usable++;
    }
    return usable;
}
static float assault_bonus(const Game *view,float p[100][12],const AssaultPlan *plan,Move m){
    if(plan->miner<0)return 0;
    bool partner=m.from==plan->miner||m.from==plan->escort;
    if(!partner&&abs(m.from%10-plan->miner%10)+abs(m.from/10-plan->miner/10)!=1)return 0;
    Piece a=view->board[m.from],target=view->board[m.to];
    if(target.side>=0){
        float failure=0;for(int r=0;r<12;r++)if(combat_result(a.rank,r)<=0)failure+=p[m.to][r];
        if(failure>.001f)return 0; /* A mission is not permission for a blind sacrifice. */
    }
    Game next=optimistic_move(view,m);
    if(ai_flag_risk(&next,view->turn)>0)return 0;
    if(!partner){
        if(expected_threat(view,p,m,true)>worth[a.rank]*.1f)return 0;
        return !assault_miner_steps(view,p,plan)&&assault_miner_steps(&next,p,plan)?16:0;
    }
    int miner=m.from==plan->miner?m.to:plan->miner,escort=m.from==plan->escort?m.to:plan->escort;
    int before[100],after[100];land_distances(plan->miner,before);land_distances(miner,after);
    int separation=after[escort];
    /* Recapturing the attacker would still lose the miner. Clear a dangerous
       approach with the officer before paying for the miner's advance. */
    if(m.from==plan->miner&&expected_threat(view,p,m,true)>.5f)return 0;
    if(m.from==plan->escort&&(expected_threat(view,p,m,true)>worth[a.rank]*.1f||spy_exposure(view,p,m,true)>0))return 0;
    float progress=plan->distance[plan->miner]-plan->distance[miner];
    if(m.from==plan->miner&&progress>0&&separation>2)return -fminf(24,8*(separation-2));
    float regroup=fmaxf(0,before[plan->escort]-1)-fmaxf(0,separation-1);
    /* Clear ahead when the miner cannot make safe progress. When it can,
       keep the escort nearby rather than sending it off on a separate hunt. */
    bool blocked=m.from==plan->escort&&!assault_miner_steps(view,p,plan);
    float clearance=m.from==plan->escort&&blocked&&separation<=4?
        fminf(fmaxf(0,-regroup),fmaxf(0,plan->distance[m.from]-plan->distance[m.to])):0;
    return fmaxf(-30,fminf(30,12*progress+8*regroup+8*clearance));
}
