"""Original molded-tower pieces and single-face insignia, built in Blender."""
import bpy
import math
from pathlib import Path

def build(export, cube, mat):
    blue=mat('Plastique bleu classique',(.025,.11,.42))
    red=mat('Plastique rouge classique',(.65,.035,.045))
    silver=mat('Plaquette argent',(.87,.89,.83),.3)
    gold=mat('Plaquette or',(.90,.71,.31),.3)
    font_path=Path(__file__).resolve().parents[1]/'assets/fonts/Barlow-SemiBold.ttf'
    font=bpy.data.fonts.load(str(font_path))
    def silhouette(points, y=-.151, depth=.006):
        n=len(points);verts=[(x,y,z) for x,z in points]+[(x,y+depth,z) for x,z in points]
        faces=[tuple(range(n-1,-1,-1)),tuple(range(n,2*n))]
        faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
        mesh=bpy.data.meshes.new('Insigne');mesh.from_pydata(verts,[],faces);mesh.update()
        obj=bpy.data.objects.new('Insigne',mesh);bpy.context.collection.objects.link(obj)
        return obj
    def circle(x,z,r):return silhouette([(x+r*math.cos(t*math.tau/32),z+r*math.sin(t*math.tau/32)) for t in range(32)])
    def line(x1,z1,x2,z2,width=.015):
        length=math.hypot(x2-x1,z2-z1);dx=(z2-z1)/length*width/2;dz=-(x2-x1)/length*width/2
        return silhouette([(x1+dx,z1+dz),(x2+dx,z2+dz),(x2-dx,z2-dz),(x1-dx,z1-dz)])
    def text(body,x,z,size):
        bpy.ops.object.text_add(location=(x,-.155,z),rotation=(math.pi/2,0,0))
        obj=bpy.context.object;obj.data.body=body;obj.data.font=font;obj.data.size=size
        obj.data.align_x='CENTER';obj.data.extrude=.001;obj.data.resolution_u=4
        bpy.ops.object.convert(target='MESH');return bpy.context.object
    def body():
        parts=[cube((0,0,.07),(.74,.42,.14)),cube((0,0,.55),(.64,.24,.88))]
        for x in [-.30,.30]:parts.append(cube((x,0,.61),(.075,.29,.98)))
        for x in [-.20,0,.20]:parts.append(cube((x,0,1.025),(.12,.24,.13)))
        # A common back molding. All ranks have exactly this same silhouette.
        parts.append(cube((0,.132,.57),(.46,.025,.65)))
        return parts
    plate=export('piece_plate',[cube((0,-.127,.57),(.49,.035,.73))],silver,.012)
    plate.hide_render=True;plate.hide_set(True)
    names=['DRAPEAU','ESPION','ECLAIREUR','DEMINEUR','SERGENT','LIEUTENANT','CAPITAINE','COMMANDANT','COLONEL','GENERAL','MARECHAL','BOMBE']
    for rank in range(13):
        material=blue if rank<6 else red
        obj=export('piece_%02d'%rank,body(),material,.015)
        x=(rank%6)*1.05-2.65;y=7+(rank//6)*1.7
        obj.location.x+=x;obj.location.y+=y
        if rank==12:continue
        panel=plate.copy();panel.data=plate.data.copy();bpy.context.collection.objects.link(panel)
        panel.hide_render=False;panel.hide_set(False);panel.data.materials.clear();panel.data.materials.append(silver if rank<6 else gold)
        panel.location.x+=x;panel.location.y+=y
        mark=[text('D' if rank==0 else 'B' if rank==11 else str(rank),0,.765,.23),text(names[rank],0,.237,.051)]
        if rank==0:
            mark += [line(-.13,.31,-.13,.67,.018),silhouette([(-.12,.66),(.04,.68),(.17,.63),(.16,.45),(.025,.50),(-.12,.47)])]
        elif rank==11:
            mark += [circle(0,.45,.13),line(.04,.56,.075,.625,.028)]
            for i in range(7):
                t=i*math.tau/7;mark.append(line(.075+math.cos(t)*.025,.65+math.sin(t)*.025,.075+math.cos(t)*.066,.65+math.sin(t)*.066,.01))
        else:
            # Military head in profile, with grade-specific headdress.
            mark.append(silhouette([(-.09,.57),(.065,.57),(.085,.52),(.13,.495),(.085,.475),(.065,.405),(.025,.39),(.025,.355),(.15,.31),(-.16,.31),(-.095,.38),(-.10,.48)]))
            if rank==1:
                mark += [silhouette([(-.18,.56),(-.11,.595),(-.09,.68),(.065,.66),(.095,.59),(.18,.555)]),line(-.09,.50,.09,.50,.035)]
            elif rank in (2,3):
                mark.append(silhouette([(-.145,.565),(-.12,.63),(-.07,.67),(.055,.67),(.11,.61),(.16,.56)]))
                if rank==3:mark += [line(-.19,.33,.16,.67),line(-.18,.64,.17,.36)]
                else:mark.append(silhouette([(-.10,.64),(-.13,.73),(-.06,.715),(-.04,.65)]))
            elif rank>=9:
                mark.append(silhouette([(-.20,.575),(-.15,.66),(-.075,.625),(.035,.63),(.14,.685),(.205,.57)]))
                if rank==10:
                    mark += [circle(.09,.68,.027),circle(.11,.712,.023),circle(.13,.735,.018)]
            else:
                height=.035*(rank-4)
                mark.append(silhouette([(-.13,.575),(-.10,.62+height),(.07,.62+height),(.10,.59),(.17,.56)]))
                mark.append(line(-.13,.552,.13,.552,.016))
            mark.append(line(-.12,.345,.02,.305,.016))
        face=export('piece_face_%02d'%rank,mark,material,0)
        face.location.x+=x;face.location.y+=y
    # Store a convenient view of the complete editable library in the .blend.
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=='VIEW_3D':
                area.spaces.active.region_3d.view_distance=15
