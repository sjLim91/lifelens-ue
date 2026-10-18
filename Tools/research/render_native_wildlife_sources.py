"""Render the selected licensed originals without changing their source bytes."""
import hashlib
import json
import math
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path('docs/asset-reviews/native-wildlife').resolve()
bpy.context.preferences.filepaths.save_version = 0
for source in sorted(ROOT.glob('*/*/source.glb')):
    receipt = json.loads((source.parent / 'receipt.json').read_text())
    assert hashlib.sha256(source.read_bytes()).hexdigest() == receipt['sha256']
    # Blender's importer creates hidden bone-shape meshes. Select-all cannot
    # remove those; retaining them contaminates the next source's bounds.
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.ops.outliner.orphans_purge(do_recursive=True)
    for action in list(bpy.data.actions):
        bpy.data.actions.remove(action)
    bpy.ops.import_scene.gltf(filepath=str(source))
    scene = bpy.context.scene
    rigs = [obj for obj in scene.objects if obj.type == 'ARMATURE']
    controls = {bone.custom_shape for rig in rigs for bone in rig.pose.bones if bone.custom_shape}
    meshes = [obj for obj in scene.objects if obj.type == 'MESH' and obj not in controls
              and not obj.hide_render
              and not any(c.name.startswith('glTF_not_exported') for c in obj.users_collection)]
    assert meshes, 'No source mesh'
    scene.frame_set(1)
    bpy.context.view_layer.update()
    points = [obj.matrix_world @ Vector(p) for obj in meshes for p in obj.bound_box]
    low = Vector([min(p[i] for p in points) for i in range(3)])
    high = Vector([max(p[i] for p in points) for i in range(3)])
    center = (low + high) / 2
    extent = max(high - low)
    assert extent > 0
    report = {
        'status': 'SOURCE_INSPECTION_NOT_RUNTIME_ADMISSION', 'runtimeAdmission': False,
        'species': source.parent.parent.name, 'sourceSha256': receipt['sha256'],
        'bounds': [list(low), list(high)],
        'triangles': sum(len(p.vertices) - 2 for obj in meshes for p in obj.data.polygons),
        'meshes': [{'name': obj.name, 'vertices': len(obj.data.vertices),
                    'uvLayers': len(obj.data.uv_layers),
                    'vertexGroups': len(obj.vertex_groups)} for obj in meshes],
        'rigs': [{'name': rig.name, 'bones': [b.name for b in rig.data.bones]} for rig in rigs],
        'actions': [{'name': a.name, 'frames': list(a.frame_range)} for a in bpy.data.actions],
        'materials': [{'name': m.name, 'textures': [n.image.name for n in m.node_tree.nodes
                        if n.type == 'TEX_IMAGE' and n.image]}
                      for m in {m for o in meshes for m in o.data.materials if m and m.use_nodes}],
    }
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 6
    scene.cycles.use_denoising = False
    scene.render.resolution_x = 512
    scene.render.resolution_y = 512
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.world.color = (.15, .15, .15)
    scene.view_settings.view_transform = 'Standard'
    bpy.ops.object.camera_add()
    camera = bpy.context.object
    scene.camera = camera
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = extent * 1.2
    camera.data.clip_start = max(.0001, extent / 10000)
    camera.data.clip_end = max(1000, extent * 20)
    for relative, energy in [((2, -3, 4), 1100), ((-3, 1, 3), 700)]:
        bpy.ops.object.light_add(type='AREA', location=center + Vector(relative) * extent * .65)
        light = bpy.context.object
        light.data.energy = energy * (extent / 3) ** 2
        light.data.size = extent * 1.5
        light.rotation_euler = (center - light.location).to_track_quat('-Z', 'Y').to_euler()
    def capture(name, angle):
        camera.location = center + Vector((math.sin(angle), -math.cos(angle), .22)) * extent
        camera.rotation_euler = (center - camera.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(source.parent / (name + '.png'))
        bpy.ops.render.render(write_still=True)
    for name, angle in [('axis-front', 0), ('axis-side', math.pi / 2),
                        ('axis-quarter', math.pi / 4), ('axis-back', math.pi)]:
        capture(name, angle)
    if bpy.data.actions:
        clip = next(iter(bpy.data.actions))
        first, last = clip.frame_range
        for index, phase in enumerate([.25, .5, .75]):
            scene.frame_set(round(first + (last - first) * phase))
            capture('authored-motion-' + str(index), math.pi / 4)
    (source.parent / 'inspection.json').write_text(json.dumps(report, indent=2) + '\n')
    print('WILDLIFE_SOURCE_REVIEW', json.dumps(report))
