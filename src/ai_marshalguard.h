/* Reconstruct where a suspect stood when the marshal previously withdrew.
   Rewind the enemy move made AFTER each own move before comparing positions.
   A new contact must not erase the older contact we deliberately avoided. */
static bool marshal_avoided(const Game *v,int marshal_id,int suspect){
    int side=v->turn,id=v->board[suspect].id,position=suspect;
    if(v->board[suspect].revealed||v->board[suspect].side!=1-side)return false;
    if(id>=0&&id<80&&!v->board[suspect].revealed&&
       (v->marshal_suspects[side][id/64]&(UINT64_C(1)<<(id%64))))return true;
    int count=v->history_count[side];if(count>8)count=8;
    if(count>v->history_count[1-side])count=v->history_count[1-side];
    for(int age=0;age<count;age++){
        int enemy=(v->history_count[1-side]-1-age)%8;
        if(v->history_id[1-side][enemy]==id)position=v->history[1-side][enemy].from;
        int own=(v->history_count[side]-1-age)%8;Move m=v->history[side][own];
        if(v->history_id[side][own]!=marshal_id)continue;
        int before=abs(m.from%10-position%10)+abs(m.from/10-position/10);
        int after=abs(m.to%10-position%10)+abs(m.to/10-position/10);
        if(before==1&&after>1)return true;
    }
    return false;
}
static bool marshal_returns_to_suspect(const Game *v,float p[100][12],Move m){
    int square=-1;Piece a=empty_piece();
    for(int s=0;s<100;s++)if(v->board[s].side==v->turn&&v->board[s].rank==MARSHAL){square=s;a=v->board[s];break;}
    if(square<0||!a.revealed)return false;
    if(m.from==square){
        if(v->board[m.to].side==1-a.side&&p[m.to][FLAG]>0)return false;
        square=m.to;
    }
    int nb[4],nn=neighbors(square,nb);
    for(int j=0;j<nn;j++){
        int s=nb[j];Piece suspect=v->board[s];
        if(s==m.from||suspect.side!=1-a.side||suspect.revealed||p[s][SPY]<=0)continue;
        if(s==m.to){
            float success=0;for(int rank=0;rank<12;rank++)if(combat_result(v->board[m.from].rank,rank)>0)success+=p[s][rank];
            if(success>.999f)continue;
        }
        if(marshal_avoided(v,a.id,s))return true;
    }
    return false;
}
static bool marshal_known_spy_exposure(const Game *v,Move m){
    Game next=optimistic_move(v,m);
    for(int s=0;s<100;s++)if(next.board[s].side==v->turn&&next.board[s].rank==MARSHAL)
        for(int e=0;e<100;e++)if(next.board[e].side==1-v->turn&&next.board[e].revealed&&next.board[e].rank==SPY&&
            public_legal(&next,(Move){e,s},1-v->turn))return true;
    return false;
}
/* A revealed spy threatening a strategic piece is a concrete tactical task,
   even in a full army. Use the cheapest certainly winning, safe recapture. */
static Move dangerous_spy_capture(const Game *v,float p[100][12],Move *moves,int n){
    Move best={-1,-1};float cheapest=1e9f;
    for(int e=0;e<100;e++){
        Piece spy=v->board[e];if(spy.side!=1-v->turn||!spy.revealed||spy.rank!=SPY)continue;
        bool urgent=false;
        for(int s=0;s<100;s++){
            Piece own=v->board[s];
            bool key=own.rank==MARSHAL||(own.rank==SPY&&v->captured[1-v->turn][MARSHAL]<army_counts[MARSHAL]);
            if(own.side==v->turn&&key&&public_legal(v,(Move){e,s},spy.side))urgent=true;
        }
        if(!urgent)continue;
        for(int i=0;i<n;i++){
            Move m=moves[i];Piece a=v->board[m.from];
            if(m.to!=e||combat_result(a.rank,SPY)<=0||known_unanswered_loss(v,m)>0)continue;
            Game next=optimistic_move(v,m);bool safe=true;
            /* Check possible counterattacks, not just revealed enemies. */
            for(int s=0;s<100;s++)if(s!=e&&next.board[s].side==1-v->turn){
                for(int rank=SPY;rank<=MARSHAL;rank++)if(p[s][rank]>0&&combat_result(rank,a.rank)>=0){
                    Game reply=next;reply.board[s].rank=rank;
                    if(public_legal(&reply,(Move){s,e},1-v->turn))safe=false;
                }
            }
            if(safe&&ai_flag_risk(&next,v->turn)<=ai_flag_risk(v,v->turn)&&worth[a.rank]<cheapest){
                cheapest=worth[a.rank];best=m;
            }
        }
    }
    return best;
}
