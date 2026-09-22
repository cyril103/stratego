/* A local, public flag race (two enemy moves, three in small endings).
   Test every legal friendly reply to an
   intruder's first step, rather than assuming that chasing it will stop it.
   Unknown intruders are evaluated by public rank probabilities, never by
   their stored identity. Other enemy moves are omitted, so this is a local
   danger estimate, not a complete game-theoretic proof. */
static bool flag_race_forced(const Game *position,int intruder,int flag,int side,int depth,int *budget){
    if((*budget)--<=0)return false;
    Piece enemy=position->board[intruder];
    if(enemy.side!=1-side||!movable(enemy))return false;
    if(public_legal(position,(Move){intruder,flag},enemy.side))return true;
    if(depth<=1)return false;
    for(int target=0;target<100;target++){
        int distance=abs(target/10-flag/10)+abs(target%10-flag%10);
        if(enemy.rank!=SCOUT&&distance>depth-1)continue;
        Move attack={intruder,target};
        if(!public_legal(position,attack,enemy.side))continue;
        Game entered=optimistic_move(position,attack);
        if(entered.board[target].side!=enemy.side)continue;
        if(depth==2&&!public_legal(&entered,(Move){target,flag},enemy.side))continue;
        bool defended=false;Move replies[MAX_MOVES];int n=game_moves(&entered,side,replies);
        for(int i=0;i<n;i++){
            Move reply=replies[i];
            if(!public_legal(&entered,reply,side))continue;
            Piece victim=entered.board[reply.to];
            /* A possible immediate victory must still be evaluated by the
               main search, not vetoed by a local defensive model. */
            if(victim.side==enemy.side&&(victim.rank==FLAG||(!victim.revealed&&!victim.moved))){defended=true;break;}
            Game response=optimistic_move(&entered,reply);
            if(response.board[target].side!=enemy.side||
               !flag_race_forced(&response,target,flag,side,depth-1,budget)){defended=true;break;}
        }
        if(!defended)return true;
    }
    return false;
}
static float flag_race_cost(const Game *view,float p[100][12],Move m){
    int side=view->turn,flag=-1;
    for(int s=0;s<100;s++)if(view->board[s].side==side&&view->board[s].rank==FLAG)flag=s;
    if(flag<0)return 0;
    Piece victim=view->board[m.to];
    if(victim.side==1-side&&(victim.rank==FLAG||(!victim.revealed&&!victim.moved)))return 0;
    Game next=optimistic_move(view,m);float worst=0;int mobile=0;
    /* Our own ranks are certain even when concealed from the opponent. */
    for(int s=0;s<100;s++)if(next.board[s].side==side){next.board[s].revealed=true;mobile+=movable(next.board[s]);}
    int depth=mobile<=6?3:2;
    for(int s=0;s<100;s++){
        Piece enemy=next.board[s];
        if(enemy.side!=1-side||(!enemy.moved&&!enemy.revealed))continue;
        int distance=abs(s%10-flag%10)+abs(s/10-flag/10);float risk=0;
        for(int r=SPY;r<=MARSHAL;r++){
            if(p[s][r]<=0||(distance>depth&&r!=SCOUT))continue;
            next.board[s].rank=r;next.board[s].revealed=true;
            int budget=400;
            if(flag_race_forced(&next,s,flag,side,depth,&budget))risk+=p[s][r];
        }
        next.board[s]=enemy;
        worst=fmaxf(worst,risk);
    }
    return 600*worst;
}
