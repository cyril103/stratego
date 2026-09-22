/* Exercise the search internals without exporting them to the game API. */
#include "../src/ai.c"
#include "replay.h"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Search line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void put(Game *g,int s,int rank,int side){g->board[s]=(Piece){rank,side,s,true,true};}
int main(void){
    /* Scout secrecy is lost only on a NEW quiet long move. Publicly known
       scouts and actual attacks keep their full long-range utility. */
    Game secrecy;game_clear(&secrecy);
    put(&secrecy,60,SCOUT,HUMAN);secrecy.board[60].revealed=false;
    CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})>0);
    CHECK(scout_disclosure_cost(&secrecy,(Move){60,50})==0);
    secrecy.board[60].moved=true;CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})>0);
    secrecy.board[60].revealed=true;CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})==0);
    secrecy.board[60].revealed=false;put(&secrecy,40,SPY,COMPUTER);
    CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})==0);
    secrecy.board[40].revealed=false;CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})==0);
    secrecy.board[40]=empty_piece();
    for(int r=SPY;r<=MARSHAL;r++)if(r!=SCOUT)secrecy.captured[HUMAN][r]=army_counts[r];
    CHECK(scout_disclosure_cost(&secrecy,(Move){60,40})==0); /* Nothing left to bluff. */
    /* A forced long-range flag capture must still take priority over secrecy. */
    game_clear(&secrecy);put(&secrecy,99,FLAG,HUMAN);put(&secrecy,0,FLAG,COMPUTER);
    put(&secrecy,60,SCOUT,HUMAN);secrecy.board[60].revealed=false;
    put(&secrecy,9,SERGEANT,COMPUTER);
    uint32_t scout_rng=519;Move scout_win=ai_choose(&secrecy,1,&scout_rng);
    CHECK(scout_win.from==60&&scout_win.to==0);
    CHECK(game_apply(&secrecy,scout_win)&&secrecy.winner==HUMAN);
    /* A quiet long move remains mandatory when it is the only way to block
       the opponent's scout ray onto our flag. */
    game_clear(&secrecy);put(&secrecy,90,FLAG,HUMAN);put(&secrecy,0,FLAG,COMPUTER);
    put(&secrecy,69,SCOUT,HUMAN);secrecy.board[69].revealed=false;
    put(&secrecy,99,SPY,HUMAN);secrecy.board[99].revealed=false;
    put(&secrecy,89,BOMB,HUMAN);put(&secrecy,98,BOMB,HUMAN);
    put(&secrecy,30,SCOUT,COMPUTER);
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)secrecy.captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(secrecy.board[s].side>=0)secrecy.captured[secrecy.board[s].side][secrecy.board[s].rank]--;
    CHECK(scout_disclosure_cost(&secrecy,(Move){69,60})>0);
    scout_rng=519;Move scout_save=ai_choose(&secrecy,1,&scout_rng);
    CHECK(scout_save.from==69&&scout_save.to==60);
    Game g;game_clear(&g);g.turn=HUMAN;
    put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);
    put(&g,64,GENERAL,HUMAN);put(&g,65,CAPTAIN,HUMAN);put(&g,80,MINER,HUMAN);
    put(&g,54,LIEUTENANT,COMPUTER);put(&g,44,MARSHAL,COMPUTER);
    SearchEntry *table=calloc(TT_SIZE,sizeof(SearchEntry));CHECK(table!=NULL);
    SearchContext plain={.budget=200000},cached={.table=table,.budget=200000};
    float a=search(&g,3,HUMAN,-1e20f,1e20f,&plain,true);
    float b=search(&g,3,HUMAN,-1e20f,1e20f,&cached,true);
    CHECK(!plain.aborted&&!cached.aborted&&fabsf(a-b)<.001f);
    int before=cached.budget;
    b=search(&g,3,HUMAN,-1e20f,1e20f,&cached,true);
    CHECK(fabsf(a-b)<.001f&&cached.hits>0&&before-cached.budget==1);
    /* A cutoff stores a bound; it must not become an exact cached value. */
    memset(table,0,TT_SIZE*sizeof(SearchEntry));cached=(SearchContext){.table=table,.budget=200000};
    search(&g,3,HUMAN,a-2,a-1,&cached,true);CHECK(!cached.aborted);
    b=search(&g,3,HUMAN,-1e20f,1e20f,&cached,true);CHECK(!cached.aborted&&fabsf(a-b)<.001f);
    uint64_t key=search_key(&g);Game other=g;
    other.repetitions[0]=3;CHECK(search_key(&other)!=key);
    other=g;other.board[64].revealed=false;CHECK(search_key(&other)!=key);
    other=g;other.history[0][0]=(Move){64,65};CHECK(search_key(&other)!=key);
    other=g;other.captured[1][MINER]++;CHECK(search_key(&other)!=key);
    /* Aborting a deeper iteration retains exactly the last completed result. */
    memset(table,0,TT_SIZE*sizeof(SearchEntry));cached=(SearchContext){.table=table,.budget=200000};
    float shallow=search(&g,1,HUMAN,-1e20f,1e20f,&cached,true);CHECK(!cached.aborted);
    int used=200000-cached.budget;
    memset(table,0,TT_SIZE*sizeof(SearchEntry));cached=(SearchContext){.table=table,.budget=used+1};
    float completed=deepen_search(&g,5,HUMAN,&cached,true,NULL);
    CHECK(cached.aborted&&cached.completed==1&&fabsf(completed-shallow)<.001f);
    SearchEntry entry=table[key&(TT_SIZE-1)];
    CHECK(!entry.valid||entry.key!=key||entry.depth==1);
    /* Faster branches cannot win merely by being evaluated at a different
       horizon from their competitors in the same sampled world. */
    RootSearch work={.depth=6,.flag_known=true};
    for(int j=0;j<SAMPLES;j++){
        work.completed[0][j]=2;work.completed[1][j]=4;
        work.levels[0][j][2]=10;work.levels[1][j][2]=11;
        work.levels[1][j][4]=-1000;
    }
    finish_search(&work,2);
    CHECK(fabsf(work.result[0]-10)<.001f&&fabsf(work.result[1]-11)<.001f);
    g.winner=GAME_DRAW;g.end_reason=END_BOTH_IMMOBILE;
    CHECK(evaluate(&g,HUMAN,true)==0&&evaluate(&g,COMPUTER,false)==0);
    uint32_t rng=1;CHECK(ai_choose(&g,1,&rng).from==-1);
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,60,SPY,HUMAN);put(&g,30,MARSHAL,COMPUTER);
    CHECK(spy_hunt(&g,(Move){60,50})>0);
    CHECK(spy_hunt(&g,(Move){60,61})==0);
    g.board[30].revealed=false;CHECK(spy_hunt(&g,(Move){60,50})==0);g.board[30].revealed=true;
    put(&g,51,SERGEANT,COMPUTER);CHECK(spy_hunt(&g,(Move){60,50})==0);
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,60,GENERAL,HUMAN);put(&g,50,MARSHAL,COMPUTER);put(&g,90,SCOUT,HUMAN);
    CHECK(known_unanswered_loss(&g,(Move){90,80})>=worth[GENERAL]*4);
    put(&g,61,SPY,HUMAN); /* Can recapture the marshal if it takes the general. */
    CHECK(known_unanswered_loss(&g,(Move){90,80})==0);
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,90,SPY,HUMAN);put(&g,40,MARSHAL,COMPUTER);
    put(&g,80,LIEUTENANT,HUMAN);put(&g,91,BOMB,HUMAN);
    CHECK(spy_hunt(&g,(Move){80,81})>0); /* Clear the spy's only exit. */
    game_clear(&g);put(&g,99,FLAG,HUMAN);put(&g,0,FLAG,COMPUTER);
    put(&g,70,MAJOR,HUMAN);put(&g,50,MINER,COMPUTER);
    for(int r=1;r<=MARSHAL;r++)g.captured[COMPUTER][r]=army_counts[r];
    g.captured[COMPUTER][MINER]--;
    CHECK(dominant_hunt(&g,(Move){70,60})>0);
    CHECK(dominant_hunt(&g,(Move){70,80})<0);
    g.captured[COMPUTER][GENERAL]--;CHECK(dominant_hunt(&g,(Move){70,60})==0);
    g.captured[COMPUTER][GENERAL]++;g.board[50].revealed=false;g.board[50].moved=false;
    CHECK(dominant_hunt(&g,(Move){70,60})==0);
    g.board[70]=empty_piece();put(&g,60,MAJOR,HUMAN);g.board[50].revealed=true;g.board[50].moved=true;
    Move endings[MAX_MOVES];int ending_count=game_moves(&g,HUMAN,endings);
    Move finish=cleanup_capture(&g,endings,ending_count);CHECK(finish.from==60&&finish.to==50);
    put(&g,98,SPY,COMPUTER);g.captured[COMPUTER][SPY]--;
    CHECK(cleanup_capture(&g,endings,ending_count).from<0); /* Flag emergency overrides liquidation. */
    game_clear(&g);put(&g,5,FLAG,HUMAN);put(&g,99,FLAG,COMPUTER);
    put(&g,4,BOMB,HUMAN);put(&g,6,BOMB,HUMAN);put(&g,15,BOMB,HUMAN);
    put(&g,19,GENERAL,HUMAN);put(&g,18,MINER,COMPUTER);put(&g,8,MARSHAL,COMPUTER);
    float odds[100][12]={{0}};odds[18][MINER]=1;
    CHECK(gate_interception(&g,odds,(Move){19,18})>0);
    g.board[6]=empty_piece();CHECK(gate_interception(&g,odds,(Move){19,18})==0);put(&g,6,BOMB,HUMAN);
    g.board[8].revealed=false;CHECK(gate_interception(&g,odds,(Move){19,18})==0);g.board[8].revealed=true;
    odds[18][MINER]=0;odds[18][LIEUTENANT]=1;CHECK(gate_interception(&g,odds,(Move){19,18})==0);
    odds[18][LIEUTENANT]=0;odds[18][MINER]=.5f;odds[18][MARSHAL]=.5f;
    CHECK(gate_interception(&g,odds,(Move){19,18})==0);
    game_clear(&g);g.turn=COMPUTER;put(&g,4,FLAG,COMPUTER);put(&g,99,FLAG,HUMAN);
    put(&g,94,MARSHAL,COMPUTER);put(&g,24,COLONEL,HUMAN);
    CHECK(recall_officer(&g,(Move){94,84})>0); /* No bomb enclosure required. */
    g.board[24].revealed=false;CHECK(recall_officer(&g,(Move){94,84})==0);g.board[24].revealed=true;
    g.board[94].rank=MAJOR;CHECK(recall_officer(&g,(Move){94,84})==0);g.board[94].rank=MARSHAL;
    put(&g,84,BOMB,COMPUTER);put(&g,93,BOMB,COMPUTER);put(&g,95,BOMB,COMPUTER);
    CHECK(recall_officer(&g,(Move){94,84})==0); /* Cannot route through own bombs. */
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);
    put(&g,40,SCOUT,COMPUTER);put(&g,61,MAJOR,HUMAN);
    CHECK(immediate_defeat(&g,(Move){61,62}));
    CHECK(!immediate_defeat(&g,(Move){61,60}));
    put(&g,70,-2,COMPUTER);g.board[70].revealed=false;
    CHECK(!public_legal(&g,(Move){40,90},COMPUTER));
    game_clear(&g);put(&g,92,FLAG,HUMAN);put(&g,7,FLAG,COMPUTER);
    put(&g,51,MARSHAL,HUMAN);put(&g,71,-2,COMPUTER);put(&g,30,GENERAL,COMPUTER);
    g.board[71].revealed=false;g.board[71].moved=true;
    memset(odds,0,sizeof(odds));odds[71][SPY]=.25f;odds[71][CAPTAIN]=.75f;odds[30][GENERAL]=1;
    CHECK(last_mobile_risk(&g,odds,(Move){51,61}));
    CHECK(!last_mobile_risk(&g,odds,(Move){51,50}));
    odds[71][SPY]=0;odds[71][CAPTAIN]=1;
    CHECK(!last_mobile_risk(&g,odds,(Move){51,61}));
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);
    put(&g,61,MINER,HUMAN);put(&g,51,-2,COMPUTER);g.board[51].revealed=false;
    g.captured[HUMAN][MINER]=army_counts[MINER]-1;
    memset(odds,0,sizeof(odds));odds[51][MINER]=.5f;odds[51][CAPTAIN]=.5f;
    float last_miner=scarce_miner_probe(&g,odds,(Move){61,51});CHECK(last_miner>0);
    g.captured[HUMAN][MINER]--;CHECK(scarce_miner_probe(&g,odds,(Move){61,51})<last_miner);
    g.captured[HUMAN][MINER]++;
    g.board[51].moved=false;CHECK(scarce_miner_probe(&g,odds,(Move){61,51})==0);
    g.board[51].moved=true;g.captured[COMPUTER][BOMB]=army_counts[BOMB];
    CHECK(scarce_miner_probe(&g,odds,(Move){61,51})==0);
    for(int r=SPY;r<=MARSHAL;r++)g.captured[COMPUTER][r]=army_counts[r];
    g.captured[COMPUTER][MINER]--;
    memset(odds,0,sizeof(odds));odds[51][MINER]=1;
    CHECK(last_mobile_risk(&g,odds,(Move){61,51})==0); /* Last ranks can be deduced without a prior reveal. */
    g.board[52]=g.board[51];g.board[51]=empty_piece();odds[51][MINER]=0;odds[52][MINER]=1;
    CHECK(last_mobile_risk(&g,odds,(Move){61,62})==0); /* Offering the same draw is not a losing exposure. */
    CHECK(replay_load(STRATEGO_TOURNAMENT_FIXTURE,763,&g));
    CHECK(!g.board[71].revealed&&g.board[71].rank==SPY);
    for(uint32_t seed=1;seed<=8;seed++){
        uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
        Game masked;int left[12];public_board(&g,&masked,left);probabilities(&masked,left,odds);
        CHECK(game_legal(&g,chosen,g.turn));
        CHECK(last_mobile_risk(&masked,odds,chosen)<last_mobile_risk(&masked,odds,(Move){51,61}));
        Game changed=g;int swap=-1;
        for(int s=0;s<100;s++)if(s!=71&&changed.board[s].side==COMPUTER&&!changed.board[s].revealed&&changed.board[s].moved&&movable(changed.board[s])){swap=s;break;}
        CHECK(swap>=0);int rank=changed.board[swap].rank;changed.board[swap].rank=SPY;changed.board[71].rank=rank;
        uint32_t changed_state=seed;Move same=ai_choose(&changed,1,&changed_state);
        CHECK(same.from==chosen.from&&same.to==chosen.to&&changed_state==state);
    }
    CHECK(replay_load(STRATEGO_MINER_FIXTURE,441,&g));
    for(uint32_t seed=1;seed<=8;seed++){
        uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
        CHECK(game_legal(&g,chosen,g.turn));
        CHECK(chosen.from!=71||chosen.to!=72);
    }
    game_clear(&g);put(&g,90,FLAG,HUMAN);put(&g,9,FLAG,COMPUTER);
    put(&g,60,MINER,COMPUTER);put(&g,64,SERGEANT,HUMAN);
    memset(odds,0,sizeof(odds));odds[60][MINER]=1;
    InterceptPlan plan;intercept_plan(&g,odds,&plan);
    CHECK(plan.count==1&&plan.threats[0].arrival==3);
    CHECK(intercept_bonus(&g,odds,&plan,(Move){64,63})>0);
    CHECK(intercept_bonus(&g,odds,&plan,(Move){64,65})<0);
    /* An extra reserve already in place avoids recalling the whole army. */
    put(&g,70,MAJOR,HUMAN);intercept_plan(&g,odds,&plan);
    CHECK(intercept_bonus(&g,odds,&plan,(Move){64,63})==0);
    g.board[70]=empty_piece();put(&g,80,BOMB,HUMAN);put(&g,91,BOMB,HUMAN);
    int travel[100];intercept_distances(&g,90,COMPUTER,MINER,travel);CHECK(travel[60]==3);
    intercept_distances(&g,90,COMPUTER,SCOUT,travel);CHECK(travel[60]==100);
    game_clear(&g);put(&g,90,FLAG,HUMAN);
    intercept_distances(&g,90,COMPUTER,SCOUT,travel);CHECK(travel[0]==1);
    put(&g,40,-2,HUMAN);g.board[40].revealed=false;
    intercept_distances(&g,90,COMPUTER,SCOUT,travel);CHECK(travel[0]>1);
    /* A top surviving rank is worth more than the same nominal rank when
       opposing superiors still exist; redundant own officers reduce its role. */
    int army[2][12]={{0}};army[HUMAN][CAPTAIN]=1;army[COMPUTER][LIEUTENANT]=3;
    float dominant=force_value(army,HUMAN,CAPTAIN);
    army[COMPUTER][GENERAL]=1;
    CHECK(force_value(army,HUMAN,CAPTAIN)<dominant);
    army[COMPUTER][GENERAL]=0;army[HUMAN][GENERAL]=1;
    CHECK(force_value(army,HUMAN,CAPTAIN)<dominant);
    army[COMPUTER][BOMB]=2;army[HUMAN][MINER]=1;
    float unique_miner=force_value(army,HUMAN,MINER);
    army[HUMAN][MINER]=3;CHECK(force_value(army,HUMAN,MINER)<unique_miner);
    army[COMPUTER][BOMB]=0;CHECK(force_value(army,HUMAN,MINER)<10);
    game_clear(&g);g.turn=HUMAN;
    CHECK(fabsf(force_advantage(&g))<.001f);
    g.captured[COMPUTER][MARSHAL]=1;CHECK(force_advantage(&g)>0);
    g.turn=COMPUTER;CHECK(force_advantage(&g)<0);
    game_clear(&g);g.turn=HUMAN;
    put(&g,64,GENERAL,HUMAN);put(&g,65,GENERAL,COMPUTER);
    put(&g,80,COLONEL,HUMAN);put(&g,20,MAJOR,COMPUTER);
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)g.captured[side][r]=army_counts[r];
    for(int s=0;s<100;s++)if(g.board[s].side>=0)g.captured[g.board[s].side][g.board[s].rank]--;
    memset(odds,0,sizeof(odds));odds[65][GENERAL]=1;
    CHECK(force_trade(&g,odds,(Move){64,65})>0);
    g.captured[HUMAN][COLONEL]++;g.captured[HUMAN][LIEUTENANT]--;
    g.board[80].rank=LIEUTENANT;
    CHECK(force_trade(&g,odds,(Move){64,65})<0);
    /* Scouts advance toward unknown contacts; opening a friendly corridor
       helps, while shuffling sideways must not count as actual progress. */
    game_clear(&g);g.turn=HUMAN;
    put(&g,80,SCOUT,HUMAN);put(&g,70,CAPTAIN,HUMAN);
    put(&g,40,SERGEANT,COMPUTER);g.board[40].revealed=false;
    memset(odds,0,sizeof(odds));odds[40][SERGEANT]=1;
    float recon[100];route_map(&g,odds,SCOUT,recon);
    CHECK(recon_progress(&g,odds,(Move){70,71},recon)>0);
    g.board[70]=empty_piece();route_map(&g,odds,SCOUT,recon);
    CHECK(recon_progress(&g,odds,(Move){80,81},recon)<=0);
    CHECK(recon_progress(&g,odds,(Move){80,70},recon)>0);
    g.board[40].revealed=true;route_map(&g,odds,SCOUT,recon);
    CHECK(recon_progress(&g,odds,(Move){80,70},recon)==0);
    /* A mobile enemy remains a flag threat after all miners are accounted
       for. Deduction must not consume an extra rank in sampled armies. */
    game_clear(&g);g.turn=HUMAN;
    put(&g,90,FLAG,HUMAN);put(&g,64,COLONEL,HUMAN);
    put(&g,60,CAPTAIN,COMPUTER);g.board[60].revealed=false;
    memset(odds,0,sizeof(odds));odds[60][CAPTAIN]=1;
    intercept_plan(&g,odds,&plan);
    CHECK(plan.count==1&&plan.threats[0].arrival==3);
    CHECK(intercept_bonus(&g,odds,&plan,(Move){64,63})>0);
    for(int r=0;r<12;r++)g.captured[COMPUTER][r]=army_counts[r];
    g.captured[COMPUTER][CAPTAIN]--;
    Game inferred;int unassigned[12];public_board(&g,&inferred,unassigned);
    CHECK(inferred.board[60].rank==CAPTAIN&&inferred.board[60].revealed);
    CHECK(unassigned[CAPTAIN]==0&&!g.board[60].revealed);
    uint32_t inference_rng=123;Game sampled;
    sample_board(&inferred,unassigned,&sampled,&inference_rng);
    CHECK(sampled.board[60].rank==CAPTAIN);
    /* Merely probable identities stay hidden when two mobile ranks remain. */
    g.captured[COMPUTER][SERGEANT]--;put(&g,20,SERGEANT,COMPUTER);
    g.board[20].revealed=false;
    public_board(&g,&inferred,unassigned);
    CHECK(inferred.board[60].rank==-2&&!inferred.board[60].revealed);
    CHECK(replay_load(STRATEGO_LATEST_LOSS,0,&g));
    CHECK(g.ply==359&&g.winner==HUMAN&&g.end_reason==END_FLAG);
    CHECK(replay_load(STRATEGO_LATEST_WIN,0,&g));
    CHECK(g.ply==367&&g.winner==COMPUTER&&g.end_reason==END_IMMOBILE);
    CHECK(replay_load(STRATEGO_LATEST_LOSS,350,&g));
    public_board(&g,&inferred,unassigned);probabilities(&inferred,unassigned,odds);
    Move hanging={26,25};Game exposed=optimistic_move(&inferred,hanging);exposed.turn=HUMAN;
    float pressure=ai_flag_risk(&inferred,COMPUTER);
    CHECK(pressure-ai_flag_risk(&exposed,COMPUTER)>90);
    CHECK(known_unanswered_loss_at(&exposed,COMPUTER,25)>0);
    CHECK(public_defense_relief(&inferred,odds,hanging,pressure)==0);
    CHECK(known_unanswered_loss_at(&exposed,COMPUTER,58)==0);
    /* A real recapture still counts as cover; avoid banning useful exchanges. */
    Game covered=exposed;put(&covered,15,MARSHAL,COMPUTER);
    CHECK(known_unanswered_loss_at(&covered,COMPUTER,25)==0);
    for(uint32_t seed=1;seed<=16;seed++){
        uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
        CHECK(game_legal(&g,chosen,g.turn));
        CHECK(!(chosen.from==26&&chosen.to==25));
    }
    for(int i=0;i<2;i++){
        CHECK(replay_load(STRATEGO_LATEST_WIN,i?96:12,&g));
        for(uint32_t seed=1;seed<=4;seed++){
            uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
            CHECK(chosen.from==35&&chosen.to==(i?36:45));
        }
    }
    /* Last miner left hanging while a distant captain patrols: preserve the
       only route through six remaining bombs, using public knowledge only. */
    CHECK(replay_load(STRATEGO_MINER_RESCUE,0,&g));
    CHECK(g.ply==558&&g.winner==COMPUTER&&g.end_reason==END_RESIGNATION);
    CHECK(replay_load(STRATEGO_MINER_RESCUE,470,&g));
    CHECK(g.captured[COMPUTER][MINER]==4&&g.captured[HUMAN][BOMB]==0);
    public_board(&g,&inferred,unassigned);
    CHECK(known_unanswered_loss(&inferred,(Move){7,6})>0);
    CHECK(known_unanswered_loss(&inferred,(Move){25,26})==0);
    Game no_bombs=inferred;no_bombs.captured[HUMAN][BOMB]=army_counts[BOMB];
    CHECK(known_unanswered_loss(&no_bombs,(Move){7,6})==0);
    Game covered_miner=inferred;put(&covered_miner,35,CAPTAIN,COMPUTER);
    CHECK(known_unanswered_loss(&covered_miner,(Move){7,6})==0);
    Game uncertain=inferred;uncertain.board[24].revealed=false;uncertain.board[24].rank=-2;
    CHECK(known_unanswered_loss(&uncertain,(Move){7,6})==0);
    for(uint32_t seed=1;seed<=16;seed++){
        uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
        CHECK(game_legal(&g,chosen,g.turn));
        CHECK(chosen.from==25&&chosen.to==26);
        CHECK(known_unanswered_loss(&inferred,chosen)==0);
    }
    CHECK(replay_load(STRATEGO_FLAG_FINISH,0,&g));
    CHECK(g.ply==378&&g.winner==COMPUTER&&g.end_reason==END_IMMOBILE);
    for(int ply=334;ply<=338;ply+=2){
        CHECK(replay_load(STRATEGO_FLAG_FINISH,ply,&g));
        CHECK(g.board[92].rank==FLAG&&!g.board[92].revealed);
        Game won=g;CHECK(game_apply(&won,(Move){93,92}));
        CHECK(won.winner==COMPUTER&&won.end_reason==END_FLAG);
        /* Merely threatening a hypothetical flag must not be worth more
           than capturing it. Actual revealed flags still give full wins. */
        CHECK(evaluate(&won,COMPUTER,false)>evaluate(&g,COMPUTER,false));
        CHECK(evaluate(&won,COMPUTER,true)==WIN);
        for(uint32_t seed=1;seed<=16;seed++){
            uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
            CHECK(chosen.from==93&&chosen.to==92);
        }
        /* A different hidden arrangement with identical public information
           must produce the same attack, even when C1 is really a bomb. */
        CHECK(!g.board[82].revealed&&!g.board[82].moved&&g.board[82].rank==BOMB);
        g.board[82].rank=FLAG;g.board[92].rank=BOMB;
        for(uint32_t seed=1;seed<=4;seed++){
            uint32_t state=seed;Move chosen=ai_choose(&g,1,&state);
            CHECK(chosen.from==93&&chosen.to==92);
        }
    }
    /* Real danger to our own flag is not suppressed by uncertainty about
       the opponent's flag: an exposed flag remains a large liability. */
    game_clear(&g);g.turn=HUMAN;
    put(&g,99,FLAG,HUMAN);put(&g,80,MINER,HUMAN);
    put(&g,0,FLAG,COMPUTER);put(&g,89,CAPTAIN,COMPUTER);
    float exposed_flag=evaluate(&g,HUMAN,false);
    g.board[79]=g.board[89];g.board[89]=empty_piece();
    CHECK(evaluate(&g,HUMAN,false)>exposed_flag+100);
    free(table);puts("Search, public safety, force balance and reserve travel: OK");return 0;
}
