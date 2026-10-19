"""Resize embedded source images only; retain every non-image buffer byte."""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import bpy

ROOT = Path('docs/asset-reviews/native-wildlife/derived').resolve()
reports = []
for path in sorted(list(ROOT.glob('*-rigged.glb')) + list(ROOT.glob('*-mid.glb'))):
    raw = path.read_bytes()
    length = struct.unpack_from('<I', raw, 12)[0]
    doc = json.loads(raw[20:20 + length])
    binary = raw[28 + length:]
    c = doc['asset']['extras']['lifelensWildlife']
    near = c['tier'] == 'near'
    default = 1024 if near else 512
    if c['species'] == 'FreshwaterFish':
        default = 512 if near else 256
    caps = {index: default for index in range(len(doc.get('images', [])))}
    if c['species'] == 'Wolf':
        for material in doc.get('materials', []):
            name = material.get('name', '').lower()
            cap = (1024 if near else 512) if 'corpo' in name else (512 if near else 256) if 'bigodes' in name else (256 if near else 128)
            textures = []
            for value in material.values():
                if isinstance(value, dict) and 'index' in value:
                    textures.append(value['index'])
            for value in material.get('pbrMetallicRoughness', {}).values():
                if isinstance(value, dict) and 'index' in value:
                    textures.append(value['index'])
            for texture in textures:
                image = doc['textures'][texture]['source']
                caps[image] = cap
    views = {image['bufferView']: (index, image) for index, image in enumerate(doc.get('images', []))}
    packed = bytearray()
    before = hashlib.sha256()
    after = hashlib.sha256()
    images = []
    for index, view in enumerate(doc['bufferViews']):
        offset = view.get('byteOffset', 0)
        piece = binary[offset:offset + view['byteLength']]
        if index in views:
            image_index, image = views[index]
            with tempfile.TemporaryDirectory() as temp:
                source = Path(temp) / 'source'
                source.write_bytes(piece)
                img = bpy.data.images.load(str(source), check_existing=False)
                width, height = img.size
                cap = caps[image_index]
                factor = min(1, cap / max(width, height))
                output_size = (max(1, round(width * factor)), max(1, round(height * factor)))
                if factor < 1:
                    img.colorspace_settings.name = 'Non-Color'
                    img.scale(*output_size)
                    img.file_format = 'PNG'
                    img.filepath_raw = str(Path(temp) / 'resized.png')
                    img.save()
                    piece = Path(img.filepath_raw).read_bytes()
                    image['mimeType'] = 'image/png'
                bpy.data.images.remove(img)
            images.append({'index': image_index, 'source': [width, height], 'output': list(output_size)})
        else:
            before.update(piece)
            after.update(piece)
        packed.extend(b'\0' * (-len(packed) % 4))
        view['byteOffset'] = len(packed)
        view['byteLength'] = len(piece)
        packed.extend(piece)
    assert before.digest() == after.digest(), 'Geometry/UV/weights/animation bytes changed during texture LOD'
    rgba = sum(i['output'][0] * i['output'][1] * 4 for i in images)
    c['texture_lod'] = {'decodedRGBABytes': rgba, 'images': images}
    doc['buffers'][0]['byteLength'] = len(packed)
    packed.extend(b'\0' * (-len(packed) % 4))
    encoded = json.dumps(doc, separators=(',', ':')).encode()
    encoded += b' ' * (-len(encoded) % 4)
    path.write_bytes(struct.pack('<4sII', b'glTF', 2, 28 + len(encoded) + len(packed))
        + struct.pack('<II', len(encoded), 0x4e4f534a) + encoded
        + struct.pack('<II', len(packed), 0x004e4942) + packed)
    reports.append({'file': path.name, 'decodedRGBABytes': rgba, 'nonImageBufferSha256': before.hexdigest(),
                    'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'bytes': path.stat().st_size, 'images': images})
    print('NATIVE_TEXTURE_LOD', path.name, rgba)
(ROOT / 'texture-lods.json').write_text(json.dumps(reports, indent=2) + '\n')
authoring = json.loads((ROOT / 'authoring.json').read_text())
for row in authoring['reports']:
    item = next(r for r in reports if r['file'] == row['contract']['clips']['idle']['name'].removesuffix('_idle') + '-rigged.glb')
    row['derivedSha256'] = item['sha256']
    row['bytes'] = item['bytes']
    row['textureLod'] = item
authoring['textureGeneratorSha256'] = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
(ROOT / 'authoring.json').write_text(json.dumps(authoring, indent=2) + '\n')
