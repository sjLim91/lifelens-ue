"""Read public upstream metadata only; never admit or redistribute the model."""
import hashlib
import json
import struct
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

COMMIT = '4eb56548351f7f8277df7eaab5354c83db6e1e16'
ROOT = f'https://raw.githubusercontent.com/google-ar/jetpack-xr-natives/{COMMIT}/'
ASSET = 'impress/core/window/Tiger.glb'

def get(path, limit):
    request = urllib.request.Request(ROOT + path, headers={'User-Agent': 'LifeLens-public-license-review/1'})
    with urllib.request.urlopen(request, timeout=45) as response:
        if response.status != 200:
            raise ValueError(f'Unexpected public response {response.status}')
        payload = response.read(limit + 1)
        if len(payload) > limit:
            raise ValueError('Public response exceeds review size limit')
        return payload

raw = get(ASSET, 16 * 1024 * 1024)
magic, version, length = struct.unpack_from('<4sII', raw)
if (magic, version, length) != (b'glTF', 2, len(raw)):
    raise ValueError('Invalid GLB header')
chunk_length, chunk_type = struct.unpack_from('<II', raw, 12)
if chunk_type != 0x4e4f534a or chunk_length > 2 * 1024 * 1024:
    raise ValueError('Invalid or oversized JSON chunk')
gltf = json.loads(raw[20:20 + chunk_length])
license_text = get('LICENSE', 128 * 1024).decode('utf-8')
notices = get('impress/apibindings/THIRD_PARTY_NOTICES.txt', 256 * 1024).decode('utf-8')
report = {
    'purpose': 'UPSTREAM_METADATA_RESEARCH_ONLY',
    'runtimeAdmission': False,
    'rightsStatus': 'UNVERIFIED_ASSET_SPECIFIC_LICENSE_AND_AUTHOR',
    'warning': 'A repository code license alone does not establish model authorship or redistribution rights.',
    'repository': 'google-ar/jetpack-xr-natives', 'commit': COMMIT,
    'assetUrl': ROOT + ASSET, 'repositoryLicenseUrl': ROOT + 'LICENSE',
    'retrievedAt': datetime.now(timezone.utc).isoformat(),
    'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
    'asset': gltf.get('asset'), 'extras': gltf.get('extras'),
    'extensions': gltf.get('extensions'), 'extensionsUsed': gltf.get('extensionsUsed'),
    'skins': [{'name': skin.get('name'), 'joints': len(skin.get('joints', []))} for skin in gltf.get('skins', [])],
    'animations': [{'name': clip.get('name'), 'channels': len(clip.get('channels', []))} for clip in gltf.get('animations', [])],
    'meshes': [{'name': mesh.get('name'), 'primitives': len(mesh.get('primitives', [])), 'extras': mesh.get('extras')} for mesh in gltf.get('meshes', [])],
    'materials': [{'name': mat.get('name'), 'alphaMode': mat.get('alphaMode', 'OPAQUE'), 'doubleSided': mat.get('doubleSided', False), 'extras': mat.get('extras')} for mat in gltf.get('materials', [])],
    'images': [{'name': image.get('name'), 'mimeType': image.get('mimeType'), 'uri': image.get('uri')} for image in gltf.get('images', [])],
    'repositoryLicense': license_text,
    'thirdPartyNoticeMentions': [line for line in notices.splitlines() if any(token in line.lower() for token in ('tiger', 'model', 'massimo', 'sketchfab', 'sloyd'))],
}
out = Path('review/upstream-tiger-metadata.json')
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')
print(json.dumps({key: report[key] for key in ('rightsStatus', 'bytes', 'sha256', 'asset', 'skins', 'animations', 'thirdPartyNoticeMentions')}, ensure_ascii=False))
