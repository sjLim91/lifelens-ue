"""Offline geometry/material diagnostic. Browser capture remains the runtime gate."""
import bpy, json, math
from pathlib import Path
from mathutils import Vector
out = Path(__file__).resolve().parent / 'review'
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene; scene.render.engine = 'CYCLES'
scene.cycles.samples = 8; scene.cycles.use_denoising = False
scene.render.threads_mode = 'FIXED'; scene.render.threads = 2
scene.world.color = (.25, .3, .34)
for row, data in enumerate(json.loads((out / 'tree-geometry.json').read_text())):
    x = (row - 1) * 9
    for part in data['parts']:
        p = part['positions']; vertices = [(p[i] * 8 + x, -p[i+2] * 8 - 5, p[i+1] * 8) for i in range(0, len(p), 3)]
        ids = part['indices']; faces = [ids[i:i+3] for i in range(0, len(ids), 3)]
        mesh = bpy.data.meshes.new(part['name']); mesh.from_pydata(vertices, [], faces); mesh.update()
        ob = bpy.data.objects.new(data['species'], mesh); scene.collection.objects.link(ob)
        color = mesh.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='POINT')
        tint = (.32, .56, .23) if part['name'] == 'foliage' else (1, 1, 1)
        c = part['colors']
        for i, value in enumerate(color.data): value.color = (*(c[i*3+j]*tint[j] for j in range(3)), 1)
        material = bpy.data.materials.new('Procedural ' + part['name']); material.use_nodes = True
        bs = material.node_tree.nodes.get('Principled BSDF'); bs.inputs['Roughness'].default_value = .88
        attr = material.node_tree.nodes.new('ShaderNodeVertexColor'); attr.layer_name = 'Color'
        material.node_tree.links.new(attr.outputs['Color'], bs.inputs['Base Color']); ob.data.materials.append(material)
    # Import the exact existing pinned GLB, including its authored alpha textures.
    before = set(scene.objects); bpy.ops.import_scene.gltf(filepath=str(out / f'tree{row+1}.glb'))
    objects = set(scene.objects) - before
    bounds = [o.matrix_world @ Vector(c) for o in objects if o.type == 'MESH' for c in o.bound_box]
    minimum = Vector(tuple(min(v[j] for v in bounds) for j in range(3)))
    maximum = Vector(tuple(max(v[j] for v in bounds) for j in range(3)))
    height = max(.001, maximum.z - minimum.z); center = (minimum + maximum) / 2
    root = bpy.data.objects.new('Legacy normalized', None); scene.collection.objects.link(root)
    for ob in objects:
        if ob.parent is None: ob.parent = root
    root.scale = (8/height,)*3
    root.location = (x-center.x*8/height, 6-center.y*8/height, -minimum.z*8/height)
    for label, y in [(data['species'] + ' procedural', -7), ('legacy GLB ' + str(row+1), 4)]:
        bpy.ops.object.text_add(location=(x-3.5,y,.025)); text=bpy.context.object
        text.data.body=label; text.data.size=.55
bpy.ops.mesh.primitive_plane_add(size=200); ground=bpy.context.object; ground.location.z=-.045
material=bpy.data.materials.new('Soil'); material.diffuse_color=(.22,.27,.17,1); ground.data.materials.append(material)
for loc,power,size in [((-12,-16,25),4500,14),((14,5,18),2500,12)]:
    bpy.ops.object.light_add(type='AREA',location=loc); light=bpy.context.object
    light.data.energy=power; light.data.shape='DISK'; light.data.size=size
    light.rotation_euler=(Vector((0,0,3))-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(16,-35,24)); camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,3.3))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'; camera.data.ortho_scale=35; scene.camera=camera
scene.render.resolution_x=1280; scene.render.resolution_y=1050; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(out/'tree-comparison.png')
bpy.ops.render.render(write_still=True)
