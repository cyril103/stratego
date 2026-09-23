#include "game.h"
#include <stdlib.h>
#include <string.h>
const int army_counts[12] = {1,1,8,5,4,4,4,3,2,1,1,6};
const char *rank_names[12] = {"Drapeau","Espion","Eclaireur","Demineur","Sergent","Lieutenant","Capitaine","Commandant","Colonel","General","Marechal","Bombe"};
const char *rank_symbols[12] = {"D","1","2","3","4","5","6","7","8","9","10","B"};
uint32_t game_random(uint32_t *s) { if (!*s) *s=0x91aabbcd; *s^=*s<<13; *s^=*s>>17; *s^=*s<<5; return *s; }
Piece empty_piece(void) { return (Piece){-1,-1,-1,false,false}; }
bool is_lake(int s) { int x=s%10,z=s/10; return s>=0&&s<100&&(z==4||z==5)&&(x==2||x==3||x==6||x==7); }
bool movable(Piece p) { return p.rank>FLAG&&p.rank<BOMB; }
void game_clear(Game *g) {
    memset(g,0,sizeof(*g));
    for(int i=0;i<100;i++) g->board[i]=empty_piece();
    g->winner=-1; g->last_move=(Move){-1,-1}; g->combat=2;
    for(int s=0;s<2;s++) g->last_id[s]=g->last_from[s]=g->last_to[s]=-1;
}
void game_deploy(Game *g,int side) {
    int ranks[40], n=0, start=side==HUMAN?60:0;
    for(int r=0;r<12;r++) for(int k=0;k<army_counts[r];k++) ranks[n++]=r;
    for(int i=39;i>0;i--) {int j=(int)(game_random(&g->rng)%(i+1));int t=ranks[i];ranks[i]=ranks[j];ranks[j]=t;}
    /* A back-rank flag and a loose bomb screen, with open escape routes. */
    int flag=(side==HUMAN?30:0)+1+(int)(game_random(&g->rng)%8);
    for(int i=0;i<40;i++) if(ranks[i]==FLAG) {int t=ranks[flag];ranks[flag]=FLAG;ranks[i]=t;break;}
    int screen[3]={flag-1,flag+1,flag+(side==HUMAN?-10:10)};
    for(int k=0;k<3;k++) if(ranks[screen[k]]!=BOMB) {
        for(int i=0;i<40;i++) if(ranks[i]==BOMB&&i!=screen[0]&&i!=screen[1]&&i!=screen[2]) {int t=ranks[screen[k]];ranks[screen[k]]=BOMB;ranks[i]=t;break;}
    }
    for(int i=0;i<40;i++) g->board[start+i]=(Piece){ranks[i],side,side*40+i,false,false};
}
void game_init(Game *g,uint32_t seed) { game_clear(g);g->rng=seed;game_deploy(g,HUMAN);game_deploy(g,COMPUTER);g->turn=HUMAN; }
bool game_legal(const Game *g,Move m,int side) {
    if(m.from<0||m.from>=100||m.to<0||m.to>=100||m.from==m.to||is_lake(m.to)) return false;
    Piece p=g->board[m.from];
    if(p.side!=side||!movable(p)||g->board[m.to].side==side) return false;
    int dx=m.to%10-m.from%10,dz=m.to/10-m.from/10;
    if(dx&&dz) return false;
    if(p.rank!=SCOUT&&abs(dx)+abs(dz)!=1) return false;
    int step=dx?(dx>0?1:-1):(dz>0?10:-10);
    for(int s=m.from+step;s!=m.to;s+=step) if(is_lake(s)||g->board[s].rank>=0) return false;
    if(g->last_id[side]==p.id&&g->repetitions[side]>=3&&m.from==g->last_to[side]&&m.to==g->last_from[side]) return false;
    return true;
}
int game_moves(const Game *g,int side,Move out[MAX_MOVES]) {
    int n=0;
    for(int a=0;a<100;a++) if(g->board[a].side==side&&movable(g->board[a])) {
        const int dx[4]={-1,1,0,0}, dz[4]={0,0,-1,1};
        for(int d=0;d<4;d++)for(int step=1;step<=(g->board[a].rank==SCOUT?9:1);step++) {
            int x=a%10+dx[d]*step,z=a/10+dz[d]*step;
            if(x<0||x>9||z<0||z>9)break;
            int b=z*10+x;
            if(is_lake(b)||g->board[b].side==side)break;
            if(game_legal(g,(Move){a,b},side)){if(out)out[n]=(Move){a,b};n++;}
            if(g->board[b].side>=0)break;
        }
    }
    return n;
}
int combat_result(int a,int d) { if(d==FLAG)return 1;if(d==BOMB)return a==MINER?1:-1;if(a==SPY&&d==MARSHAL)return 1;return a==d?0:(a>d?1:-1); }
static bool has_legal_move(const Game *g,int side){
    for(int s=0;s<100;s++)if(g->board[s].side==side&&movable(g->board[s])){
        int nb[4]={s>=10?s-10:-1,s<90?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1};
        /* Every legal scout ray starts with a legal adjacent square. */
        for(int k=0;k<4;k++)if(game_legal(g,(Move){s,nb[k]},side))return true;
        /* A repetition restriction can forbid the adjacent destination while
           a longer scout move on the same open ray remains legal. */
        if(g->board[s].rank==SCOUT)for(int t=0;t<100;t++)if(game_legal(g,(Move){s,t},side))return true;
    }
    return false;
}
void game_check_end(Game *g) {
    if(g->winner>=0)return;
    bool human=has_legal_move(g,HUMAN),computer=has_legal_move(g,COMPUTER);
    if(!human&&!computer){g->winner=GAME_DRAW;g->end_reason=END_BOTH_IMMOBILE;}
    else if(!human||!computer){g->winner=human?HUMAN:COMPUTER;g->end_reason=END_IMMOBILE;}
}
bool game_resign(Game *g,int side){
    if(side!=HUMAN&&side!=COMPUTER)return false;
    game_check_end(g);if(g->winner>=0)return false;
    g->winner=1-side;g->end_reason=END_RESIGNATION;return true;
}
bool game_agree_draw(Game *g,bool human_agrees,bool computer_agrees){
    if(!human_agrees||!computer_agrees)return false;
    game_check_end(g);if(g->winner>=0)return false;
    g->winner=GAME_DRAW;g->end_reason=END_AGREEMENT;return true;
}
bool game_end_playing_period(Game *g){
    game_check_end(g);if(g->winner>=0)return false;
    g->winner=GAME_DRAW;g->end_reason=END_PLAYING_PERIOD;return true;
}
/* Observe choices only against ranks the moving player has actually seen.
   The mover's hidden rank is deliberately not consulted. Bluffing remains
   possible; these facts are evidence, never an identification. */
