#ifndef STRATEGO_UI_THEME_H
#define STRATEGO_UI_THEME_H

/* Resolution-independent brass, enamel and engraved rules for the imperial UI. */
static void ui_diamond(float x,float y,float radius,Color color) {
    DrawPoly((Vector2){x,y},4,radius,0,color);
}
static void ui_rule(float x,float y,float width,Color color) {
    DrawLineEx((Vector2){x,y},(Vector2){x+width/2-9,y},1,Fade(color,.55f));
    ui_diamond(x+width/2,y,3,color);
    DrawLineEx((Vector2){x+width/2+9,y},(Vector2){x+width,y},1,Fade(color,.55f));
}
static void ui_panel(Rectangle r,bool ornate) {
    Color edge={155,121,65,255};
    DrawRectangleRec((Rectangle){r.x+5,r.y+8,r.width,r.height},(Color){0,0,0,85});
    DrawRectangleGradientV((int)r.x,(int)r.y,(int)r.width,(int)r.height,(Color){27,37,44,249},(Color){10,18,25,252});
    for(int i=12;i<r.height-8;i+=5)
        DrawLine((int)r.x+8,(int)r.y+i,(int)(r.x+r.width)-8,(int)r.y+i,(Color){202,186,144,3});
    DrawRectangleLinesEx(r,1,Fade(edge,.72f));
    DrawRectangleLinesEx((Rectangle){r.x+5,r.y+5,r.width-10,r.height-10},1,Fade(edge,.22f));
    if(!ornate)return;
    for(int i=0;i<4;i++) {
        float x=(i%2)?r.x+r.width-1:r.x+1,y=(i/2)?r.y+r.height-1:r.y+1;
        float dx=(i%2)?-1:1,dy=(i/2)?-1:1;
        DrawLineEx((Vector2){x,y},(Vector2){x+dx*25,y},2,edge);
        DrawLineEx((Vector2){x,y},(Vector2){x,y+dy*25},2,edge);
        ui_diamond(x+dx*9,y+dy*9,3,edge);
    }
    ui_rule(r.x+r.width*.23f,r.y+14,r.width*.54f,edge);
}
static float ui_hover(const char *text,Rectangle r,bool hover) {
    typedef struct { unsigned int key;float value; } Hover;
    static Hover states[128];
    unsigned int key=2166136261u;
    for(const unsigned char *p=(const unsigned char *)text;*p;p++)key=(key^*p)*16777619u;
    key^=(unsigned int)r.x*31u+(unsigned int)r.y*131u+(unsigned int)r.width*17u;
    if(!key)key=1;
    unsigned int slot=key%128;
    for(int i=0;i<128;i++,slot=(slot+1)%128)if(!states[slot].key||states[slot].key==key)break;
    if(states[slot].key!=key)states[slot]=(Hover){key,0};
    float step=fminf(GetFrameTime()*12,1);
    states[slot].value+=((hover?1.0f:0.0f)-states[slot].value)*step;
    return states[slot].value;
}
static void ui_button_skin(Rectangle r,bool primary,bool enabled,float hover,bool pressed) {
    Color gold={187,147,79,255};
    Color top=primary?(Color){143,105,50,255}:(Color){36,49,58,255};
    Color bottom=primary?(Color){72,49,25,255}:(Color){14,23,31,255};
    if(!enabled){top=(Color){33,37,39,255};bottom=(Color){22,26,29,255};gold=(Color){78,77,66,255};}
    DrawRectangleRec((Rectangle){r.x+2,r.y+3,r.width,r.height},(Color){0,0,0,100});
    DrawRectangleGradientV((int)r.x,(int)r.y,(int)r.width,(int)r.height,pressed?bottom:top,pressed?top:bottom);
    if(enabled)DrawRectangleRec(r,Fade((Color){222,179,101,255},hover*.16f));
    DrawRectangleLinesEx(r,1,enabled?ColorLerp(gold,(Color){255,227,163,255},hover):gold);
    DrawRectangleLinesEx((Rectangle){r.x+3,r.y+3,r.width-6,r.height-6},1,Fade(gold,primary?.5f:.22f));
    DrawLineEx((Vector2){r.x+5,r.y+1},(Vector2){r.x+r.width-5,r.y+1},1,Fade((Color){255,231,180,255},enabled?.35f:.08f));
    if(r.width>135) {
        ui_diamond(r.x+12,r.y+r.height/2,2,Fade(gold,.8f));
        ui_diamond(r.x+r.width-12,r.y+r.height/2,2,Fade(gold,.8f));
    }
    if(hover>.05f)DrawLineEx((Vector2){r.x+r.width*.25f,r.y+r.height-1},(Vector2){r.x+r.width*.75f,r.y+r.height-1},2,Fade(gold,hover));
}
#endif
