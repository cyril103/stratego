"""Render the actual Blender mesh library for visual review."""
import bpy
from mathutils import Vector
from pathlib import Path
root=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(root/'assets/stratego.blend'))
for name in ['board','inlay','piece_12']:
    bpy.data.objects[name].hide_render=True
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,8,-.012))
floor=bpy.context.object;floor.name='Fond studio'
material=bpy.data.materials.new('Fond bleu nuit');material.diffuse_color=(.025,.04,.06,1);floor.data.materials.append(material)
bpy.ops.object.camera_add(location=(.0,-3.8,8.5))
camera=bpy.context.object;camera.name='Catalogue des grades';target=Vector((0,7.85,.52))
camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler();camera.data.type='ORTHO';camera.data.ortho_scale=7.3
bpy.context.scene.camera=camera
for loc,power,size in [((-4,3,8),1000,7),((4,6,5),700,5),((0,12,6),900,4)]:
    bpy.ops.object.light_add(type='AREA',location=loc);lamp=bpy.context.object;lamp.data.energy=power;lamp.data.size=size
    lamp.rotation_euler=(target-lamp.location).to_track_quat('-Z','Y').to_euler()
scene=bpy.context.scene;scene.render.engine='BLENDER_EEVEE';scene.eevee.use_gtao=True;scene.eevee.gtao_distance=3
scene.eevee.taa_render_samples=64;scene.world.color=(.18,.18,.18)
scene.render.resolution_x=1600;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
scene.view_settings.view_transform='Standard';scene.view_settings.look='Medium High Contrast';scene.view_settings.exposure=0
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(root/'assets/pieces_catalog.png')
bpy.ops.render.render(write_still=True)
# Preserve the editable working library separately from the presentation setup.
bpy.ops.wm.save_as_mainfile(filepath=str(root/'assets/pieces_catalog.blend'))