static void observe_choice(Game *g,Move m){
    Piece a=g->board[m.from];
    if(g->board[m.to].side>=0||abs(m.from%10-m.to%10)+abs(m.from/10-m.to/10)!=1)return;
    for(int s=0;s<100;s++){
        Piece actor=g->board[s];
        if(actor.side!=a.side||actor.revealed||actor.id<0||actor.id>=80)continue;
        PublicEvidence *e=&g->evidence[actor.id];
        int cells[8]={s>=10?s-10:-1,s<90?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1,
            m.to>=10?m.to-10:-1,m.to<90?m.to+10:-1,m.to%10?m.to-1:-1,m.to%10<9?m.to+1:-1};
        for(int k=0;k<(s==m.from?8:4);k++){
            int t=cells[k];if(t<0)continue;
            bool duplicate=false;for(int j=0;j<k;j++)if(cells[j]==t)duplicate=true;
            if(duplicate)continue;
            Piece target=g->board[t];
            if(target.side!=1-a.side||!target.revealed||!movable(target))continue;
            int before=abs(s%10-t%10)+abs(s/10-t/10);
            int after=s==m.from?abs(m.to%10-t%10)+abs(m.to/10-t/10):before;
            uint16_t bit=(uint16_t)(1u<<target.rank);
            if(s==m.from&&before>1&&after==1){e->pursued|=bit;if(e->approaches<6)e->approaches++;e->last_ply=g->ply+1;}
            if(s==m.from&&before==1&&after>1){e->avoided|=bit;if(e->retreats<6)e->retreats++;e->retreat_ply=g->ply+1;}
            if(s!=m.from&&before==1)e->declined|=bit;
        }
    }
}
static bool apply_move(Game *g,Move m,bool observe) {
    if(g->winner>=0||!game_legal(g,m,g->turn))return false;
    Piece a=g->board[m.from],d=g->board[m.to];int s=a.side;
    if(observe)observe_choice(g,m);
    if(a.rank==MARSHAL&&a.revealed){
        for(int e=0;e<100;e++){
            Piece suspect=g->board[e];
            if(suspect.side!=1-s||suspect.revealed||suspect.id<0||suspect.id>=80)continue;
            int before=abs(e%10-m.from%10)+abs(e/10-m.from/10);
            int after=abs(e%10-m.to%10)+abs(e/10-m.to/10);
            if(before==1&&after>1)g->marshal_suspects[s][suspect.id/64]|=UINT64_C(1)<<(suspect.id%64);
        }
    }
    int history_slot=g->history_count[s]++%8;
    g->history[s][history_slot]=m;g->history_id[s][history_slot]=a.id;
    bool reverse=g->last_id[s]==a.id&&m.from==g->last_to[s]&&m.to==g->last_from[s];
    g->repetitions[s]=reverse?g->repetitions[s]+1:1;
    g->last_id[s]=a.id;g->last_from[s]=m.from;g->last_to[s]=m.to;
    a.moved=true;
    if(abs(m.to%10-m.from%10)+abs(m.to/10-m.from/10)>1)a.revealed=true;
    g->combat=2;g->attack_rank=a.rank;g->defend_rank=d.rank;
    g->board[m.from]=empty_piece();
    if(d.rank<0)g->board[m.to]=a;
    else {
        a.revealed=d.revealed=true;g->combat=combat_result(a.rank,d.rank);
        if(d.rank==BOMB&&g->combat>0)g->cleared_bombs[d.side][m.to]=true;
        if(g->combat>=0)g->captured[d.side][d.rank]++;
        if(g->combat<=0)g->captured[a.side][a.rank]++;
        g->board[m.to]=g->combat>0?a:(g->combat<0?d:empty_piece());
        if(d.rank==FLAG){g->winner=s;g->end_reason=END_FLAG;}
    }
    /* Observation invalidates suspicion, regardless of the revealed rank.
       Combat also clears captured identities; no hidden rank is inspected. */
    if(a.revealed&&a.id>=0&&a.id<80)for(int side=0;side<2;side++)
        g->marshal_suspects[side][a.id/64]&=~(UINT64_C(1)<<(a.id%64));
    if(g->combat!=2&&d.id>=0&&d.id<80)for(int side=0;side<2;side++)
        g->marshal_suspects[side][d.id/64]&=~(UINT64_C(1)<<(d.id%64));
    g->last_move=m;g->ply++;g->turn=1-s;game_check_end(g);return true;
}
bool game_apply(Game *g,Move m){return apply_move(g,m,true);}
bool game_apply_search(Game *g,Move m){return apply_move(g,m,false);}
