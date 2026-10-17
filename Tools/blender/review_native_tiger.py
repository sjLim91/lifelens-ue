"""Render all authored native-skeleton states; preserve a packed Blender source."""
import json
import hashlib
import math
from pathlib import Path
import sys
import bpy
from mathutils import Vector

out=Path(sys.argv[sys.argv.index('--')+1]).resolve()
prior=out/'blender-review.json'
if prior.is_file():
 saved=json.loads(prior.read_text())
 names=['idle-front','idle-side','idle-quarter','idle-back','idle-face','idle-feet','walk','run','hunt','forage','drink','rest']
 if saved.get('sourceSha256')==hashlib.sha256((out/'tiger-rigged.glb').read_bytes()).hexdigest() and all((out/(name+'.png')).is_file() for name in names) and (out/'tiger-native-authoring.blend').is_file():
  print('REUSE_IDENTICAL_NATIVE_REVIEW');sys.exit(0)
bpy.context.preferences.filepaths.save_version=0
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
source_hash=hashlib.sha256((out/'tiger-rigged.glb').read_bytes()).hexdigest()
bpy.ops.import_scene.gltf(filepath=str(out/'tiger-rigged.glb'))
scene=bpy.context.scene
objects=list(scene.objects);meshes=[obj for obj in objects if obj.type=='MESH']
scene.render.engine='CYCLES';scene.cycles.samples=8;scene.cycles.use_denoising=False
scene.render.resolution_x=640;scene.render.resolution_y=640;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.world.color=(.15,.15,.15)
scene.view_settings.view_transform='Standard'
for position,energy in [((3,-4,5),500),((-3,2,4),350)]:
 bpy.ops.object.light_add(type='AREA',location=position);light=bpy.context.object
 light.data.energy=energy;light.data.size=4;light.rotation_euler=(Vector((0,0,.6))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();camera=bpy.context.object;camera.data.type='ORTHO';camera.data.ortho_scale=3.7;scene.camera=camera
bpy.ops.mesh.primitive_plane_add(size=20,location=(0,0,-.003));floor=bpy.context.object
mat=bpy.data.materials.new('NeutralReviewGround');mat.diffuse_color=(.18,.18,.18,1);floor.data.materials.append(mat)
report=[]
def activate(name,phase):
 for obj in objects:
  if not obj.animation_data:continue
  obj.animation_data.action=None
  for track in obj.animation_data.nla_tracks:track.mute=track.name!='tiger_'+name
 duration={'walk':.7368406057,'run':.5329999924,'hunt':1.4,'rest':3}.get(name,2)
 scene.frame_set(round(phase*duration*scene.render.fps));bpy.context.view_layer.update()
def render(name,angle,phase=.15,close=False):
 activate(name.split('-')[0],phase)
 target=Vector((0,0,.65))
 if close:
  rig=next(obj for obj in objects if obj.type=='ARMATURE');bone=next(b for b in rig.pose.bones if ('L_Hand_' if close=='feet' else 'Head_') in b.name)
  target=rig.matrix_world@bone.matrix.translation;camera.data.ortho_scale=.6 if close=='feet' else .85
 else:camera.data.ortho_scale=3.7
 camera.location=target+Vector((math.sin(angle)*4,-math.cos(angle)*4,1.1 if not close else .3));camera.rotation_euler=(target-camera.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
 dg=bpy.context.evaluated_depsgraph_get();points=[]
 for obj in meshes:
  evaluated=obj.evaluated_get(dg);mesh=evaluated.to_mesh();points.extend(evaluated.matrix_world@v.co for v in mesh.vertices);evaluated.to_mesh_clear()
 report.append({'view':name,'bodyBounds':[[min(p[i] for p in points) for i in range(3)],[max(p[i] for p in points) for i in range(3)]]})
for name,angle in [('idle-front',0),('idle-side',math.pi/2),('idle-quarter',math.pi/4),('idle-back',math.pi)]:render(name,angle)
render('idle-face',math.pi/8,close=True)
render('idle-feet',math.pi/8,close='feet')
for name in ['walk','run','hunt','forage','drink','rest']:render(name,math.pi/4)
activate('idle',0)
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(out/'tiger-native-authoring.blend'))
(out/'blender-review.json').write_text(json.dumps({'status':'VISUAL_REVIEW_REQUIRED','sourceSha256':source_hash,'runtimeAdmission':False,'views':report},indent=2)+'\n')
print('NATIVE_TIGER_REVIEW',json.dumps(report))
