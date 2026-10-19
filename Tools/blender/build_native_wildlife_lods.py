"""Source-derived mid skins and static, vertex-colored far pose batches."""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import bpy

ROOT = Path('docs/asset-reviews/native-wildlife/derived').resolve()


def document(path):
    raw = path.read_bytes()
    size = struct.unpack_from('<I', raw, 12)[0]
    return json.loads(raw[20:20 + size]), raw[28 + size:]


def contract_write(path, contract):
    doc, binary = document(path)
    doc['asset'].setdefault('extras', {})['lifelensWildlife'] = contract
    encoded = json.dumps(doc, separators=(',', ':')).encode()
    encoded += b' ' * (-len(encoded) % 4)
    binary += b'\0' * (-len(binary) % 4)
    path.write_bytes(struct.pack('<4sII', b'glTF', 2, 28 + len(encoded) + len(binary))
        + struct.pack('<II', len(encoded), 0x4e4f534a) + encoded
        + struct.pack('<II', len(binary), 0x004e4942) + binary)


reports = []
for source in sorted(ROOT.glob('*-rigged.glb')):
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.ops.outliner.orphans_purge(do_recursive=True)
    doc, _ = document(source)
    contract = doc['asset']['extras']['lifelensWildlife']
    slug = source.name.removesuffix('-rigged.glb')
    bpy.ops.import_scene.gltf(filepath=str(source))
    original = [o for o in bpy.context.scene.objects
        if not any(c.name.startswith('glTF_not_exported') for c in o.users_collection)]
    meshes = [o for o in original if o.type == 'MESH']
    def activate(clip):
        for obj in original:
            if obj.animation_data:
                obj.animation_data.action = None
                for track in obj.animation_data.nla_tracks:
                    track.mute = track.name != slug + '_' + clip
        bpy.context.scene.frame_set(0)
        bpy.context.view_layer.update()
    total = sum(len(p.vertices) - 2 for obj in meshes for p in obj.data.polygons)
    pixels = {}
    def color(material, uv):
        if not material or not material.use_nodes:
            return (.25, .2, .15, 1)
        bsdf = next((n for n in material.node_tree.nodes if n.type == 'BSDF_PRINCIPLED'), None)
        if not bsdf:
            return (.25, .2, .15, 1)
        socket = bsdf.inputs['Base Color']
        node = socket.links[0].from_node if socket.links else None
        if not node or node.type != 'TEX_IMAGE' or not node.image:
            return tuple(socket.default_value)
        image = node.image
        if image.name not in pixels:
            pixels[image.name] = (image.size[:], list(image.pixels[:]))
        (w, h), data = pixels[image.name]
        x = min(w - 1, max(0, int((uv.x % 1) * w)))
        y = min(h - 1, max(0, int((uv.y % 1) * h)))
        i = (y * w + x) * 4
        return tuple(data[i:i + 4])
    # Far meshes carry colors sampled from the licensed source's actual UVs.
    # They contain no skeleton or mixer per animal.
    far_objects = []
    for pose, clip in [('Idle', 'idle'), ('Rest', 'rest'), ('Forage', 'forage')]:
        activate(clip if clip in contract['clips'] else 'idle')
        parts = {'Body': [], 'Antlers': []}
        dg = bpy.context.evaluated_depsgraph_get()
        for obj in meshes:
            evaluated = obj.evaluated_get(dg)
            data = bpy.data.meshes.new_from_object(evaluated, depsgraph=dg)
            data.transform(obj.matrix_world)
            colors = data.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='CORNER')
            for face in data.polygons:
                material = obj.data.materials[face.material_index] if face.material_index < len(obj.data.materials) else None
                for loop in face.loop_indices:
                    uv = data.uv_layers.active.data[loop].uv if data.uv_layers.active else type('UV', (), {'x': 0, 'y': 0})()
                    colors.data[loop].color = color(material, uv)
            part = bpy.data.objects.new('FarPart', data)
            bpy.context.scene.collection.objects.link(part)
            antler = contract['species'] == 'Deer' and any(m and 'jiao_tex' in m.name for m in obj.data.materials)
            parts['Antlers' if antler else 'Body'].append(part)
        for category, pieces in parts.items():
            if not pieces:
                continue
            bpy.ops.object.select_all(action='DESELECT')
            for obj in pieces:
                obj.select_set(True)
            bpy.context.view_layer.objects.active = pieces[0]
            bpy.ops.object.join()
            merged = pieces[0]
            merged.name = 'LifeLensNativeFar' + pose + category
            merged.data.materials.clear()
            material = bpy.data.materials.new('FarSourceVertexColors')
            material.use_nodes = True
            bsdf = material.node_tree.nodes.get('Principled BSDF')
            bsdf.inputs['Roughness'].default_value = .88
            attr = material.node_tree.nodes.new('ShaderNodeVertexColor')
            attr.layer_name = 'Color'
            material.node_tree.links.new(attr.outputs['Color'], bsdf.inputs['Base Color'])
            merged.data.materials.append(material)
            count = sum(len(p.vertices) - 2 for p in merged.data.polygons)
            target = 260 if category == 'Antlers' else 780 if contract['species'] != 'FreshwaterFish' else 450
            if count > target:
                mod = merged.modifiers.new('SourceDerivedFarReduction', 'DECIMATE')
                mod.ratio = target / count
                mod.use_collapse_triangulate = True
                bpy.ops.object.modifier_apply(modifier=mod.name)
            far_objects.append(merged)
    bpy.ops.object.select_all(action='DESELECT')
    for obj in far_objects:
        obj.select_set(True)
    far_path = ROOT / (slug + '-far.glb')
    bpy.ops.export_scene.gltf(filepath=str(far_path), export_format='GLB', use_selection=True,
        export_animations=False, export_skins=False, export_extras=True)
    far_contract = {**contract, 'tier': 'far', 'render_mode': 'instanced-static', 'bone_count': 0, 'paws': []}
    contract_write(far_path, far_contract)
    for obj in far_objects:
        bpy.data.objects.remove(obj, do_unlink=True)
    if '--far-only' in sys.argv:
        mid_path = ROOT / (slug + '-mid.glb')
        mid_doc, _ = document(mid_path)
        row = {'species': contract['species'], 'sourceTriangles': total,
               'midTriangles': sum(mid_doc['accessors'][p['indices']]['count'] // 3 for m in mid_doc['meshes'] for p in m['primitives']),
               'files': [{'file': p.name, 'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source, mid_path, far_path]]}
        reports.append(row)
        print('SOURCE_DERIVED_LODS', json.dumps(row))
        continue
    # Apply actual topology reduction while retaining native weights and UVs.
    target = {'Deer': 8000, 'Wolf': 3500, 'Hare': 3500, 'FreshwaterFish': 1500}[contract['species']]
    ratio = min(1, target / total)
    for obj in meshes:
        bpy.context.view_layer.objects.active = obj
        mod = obj.modifiers.new('SourceDerivedMidReduction', 'DECIMATE')
        mod.ratio = ratio
        mod.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    # All original object tracks remain active in their named NLA groups.
    for obj in original:
        if obj.animation_data:
            for track in obj.animation_data.nla_tracks:
                track.mute = False
    bpy.ops.object.select_all(action='DESELECT')
    for obj in original:
        obj.select_set(True)
    mid_path = ROOT / (slug + '-mid.glb')
    params = {'filepath': str(mid_path), 'export_format': 'GLB', 'use_selection': True,
              'export_animations': True, 'export_skins': True, 'export_extras': True,
              'export_force_sampling': False}
    keys = bpy.ops.export_scene.gltf.get_rna_type().properties.keys()
    if 'export_animation_mode' in keys:
        params['export_animation_mode'] = 'NLA_TRACKS'
    else:
        params['export_nla_strips'] = True
    bpy.ops.export_scene.gltf(**params)
    mid_contract = {**contract, 'tier': 'mid', 'paws': [{k: v for k, v in p.items() if k != 'sole_vertices'} for p in contract['paws']]}
    contract_write(mid_path, mid_contract)
    row = {'species': contract['species'], 'sourceTriangles': total,
           'midTriangles': sum(len(p.vertices) - 2 for obj in meshes for p in obj.data.polygons),
           'files': [{'file': p.name, 'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in [source, mid_path, far_path]]}
    reports.append(row)
    print('SOURCE_DERIVED_LODS', json.dumps(row))
(ROOT / 'lods.json').write_text(json.dumps({'status': 'DRAFT_REQUIRES_RUNTIME_REVIEW', 'reports': reports}, indent=2) + '\n')
