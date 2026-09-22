#ifndef STRATEGO_BATTLEFIELD_H
#define STRATEGO_BATTLEFIELD_H

static Shader field_shader[6];
static Texture2D field_texture[3];
static Model lake_water[2],lake_bank[2],lake_land[2];
static Model bank_stone,frame[5],ornament;
static Texture2D field_environment;
static int field_eye[6],field_time[6];

static float lake_radius(float angle,int lake) {
    float seed=lake?2.1f:.6f;
    return (.82f+.045f*sinf(3*angle+seed)+.025f*cosf(5*angle-seed))/powf(powf(fabsf(cosf(angle)),4)+powf(fabsf(sinf(angle)),4),.25f);
}
static void field_vertex(Mesh *mesh,int *index,Vector3 p,Vector3 n) {
    int i=(*index)++;
    mesh->vertices[i*3]=p.x;mesh->vertices[i*3+1]=p.y;mesh->vertices[i*3+2]=p.z;
    mesh->normals[i*3]=n.x;mesh->normals[i*3+1]=n.y;mesh->normals[i*3+2]=n.z;
    mesh->texcoords[i*2]=p.x;mesh->texcoords[i*2+1]=p.z;
}
static void field_triangle(Mesh *mesh,int *index,Vector3 a,Vector3 b,Vector3 c) {
    Vector3 n=Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b,a),Vector3Subtract(c,a)));
    field_vertex(mesh,index,a,n);field_vertex(mesh,index,b,n);field_vertex(mesh,index,c,n);
}
static Vector3 lake_point(float angle,int lake,int part,bool outer) {
    float r=lake_radius(angle,lake),y=.045f;
    if(part==0)r=outer?r:0;
    else if(part==1){r+=outer?.10f:0;y=outer?.069f:.045f;}
    else {r=outer?.999f/fmaxf(fabsf(cosf(angle)),fabsf(sinf(angle))):r+.10f;y=.069f;}
    return (Vector3){(lake?2:-2)+r*cosf(angle),y,r*sinf(angle)};
}
static Model lake_mesh(int lake,int part) {
    const int segments=128,rings=part==0?20:1;
    Mesh mesh={0};mesh.vertexCount=segments*rings*6;mesh.triangleCount=mesh.vertexCount/3;
    mesh.vertices=MemAlloc(mesh.vertexCount*3*sizeof(float));
    mesh.normals=MemAlloc(mesh.vertexCount*3*sizeof(float));
    mesh.texcoords=MemAlloc(mesh.vertexCount*2*sizeof(float));
    int index=0;
    for(int ring=0;ring<rings;ring++)for(int i=0;i<segments;i++) {
        float a=2*PI*i/segments,b=2*PI*(i+1)/segments;
        Vector3 inner_a=lake_point(a,lake,part,false),inner_b=lake_point(b,lake,part,false);
        Vector3 outer_a=lake_point(a,lake,part,true),outer_b=lake_point(b,lake,part,true);
        if(part==0){
            Vector3 center={(lake?2:-2),.045f,0};
            inner_a=Vector3Lerp(center,outer_a,(float)ring/rings);
            inner_b=Vector3Lerp(center,outer_b,(float)ring/rings);
            outer_a=Vector3Lerp(center,outer_a,(float)(ring+1)/rings);
            outer_b=Vector3Lerp(center,outer_b,(float)(ring+1)/rings);
        }
        field_triangle(&mesh,&index,inner_a,outer_b,outer_a);
        field_triangle(&mesh,&index,inner_a,inner_b,outer_b);
    }
    UploadMesh(&mesh,false);
    Model model=LoadModelFromMesh(mesh);
    int kind=part==0?2:part==1?1:0;
    model.materials[0].shader=field_shader[kind];
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=field_texture[kind==0?0:1];
    model.materials[0].maps[MATERIAL_MAP_SPECULAR].texture=field_texture[1];
    model.materials[0].maps[MATERIAL_MAP_NORMAL].texture=field_environment;
    return model;
}
static void field_material(Model *model,int kind) {
    model->materials[0].shader=field_shader[kind];
    model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=field_texture[kind==3?2:kind==0?0:1];
    model->materials[0].maps[MATERIAL_MAP_SPECULAR].texture=field_texture[1];
    model->materials[0].maps[MATERIAL_MAP_NORMAL].texture=field_environment;
}
static Vector3 frame_point(Vector2 profile,int corner){
    float h=profile.x,c=.22f;
    Vector2 points[8]={{h,h-c},{h-c,h},{-h+c,h},{-h,h-c},{-h,-h+c},{-h+c,-h},{h-c,-h},{h,-h+c}};
    return (Vector3){points[corner%8].x,profile.y,points[corner%8].y};
}
static Model frame_mesh(const Vector2 *profile,int count,int kind){
    Mesh mesh={0};mesh.vertexCount=(count-1)*8*6;mesh.triangleCount=mesh.vertexCount/3;
    mesh.vertices=MemAlloc(mesh.vertexCount*3*sizeof(float));mesh.normals=MemAlloc(mesh.vertexCount*3*sizeof(float));mesh.texcoords=MemAlloc(mesh.vertexCount*2*sizeof(float));
    int index=0;
    for(int r=0;r<count-1;r++)for(int i=0;i<8;i++){
        Vector3 a=frame_point(profile[r],i),b=frame_point(profile[r],i+1),c=frame_point(profile[r+1],i),d=frame_point(profile[r+1],i+1);
        field_triangle(&mesh,&index,a,d,c);field_triangle(&mesh,&index,a,b,d);
    }
    UploadMesh(&mesh,false);Model model=LoadModelFromMesh(mesh);field_material(&model,kind);return model;
}
static void ornament_append(Mesh *target,int *index,Mesh source,Vector3 position,float angle,Vector3 scale){
    for(int i=0;i<source.vertexCount;i++){
        Vector3 p={source.vertices[i*3]*scale.x,source.vertices[i*3+1]*scale.y,source.vertices[i*3+2]*scale.z};
        Vector3 n={source.normals[i*3]/scale.x,source.normals[i*3+1]/scale.y,source.normals[i*3+2]/scale.z};
        p=Vector3Add(position,Vector3RotateByAxisAngle(p,(Vector3){0,1,0},angle*DEG2RAD));
        n=Vector3Normalize(Vector3RotateByAxisAngle(n,(Vector3){0,1,0},angle*DEG2RAD));
        field_vertex(target,index,p,n);
    }
}
static Model imperial_ornaments(void){
    // Bake all 224 decorative elements once: a single draw call during play.
    Mesh source=GenMeshSphere(1,8,12),mesh={0};mesh.vertexCount=source.vertexCount*224;mesh.triangleCount=mesh.vertexCount/3;
    mesh.vertices=MemAlloc(mesh.vertexCount*3*sizeof(float));mesh.normals=MemAlloc(mesh.vertexCount*3*sizeof(float));mesh.texcoords=MemAlloc(mesh.vertexCount*2*sizeof(float));int index=0;
    for(int x=-1;x<=1;x+=2)for(int z=-1;z<=1;z+=2){
        Vector3 center={x*5.43f,.18f,z*5.43f};
        for(int j=0;j<8;j++){
            float a=j*PI/4;Vector3 p={center.x+.105f*cosf(a),center.y,center.z+.105f*sinf(a)};
            ornament_append(&mesh,&index,source,p,90-j*45,(Vector3){.038f,.025f,.09f});
        }
        ornament_append(&mesh,&index,source,center,0,(Vector3){.065f,.048f,.065f});
    }
    // A raised bead-and-reel molding, interrupted around the corner rosettes.
    for(int side=-1;side<=1;side+=2)for(int i=-23;i<=23;i++){
        float p=i*.22f;
        ornament_append(&mesh,&index,source,(Vector3){p,.148f,side*5.57f},0,(Vector3){.037f,.02f,.026f});
        ornament_append(&mesh,&index,source,(Vector3){side*5.57f,.148f,p},0,(Vector3){.026f,.02f,.037f});
    }
    UnloadMesh(source);UploadMesh(&mesh,false);Model model=LoadModelFromMesh(mesh);field_material(&model,4);return model;
}
static void imperial_frame_draw(void){
    for(int i=0;i<5;i++)DrawModel(frame[i],(Vector3){0},1,WHITE);
    DrawModel(ornament,(Vector3){0},1,WHITE);
}
static bool battlefield_load(void) {
    field_environment=LoadTexture("assets/lighting/studio_small_09_1k.hdr");
    if(!field_environment.id)return false;
    GenTextureMipmaps(&field_environment);SetTextureFilter(field_environment,TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(field_environment,TEXTURE_WRAP_REPEAT);
    const char *paths[]={"assets/textures/battlefield-meadow.png","assets/textures/lake-shore.png","assets/textures/campaign-walnut.png"};
    for(int i=0;i<3;i++) {
        field_texture[i]=LoadTexture(paths[i]);
        if(!field_texture[i].id)return false;
        GenTextureMipmaps(&field_texture[i]);SetTextureFilter(field_texture[i],TEXTURE_FILTER_ANISOTROPIC_8X);
        SetTextureWrap(field_texture[i],TEXTURE_WRAP_REPEAT);
    }
    for(int i=0;i<6;i++) {
        field_shader[i]=LoadShader("assets/shaders/battlefield.vs","assets/shaders/battlefield.fs");
        if(!IsShaderValid(field_shader[i])||field_shader[i].id==rlGetShaderIdDefault())return false;
        field_shader[i].locs[SHADER_LOC_MATRIX_MODEL]=GetShaderLocation(field_shader[i],"matModel");
        float kind=(float)i;SetShaderValue(field_shader[i],GetShaderLocation(field_shader[i],"materialKind"),&kind,SHADER_UNIFORM_FLOAT);
        field_eye[i]=GetShaderLocation(field_shader[i],"eyePosition");field_time[i]=GetShaderLocation(field_shader[i],"time");
    }
    for(int i=0;i<2;i++){lake_water[i]=lake_mesh(i,0);lake_bank[i]=lake_mesh(i,1);lake_land[i]=lake_mesh(i,2);}
    bank_stone=LoadModelFromMesh(GenMeshSphere(1,6,8));
    field_material(&bank_stone,1);
    const Vector2 wood[]={{5.02f,-.54f},{5.02f,.09f},{5.14f,.16f},{5.63f,.14f},{5.74f,.02f},{5.74f,-.42f},{5.64f,-.57f},{5.02f,-.57f},{5.02f,-.54f}};
    const Vector2 outer[]={{5.59f,.148f},{5.64f,.148f},{5.71f,.09f},{5.749f,.01f}};
    const Vector2 inner[]={{5.025f,.075f},{5.045f,.15f},{5.095f,.185f},{5.14f,.164f},{5.16f,.152f}};
    const Vector2 lower[]={{5.746f,-.34f},{5.80f,-.39f},{5.80f,-.43f},{5.72f,-.50f}};
    const Vector2 enamel[]={{5.744f,-.055f},{5.761f,-.10f},{5.761f,-.29f},{5.744f,-.34f}};
    frame[0]=frame_mesh(wood,9,3);frame[1]=frame_mesh(outer,4,4);frame[2]=frame_mesh(inner,5,4);frame[3]=frame_mesh(lower,4,4);frame[4]=frame_mesh(enamel,4,5);
    ornament=imperial_ornaments();
    return true;
}
static void battlefield_update(Vector3 eye) {
    float time=(float)GetTime();
    for(int i=0;i<6;i++){SetShaderValue(field_shader[i],field_eye[i],&eye,SHADER_UNIFORM_VEC3);SetShaderValue(field_shader[i],field_time[i],&time,SHADER_UNIFORM_FLOAT);}
}
static void battlefield_lakes(void) {
    for(int i=0;i<2;i++) {
        DrawModel(lake_land[i],(Vector3){0,0,0},1,WHITE);
        DrawModel(lake_bank[i],(Vector3){0,0,0},1,WHITE);
        DrawModel(lake_water[i],(Vector3){0,0,0},1,WHITE);
        for(int j=0;j<15;j++) {
            float a=j*2*PI/15+i*.29f,r=lake_radius(a,i)+.055f;
            Vector3 p={(i?2:-2)+r*cosf(a),.068f,r*sinf(a)};
            float size=.027f+.012f*(1+sinf(j*12.3f));
            DrawModelEx(bank_stone,p,(Vector3){0,1,0},j*37,(Vector3){size,size*.6f,size*.8f},(Color){185,178,153,255});
            if(j%3==0)for(int blade=0;blade<3;blade++) {
                float dx=(blade-1)*.018f,h=.09f+blade*.034f;
                Vector3 a0={p.x+dx-.007f,p.y,p.z},b={p.x+dx+.007f,p.y,p.z},tip={p.x+dx+.018f,p.y+h,p.z+.012f};
                DrawTriangle3D(a0,tip,b,(Color){96,102,50,255});DrawTriangle3D(b,tip,a0,(Color){76,88,43,255});
            }
        }
    }
}
static void battlefield_unload(void) {
    for(int i=0;i<2;i++){UnloadModel(lake_water[i]);UnloadModel(lake_bank[i]);UnloadModel(lake_land[i]);}
    UnloadModel(bank_stone);UnloadModel(ornament);for(int i=0;i<5;i++)UnloadModel(frame[i]);
    UnloadTexture(field_environment);
    for(int i=0;i<6;i++)UnloadShader(field_shader[i]);
    for(int i=0;i<3;i++)UnloadTexture(field_texture[i]);
}
#endif
