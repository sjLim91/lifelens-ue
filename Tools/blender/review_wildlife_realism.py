"""Same-lighting source/candidate comparisons; render actual GLB bytes."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import bpy
from mathutils import Vector


def render(source, output, views=('quarter', 'side')):
    bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
    bpy.ops.outliner.orphans_purge(do_recursive=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    slug = source.name.removesuffix('-rigged.glb').removesuffix('-mid.glb')
    for obj in bpy.context.scene.objects:
        if obj.animation_data:
            obj.animation_data.action = None
            for track in obj.animation_data.nla_tracks: track.mute = track.name != slug+'_idle'
    scene = bpy.context.scene
    scene.frame_set(0); bpy.context.view_layer.update()
    meshes = [o for o in scene.objects if o.type=='MESH' and not any(c.name.startswith('glTF_not_exported') for c in o.users_collection)]
    dg = bpy.context.evaluated_depsgraph_get()
    points = []
    for o in meshes:
        e = o.evaluated_get(dg); m = e.to_mesh()
        points.extend(o.matrix_world@v.co for v in m.vertices); e.to_mesh_clear()
    low = Vector([min(p[i] for p in points) for i in range(3)])
    high = Vector([max(p[i] for p in points) for i in range(3)])
    center = (low+high)/2; extent = max(high-low)
    scene.render.engine = 'CYCLES'; scene.cycles.samples = 48; scene.cycles.use_denoising = False
    scene.render.resolution_x = scene.render.resolution_y = 768
    scene.render.resolution_percentage = 100
    scene.world.use_nodes = True
    scene.world.node_tree.nodes.get('Background').inputs[0].default_value = (.20, .20, .20, 1)
    scene.world.node_tree.nodes.get('Background').inputs[1].default_value = .35
    scene.view_settings.view_transform = 'AgX'
    for pos, energy, size in [((3,-4,5), 650, 2.8), ((-3,-1,2), 220, 3), ((1,3,4), 500, 2)]:
        bpy.ops.object.light_add(type='AREA', location=center+Vector(pos)*extent/3)
        light = bpy.context.object; light.data.energy = energy*(extent/2)**2; light.data.size = extent*size/3
        light.rotation_euler = (center-light.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.ops.mesh.primitive_plane_add(size=extent*200, location=(0,0,low.z-.004*extent))
    mat = bpy.data.materials.new('NeutralReviewFloor'); mat.diffuse_color = (.08,.09,.095,1)
    bpy.context.object.data.materials.append(mat)
    bpy.ops.object.camera_add(); camera = bpy.context.object; scene.camera = camera
    camera.data.type = 'ORTHO'; camera.data.ortho_scale = extent*1.13
    camera.data.clip_start = extent*.001; camera.data.clip_end = extent*200
    output.mkdir(parents=True, exist_ok=True)
    for view in views:
        angle = {'quarter':math.pi/4,'side':math.pi/2,'front':0}[view]
        camera.location = center+Vector((math.sin(angle),-math.cos(angle),.18))*extent*2
        camera.rotation_euler = (center-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath = str(output/(view+'.png'))
        bpy.ops.render.render(write_still=True)
    (output/'receipt.json').write_text(json.dumps({'sourceSha256':hashlib.sha256(source.read_bytes()).hexdigest(),'views':list(views),'engine':'Cycles','samples':48,'lighting':'shared-neutral-v1','status':'VISUAL_REVIEW_REQUIRED'}, indent=2)+'\n')


if __name__=='__main__':
    import sys
    p = argparse.ArgumentParser(); p.add_argument('--source',type=Path,required=True); p.add_argument('--output',type=Path,required=True); p.add_argument('--view',default='quarter')
    a = p.parse_args(sys.argv[sys.argv.index('--')+1:]); render(a.source,a.output,tuple(a.view.split(',')))
