"""Neutral renders of the source-derived native joints and all runtime clips."""
import hashlib
import json
import math
from pathlib import Path
import bpy
from mathutils import Vector

ROOT = Path('docs/asset-reviews/native-wildlife/derived').resolve()
GENERATOR = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
for source in sorted(ROOT.glob('*-rigged.glb')):
    slug = source.name.removesuffix('-rigged.glb')
    prior = ROOT / slug / 'review.json'
    source_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    if prior.is_file():
        saved = json.loads(prior.read_text())
        if saved.get('sourceSha256') == source_hash and saved.get('generatorSha256') == GENERATOR and (ROOT / slug / (slug + '-authoring.blend')).is_file() and all((ROOT / slug / (v + '.png')).is_file() for v in saved.get('views', [])):
            print('REUSE_IDENTICAL_NATIVE_REVIEW', slug)
            continue
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.ops.outliner.orphans_purge(do_recursive=True)
    bpy.ops.import_scene.gltf(filepath=str(source))
    objects = list(bpy.context.scene.objects)
    meshes = [o for o in objects if o.type == 'MESH'
              and not any(c.name.startswith('glTF_not_exported') for c in o.users_collection)]
    prefix = source.name.removesuffix('-rigged.glb')
    folder = ROOT / prefix
    folder.mkdir(exist_ok=True)
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 12
    scene.cycles.use_denoising = False
    scene.render.resolution_x = scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.world.color = (.15, .15, .15)
    scene.view_settings.view_transform = 'Standard'
    def activate(name, phase=0):
        for obj in objects:
            if not obj.animation_data:
                continue
            obj.animation_data.action = None
            for track in obj.animation_data.nla_tracks:
                track.mute = track.name != prefix + '_' + name
        # Every authored clip starts at frame zero, with an exact loop endpoint.
        duration = .5 if name == 'run' and prefix == 'hare' else .65 if name == 'run' else .7 if name in ('walk', 'hunt') and prefix == 'hare' else 1.15 if name in ('walk', 'hunt') else 1 if name == 'swim' else 2
        scene.frame_set(round(phase * duration * scene.render.fps))
        bpy.context.view_layer.update()
    activate('idle')
    points = [o.matrix_world @ Vector(p) for o in meshes for p in o.bound_box]
    low = Vector([min(p[i] for p in points) for i in range(3)])
    high = Vector([max(p[i] for p in points) for i in range(3)])
    center = (low + high) / 2
    extent = max(high - low)
    for position, energy in [((3, -4, 5), 500), ((-3, 2, 4), 350)]:
        bpy.ops.object.light_add(type='AREA', location=center + Vector(position) * extent / 3)
        light = bpy.context.object
        light.data.energy = energy * (extent / 2) ** 2
        light.data.size = extent * 1.5
        light.rotation_euler = (center - light.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.ops.object.camera_add()
    camera = bpy.context.object
    scene.camera = camera
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = extent * 1.2
    camera.data.clip_start = .001
    camera.data.clip_end = 100
    bpy.ops.mesh.primitive_plane_add(size=20, location=(0, 0, -.004))
    floor = bpy.context.object
    mat = bpy.data.materials.new('NeutralGround')
    mat.diffuse_color = (.16, .16, .16, 1)
    floor.data.materials.append(mat)
    views = []
    def render(name, behavior, angle, phase=.15):
        activate(behavior, phase)
        camera.location = center + Vector((math.sin(angle), -math.cos(angle), .24)) * extent
        camera.rotation_euler = (center - camera.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(folder / (name + '.png'))
        bpy.ops.render.render(write_still=True)
        views.append(name)
    for name, angle in [('front', 0), ('side', math.pi / 2), ('quarter', math.pi / 4), ('back', math.pi)]:
        render('idle-' + name, 'idle', angle)
    for behavior in (['swim'] if prefix == 'freshwater-fish' else ['walk', 'run', 'hunt', 'forage', 'drink', 'rest']):
        render(behavior, behavior, math.pi / 4)
    activate('idle')
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(folder / (prefix + '-authoring.blend')))
    (folder / 'review.json').write_text(json.dumps({'status': 'REQUIRES_VISUAL_AND_RUNTIME_REVIEW',
        'sourceSha256': source_hash, 'generatorSha256': GENERATOR, 'views': views,
        'bounds': [list(low), list(high)]}, indent=2) + '\n')
