"""Run with Blender 2.93+ : blender --background --python tools/create_assets.py.
Original meshes, no downloaded artwork. Exports triangulated Y-up OBJ files.
"""
import bpy
import math
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'assets' / 'models'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def mat(name, color, metallic=0):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*color, 1)
    bsdf.inputs['Metallic'].default_value = metallic
    bsdf.inputs['Roughness'].default_value = .38
    return m

ivory = mat('Porcelaine ivoire', (.83, .75, .56))
wood = mat('Noyer', (.12, .065, .038))
gold = mat('Laiton', (.65, .44, .18), .65)

def cylinder(radius, depth, z, vertices=12):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=(0,0,z))
    return bpy.context.object

def cube(loc, scale):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o=bpy.context.object
    o.scale=scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return o

def sphere(radius,z):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=6, radius=radius, location=(0,0,z))
    return bpy.context.object

def export(name, objects, material, bevel_width=.025):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objects:o.select_set(True)
    bpy.context.view_layer.objects.active=objects[0]
    bpy.ops.object.join()
    o=bpy.context.object;o.name=name
    o.data.materials.clear();o.data.materials.append(material)
    if bevel_width:
        bevel=o.modifiers.new('Soft crafted edges','BEVEL');bevel.width=bevel_width;bevel.segments=2
        bpy.ops.object.modifier_apply(modifier=bevel.name)
    tri=o.modifiers.new('Triangles','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=tri.name)
    # Direct mesh export is stable across Blender versions; positions include matrix_world.
    with (OUT/(name+'.obj')).open('w') as f:
        f.write('# Original asset generated in Blender\n')
        for v in o.data.vertices:
            p=o.matrix_world @ v.co
            f.write('v %.6f %.6f %.6f\n'%(p.x,p.z,-p.y))
        normal_matrix=o.matrix_world.to_3x3().inverted().transposed()
        for face in o.data.polygons:
            n=(normal_matrix @ face.normal).normalized()
            f.write('vn %.6f %.6f %.6f\n'%(n.x,n.z,-n.y))
        for i,face in enumerate(o.data.polygons):
            f.write('f '+' '.join('%d//%d'%(v+1,i+1) for v in face.vertices)+'\n')
    return o

import sys
sys.path.insert(0, str(ROOT/'tools'))
from boardgame_pieces import build
build(export, cube, mat)

parts=[cube((0,0,-.26),(10.75,10.75,.42))]
for x in [-5.25,5.25]:parts.append(cube((x,0,-.015),(.25,10.6,.20)))
for y in [-5.25,5.25]:parts.append(cube((0,y,-.015),(10.6,.25,.20)))
export('board',parts,wood)
parts=[]
for x in [-5.08,5.08]:parts.append(cube((x,0,.095),(.025,10.2,.018)))
for y in [-5.08,5.08]:parts.append(cube((0,y,.095),(10.2,.025,.018)))
export('inlay',parts,gold)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'assets'/'stratego.blend'))
print('Stratego assets exported:', OUT)

