#include "ai_models.h"
/* Same frozen baseline used by the paired benchmark. */
Move ai_previous(const Game *g,int difficulty,uint32_t *rng);
const char *ai_model_name(int model){
    switch(model){
        case AI_IMPROVED:return "Expert+ Improved";
        case AI_CLASSIC:return "Expert+ Classique";
        case AI_LEARNED:return "IA entrainee";
        default:return "Decouverte";
    }
}
const char *ai_model_description(int model){
    switch(model){
        case AI_IMPROVED:return "Derniere version du comparatif.";
        case AI_CLASSIC:return "Version precedente du comparatif.";
        case AI_LEARNED:return "Auto-apprentissage experimental.";
        default:return "Une IA accessible pour apprendre.";
    }
}
int ai_model_next(int model,bool learned_available){
    switch(model){
        case AI_DISCOVERY:return AI_CLASSIC;
        case AI_CLASSIC:return AI_IMPROVED;
        case AI_IMPROVED:return learned_available?AI_LEARNED:AI_DISCOVERY;
        default:return AI_DISCOVERY;
    }
}
Move ai_model_choose(const Game *g,int model,uint32_t *rng){
    if(g->winner>=0)return (Move){-1,-1};
    return model==AI_CLASSIC?ai_previous(g,1,rng):ai_choose(g,model,rng);
}
