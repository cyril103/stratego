#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Defense roles line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static Game changed_hidden(Game g){
    for(int s=0;s<100;s++)for(int t=s+1;t<100;t++){
        Piece a=g.board[s],b=g.board[t];
        if(a.side==HUMAN&&b.side==HUMAN&&!a.revealed&&!b.revealed&&a.moved==b.moved&&a.rank!=b.rank){
            g.board[s].rank=b.rank;g.board[t].rank=a.rank;return g;
        }
    }
    return g;
}
int main(void){
    Game g,v;int remaining[12];float p[100][12];
    CHECK(replay_load(STRATEGO_ROLES_FIXTURE,0,&g));CHECK(g.winner==HUMAN&&g.ply==625);
    CHECK(replay_load(STRATEGO_ROLES_FIXTURE,610,&g));public_board(&g,&v,remaining);probabilities(&v,remaining,p);
    InterceptPlan plan,disclosed;intercept_plan(&v,p,&plan);CHECK(plan.count>0);
    Game own_known=v;for(int s=0;s<100;s++)if(own_known.board[s].side==COMPUTER)own_known.board[s].revealed=true;
    intercept_plan(&own_known,p,&disclosed);CHECK(plan.count==disclosed.count);
    for(int i=0;i<plan.count;i++)CHECK(plan.threats[i].arrival==disclosed.threats[i].arrival&&plan.threats[i].enemy==disclosed.threats[i].enemy);
    int miner[100],officer[100];intercept_distances(&v,0,HUMAN,MINER,miner);intercept_distances(&v,0,HUMAN,MAJOR,officer);
    CHECK(miner[51]<100&&officer[51]==100);
    int positions[]={350,400,526,600};uint32_t seeds[]={1,2,3,519};
    for(int k=0;k<4;k++){
        CHECK(replay_load(STRATEGO_ROLES_FIXTURE,positions[k],&g));Game changed=changed_hidden(g);
        public_board(&g,&v,remaining);probabilities(&v,remaining,p);
        if(k==0)CHECK(known_unanswered_loss(&v,(Move){25,24})>known_unanswered_loss(&v,(Move){5,4}));
        if(k==2)CHECK(counter_capture_cost(&v,p,(Move){23,24})>counter_capture_cost(&v,p,(Move){23,33}));
        for(int i=0;i<4;i++){
            uint32_t a=seeds[i],b=a;Move m=ai_choose(&g,1,&a),same=ai_choose(&changed,1,&b);
            printf("Roles before %d seed %u: %d -> %d\n",positions[k],seeds[i],m.from,m.to);fflush(stdout);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==same.from&&m.to==same.to&&a==b);
            if(k==0){Game next=optimistic_move(&v,m);CHECK(m.from==5&&known_unanswered_loss_at(&next,COMPUTER,m.to)==0);}
            if(k==1)CHECK(m.from==21);
            if(k==2)CHECK(m.from==23&&m.to==33);
            if(k==3)CHECK(reserve_home_bonus(&v,p,m)>0);
        }
    }
    for(int seed=0;seed<2;seed++){
        CHECK(replay_load(STRATEGO_ROLES_FIXTURE,600,&g));uint32_t rng=seed?519:1;
        FILE *file=fopen(STRATEGO_ROLES_FIXTURE,"r");CHECK(file);char line[8192];
        while(fgets(line,sizeof(line),file)){
            int ply,side,from,to;
            if(sscanf(line,"{\"ply\":%d,\"side\":%d,\"from\":%d,\"to\":%d",&ply,&side,&from,&to)!=4||ply<600)continue;
            Move m=side==COMPUTER?ai_choose(&g,1,&rng):(Move){from,to};
            if(side==HUMAN&&!game_legal(&g,m,side))break;
            CHECK(game_apply(&g,m));CHECK(g.winner!=HUMAN);
            if(g.winner>=0)break;
        }
        fclose(file);CHECK(g.board[0].side==COMPUTER&&g.board[0].rank==FLAG);
        printf("Recorded final attack stopped, seed %d, ply %d\n",seed?519:1,g.ply);fflush(stdout);
    }
    puts("Public bomb routes, endangered roles and reserve defense OK");return 0;
}
