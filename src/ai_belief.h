#ifndef STRATEGO_AI_BELIEF_H
#define STRATEGO_AI_BELIEF_H
#include "game.h"
/* Bounded, fading likelihood ratios: a bluff can produce the same public
   behaviour as a strong piece. Never remove a legal rank on this evidence. */
static inline float ai_behavior_likelihood(const Game *view,int s,int rank){
    Piece piece=view->board[s];
    if(rank<SPY||rank>MARSHAL||piece.revealed||piece.id<0||piece.id>=80)return 1;
    const PublicEvidence *e=&view->evidence[piece.id];
    int age=view->ply-e->last_ply;
    float fade=age>=0&&age<120?(120-age)/120.0f:0,weight=1;
    for(int target=LIEUTENANT;target<=MARSHAL;target++)if(e->approaches>=2&&(e->pursued&(1u<<target))){
        float strength=.04f*e->approaches;
        if(target==MARSHAL)strength*=1.5f;
        float change=combat_result(rank,target)>0?strength:-.04f;
        if(change>0){if(weight<1+change*fade)weight=1+change*fade;}
        else if(weight==1)weight+=change*fade;
    }
    /* Repeated withdrawals weaken a claim of superiority, without making
       an officer impossible: retreats also serve traps and flag defence. */
    age=view->ply-e->retreat_ply;
    float retreat_fade=age>=0&&age<120?(120-age)/120.0f:0;
    if(e->retreats>=2)for(int target=LIEUTENANT;target<=MARSHAL;target++)
        if((e->avoided&(1u<<target))&&combat_result(rank,target)>0){weight*=1-.06f*retreat_fade;break;}
    return weight;
}
/* Relative likelihood, never a revealed identity. Inspect only public motion
   and public neighboring ranks; stationary officers remain possible. */
static inline float ai_rank_prior(const Game *view,int s,int rank){
    Piece piece=view->board[s];
    if(piece.moved&&(rank==FLAG||rank==BOMB))return 0;
    int back=piece.side==COMPUTER?s/10:9-s/10;
    int neighbors[4]={s>=10?s-10:-1,s<90?s+10:-1,s%10?s-1:-1,s%10<9?s+1:-1};
    int stable=0,known_bombs=0;
    for(int i=0;i<4;i++)if(neighbors[i]>=0){
        Piece p=view->board[neighbors[i]];
        if(p.side!=piece.side)continue;
        if(p.revealed){if(p.rank==BOMB){known_bombs++;stable++;}}
        else if(!p.moved)stable++;
    }
    float evidence=(view->ply-40)/240.0f;
    if(evidence<0)evidence=0;
    if(evidence>1)evidence=1;
    if(piece.moved)evidence=0;
    if(rank==FLAG){
        float weight=back==0?5:back==1?2:1;
        for(int i=0;i<known_bombs;i++)weight*=2;
        return weight*(1+evidence*(1.0f+.8f*stable));
    }
    if(rank==BOMB)return (back<=1?2.0f:1.0f)*(1+evidence*(.65f+.25f*stable));
    return ai_behavior_likelihood(view,s,rank);
}
/* Balance marginal beliefs against the public remaining army. Independent
   square normalization can otherwise invent several flags or too many spies.
   This is a constrained approximation, not an exact Bayesian posterior. */
static inline void ai_balance_beliefs(const Game *view,const int remaining[12],float p[100][12]){
    int squares[40],n=0,stationary=0,total=0;
    for(int s=0;s<100;s++)if(view->board[s].side==1-view->turn&&view->board[s].rank<0){
        if(n==40)return;
        squares[n++]=s;stationary+=!view->board[s].moved;
    }
    for(int r=0;r<12;r++){if(remaining[r]<0)return;total+=remaining[r];}
    int fixed=remaining[FLAG]+remaining[BOMB];
    /* Sparse diagnostic boards may deliberately omit the casualty ledger. */
    if(!n||total!=n||fixed>stationary)return;
    if(fixed==stationary)for(int i=0;i<n;i++)if(!view->board[squares[i]].moved)
        for(int r=SPY;r<=MARSHAL;r++)p[squares[i]][r]=0;
    for(int iteration=0;iteration<80;iteration++){
        float sums[12]={0},error=0;
        for(int i=0;i<n;i++)for(int r=0;r<12;r++)sums[r]+=p[squares[i]][r];
        for(int r=0;r<12;r++){
            float delta=sums[r]-remaining[r];if(delta<0)delta=-delta;
            if(delta>error)error=delta;
            if(sums[r]>0)for(int i=0;i<n;i++)p[squares[i]][r]*=remaining[r]/sums[r];
        }
        for(int i=0;i<n;i++){
            float sum=0;for(int r=0;r<12;r++)sum+=p[squares[i]][r];
            if(sum>0)for(int r=0;r<12;r++)p[squares[i]][r]/=sum;
        }
        if(error<.00001f)break;
    }
}
/* Concentration relative to a uniform flag location, for choosing missions.
   Absolute old per-square thresholds do not survive inventory balancing. */
static inline float ai_flag_focus_threshold(const Game *view,float p[100][12],float lift){
    int candidates=0;
    for(int s=0;s<100;s++)if(view->board[s].side==1-view->turn&&p[s][FLAG]>0)candidates++;
    float threshold=candidates?lift/candidates:1;
    return threshold<.5f?threshold:.5f;
}
#endif
