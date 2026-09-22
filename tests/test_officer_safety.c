#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Officer safety line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g,view;int remaining[12];float p[100][12];
    CHECK(replay_load(STRATEGO_BLOCKED_FIXTURE,0,&g));CHECK(g.ply==429&&g.winner==HUMAN);
    for(int position=0;position<2;position++){
        int ply=position?420:322;CHECK(replay_load(STRATEGO_BLOCKED_FIXTURE,ply,&g));
        public_board(&g,&view,remaining);probabilities(&view,remaining,p);
        Game changed=g;int first=-1,second=-1;
        for(int s=0;s<100&&second<0;s++)if(g.board[s].side==HUMAN&&!g.board[s].revealed)
            for(int t=s+1;t<100;t++)if(g.board[t].side==HUMAN&&!g.board[t].revealed&&
                g.board[s].moved==g.board[t].moved&&g.board[s].rank!=g.board[t].rank){first=s;second=t;break;}
        CHECK(second>=0);int rank=changed.board[first].rank;changed.board[first].rank=changed.board[second].rank;changed.board[second].rank=rank;
        if(!position){
            CHECK(supported_capture_cost(&view,p,(Move){58,68})>0);
            CHECK(officer_clearance(&view,p,(Move){48,38})>0);
            Game open=optimistic_move(&view,(Move){48,38});
            CHECK(officer_exits(&open,p,58)>0);
            CHECK(officer_clearance(&open,p,(Move){59,49})==0);
        }else CHECK(last_guard_trade(&view,p,(Move){7,6})>0);
        for(uint32_t seed=1;seed<=8;seed++){
            uint32_t rng=seed;Move m=ai_choose(&g,1,&rng);
            uint32_t other_rng=seed;Move same=ai_choose(&changed,1,&other_rng);
            CHECK(m.from==same.from&&m.to==same.to&&rng==other_rng);
            CHECK(game_legal(&g,m,g.turn));
            CHECK(position?!(m.from==7&&m.to==6):officer_clearance(&view,p,m)>0);
            printf("Blocked officer / guard before %d seed %u: %d -> %d\n",ply,seed,m.from,m.to);fflush(stdout);
        }
    }
    CHECK(replay_load(STRATEGO_APPROACH_FIXTURE,0,&g));
    CHECK(g.ply==293&&g.winner==HUMAN&&g.end_reason==END_FLAG);
    CHECK(replay_load(STRATEGO_PATIENT_FIXTURE,0,&g));CHECK(g.ply==369&&g.winner==HUMAN);
    for(int position=0;position<2;position++){
        int ply=position?176:48;CHECK(replay_load(STRATEGO_PATIENT_FIXTURE,ply,&g));
        Game changed=g;int suspect=position?31:61,swap=-1;
        for(int s=0;s<100;s++)if(s!=suspect&&g.board[s].side==HUMAN&&!g.board[s].revealed&&
            g.board[s].moved==g.board[suspect].moved&&g.board[s].rank!=g.board[suspect].rank){swap=s;break;}
        CHECK(swap>=0);int rank=changed.board[swap].rank;changed.board[swap].rank=changed.board[suspect].rank;changed.board[suspect].rank=rank;
        for(uint32_t seed=1;seed<=8;seed++){
            uint32_t a=seed,b=seed;Move m=ai_choose(&g,1,&a),same=ai_choose(&changed,1,&b);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==same.from&&m.to==same.to&&a==b);
            CHECK(position?!(m.from==33&&m.to==32):!(m.from==50&&m.to==51));
            printf("Patient threat before %d seed %u: %d -> %d\n",ply,seed,m.from,m.to);fflush(stdout);
        }
    }
    /* In the recorded quiet repositioning, retain the scout's disguise by
       taking one step instead of disclosing it with D9-B9. */
    CHECK(replay_load(STRATEGO_APPROACH_FIXTURE,210,&g));
    CHECK(g.board[13].rank==SCOUT&&!g.board[13].revealed);
    {
        uint32_t rng=519;Move m=ai_choose(&g,1,&rng);
        CHECK(game_legal(&g,m,g.turn));
        CHECK(m.from==13&&m.to==12);
        printf("Scout concealment seed 519: %d -> %d\n",m.from,m.to);
    }
    for(int position=0;position<2;position++){
        int ply=position?44:38;
        CHECK(replay_load(STRATEGO_APPROACH_FIXTURE,ply,&g));
        Game other=g;int suspect=position?61:65,swap=-1;
        for(int s=0;s<100;s++)if(s!=suspect&&g.board[s].side==HUMAN&&!g.board[s].revealed&&
            g.board[s].moved==g.board[suspect].moved&&g.board[s].rank!=g.board[suspect].rank){swap=s;break;}
        CHECK(swap>=0);
        int rank=other.board[swap].rank;other.board[swap].rank=other.board[suspect].rank;other.board[suspect].rank=rank;
        for(uint32_t seed=1;seed<=8;seed++){
            uint32_t a=seed,b=seed;Move m=ai_choose(&g,1,&a),n=ai_choose(&other,1,&b);
            CHECK(game_legal(&g,m,g.turn));CHECK(m.from==n.from&&m.to==n.to&&a==b);
            if(position)CHECK(m.from==60&&m.to==50);
            else CHECK(!(m.from==45&&m.to==55));
            printf("Before %d seed %u: %d -> %d\n",ply,seed,m.from,m.to);
        }
    }
    /* Independent geometry: an approaching hidden spy threatens a revealed
       marshal; a dead spy or a known scout must not cause the same alarm. */
    game_clear(&g);g.turn=COMPUTER;
    g.board[25]=(Piece){MARSHAL,COMPUTER,40,true,true};
    g.board[26]=(Piece){SPY,HUMAN,0,false,true};g.last_move=(Move){27,26};
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    CHECK(approaching_officer_risk(&view,p)>0);
    float persistent=approaching_officer_risk(&view,p);
    view.last_move=(Move){80,81};view.ply+=20;
    CHECK(approaching_officer_risk(&view,p)==persistent);
    Game mirrored=view;for(int s=0;s<100;s++)mirrored.board[99-s]=view.board[s];
    mirrored.last_move=(Move){72,73};float reflected[100][12];
    for(int s=0;s<100;s++)memcpy(reflected[99-s],p[s],sizeof(p[s]));
    CHECK(approaching_officer_risk(&mirrored,reflected)==approaching_officer_risk(&view,p));
    g.captured[HUMAN][SPY]=1;public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    CHECK(approaching_officer_risk(&view,p)==0);
    g.captured[HUMAN][SPY]=0;g.board[26].rank=SCOUT;g.board[26].revealed=true;
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    CHECK(approaching_officer_risk(&view,p)==0);
    g.board[26].revealed=false;g.last_move=(Move){16,26};
    public_board(&g,&view,remaining);probabilities(&view,remaining,p);
    float exposed_risk=approaching_officer_risk(&view,p);
    view.board[25].revealed=false;
    CHECK(approaching_officer_risk(&view,p)>0&&approaching_officer_risk(&view,p)<exposed_risk);
    puts("Hidden approaches: rescue, spy trap, symmetry and information isolation OK");return 0;
}
