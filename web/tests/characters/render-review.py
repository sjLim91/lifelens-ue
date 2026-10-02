import bpy,json,math
from pathlib import Path
OUT = Path(__file__).resolve().parent / "review"
from mathutils import Vector
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
sc=bpy.context.scene;sc.render.engine='CYCLES';sc.cycles.samples=16;sc.cycles.use_denoising=False
sc.world.color=(.18,.18,.18)
mat=bpy.data.materials.new('Vertex color');mat.use_nodes=True
bs=mat.node_tree.nodes.get('Principled BSDF');bs.inputs['Roughness'].default_value=.85
col=mat.node_tree.nodes.new('ShaderNodeVertexColor');col.layer_name='Color';mat.node_tree.links.new(col.outputs['Color'],bs.inputs['Base Color'])
poses=json.load(open(OUT / "poses.json"))
for i,pose in enumerate(poses):
 row=i//4;column=i%4;ox=(column-1.5)*2;oz=-row*2.5
 for j,data in enumerate(pose['meshes']):
  p=data['positions'];verts=[(p[k]+ox,-p[k+2],p[k+1]+oz) for k in range(0,len(p),3)]
  ind=data['indices'];faces=[ind[k:k+3] for k in range(0,len(ind),3)]
  mesh=bpy.data.meshes.new('mesh');mesh.from_pydata(verts,[],faces);mesh.update();ob=bpy.data.objects.new('pose',mesh);sc.collection.objects.link(ob);ob.data.materials.append(mat)
  attr=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT');c=data['colors']
  for k,v in enumerate(attr.data):v.color=(c[k*3],c[k*3+1],c[k*3+2],1)
  for f in mesh.polygons:f.use_smooth=True
 bpy.ops.object.text_add(location=(ox-.78,-.6,oz-.25),rotation=(math.pi/2,0,0));text=bpy.context.object;text.data.body=pose['label'];text.data.size=.115
 textmat=bpy.data.materials.get('Label') or bpy.data.materials.new('Label');textmat.diffuse_color=(.85,.9,.92,1);text.data.materials.append(textmat)
for loc,power,size in [((-3,-7,6),1600,7),((5,-3,1),900,6)]:
 bpy.ops.object.light_add(type='AREA',location=loc);light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size;light.rotation_euler=(Vector((0,0,-2))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-22,2));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,-2.7))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=10;sc.camera=cam
sc.render.resolution_x=1500;sc.render.resolution_y=1600;sc.render.resolution_percentage=100
sc.render.image_settings.file_format='PNG';sc.render.filepath=str(OUT / "character-poses.png")
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "character-poses.blend"));bpy.ops.render.render(write_still=True)
