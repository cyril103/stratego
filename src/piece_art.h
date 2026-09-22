#ifndef STRATEGO_PIECE_ART_H
#define STRATEGO_PIECE_ART_H

static RenderTexture2D piece_paint[12];

static bool piece_art_load(Model faces[12],Shader shader,Font rank_font) {
    Texture2D atlas=LoadTexture("assets/pieces/napoleonic-ranks.png");
    if(!atlas.id)return false;
    SetTextureFilter(atlas,TEXTURE_FILTER_BILINEAR);
    const int width=384,height=576;
    for(int rank=0;rank<12;rank++) {
        piece_paint[rank]=LoadRenderTexture(width,height);
        if(!IsRenderTextureValid(piece_paint[rank])){UnloadTexture(atlas);return false;}
        /* Measured panel boundaries of the generated atlas, normalized for scaling. */
        const float columns[]={0,.245f,.500f,.750f,1};
        const float rows[]={0,.333f,.647f,1};
        int column=rank%4,row=rank/4;
        Rectangle source={columns[column]*atlas.width+2,rows[row]*atlas.height+2,
                          (columns[column+1]-columns[column])*atlas.width-4,
                          (rows[row+1]-rows[row])*atlas.height-4};
        BeginTextureMode(piece_paint[rank]);ClearBackground((Color){14,19,24,255});
        float fit=fminf((width-16)/source.width,(height-152)/source.height);
        Rectangle art={(width-source.width*fit)/2,146,source.width*fit,source.height*fit};
        DrawTexturePro(atlas,source,art,(Vector2){0},0,WHITE);
        /* Typeset ranks independently of the artwork: exact, large and legible. */
        DrawRectangle(0,0,width,140,(Color){14,19,24,255});
        Vector2 text_size=MeasureTextEx(rank_font,rank_symbols[rank],142,0);
        DrawTextEx(rank_font,rank_symbols[rank],(Vector2){(width-text_size.x)/2,-3},142,0,(Color){255,238,190,255});
        DrawRectangle(36,141,width-72,3,(Color){191,146,70,255});
        DrawRectangleLinesEx((Rectangle){5,5,width-10,height-10},3,(Color){181,133,62,255});
        EndTextureMode();
        GenTextureMipmaps(&piece_paint[rank].texture);
        SetTextureFilter(piece_paint[rank].texture,TEXTURE_FILTER_ANISOTROPIC_8X);
        SetTextureWrap(piece_paint[rank].texture,TEXTURE_WRAP_CLAMP);

        Mesh mesh={0};mesh.vertexCount=6;mesh.triangleCount=2;
        mesh.vertices=MemAlloc(18*sizeof(float));mesh.normals=MemAlloc(18*sizeof(float));mesh.texcoords=MemAlloc(12*sizeof(float));
        /* Leave enough depth separation from the bevel to avoid distant z-fighting. */
        const float vertices[]={-.225f,.225f,.160f, .225f,.225f,.160f, .225f,.915f,.160f,
                                -.225f,.225f,.160f, .225f,.915f,.160f, -.225f,.915f,.160f};
        /* Render textures have their vertical axis inverted. */
        const float uv[]={0,0,1,0,1,1,0,0,1,1,0,1};
        memcpy(mesh.vertices,vertices,sizeof(vertices));memcpy(mesh.texcoords,uv,sizeof(uv));
        for(int i=0;i<6;i++){mesh.normals[i*3]=0;mesh.normals[i*3+1]=0;mesh.normals[i*3+2]=1;}
        UploadMesh(&mesh,false);faces[rank]=LoadModelFromMesh(mesh);
        faces[rank].materials[0].shader=shader;
        faces[rank].materials[0].maps[MATERIAL_MAP_DIFFUSE].texture=piece_paint[rank].texture;
    }
    UnloadTexture(atlas);return true;
}
static void piece_art_unload(void) {
    for(int rank=0;rank<12;rank++)UnloadRenderTexture(piece_paint[rank]);
}
#endif
