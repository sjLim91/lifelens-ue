"""Bounded unmodified-source review; no anatomy, materials or weights are edited."""
import json
import math
from pathlib import Path
import bpy
from mathutils import Vector

ROOT=Path('docs/asset-reviews/distributed-tigers').resolve()
bpy.context.preferences.filepaths.save_version=0
for source in sorted(ROOT.glob('*/source.glb')):
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    for action in list(bpy.data.actions):bpy.data.actions.remove(action)
    bpy.ops.import_scene.gltf(filepath=str(source))
    scene=bpy.context.scene;scene.frame_set(1)
    meshes=[obj for obj in scene.objects if obj.type=='MESH']
    rigs=[obj for obj in scene.objects if obj.type=='ARMATURE']
    if not meshes:raise ValueError('Candidate has no meshes')
    def bounds():
        dg=bpy.context.evaluated_depsgraph_get()
        points=[obj.matrix_world@Vector(p) for item in meshes for obj in [item.evaluated_get(dg)] for p in obj.bound_box]
        return Vector([min(p[i] for p in points) for i in range(3)]),Vector([max(p[i] for p in points) for i in range(3)])
    low,high=bounds();center=(low+high)/2;extent=max(high-low)
    if extent<=0:raise ValueError('Candidate has zero extent')
    scene.render.engine='CYCLES';scene.cycles.samples=8;scene.cycles.use_denoising=False
    scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
    scene.render.image_settings.file_format='PNG';scene.world.color=(.15,.15,.15)
    scene.view_settings.view_transform='Standard'
    bpy.ops.object.camera_add();camera=bpy.context.object;scene.camera=camera
    camera.data.type='ORTHO';camera.data.ortho_scale=extent*1.18
    camera.data.clip_end=max(1000,extent*20)
    for relative,energy in [((2,-3,4),1100),((-3,1,3),700)]:
        bpy.ops.object.light_add(type='AREA',location=center+Vector(relative)*extent*.65)
        light=bpy.context.object;light.data.energy=energy*(extent/3)**2;light.data.size=extent*1.5
        light.rotation_euler=(center-light.location).to_track_quat('-Z','Y').to_euler()
    def capture(name,angle,focus=center,zoom=1.18):
        camera.data.ortho_scale=extent*zoom
        camera.location=focus+Vector((math.sin(angle),-math.cos(angle),.22))*extent
        camera.rotation_euler=(focus-camera.location).to_track_quat('-Z','Y').to_euler()
        scene.render.filepath=str(source.parent/(name+'.png'));bpy.ops.render.render(write_still=True)
    for name,angle in [('axis-front',0),('axis-side',math.pi/2),('axis-quarter',math.pi/4),('axis-back',math.pi)]:capture(name,angle)
    report={'status':'REQUIRES_VISUAL_REVIEW','runtimeAdmission':False,'bounds':[list(low),list(high)],
            'triangles':sum(len(p.vertices)-2 for obj in meshes for p in obj.data.polygons),
            'rigs':[{'name':rig.name,'bones':[b.name for b in rig.data.bones]} for rig in rigs],
            'actions':[{'name':a.name,'frames':list(a.frame_range)} for a in bpy.data.actions],
            'materials':[{'name':m.name,'textures':[n.image.name for n in m.node_tree.nodes if n.type=='TEX_IMAGE' and n.image]} for obj in meshes for m in obj.data.materials if m and m.use_nodes]}
    if bpy.data.actions:
        clip=next(iter(bpy.data.actions));first,last=clip.frame_range
        for index,t in enumerate([.25,.5,.75]):
            scene.frame_set(round(first+(last-first)*t));capture('authored-motion-'+str(index),math.pi/4)
    (source.parent/'inspection.json').write_text(json.dumps(report,indent=2)+'\n')
    print('TIGER_SOURCE_REVIEW',json.dumps(report))
