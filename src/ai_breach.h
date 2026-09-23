/* Exploit a publicly observed bomb opening even after its miner dies.
   The candidate flag remains a hypothesis; budget the entire attacker's loss. */
typedef struct {Move step;int goal;float bonus;} BreachPlan;
static BreachPlan breach_plan(const Game *v,float p[100][12]){
    BreachPlan plan={{-1,-1},-1,0};int side=v->turn,enemy=1-side;
    if(ai_flag_risk(v,side)>0)return plan;
    float material[2]={0,0};int strongest=SPY;
    for(int s=0;s<2;s++)for(int r=SPY;r<=MARSHAL;r++){
        int count=army_counts[r]-v->captured[s][r];
        material[s]+=count*worth[r];if(s==enemy&&count>0)strongest=r;
    }
    float best=0;
    float focus=ai_flag_focus_threshold(v,p,1.25f);
    for(int goal=0;goal<100;goal++){
        Piece target=v->board[goal];
        if(target.side!=enemy||target.revealed||target.moved||p[goal][FLAG]<focus)continue;
        int back=enemy==COMPUTER?goal/10:9-goal/10;
        if(back>1)continue;
        int gates[4],ng=neighbors(goal,gates);bool breached=false;
        for(int j=0;j<ng;j++)if(v->cleared_bombs[enemy][gates[j]])breached=true;
        if(!breached)continue;
        for(int from=0;from<100;from++){
            Piece a=v->board[from];if(a.side!=side||!movable(a))continue;
            /* A failed probe must leave both a material edge and a dominant
               mobile reserve. No last-officer desperation disguised as attack. */
            if(material[side]-worth[a.rank]<material[enemy]+worth[CAPTAIN])continue;
            bool reserve=false;
            for(int s=0;s<100;s++)if(s!=from&&v->board[s].side==side&&movable(v->board[s])&&v->board[s].rank>strongest)reserve=true;
            if(!reserve)continue;
            int distance[100],queue[100],head=0,tail=0;
            for(int s=0;s<100;s++)distance[s]=100;
            distance[goal]=0;queue[tail++]=goal;
            while(head<tail){
                int s=queue[head++];if(distance[s]>=2)continue;
                int nb[4],nn=neighbors(s,nb);
                for(int j=0;j<nn;j++){int t=nb[j];
                    if(is_lake(t)||distance[t]<100||(t!=from&&v->board[t].side>=0))continue;
                    distance[t]=distance[s]+1;queue[tail++]=t;
                }
            }
            if(distance[from]>2)continue;
            int nb[4],nn=neighbors(from,nb);
            for(int j=0;j<nn;j++){
                Move m={from,nb[j]};if(distance[m.to]!=distance[from]-1||!public_legal(v,m,side)||immediate_defeat(v,m))continue;
                Game next=optimistic_move(v,m);
                if(ai_flag_risk(&next,side)>0||known_unanswered_loss_at(&next,side,m.to)>0||spy_exposure(v,p,m,true)>spy_exposure(v,p,m,false))continue;
                /* Check the failure branch as well: sacrificing this piece
                   must not uncover our flag. */
                next=*v;next.board[from]=empty_piece();
                if(ai_flag_risk(&next,side)>0)continue;
                float score=120+160*p[goal][FLAG]-18*distance[from]-.4f*worth[a.rank];
                if(score>best){best=score;plan=(BreachPlan){m,goal,score};}
            }
        }
    }
    return plan;
}
static bool breach_step(const BreachPlan *plan,Move m){return plan->step.from==m.from&&plan->step.to==m.to;}
