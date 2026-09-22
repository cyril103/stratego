#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "game.h"
#include "ai_worker.h"
#include "ai_models.h"
#include "ai_strategy.h"
#include "match_log.h"
#include "reveal_window.h"
#include "ml.h"
#include "deployment.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ui_theme.h"

static int view_width=1440;
#define VW view_width
#define VH 900
#define SW (VW-424)
#define SH 704
static const Color BG={9,16,23,255}, INK={240,231,208,255};
static const Color MUTED={200,204,200,255}, BRASS={226,189,121,255}, REDTEAM={202,30,44,255}, IVORY={239,229,200,255};
static Font regular,bold,imperial;
static Vector2 mouse;
static bool clicked;
static bool quit_requested=false;
static int windowed_width=1440,windowed_height=VH;
static void toggle_fullscreen(void) {
    if(IsWindowFullscreen()) {
        ToggleFullscreen();
        SetWindowSize(windowed_width,windowed_height);
    } else {
        windowed_width=GetScreenWidth();windowed_height=GetScreenHeight();
        int monitor=GetCurrentMonitor();
        int width=GetMonitorWidth(monitor),height=GetMonitorHeight(monitor);
        SetWindowSize(width,height);
        ToggleFullscreen();
        /* The decorated window may be clamped to the work area before the switch. */
        SetWindowSize(width,height);
    }
}
static Model pieces[13],piece_faces[12],piece_plate,board_model,inlay;
static const Color BLUE_TEAM={35,83,183,255};
static Shader lighting,metal_lighting,matte_lighting;
static Shader wood_lighting,cloth_lighting;
static Model table_model,tile_model;
static Model contact_shadow;
static Texture2D shadow_texture;
static Texture2D battlefield_background;
static Texture2D imperial_emblem;
static void draw_background(float x,float y) {
    if(!battlefield_background.id)return;
    float w=(float)battlefield_background.width,h=(float)battlefield_background.height;
    float scale=fmaxf(VW/w,VH/h);
    Rectangle source={(w-VW/scale)/2,(h-VH/scale)/2,VW/scale,VH/scale};
    DrawTexturePro(battlefield_background,source,(Rectangle){x,y,VW,VH},(Vector2){0,0},0,WHITE);
}
static int layout_width(void) {
    return (int)fmaxf(1440,roundf((float)GetScreenWidth()*VH/fmaxf(1,GetScreenHeight())));
}
static RenderTexture2D captured_icons[2][12];
static int tray_side=COMPUTER;
static bool tray_journal=false;
static int latest_lost[2]={-1,-1};
static int formation_bag[AI_FORMATIONS],formation_count=0,last_formation=-1;
static Camera3D camera;
static float azimuth=0, elevation=1.02f, zoom=15.6f;
static Vector3 camera_focus={0,0,0};
static Game game;
static RevealWindow reveal_window;
static bool face_hidden(Piece p){return game.winner==GAME_ONGOING&&p.side==COMPUTER&&!reveal_recent(&reveal_window,&game,p);}
static Deployment deployment;
static bool deployment_ready=false;
static const char *deployment_path="saves/placement.txt";
static const char *deployment_status="";
static int reserve_rank=-1;
static int phase=0,selected=-1,difficulty=1; /* 0 welcome, 1 deployment, 2 battle */
static bool help=false,muted=false,animating=false;
static bool resign_confirm=false,resigned=false,end_dismissed=false;
static bool draw_confirm=false;
static bool ai_draw_pending=false;
static int last_combat_ply=0,last_ai_draw_offer=-100;
static int last_draw_offer=-100;
static const char *end_reason_text(void);
static float end_time=0;
static Move pending={-1,-1};
static float animation=0,ai_timer=.75f;
static uint32_t ai_rng=3217;
static char logs[5][110];
static Sound click_sound,combat_sound;
static bool audio_ok=false;
static bool combat_animation(void){return animating&&game.board[pending.to].side>=0;}
static float ease(float t){t=Clamp(t,0,1);return t*t*(3-2*t);}
static float move_duration(void){return combat_animation()?2.55f:.42f;}
static Font interface_font(bool heavy){
    const char *fallback=heavy?"assets/fonts/Barlow-Bold.ttf":"assets/fonts/Barlow-SemiBold.ttf";
    return FileExists(fallback)?LoadFontEx(fallback,64,NULL,0):GetFontDefault();
}
static void label(const char *s,float x,float y,float size,Color c,bool heavy) {
    size=fmaxf(size,18);
    DrawTextEx(heavy?bold:regular,s,(Vector2){x,y},size,.35f,c);
}
static void center(const char *s,float x,float y,float size,Color c) {
    size=fmaxf(size,18);
    Vector2 d=MeasureTextEx(bold,s,size,.35f);label(s,x-d.x/2,y,size,c,true);
}
static void title(const char *s,float x,float y,float size,Color color) {
    DrawTextEx(imperial,s,(Vector2){x+1,y+2},size,1,Fade(BLACK,.65f));
    DrawTextEx(imperial,s,(Vector2){x,y},size,1,color);
}
static void emblem(Rectangle r,float alpha) {
    if(imperial_emblem.id)DrawTexturePro(imperial_emblem,(Rectangle){0,0,(float)imperial_emblem.width,(float)imperial_emblem.height},r,(Vector2){0,0},0,Fade(WHITE,alpha));
}
static bool button(const char *s,Rectangle r,bool primary,bool enabled) {
    bool hover=enabled&&CheckCollisionPointRec(mouse,r);
    float transition=ui_hover(s,r,hover);
    bool pressed=hover&&IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    ui_button_skin(r,primary,enabled,transition,pressed);
    Color text=enabled?(primary?(Color){255,240,201,255}:INK):(Color){161,164,161,255};
    float size=r.height<35?18:20;
    while(size>16&&MeasureTextEx(bold,s,size,.35f).x>r.width-28)size-=.5f;
    float width=MeasureTextEx(bold,s,size,.35f).x;
    DrawTextEx(bold,s,(Vector2){r.x+(r.width-width)/2,r.y+(r.height-size)/2+(pressed?1:0)},size,.35f,text);
    return hover&&clicked;
}
static void push_log(const char *s) {for(int i=4;i>0;i--)memcpy(logs[i],logs[i-1],110);snprintf(logs[0],110,"%s",s);}
static Vector3 square_pos(int s) {return (Vector3){s%10-4.5f,.075f,s/10-4.5f};}
static void set_camera(void) {
    float radius=zoom/(2*tanf(14*DEG2RAD));
    camera=(Camera3D){.position={camera_focus.x+sinf(azimuth)*cosf(elevation)*radius,sinf(elevation)*radius,camera_focus.z+cosf(azimuth)*cosf(elevation)*radius},.target=camera_focus,.up={0,1,0},.fovy=28,.projection=CAMERA_PERSPECTIVE};
}
static Vector3 mouse_on_board(Vector2 pointer) {
    Ray ray=GetScreenToWorldRayEx((Vector2){pointer.x-24,pointer.y-142},camera,SW,SH);
    return Vector3Add(ray.position,Vector3Scale(ray.direction,(.075f-ray.position.y)/ray.direction.y));
}
static void shift_focus(Vector3 delta) {
    camera_focus.x=Clamp(camera_focus.x+delta.x,-6,6);
    camera_focus.z=Clamp(camera_focus.z+delta.z,-6,6);
    set_camera();
}
static void apply_shader(Model *m) {for(int i=0;i<m->materialCount;i++)m->materials[i].shader=lighting;}
static Shader studio_shader(float roughness,float metallic) {
    Shader shader=LoadShader("assets/shaders/lighting.vs","assets/shaders/lighting.fs");
    shader.locs[SHADER_LOC_MATRIX_MODEL]=GetShaderLocation(shader,"matModel");
    SetShaderValue(shader,GetShaderLocation(shader,"roughness"),&roughness,SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader,GetShaderLocation(shader,"metallic"),&metallic,SHADER_UNIFORM_FLOAT);
    return shader;
}
static void create_contact_shadow(void) {
    Image image=GenImageColor(128,128,BLANK);Color *pixels=image.data;
    for(int y=0;y<128;y++)for(int x=0;x<128;x++) {
        float u=(x-63.5f)/64,v=(y-63.5f)/64;
        float dx=fmaxf(0,fabsf(u)-.36f),dz=fmaxf(0,fabsf(v)-.16f);
        float near=expf(-(dx*dx+dz*dz)*32)*.30f;
        float far=expf(-((u-.14f)*(u-.14f)*4+(v-.22f)*(v-.22f)*3))*.18f;
        float edge=Clamp((1-fmaxf(fabsf(u),fabsf(v)))*10,0,1);
        pixels[y*128+x]=(Color){9,15,25,(unsigned char)(255*(near+far)*edge)};
    }
    shadow_texture=LoadTextureFromImage(image);UnloadImage(image);SetTextureFilter(shadow_texture,TEXTURE_FILTER_BILINEAR);
    contact_shadow=LoadModelFromMesh(GenMeshPlane(1.65f,1.65f,1,1));
    contact_shadow.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=shadow_texture;
}
static Sound tone(float hz,float duration) {
    int n=(int)(22050*duration);short *samples=MemAlloc((unsigned int)n*sizeof(short));
    for(int i=0;i<n;i++){float t=(float)i/22050,env=expf(-t*19)*fminf(t*160,1);samples[i]=(short)(sinf(t*hz*2*PI)*env*5000);}
    Wave w={(unsigned int)n,22050,16,1,samples};Sound s=LoadSoundFromWave(w);UnloadWave(w);return s;
}
static void reset_game(void) {
    draw_confirm=false;last_draw_offer=-100;
    ai_draw_pending=false;last_combat_ply=0;last_ai_draw_offer=-100;
    ai_worker_stop();
    match_log_close(&game);
    game_init(&game,(uint32_t)time(NULL)+game_random(&ai_rng));selected=-1;animating=false;animation=0;ai_timer=.75f;memset(logs,0,sizeof(logs));
    if(!formation_count){
        if(last_formation<0){FILE *f=fopen("reports/last_formation.txt","r");if(f){if(fscanf(f,"%d",&last_formation)!=1||last_formation<0||last_formation>=AI_FORMATIONS)last_formation=-1;fclose(f);}}
        for(int i=0;i<AI_FORMATIONS;i++)formation_bag[i]=i;
        for(int i=AI_FORMATIONS-1;i>0;i--){int j=game_random(&game.rng)%(i+1);int t=formation_bag[i];formation_bag[i]=formation_bag[j];formation_bag[j]=t;}
        if(formation_bag[AI_FORMATIONS-1]==last_formation){int t=formation_bag[0];formation_bag[0]=last_formation;formation_bag[AI_FORMATIONS-1]=t;}
        formation_count=AI_FORMATIONS;
    }
    last_formation=formation_bag[--formation_count];ai_deploy_template(&game,COMPUTER,last_formation);
    if(!DirectoryExists("reports"))MakeDirectory("reports");
    FILE *formation_file=fopen("reports/last_formation.txt","w");if(formation_file){fprintf(formation_file,"%d\n",last_formation);fclose(formation_file);}
    reveal_reset(&reveal_window);
    tray_side=COMPUTER;tray_journal=false;latest_lost[0]=latest_lost[1]=-1;
    resign_confirm=resigned=end_dismissed=false;end_time=0;deployment_ready=false;reserve_rank=-1;deployment_status="";
    push_log("Votre armee attend vos ordres.");
}
static void start_move(Move m) {pending=m;animation=0;animating=true;selected=-1;if(audio_ok&&!muted)PlaySound(click_sound);}
static void finish_move(void) {
    int side=game.turn;game_apply(&game,pending);reveal_observe(&reveal_window,&game);char text[110];
    match_log_move(&game,pending);
    char from[4],to[4];snprintf(from,sizeof(from),"%c%d",'A'+pending.from%10,10-pending.from/10);snprintf(to,sizeof(to),"%c%d",'A'+pending.to%10,10-pending.to/10);
    if(game.combat!=2){
        last_combat_ply=game.ply;
        if(game.combat<=0)latest_lost[side]=game.attack_rank;
        if(game.combat>=0)latest_lost[1-side]=game.defend_rank;
        snprintf(text,sizeof(text),"%s %s > %s : %s",side==HUMAN?"Vous :":"IA :",from,to,game.combat>0?"victoire":(game.combat<0?"perte":"egalite"));
        if(audio_ok&&!muted)PlaySound(combat_sound);
    } else snprintf(text,sizeof(text),"%s  %s > %s",side==HUMAN?"Votre mouvement":"Mouvement adverse",from,to);
    push_log(text);animating=false;ai_timer=.85f;
}
static int pick_square(void) {
    Vector2 p={mouse.x-24,mouse.y-142};if(p.x<0||p.y<0||p.x>=SW||p.y>=SH)return -1;
    Ray ray=GetScreenToWorldRayEx(p,camera,SW,SH);
    if(fabsf(ray.direction.y)<.0001f)return -1;
    float t=(.075f-ray.position.y)/ray.direction.y;
    Vector3 hit=Vector3Add(ray.position,Vector3Scale(ray.direction,t));
    int x=(int)floorf(hit.x+5),z=(int)floorf(hit.z+5);
    return t>0&&x>=0&&x<10&&z>=0&&z<10?z*10+x:-1;
}
static void draw_piece_pose(Piece p,Vector3 pos,bool hidden,float angle,float scale) {
    if(scale<.01f)return;
    Color color=p.side==HUMAN?BLUE_TEAM:REDTEAM;
    Vector3 size={scale,scale,scale};
    DrawModelEx(pieces[12],pos,(Vector3){0,1,0},angle,size,color);
    /* Never submit a hidden rank's emblem, even when the camera rotates behind it. */
    if(!hidden){
        DrawModelEx(piece_plate,pos,(Vector3){0,1,0},angle,size,p.side==HUMAN?(Color){227,232,220,255}:(Color){240,201,114,255});
        DrawModelEx(piece_faces[p.rank],pos,(Vector3){0,1,0},angle,size,color);
    }
}
static void draw_piece(Piece p,Vector3 pos,bool hidden){draw_piece_pose(p,pos,hidden,hidden?180:0,1);}
static void create_captured_icons(void){
    Camera3D portrait={.position={0,1.35f,4},.target={0,.55f,0},.up={0,1,0},.fovy=1.27f,.projection=CAMERA_ORTHOGRAPHIC};
    Shader studio[]={lighting,metal_lighting,matte_lighting};
    for(int i=0;i<3;i++)SetShaderValue(studio[i],GetShaderLocation(studio[i],"eyePosition"),&portrait.position,SHADER_UNIFORM_VEC3);
    for(int side=0;side<2;side++)for(int rank=0;rank<12;rank++){
        captured_icons[side][rank]=LoadRenderTexture(96,128);
        BeginTextureMode(captured_icons[side][rank]);ClearBackground(BLANK);BeginMode3D(portrait);
        draw_piece_pose((Piece){rank,side,0,true,false},(Vector3){0,0,0},false,0,1);
        EndMode3D();EndTextureMode();SetTextureFilter(captured_icons[side][rank].texture,TEXTURE_FILTER_BILINEAR);
    }
}
static void captured_tray(float x,bool ended){
    if(button("Pieces sorties",(Rectangle){x,544,148,30},!tray_journal,true))tray_journal=false;
    if(button("Journal",(Rectangle){x+156,544,148,30},tray_journal,true))tray_journal=true;
    if(tray_journal){
        float y=594;
        for(int i=0;i<5&&y<=795;i++)if(logs[i][0]){
            const char *text=logs[i];
            while(*text&&y<=795) {
                char line[110];snprintf(line,sizeof(line),"%s",text);
                int length=(int)strlen(line);
                while(length>1&&MeasureTextEx(regular,line,18,.35f).x>304)line[--length]='\0';
                if(text[length]) {
                    int word=length;while(word>0&&text[word]!=' ')word--;
                    if(word>0){length=word;line[length]='\0';}
                }
                label(line,x,y,18,i==0?INK:MUTED,false);y+=21;
                text+=length;while(*text==' ')text++;
            }
            y+=10;
        }
    }else{
        int totals[2]={0};for(int side=0;side<2;side++)for(int r=0;r<12;r++)totals[side]+=game.captured[side][r];
        if(button(TextFormat("Bleues  %d",totals[HUMAN]),(Rectangle){x,582,148,30},tray_side==HUMAN,true))tray_side=HUMAN;
        if(button(TextFormat("Rouges  %d",totals[COMPUTER]),(Rectangle){x+156,582,148,30},tray_side==COMPUTER,true))tray_side=COMPUTER;
        const int order[12]={MARSHAL,GENERAL,COLONEL,MAJOR,CAPTAIN,LIEUTENANT,SERGEANT,MINER,SCOUT,SPY,BOMB,FLAG};
        int hovered=-1;
        for(int i=0;i<12;i++){
            int r=order[i],lost=game.captured[tray_side][r];Rectangle slot={x+(i%3)*103,620+(i/3)*45,98,41};
            bool hover=CheckCollisionPointRec(mouse,slot);if(hover)hovered=r;
            DrawRectangleRec(slot,hover?(Color){40,48,53,255}:(Color){13,21,28,255});
            DrawRectangleLinesEx(slot,1,Fade(BRASS,hover?.65f:.18f));
            if(r==latest_lost[tray_side])DrawRectangleLinesEx(slot,1,BRASS);
            Texture2D icon=captured_icons[tray_side][r].texture;
            DrawTexturePro(icon,(Rectangle){0,0,96,-128},(Rectangle){slot.x+2,slot.y+1,29,39},(Vector2){0,0},0,Fade(WHITE,lost?1:.22f));
            label(rank_symbols[r],slot.x+35,slot.y,22,INK,true);
            label(TextFormat("%d/%d",lost,army_counts[r]),slot.x+35,slot.y+22,18,lost?BRASS:INK,true);
        }
        if(!ended){
            if(hovered>=0)label(TextFormat("%s : %d restant%s",rank_names[hovered],army_counts[hovered]-game.captured[tray_side][hovered],army_counts[hovered]-game.captured[tray_side][hovered]>1?"s":""),x,815,14,INK,false);
            else label("Sorties / total. Survolez un grade.",x,815,13,MUTED,false);
        }
    }
    if(ended&&button("Rejouer",(Rectangle){x,808,304,30},true,true)){reset_game();phase=1;}
}
static void draw_move_animation(void) {
    Piece a=game.board[pending.from];Vector3 from=square_pos(pending.from),to=square_pos(pending.to);
    if(!combat_animation()){
        float t=Clamp(animation/.42f,0,1);Vector3 pos=Vector3Lerp(from,to,ease(t));pos.y+=sinf(t*PI)*.30f;
        draw_piece(a,pos,face_hidden(a));return;
    }
    Piece d=game.board[pending.to];int result=combat_result(a.rank,d.rank);
    Vector3 direction=Vector3Normalize(Vector3Subtract(to,from));
    Vector3 stop=Vector3Subtract(to,Vector3Scale(direction,.88f));
    Vector3 pos=Vector3Lerp(from,stop,ease(animation/.30f));
    float reveal=ease((animation-.30f)/.45f),vanish=ease((animation-1.65f)/.40f),arrive=ease((animation-2.05f)/.50f);
    bool opening=animation>=.30f;
    bool ah=face_hidden(a),dh=face_hidden(d);
    if(result>0)pos=Vector3Lerp(pos,to,arrive);
    float as=result<=0?1-vanish:1,ds=result>=0?1-vanish:1;
    draw_piece_pose(a,pos,ah&&!opening,ah?180*(1-reveal):0,as);
    draw_piece_pose(d,to,dh&&!opening,dh?180*(1-reveal):0,ds);
}
static void draw_scene(RenderTexture2D target) {
    Shader studio[]={lighting,metal_lighting,matte_lighting,wood_lighting,cloth_lighting};
    for(int i=0;i<5;i++)SetShaderValue(studio[i],GetShaderLocation(studio[i],"eyePosition"),&camera.position,SHADER_UNIFORM_VEC3);
    BeginTextureMode(target);ClearBackground(BG);
    BeginMode2D((Camera2D){.zoom=2});
    draw_background(-24,-142);
    DrawRectangle(0,0,SW,SH,Fade(BG,.24f));
    EndMode2D();BeginMode3D(camera);
    DrawModel(table_model,(Vector3){0,-.83f,0},1,(Color){94,59,37,255});
    rlDrawRenderBatchActive();rlDisableDepthMask();
    DrawModelEx(contact_shadow,(Vector3){.13f,-.474f,.17f},(Vector3){0,1,0},0,(Vector3){9.4f,1,13.6f},Fade(WHITE,.9f));
    rlDrawRenderBatchActive();rlEnableDepthMask();
    DrawModel(board_model,(Vector3){0,0,0},1,(Color){119,78,45,255});DrawModel(inlay,(Vector3){0,0,0},1,BRASS);
    int hovered=help?-1:pick_square();
    for(int s=0;s<100;s++) {
        Vector3 p=square_pos(s);
        if(is_lake(s))continue;
        Color tile=(s%10+s/10)%2?(Color){102,113,98,255}:(Color){123,134,115,255};
        if(phase==1&&s>=60)tile=(s%10+s/10)%2?(Color){137,135,107,255}:(Color){155,151,122,255};
        DrawModelEx(tile_model,(Vector3){p.x,.018f,p.z},(Vector3){0,1,0},0,(Vector3){.978f,.10f,.978f},tile);
        if(game.last_move.to==s||game.last_move.from==s)DrawCube((Vector3){p.x,.074f,p.z},.95f,.012f,.95f,(Color){181,159,91,255});
        if(selected==s)DrawCube((Vector3){p.x,.087f,p.z},.96f,.018f,.96f,BRASS);
        bool legal=selected>=0&&phase==2&&game_legal(&game,(Move){selected,s},HUMAN);
        if(legal) {
            DrawCylinder((Vector3){p.x,.09f,p.z},game.board[s].side==COMPUTER?.42f:.12f,game.board[s].side==COMPUTER?.42f:.12f,.012f,24,game.board[s].side==COMPUTER?(Color){228,136,100,255}:BRASS);
        }
        if(s==hovered&&phase>0&&(!is_lake(s)))DrawCubeWires((Vector3){p.x,.10f,p.z},.94f,.02f,.94f,IVORY);
    }
    for(int lake=0;lake<2;lake++) {
        float x=lake?2:-2;
        DrawCube((Vector3){x,-.015f,0},1.96f,.10f,1.96f,(Color){43,92,104,255});
        for(int i=0;i<8;i++) {
            float z=-.8f+i*.22f;float offset=sinf((float)GetTime()*.7f+i)*.09f;
            DrawLine3D((Vector3){x-.72f+offset,.047f,z},(Vector3){x+.55f+offset,.047f,z},(Color){74,125,133,255});
        }
    }
    rlDrawRenderBatchActive();rlDisableDepthMask();
    for(int s=0;s<100;s++) {
        Piece p=game.board[s];if(p.side<0||(animating&&s==pending.from))continue;
        Vector3 shadow=square_pos(s);shadow.y=.108f;DrawModel(contact_shadow,shadow,1,WHITE);
    }
    if(animating){float t=Clamp(animation/move_duration(),0,1);Vector3 shadow=Vector3Lerp(square_pos(pending.from),square_pos(pending.to),ease(t));shadow.y=.108f;DrawModel(contact_shadow,shadow,1+sinf(t*PI)*.2f,Fade(WHITE,1-sinf(t*PI)*.3f));}
    rlEnableDepthMask();
    for(int s=0;s<100;s++) {
        Piece p=game.board[s];if(p.side<0||(animating&&s==pending.from))continue;
        if(combat_animation()&&s==pending.to)continue;
        draw_piece(p,square_pos(s),face_hidden(p));
    }
    if(animating)draw_move_animation();
    EndMode3D();
    BeginMode2D((Camera2D){.zoom=2});
    /* Screen-facing rank markers preserve readability at any camera angle. */
    for(int s=0;s<100;s++) {
        Piece p=game.board[s];if(p.side<0||(animating&&s==pending.from))continue;
        Vector3 pos=square_pos(s);pos.y+=1.1f;Vector2 xy=GetWorldToScreenEx(pos,camera,SW,SH);
        bool known=!face_hidden(p);
        /* Grade lives on the model's face. Show an aid only for the selected unit. */
        if(known&&selected==s){DrawCircleV(xy,19,(Color){24,34,50,255});center(rank_symbols[p.rank],xy.x,xy.y-14,28,IVORY);}
        if(reveal_recent(&reveal_window,&game,p)&&p.side==HUMAN)DrawCircle((int)xy.x+9,(int)xy.y+7,2,BRASS);
    }
    for(int x=0;x<10;x++) {
        Vector2 xy=GetWorldToScreenEx((Vector3){x-4.5f,.14f,5.34f},camera,SW,SH);center(TextFormat("%c",'A'+x),xy.x,xy.y,14,BRASS);
        xy=GetWorldToScreenEx((Vector3){-5.34f,.14f,x-4.5f},camera,SW,SH);center(TextFormat("%d",10-x),xy.x,xy.y-8,14,BRASS);
    }
    EndMode2D();EndTextureMode();
}
static int remaining(int side) {int n=0;for(int s=0;s<100;s++)n+=game.board[s].side==side;return n;}
static void help_overlay(void) {
    DrawRectangle(0,0,VW,VH,(Color){5,10,14,225});
    float dx=(VW-1440)/2.0f;mouse.x-=dx;
    BeginMode2D((Camera2D){.offset={dx,0},.zoom=1});
    ui_panel((Rectangle){255,125,930,650},true);
    label("LE MANUEL DU STRATEGE",300,160,15,BRASS,true);title("Chaque information compte.",300,195,30,INK);
    const char *lines[]={"OBJECTIF   Capturer le drapeau ou immobiliser toute l'armee adverse.","PLACEMENT   Cliquez deux de vos pieces pour echanger leurs positions.","MOUVEMENT   Une case horizontale ou verticale. Aucun saut ni diagonale.","ECLAIREUR (2)   Traverse et attaque en ligne droite sur les cases libres.","COMBAT   Le rang le plus fort gagne. A egalite, les deux pieces tombent.","DEMINEUR (3)   Seul a desamorcer les bombes. Les autres attaquants meurent.","ESPION (1)   Elimine le marechal (10) uniquement lorsqu'il attaque.","DRAPEAU / BOMBE   Ne bougent jamais. Les lacs sont infranchissables.","RENSEIGNEMENT   Rang visible pendant le combat et le demi-tour suivant.","REPETITION   Un quatrieme trajet consecutif entre deux cases est interdit.","NULLE   Deux armees immobilisees ou accord des deux joueurs (ISF 12.3)."};
    for(int i=0;i<11;i++)label(lines[i],300,260+i*35,18,i%2?MUTED:INK,false);
    label("Clic droit : orbite / Molette : zoom cible / Clic molette : deplacer la vue",300,635,18,BRASS,false);
    label("C : vue initiale / Echap : fermer l'aide ou annuler / M : son",300,665,18,MUTED,false);
    if(button("Reprendre",(Rectangle){945,710,195,42},true,true))help=false;
    EndMode2D();mouse.x+=dx;
}
static void interface(void) {
    ClearBackground(BG);
    draw_background(0,0);
    DrawRectangle(0,0,VW,VH,Fade(BG,.24f));
    DrawRectangleGradientV(0,0,VW,142,Fade(BG,.97f),Fade(BG,.84f));
    DrawRectangleGradientV(0,846,VW,54,Fade(BG,.7f),Fade(BG,.94f));
    emblem((Rectangle){22,16,83,83},1);
    label("LES GUERRES DE L'EMPIRE",120,24,12,BRASS,true);
    title("STRATEGO",116,44,42,INK);
    label(phase==2?"Chaque coup compte.":"L'audace. La ruse. La victoire.",410,67,16,MUTED,false);
    if(button("?  Regles",(Rectangle){VW-376,47,168,43},false,true))help=true;
    if(button("Nouvelle partie",(Rectangle){VW-196,47,172,43},false,true)){reset_game();phase=1;}
    ui_rule(24,111,VW-48,BRASS);
    label(phase==0?"LE CHAMP DE BATAILLE":(phase==1?"01   DEPLOYEZ VOS FORCES":"02   LA BATAILLE"),28,122,13,BRASS,true);
    if(phase>0)label("VOTRE ARMEE  /  BLEUS                       ADVERSAIRE  /  ROUGES",627,123,11,MUTED,false);
    ui_panel((Rectangle){VW-376,142,352,704},true);
    if(phase==0) {
        DrawRectangleGradientH(0,143,SW,702,Fade(BG,.72f),Fade(BG,.02f));
        float x=84;
        emblem((Rectangle){x-5,196,154,154},.95f);
        label("ENTREZ DANS L'HISTOIRE",x,374,14,BRASS,true);
        title("L'ART DE",x-4,409,64,INK);
        title("LA GUERRE",x-4,480,64,INK);
        ui_rule(x,574,360,BRASS);
        label("Deux armees. Un drapeau.",x,601,24,INK,false);
        label("Une seule decision peut changer la bataille.",x,642,18,MUTED,false);
        label("STRATEGIE  /  BLUFF  /  CONQUETE",x,755,12,BRASS,true);
    }
}
static void deployment_board(void){
    ui_panel((Rectangle){24,142,SW,SH},false);
    const float bx=24+(SW-600)/2.0f,by=185,cell=60;
    center("VOTRE PLAN DE BATAILLE",24+SW/2.0f,150,20,BRASS);
    int hovered=-1;
    for(int s=0;s<100;s++){
        Rectangle box={bx+(s%10)*cell,by+(s/10)*cell,cell-2,cell-2};
        Color color=is_lake(s)?(Color){30,66,80,255}:s>=60?((s+s/10)%2?(Color){75,84,76,255}:(Color){91,99,87,255}):(Color){32,43,48,255};
        DrawRectangleRec(box,color);
        DrawRectangleLinesEx(box,1,Fade(BRASS,s>=60?.3f:.12f));
        if(s>=60&&CheckCollisionPointRec(mouse,box))hovered=s;
        if(s==selected)DrawRectangleLinesEx(box,3,BRASS);
        if(s>=60&&game.board[s].side==HUMAN){
            int r=game.board[s].rank;Texture2D icon=captured_icons[HUMAN][r].texture;
            DrawTexturePro(icon,(Rectangle){0,0,96,-128},(Rectangle){box.x+2,box.y+2,36,52},(Vector2){0,0},0,WHITE);
            label(rank_symbols[r],box.x+35,box.y+17,22,INK,true);
        }
    }
    center("ZONE ADVERSE",24+SW/2.0f,281,22,MUTED);
    for(int i=0;i<10;i++){center(TextFormat("%c",'A'+i),bx+i*cell+29,by+605,17,BRASS);label(TextFormat("%d",10-i),bx-31,by+i*cell+21,18,BRASS,true);}
    center("Placez vos pieces dans les quatre rangees claires.",24+SW/2.0f,817,17,INK);
    if(hovered>=0&&!help){
        Rectangle box={bx+(hovered%10)*cell,by+(hovered/10)*cell,cell-2,cell-2};DrawRectangleLinesEx(box,2,IVORY);
        if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){deployment_remove(&deployment,&game,hovered);selected=-1;}
        else if(clicked){
            if(reserve_rank>=0){deployment_place(&deployment,&game,reserve_rank,hovered);if(!deployment_count(&deployment,reserve_rank))reserve_rank=-1;}
            else if(selected>=0){Piece p=game.board[hovered];game.board[hovered]=game.board[selected];game.board[selected]=p;selected=-1;}
            else if(game.board[hovered].side==HUMAN)selected=hovered;
        }
    }
}
static void side_panel(void) {
    float x=VW-352;
    if(phase==0) {
        label("VOTRE QUARTIER GENERAL",x,180,12,BRASS,true);
        title("Prenez le",x,222,29,INK);title("commandement",x,262,26,INK);
        ui_rule(x,315,304,BRASS);
        label("40 pieces sous vos ordres.",x,341,20,INK,false);
        label("Observez. Sondez. Avancez.",x,375,18,MUTED,false);
        label("Protegez votre drapeau et percez",x,432,17,MUTED,false);label("les lignes de votre adversaire.",x,459,17,MUTED,false);
        label("CHOISIR VOTRE ADVERSAIRE",x,515,12,BRASS,true);
        if(button(ai_model_name(difficulty),(Rectangle){x,545,304,47},false,true))difficulty=ai_model_next(difficulty,ml_ready());
        label(ai_model_description(difficulty),x,608,14,MUTED,false);
        label("Cliquer pour changer d'adversaire.",x,639,13,MUTED,false);
        if(button("Preparer mon armee",(Rectangle){x,688,304,55},true,true))phase=1;
        ui_rule(x+42,773,220,BRASS);
        center("LA VICTOIRE SE PREPARE ICI",x+152,797,11,BRASS);
    } else if(phase==1) {
        label("ORDRE DE BATAILLE",x,176,13,BRASS,true);
        title(TextFormat("%d / 40 placees",40-deployment_count(&deployment,-1)),x,209,28,INK);
        label("Choisissez un grade, puis",x,252,17,INK,false);
        label("une case de votre camp.",x,277,17,INK,false);
        const int order[12]={10,9,8,7,6,5,4,3,2,1,11,0};
        for(int i=0;i<12;i++){
            int r=order[i],n=deployment_count(&deployment,r);
            Rectangle cell={x+(i%3)*103,322+(i/3)*65,98,59};
            if(button("",cell,reserve_rank==r,n>0)){reserve_rank=r;selected=-1;}
            Texture2D icon=captured_icons[HUMAN][r].texture;
            DrawTexturePro(icon,(Rectangle){0,0,96,-128},(Rectangle){cell.x+3,cell.y+2,37,51},(Vector2){0,0},0,Fade(WHITE,n?1:.25f));
            label(rank_symbols[r],cell.x+45,cell.y+3,22,n?INK:MUTED,true);
            label(TextFormat("x%d",n),cell.x+45,cell.y+31,20,n?INK:MUTED,true);
        }
        label(reserve_rank>=0?rank_names[reserve_rank]:"Cliquez une piece pour deplacer",x,588,15,INK,true);
        label("Clic droit : remettre en reserve",x,612,15,MUTED,false);
        if(button("Vider",(Rectangle){x,653,96,42},false,true)){deployment_clear(&deployment,&game);selected=reserve_rank=-1;}
        if(button("Melanger",(Rectangle){x+104,653,200,42},false,true)){
            game_deploy(&game,HUMAN);for(int i=0;i<40;i++)deployment.reserve[i]=empty_piece();selected=reserve_rank=-1;
        }
        if(button("Sauvegarder",(Rectangle){x,704,148,42},false,deployment_complete(&deployment,&game))){
            if(!DirectoryExists("saves"))MakeDirectory("saves");
            deployment_status=deployment_save(&deployment,&game,deployment_path)?"Placement sauvegarde.":"Echec de la sauvegarde.";
        }
        if(button("Charger",(Rectangle){x+156,704,148,42},false,FileExists(deployment_path))){
            if(deployment_load(&deployment,&game,deployment_path)){selected=reserve_rank=-1;deployment_status="Placement charge et modifiable.";}
            else deployment_status="Sauvegarde illisible ou invalide.";
        }
        if(button("Engager la bataille",(Rectangle){x,760,304,48},true,deployment_complete(&deployment,&game))){phase=2;selected=-1;push_log("Vous jouez le premier coup.");game_check_end(&game);match_log_begin(&game,difficulty);}
        label(*deployment_status?deployment_status:"Placez les 40 pieces pour jouer.",x,820,14,MUTED,false);
    } else {
        bool ended=game.winner>=0;
        label(ended?"FIN DE LA BATAILLE":"SITUATION TACTIQUE",x,170,13,BRASS,true);
        label(combat_animation()?(animation<.75f?"Revelation des grades":animation<1.65f?"Face a face":"Resolution du combat"):ended?(game.winner==GAME_DRAW?"Partie nulle":game.winner==HUMAN?"Victoire !":"Defaite"):game.turn==HUMAN?"A vous de jouer":"L'IA reflechit...",x,207,combat_animation()?25:30,INK,true);
        label(ended?end_reason_text():TextFormat("Tour %02d / %s",game.ply/2+1,ai_model_name(difficulty)),x,251,16,MUTED,false);
        ui_rule(x,287,304,BRASS);
        label("VOS FORCES",x,308,12,MUTED,true);label("ADVERSAIRE",x+170,308,12,MUTED,true);
        title(TextFormat("%02d",remaining(HUMAN)),x,328,44,IVORY);title(TextFormat("%02d",remaining(COMPUTER)),x+170,328,44,(Color){228,130,110,255});
        label("pieces en jeu",x,377,14,MUTED,false);label("pieces en jeu",x+170,377,14,MUTED,false);
        if(combat_animation()&&animation>=.75f) {
            Piece a=game.board[pending.from],d=game.board[pending.to];
            label("ATTAQUANT",x,411,12,BRASS,true);label(rank_names[a.rank],x,433,23,INK,true);center(rank_symbols[a.rank],x+280,428,34,BRASS);
            label("DEFENSEUR",x,473,12,BRASS,true);label(rank_names[d.rank],x,495,23,INK,true);center(rank_symbols[d.rank],x+280,490,34,BRASS);
            if(animation>=1.65f){int r=combat_result(a.rank,d.rank);label(r>0?"L'attaquant prend la case.":r<0?"Le defenseur tient sa position.":"Egalite : les deux sont retires.",x,526,16,BRASS,false);}
        } else if(selected>=0) {
            Piece p=game.board[selected];label("UNITE SELECTIONNEE",x,426,12,BRASS,true);label(rank_names[p.rank],x,451,27,INK,true);
            label(!movable(p)?"Cette piece est immobile.":"Cliquez une destination doree.",x,492,16,MUTED,false);
        } else {label(ended?"La bataille est terminee.":"Selectionnez une de vos pieces.",x,443,17,INK,false);label(ended?"Tous les rangs sont reveles.":"Les rangs ennemis restent caches.",x,475,16,MUTED,false);}
        captured_tray(x,ended);
    }
}
static const char *end_reason_text(void){
    if(game.end_reason==END_AGREEMENT)return "Nulle par accord mutuel.";
    if(game.end_reason==END_BOTH_IMMOBILE)return "Deux armees immobilisees.";
    if(game.end_reason==END_PLAYING_PERIOD)return "Duree de jeu ecoulee.";
    if(game.end_reason==END_RESIGNATION)return "Vous avez capitule.";
    if(game.end_reason==END_FLAG)return "Le drapeau a ete capture.";
    return game.winner==HUMAN?"L'adversaire ne peut plus jouer.":"Vous ne pouvez plus jouer.";
}
static void end_overlay_content(void){
    if(phase!=2)return;
    if(ai_draw_pending){
        ui_panel((Rectangle){232,292,600,272},true);
        center("L'IA propose une partie nulle",532,350,30,INK);
        center("Accepter termine la partie sans vainqueur.",532,412,18,MUTED);
        if(button("Refuser",(Rectangle){312,478,210,48},true,true)){
            ai_draw_pending=false;push_log("Vous refusez la nulle : la partie continue.");
        }
        if(button("Accepter",(Rectangle){542,478,210,48},false,true)){
            ai_draw_pending=false;
            if(game_agree_draw(&game,true,true)){
                selected=-1;push_log("Vous acceptez : partie nulle par accord.");match_log_close(&game);
            }
        }
        return;
    }
    if(draw_confirm){
        ui_panel((Rectangle){232,292,600,272},true);
        center("Proposer une partie nulle ?",532,350,32,INK);
        center("L'IA peut accepter ou refuser votre proposition.",532,412,18,MUTED);
        if(button("Continuer",(Rectangle){312,478,210,48},true,true))draw_confirm=false;
        if(button("Proposer",(Rectangle){542,478,210,48},false,true)){
            draw_confirm=false;last_draw_offer=game.ply;
            if(ai_accept_draw(&game,COMPUTER)&&game_agree_draw(&game,true,true)){
                selected=-1;push_log("L'IA accepte : partie nulle par accord.");match_log_close(&game);
            }else push_log("L'IA refuse la nulle : la partie continue.");
        }
        return;
    }
    if(resign_confirm){
        ui_panel((Rectangle){232,292,600,272},true);
        center("Capituler ?",532,350,38,INK);
        center("La victoire sera accordee a l'ordinateur.",532,412,19,MUTED);
        if(button("Continuer",(Rectangle){312,478,210,48},true,true))resign_confirm=false;
        if(button("Capituler",(Rectangle){542,478,210,48},false,true)){
            resigned=game_resign(&game,HUMAN);resign_confirm=false;selected=-1;
            push_log("Vous capitulez. Victoire de l'IA.");match_log_close(&game);
        }
        return;
    }
    if(game.winner<0||end_dismissed)return;
    bool won=game.winner==HUMAN,draw=game.winner==GAME_DRAW;
    float t=ease(end_time/1.2f),y=320-45*t;
    Color accent=draw?MUTED:won?BRASS:(Color){228,130,110,255};
    ui_panel((Rectangle){202,185,660,540},true);
    BeginScissorMode(24+(VW-1440)/2,142,1016,SH);
    for(int i=0;i<52;i++){
        float px=40+(i*137)%980,py=142+fmodf(i*83+end_time*(won?52:25),704);
        float alpha=t*(end_time<5?1:Clamp((7-end_time)/2,0,1));
        if(won)DrawRectanglePro((Rectangle){px,py,5,10},(Vector2){2,5},i*31+end_time*75,Fade(i%2?BRASS:IVORY,alpha));
        else if(!draw)DrawLineEx((Vector2){px,py},(Vector2){px-8,py+22},2,Fade(accent,.3f*alpha));
    }
    /* A raised standard for victory, a lowered white standard for surrender. */
    float pole=won?y+15:y+45+20*t;
    DrawLineEx((Vector2){505,y-48},(Vector2){505,y+72},4,Fade(INK,t));
    DrawTriangle((Vector2){508,pole-42},(Vector2){508,pole+1},(Vector2){572,pole-20+4*sinf(end_time*4)},Fade(won?BRASS:IVORY,t));
    center(draw?"PARTIE NULLE":won?"VICTOIRE":"DEFAITE",532,y+95,52,Fade(accent,t));
    const char *reason=end_reason_text();
    center(reason,532,y+165,22,Fade(INK,t));
    if(game.end_reason==END_IMMOBILE)center("Aucun coup legal pour l'armee perdante.",532,y+200,18,Fade(MUTED,t));
    if(end_time>.8f&&button("Voir le plateau",(Rectangle){417,y+260,230,45},false,true))end_dismissed=true;
    EndScissorMode();
}
static void end_overlay(void) {
    if(phase!=2||(!ai_draw_pending&&!draw_confirm&&!resign_confirm&&(game.winner<0||end_dismissed)))return;
    DrawRectangle(24,142,SW,SH,Fade(BG,.78f));
    float dx=(VW-1440)/2.0f;mouse.x-=dx;
    BeginMode2D((Camera2D){.offset={dx,0},.zoom=1});
    end_overlay_content();
    EndMode2D();mouse.x+=dx;
}
int main(int argc,char **argv) {
    bool journal_demo=false;for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--journal-demo"))journal_demo=true;
    bool ui_help_demo=false,ui_hover_demo=false;
    for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--ui-help-demo"))ui_help_demo=true;if(!strcmp(argv[i],"--ui-hover-demo"))ui_hover_demo=true;}
    bool quit_demo=false;for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--quit-demo"))quit_demo=true;
    bool deployment_demo=false;for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--deployment-demo"))deployment_demo=true;
    bool resign_demo=false,immobile_demo=false,draw_demo=false;
    bool models_demo=false;for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--models-demo"))models_demo=true;
    int ai_draw_demo=0;
    for(int i=1;i<argc;i++)if(sscanf(argv[i],"--ai-draw-demo=%d",&ai_draw_demo)==1&&(ai_draw_demo<1||ai_draw_demo>2))return 4;
    for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--resign-demo"))resign_demo=true;if(!strcmp(argv[i],"--immobile-demo"))immobile_demo=true;if(!strcmp(argv[i],"--draw-demo"))draw_demo=true;}
    bool smoke=false,smoke_battle=false,closeup=false;for(int i=1;i<argc;i++){if(!strcmp(argv[i],"--smoke"))smoke=true;if(!strcmp(argv[i],"--battle"))smoke_battle=true;if(!strcmp(argv[i],"--closeup"))closeup=true;}
    if(journal_demo)smoke=smoke_battle=true;
    if(resign_demo||immobile_demo||deployment_demo||draw_demo||ai_draw_demo||models_demo||ui_help_demo||ui_hover_demo)smoke=true;
    bool combat_demo=false,reveal_captured=false;int demo_a=6,demo_d=5,demo_side=HUMAN;Game expected_demo;
    for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--combat-enemy"))demo_side=COMPUTER;
    for(int i=1;i<argc;i++)if(sscanf(argv[i],"--combat-demo=%d,%d",&demo_a,&demo_d)==2){if(demo_a<1||demo_a>10||demo_d<0||demo_d>11)return 4;combat_demo=smoke=true;}
    const char *appdir=GetApplicationDirectory();if(!DirectoryExists("assets/models"))ChangeDirectory(appdir);
    ml_init("assets/models/selfplay.policy");
    for(int i=1;i<argc;i++)if(!strcmp(argv[i],"--ml")&&ml_ready())difficulty=2;
    unsigned int window_flags=FLAG_MSAA_4X_HINT|FLAG_WINDOW_RESIZABLE;
    /* Windows GLFW sizes are already pixels: HIGHDPI would scale the canvas twice. */
#if defined(__APPLE__)
    window_flags|=FLAG_WINDOW_HIGHDPI;
#endif
    SetConfigFlags(window_flags);InitWindow(VW,VH,"Stratego 3D | Atelier de strategie");SetWindowMinSize(1024,640);SetTargetFPS(60);SetExitKey(KEY_NULL);
    toggle_fullscreen();
    if(quit_demo&&!IsWindowFullscreen()){CloseWindow();return 4;}
    regular=interface_font(false);
    bold=interface_font(true);
    imperial=LoadFontEx("assets/fonts/Cinzel-Bold.ttf",112,NULL,0);
    SetTextureFilter(imperial.texture,TEXTURE_FILTER_BILINEAR);
    imperial_emblem=LoadTexture("assets/ui/imperial-emblem.png");
    if(imperial_emblem.id){GenTextureMipmaps(&imperial_emblem);SetTextureFilter(imperial_emblem,TEXTURE_FILTER_TRILINEAR);}
    SetTextureFilter(regular.texture,TEXTURE_FILTER_BILINEAR);SetTextureFilter(bold.texture,TEXTURE_FILTER_BILINEAR);
    lighting=studio_shader(.36f,0);metal_lighting=studio_shader(.42f,.3f);matte_lighting=studio_shader(.85f,0);
    if(!IsShaderValid(lighting)||!IsShaderValid(metal_lighting)||!IsShaderValid(matte_lighting)){TraceLog(LOG_ERROR,"Lighting shader failed");CloseWindow();return 1;}
    lighting.locs[SHADER_LOC_MATRIX_MODEL]=GetShaderLocation(lighting,"matModel");
    for(int i=0;i<13;i++){pieces[i]=LoadModel(TextFormat("assets/models/piece_%02d.obj",i));if(!IsModelValid(pieces[i])){TraceLog(LOG_ERROR,"Missing piece assets; run tools/create_assets.py with Blender");CloseWindow();return 1;}apply_shader(&pieces[i]);}
    piece_plate=LoadModel("assets/models/piece_plate.obj");if(!IsModelValid(piece_plate)){CloseWindow();return 1;}apply_shader(&piece_plate);
    for(int i=0;i<12;i++){piece_faces[i]=LoadModel(TextFormat("assets/models/piece_face_%02d.obj",i));if(!IsModelValid(piece_faces[i])){CloseWindow();return 1;}apply_shader(&piece_faces[i]);}
    board_model=LoadModel("assets/models/board.obj");inlay=LoadModel("assets/models/inlay.obj");apply_shader(&board_model);apply_shader(&inlay);
    for(int i=0;i<board_model.materialCount;i++)board_model.materials[i].shader=matte_lighting;
    for(int i=0;i<inlay.materialCount;i++)inlay.materials[i].shader=metal_lighting;
    for(int i=0;i<piece_plate.materialCount;i++)piece_plate.materials[i].shader=metal_lighting;
    wood_lighting=studio_shader(.85f,0);cloth_lighting=studio_shader(.94f,0);
    float wood_kind=1,cloth_kind=2;
    SetShaderValue(wood_lighting,GetShaderLocation(wood_lighting,"surfaceKind"),&wood_kind,SHADER_UNIFORM_FLOAT);
    SetShaderValue(cloth_lighting,GetShaderLocation(cloth_lighting,"surfaceKind"),&cloth_kind,SHADER_UNIFORM_FLOAT);
    table_model=LoadModelFromMesh(GenMeshCube(13,.7f,14));table_model.materials[0].shader=wood_lighting;
    tile_model=LoadModelFromMesh(GenMeshCube(1,1,1));tile_model.materials[0].shader=cloth_lighting;
    for(int i=0;i<board_model.materialCount;i++)board_model.materials[i].shader=wood_lighting;
    battlefield_background=LoadTexture("assets/backgrounds/napoleonic-battlefield.png");
    if(battlefield_background.id)SetTextureFilter(battlefield_background,TEXTURE_FILTER_BILINEAR);
    create_contact_shadow();
    create_captured_icons();
    if(!IsModelValid(board_model)||!IsModelValid(inlay)){TraceLog(LOG_ERROR,"Missing board assets");CloseWindow();return 1;}
    InitAudioDevice();audio_ok=IsAudioDeviceReady();if(audio_ok){click_sound=tone(460,.14f);combat_sound=tone(135,.30f);}
    view_width=layout_width();
    RenderTexture2D canvas=LoadRenderTexture(VW,VH),scene=LoadRenderTexture(SW*2,SH*2);SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);SetTextureFilter(scene.texture,TEXTURE_FILTER_BILINEAR);
    reset_game();if(smoke)phase=smoke_battle?2:1;
    if(smoke) {
        set_camera();
        for(int s=0;s<100;s++) {
            Vector2 p=GetWorldToScreenEx(square_pos(s),camera,SW,SH);mouse=(Vector2){p.x+24,p.y+142};
            if(pick_square()!=s){TraceLog(LOG_ERROR,"Picking mismatch at square %d",s);CloseWindow();return 2;}
        }
        TraceLog(LOG_INFO,"SMOKE: all 100 projected squares pass mouse picking");
    }
    if(closeup){camera_focus=square_pos(74);camera_focus.y=0;zoom=1.6f;set_camera();}
    if(combat_demo){
        game_clear(&game);phase=2;
        game.board[99]=(Piece){FLAG,HUMAN,1,false,false};
        if(demo_d!=FLAG)game.board[0]=(Piece){FLAG,COMPUTER,2,false,false};
        game.board[9]=(Piece){SCOUT,COMPUTER,3,false,false};
        game.turn=demo_side;game.board[64]=(Piece){demo_a,demo_side,4,false,false};game.board[54]=(Piece){demo_d,1-demo_side,5,false,false};
        expected_demo=game;if(!game_apply(&expected_demo,(Move){64,54}))return 4;
        camera_focus=square_pos(54);camera_focus.z+=.45f;zoom=3.5f;elevation=.8f;set_camera();start_move((Move){64,54});
    }
    if(resign_demo){phase=2;resign_confirm=true;}
    if(immobile_demo){
        phase=2;
        for(int s=0;s<100;s++)if(game.board[s].side==HUMAN&&movable(game.board[s])){game.captured[HUMAN][game.board[s].rank]++;game.board[s]=(Piece){0,-1,0,false,false};}
        game_check_end(&game);
        if(game.winner!=COMPUTER)return 4;
    }
    if(draw_demo){game_clear(&game);phase=2;game.board[98]=(Piece){FLAG,HUMAN,1,false,false};game.board[8]=(Piece){FLAG,COMPUTER,2,false,false};game_check_end(&game);}
    int frames=0;
    if(models_demo)phase=0;
    if(journal_demo){tray_journal=true;push_log("Vous refusez la nulle : la partie continue.");}
    if(ui_help_demo||ui_hover_demo)phase=0;
    if(ui_help_demo)help=true;
    if(deployment_demo)deployment_path="reports/deployment_demo_save.txt";
    if(ai_draw_demo){phase=2;ai_draw_pending=true;last_ai_draw_offer=game.ply;}
    while(!quit_requested&&!WindowShouldClose()) {
        if(IsKeyPressed(KEY_F11)||(quit_demo&&(frames==3||frames==6)))toggle_fullscreen();
        if(quit_demo&&(frames==4||frames==9)) {
            bool expected_fullscreen=frames==9;
            int monitor=GetCurrentMonitor();
            int width=expected_fullscreen?GetMonitorWidth(monitor):windowed_width;
            int height=expected_fullscreen?GetMonitorHeight(monitor):windowed_height;
            if(IsWindowFullscreen()!=expected_fullscreen||GetScreenWidth()!=width||GetScreenHeight()!=height||GetRenderWidth()!=width||GetRenderHeight()!=height){TraceLog(LOG_ERROR,"Fullscreen viewport mismatch: fullscreen=%d screen=%dx%d render=%dx%d expected=%dx%d",IsWindowFullscreen(),GetScreenWidth(),GetScreenHeight(),GetRenderWidth(),GetRenderHeight(),width,height);CloseWindow();return 4;}
            TraceLog(LOG_INFO,"DISPLAY: fullscreen=%d screen=%dx%d render=%dx%d",IsWindowFullscreen(),GetScreenWidth(),GetScreenHeight(),GetRenderWidth(),GetRenderHeight());
        }
        int next_width=layout_width();
        if(next_width!=VW) {
            UnloadRenderTexture(canvas);UnloadRenderTexture(scene);view_width=next_width;
            canvas=LoadRenderTexture(VW,VH);scene=LoadRenderTexture(SW*2,SH*2);
            SetTextureFilter(canvas.texture,TEXTURE_FILTER_BILINEAR);SetTextureFilter(scene.texture,TEXTURE_FILTER_BILINEAR);
        }
        float dt=fminf(GetFrameTime(),.1f);float scale=fminf((float)GetScreenWidth()/VW,(float)GetScreenHeight()/VH);
        Vector2 offset={(GetScreenWidth()-VW*scale)/2,(GetScreenHeight()-VH*scale)/2};mouse=Vector2Scale(Vector2Subtract(GetMousePosition(),offset),1/scale);clicked=IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if(quit_demo&&frames==10){mouse=(Vector2){VW-100,870};clicked=true;}
        if(resign_demo&&frames==20){mouse=(Vector2){640+(VW-1440)/2.0f,500};clicked=true;}
        if(ui_hover_demo){mouse=(Vector2){VW-200,714};clicked=false;}
        if(models_demo&&(frames==5||frames==10||frames==15||(ml_ready()&&frames==20))){mouse=(Vector2){VW-210,565};clicked=true;}
        if(ai_draw_demo&&frames==30){mouse=(Vector2){(ai_draw_demo==1?640:410)+(VW-1440)/2.0f,500};clicked=true;}
        if(deployment_demo){
            if(frames==5){mouse=(Vector2){VW-310,340};clicked=true;}
            if(frames==10){mouse=(Vector2){252+(VW-1440)/2.0f,565};clicked=true;}
            if(frames==14&&(game.board[60].rank!=MARSHAL||deployment_count(&deployment,MARSHAL)!=0))return 4;
            if(frames==15){mouse=(Vector2){VW-160,670};clicked=true;}
            if(frames==16){mouse=(Vector2){VW-300,725};clicked=true;}
            if(frames==17){mouse=(Vector2){VW-310,670};clicked=true;}
            if(frames==18){mouse=(Vector2){VW-130,725};clicked=true;}
            if(frames==19&&!deployment_complete(&deployment,&game))return 4;
            if(frames==20){mouse=(Vector2){VW-240,785};clicked=true;}
        }
        if(IsKeyPressed(KEY_F1))help=!help;
        if(IsKeyPressed(KEY_ESCAPE)){if(help)help=false;else if(ai_draw_pending){ai_draw_pending=false;push_log("Vous refusez la nulle : la partie continue.");}else if(resign_confirm)resign_confirm=false;else if(draw_confirm)draw_confirm=false;else if(game.winner>=0)end_dismissed=true;else {selected=-1;reserve_rank=-1;}}
        if(IsKeyPressed(KEY_M))muted=!muted;
        if(IsKeyPressed(KEY_C)){azimuth=0;elevation=1.02f;zoom=15.6f;camera_focus=(Vector3){0,0,0};}
        set_camera();
        if(!help&&phase!=1&&CheckCollisionPointRec(mouse,(Rectangle){24,142,SW,SH})) {
            float wheel=GetMouseWheelMove();
            if(wheel!=0) {
                Vector3 before=mouse_on_board(mouse);
                zoom=Clamp(zoom*powf(.84f,wheel),1.6f,22.0f);
                set_camera();
                shift_focus(Vector3Subtract(before,mouse_on_board(mouse)));
            }
            if(IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
                Vector2 previous=Vector2Subtract(mouse,Vector2Scale(GetMouseDelta(),1/scale));
                shift_focus(Vector3Subtract(mouse_on_board(previous),mouse_on_board(mouse)));
            }
            if(IsMouseButtonDown(MOUSE_BUTTON_RIGHT)){Vector2 delta=GetMouseDelta();azimuth-=delta.x*.006f;elevation=Clamp(elevation+delta.y*.004f,.52f,1.55f);}
        }
        set_camera();
        if(!help&&!resign_confirm&&!draw_confirm&&!ai_draw_pending) {
            if(animating){animation+=dt;if(animation>=move_duration())finish_move();}
            else if(!combat_demo&&phase==2&&game.winner<0&&game.turn==COMPUTER){
                int latest_offer=last_ai_draw_offer>last_draw_offer?last_ai_draw_offer:last_draw_offer;
                if(!ai_worker_busy()&&ai_offer_draw(&game,COMPUTER,last_combat_ply,latest_offer)){
                    ai_draw_pending=true;last_ai_draw_offer=game.ply;clicked=false;
                    push_log("L'IA vous propose une partie nulle.");
                }else{
                ai_timer-=dt;
                if(!ai_worker_busy()&&!ai_worker_start(&game,difficulty,ai_rng)){
                    Move m=ai_model_choose(&game,difficulty,&ai_rng);if(m.from>=0)start_move(m);else game_check_end(&game);
                } else if(ai_timer<=0){Move m;if(ai_worker_poll(&m,&ai_rng)){if(m.from>=0)start_move(m);else game_check_end(&game);}}
                }
            }
            else if(clicked&&phase==2&&game.winner<0) {
                int s=pick_square();
                if(s>=0&&!is_lake(s)) {
                    if(phase==1&&game.board[s].side==HUMAN){if(selected>=0&&selected!=s){Piece t=game.board[s];game.board[s]=game.board[selected];game.board[selected]=t;selected=-1;}else selected=selected==s?-1:s;}
                    else if(phase==2&&game.turn==HUMAN){if(selected>=0&&game_legal(&game,(Move){selected,s},HUMAN))start_move((Move){selected,s});else selected=game.board[s].side==HUMAN&&s!=selected?s:-1;}
                }
            }
        }
        if(phase==2&&game.winner>=0&&!help)end_time+=dt;
        if(!combat_demo&&smoke&&smoke_battle&&!animating&&game.winner<0&&game.turn==HUMAN&&frames<110){Move m=ai_choose(&game,1,&ai_rng);if(m.from>=0)start_move(m);}
        if(phase==1&&!deployment_ready){deployment_begin(&deployment,&game);deployment_ready=true;}
        draw_scene(scene);
        BeginTextureMode(canvas);
        bool raw_click=clicked;if(help||resign_confirm||draw_confirm||ai_draw_pending)clicked=false;
        interface();if(phase==2)DrawTexturePro(scene.texture,(Rectangle){0,0,SW*2,-SH*2},(Rectangle){24,142,SW,SH},(Vector2){0,0},0,WHITE);if(phase==1)deployment_board();side_panel();
        if(phase==2&&game.winner<0&&button("Proposer nulle",(Rectangle){650,48,190,42},false,!animating&&game.turn==HUMAN&&game.ply-last_draw_offer>=20)){draw_confirm=true;raw_click=false;}
        if(phase==2&&game.winner<0&&button("Capituler",(Rectangle){850,48,200,42},false,!animating)){resign_confirm=true;raw_click=false;}
        label("CLIC  Selection / deplacement",28,864,14,MUTED,false);label("DROIT  Orbite   MOLETTE  Zoom / glisser   C  Recentrer",374,864,14,MUTED,false);
        label(muted?"M  Son coupe":"M  Son actif",VW-460,864,14,BRASS,false);label("F11  Ecran",VW-315,864,14,MUTED,false);
        clicked=help?false:raw_click;end_overlay();clicked=raw_click;if(help)help_overlay();
        if(button("Quitter",(Rectangle){VW-180,852,156,36},false,true))quit_requested=true;
        EndTextureMode();
        BeginDrawing();ClearBackground(BLACK);DrawTexturePro(canvas.texture,(Rectangle){0,0,VW,-VH},(Rectangle){offset.x,offset.y,VW*scale,VH*scale},(Vector2){0,0},0,WHITE);
        if(quit_demo&&frames==9) {
            /* Capture the actual framebuffer, including final scaling and letterboxing. */
            rlDrawRenderBatchActive();
            Image display={rlReadScreenPixels(GetRenderWidth(),GetRenderHeight()),GetRenderWidth(),GetRenderHeight(),1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
            bool saved=ExportImage(display,"reports/fullscreen-display.png");UnloadImage(display);
            if(!saved){CloseWindow();return 4;}
        }
        EndDrawing();
        if(quit_demo&&frames==10&&!quit_requested){TraceLog(LOG_ERROR,"Quit button did not close the game");CloseWindow();return 4;}
        if(combat_demo&&!reveal_captured&&animation>=.9f){
            if(game.ply!=0||game.board[64].side!=demo_side||game.board[54].side!=1-demo_side){TraceLog(LOG_ERROR,"Combat committed before reveal");return 4;}
            Image capture=LoadImageFromTexture(canvas.texture);ImageFlipVertical(&capture);bool saved=ExportImage(capture,"combat_reveal.png");UnloadImage(capture);if(!saved)return 4;reveal_captured=true;
        }
        if(ai_draw_demo&&frames==10){
            if(!ai_draw_pending||game.ply!=0||animating)return 4;
            Image capture=LoadImageFromTexture(canvas.texture);ImageFlipVertical(&capture);
            bool saved=ExportImage(capture,"ai_draw_offer.png");UnloadImage(capture);if(!saved)return 4;
        }
        frames++;if(smoke&&frames==(ai_draw_demo?31:combat_demo?240:150)){
            if(models_demo&&(phase!=0||difficulty!=AI_IMPROVED))return 4;
            if(ai_draw_demo&&(ai_draw_pending||game.ply!=0||animating||(ai_draw_demo==1?(game.winner!=GAME_DRAW||game.end_reason!=END_AGREEMENT):game.winner!=GAME_ONGOING)))return 4;
            if(deployment_demo){
                if(phase!=2||!deployment_complete(&deployment,&game)||game.ply!=0)return 4;
                remove(deployment_path);
            }
            if(draw_demo&&(game.winner!=GAME_DRAW||game.end_reason!=END_BOTH_IMMOBILE))return 4;
            if((resign_demo||immobile_demo)&&(game.winner!=COMPUTER||game.ply!=0||animating||(resign_demo&&!resigned)))return 4;
            if(combat_demo){
                bool valid=reveal_captured&&!animating&&game.ply==1&&game.turn==expected_demo.turn&&game.winner==expected_demo.winner&&game.combat==expected_demo.combat&&!memcmp(game.captured,expected_demo.captured,sizeof(game.captured));
                for(int s=0;s<100;s++){Piece a=game.board[s],b=expected_demo.board[s];if(a.rank!=b.rank||a.side!=b.side||a.id!=b.id||a.revealed!=b.revealed||a.moved!=b.moved)valid=false;}
                if(!valid){TraceLog(LOG_ERROR,"Combat animation result differs from rules");return 4;}
            }
            /* Read the actual canvas: raylib 5.5 TakeScreenshot applies Windows
               DPI twice and can add uninitialized borders to the exported PNG. */
            Image capture=LoadImageFromTexture(canvas.texture);ImageFlipVertical(&capture);
            bool exported=ExportImage(capture,draw_demo?"draw_result.png":resign_demo?"resign_result.png":immobile_demo?"immobile_result.png":combat_demo?"combat_result.png":closeup?"closeup.png":(smoke_battle?"battle.png":"preview.png"));UnloadImage(capture);
            if(!exported){TraceLog(LOG_ERROR,"Cannot export smoke capture");CloseWindow();return 3;}
            TraceLog(LOG_INFO,"SMOKE: rendered %d frames, %d completed moves",frames,game.ply);break;
        }
    }
    ai_worker_stop();
    if(quit_demo)TraceLog(LOG_INFO,"QUIT: fullscreen launch and exit button passed (%d frames)",frames);
    match_log_close(&game);
    for(int side=0;side<2;side++)for(int r=0;r<12;r++)UnloadRenderTexture(captured_icons[side][r]);
    UnloadModel(table_model);UnloadModel(tile_model);UnloadShader(wood_lighting);UnloadShader(cloth_lighting);
    if(battlefield_background.id)UnloadTexture(battlefield_background);
    if(imperial_emblem.id)UnloadTexture(imperial_emblem);
    if(imperial.texture.id!=GetFontDefault().texture.id)UnloadFont(imperial);
    UnloadRenderTexture(canvas);UnloadRenderTexture(scene);for(int i=0;i<13;i++)UnloadModel(pieces[i]);for(int i=0;i<12;i++)UnloadModel(piece_faces[i]);UnloadModel(piece_plate);UnloadModel(board_model);UnloadModel(inlay);UnloadModel(contact_shadow);UnloadTexture(shadow_texture);UnloadShader(lighting);UnloadShader(metal_lighting);UnloadShader(matte_lighting);
    if(regular.texture.id!=GetFontDefault().texture.id)UnloadFont(regular);
    if(bold.texture.id!=GetFontDefault().texture.id)UnloadFont(bold);
    if(audio_ok){UnloadSound(click_sound);UnloadSound(combat_sound);CloseAudioDevice();}CloseWindow();return 0;
}



