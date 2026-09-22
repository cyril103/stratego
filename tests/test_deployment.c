#include "deployment.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    Game g;Deployment d;game_init(&g,42);Piece enemy=g.board[0];deployment_begin(&d,&g);
    CHECK(deployment_count(&d,-1)==40);CHECK(!deployment_complete(&d,&g));
    CHECK(!deployment_place(&d,&g,FLAG,59));CHECK(g.board[0].id==enemy.id);
    CHECK(deployment_place(&d,&g,FLAG,99));CHECK(!deployment_place(&d,&g,FLAG,98));
    CHECK(deployment_place(&d,&g,BOMB,99));CHECK(deployment_count(&d,FLAG)==1);
    CHECK(deployment_remove(&d,&g,99));CHECK(deployment_count(&d,-1)==40);
    int square=60;
    for(int r=0;r<12;r++)for(int n=0;n<army_counts[r];n++)CHECK(deployment_place(&d,&g,r,square++));
    CHECK(deployment_complete(&d,&g));CHECK(deployment_remove(&d,&g,70));CHECK(!deployment_complete(&d,&g));
    deployment_clear(&d,&g);CHECK(deployment_count(&d,-1)==40);
    game_deploy(&g,HUMAN);for(int i=0;i<40;i++)d.reserve[i]=empty_piece();CHECK(deployment_complete(&d,&g));
    CHECK(g.board[0].id==enemy.id&&g.board[0].rank==enemy.rank);
    const char *path="deployment_test_save.txt";
    Game saved=g;
    CHECK(deployment_save(&d,&g,path));
    deployment_clear(&d,&g);
    CHECK(!deployment_save(&d,&g,path)); /* incomplete save must preserve the file */
    CHECK(deployment_load(&d,&g,path));CHECK(deployment_complete(&d,&g));
    for(int i=0;i<100;i++)CHECK(g.board[i].rank==saved.board[i].rank&&g.board[i].side==saved.board[i].side);
    CHECK(g.rng==saved.rng&&g.turn==saved.turn);
    Piece swap=g.board[60];g.board[60]=g.board[99];g.board[99]=swap;
    CHECK(deployment_save(&d,&g,path)); /* replacing an existing save */
    int first=g.board[60].rank;deployment_clear(&d,&g);
    CHECK(deployment_load(&d,&g,path));CHECK(g.board[60].rank==first);
    CHECK(deployment_remove(&d,&g,70)); /* loaded placements remain editable */
    Game before=g;Deployment reserve=d;
    const char *invalid[]={"", "WRONG_VERSION\n", "STRATEGO_DEPLOYMENT_V1\n1 2", "STRATEGO_DEPLOYMENT_V1\n99999999999999999999999999"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){
        FILE *f=fopen(path,"w");CHECK(f);fputs(invalid[i],f);fclose(f);
        CHECK(!deployment_load(&d,&g,path));CHECK(!memcmp(&g,&before,sizeof(g)));CHECK(!memcmp(&d,&reserve,sizeof(d)));
    }
    FILE *f=fopen(path,"w");CHECK(f);fputs("STRATEGO_DEPLOYMENT_V1\n",f);
    for(int i=0;i<40;i++)fprintf(f,"0 ");fclose(f);
    CHECK(!deployment_load(&d,&g,path));CHECK(!memcmp(&g,&before,sizeof(g)));
    CHECK(remove(path)==0);CHECK(!deployment_load(&d,&g,path));
    puts("Deployment placement, persistence, replacement and invalid-file protection: OK");return 0;
}
