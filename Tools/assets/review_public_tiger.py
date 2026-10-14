"""Acquire an openly published CC0 candidate for visual review, never for runtime admission."""
from __future__ import annotations

import datetime as dt
import hashlib
import json
import pathlib
import struct
import sys
import urllib.request

SOURCE = 'https://3dassets.dev/assets/exotic-wildlife-hd-bengal-tiger-prowling-b1de9790'
DOWNLOAD = 'https://cdn.3dassets.dev/assets/30235/v1/model.glb'


def main():
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(DOWNLOAD, headers={'User-Agent': 'LifeLens-source-review/1.0'})
    with urllib.request.urlopen(request, timeout=30) as response:
        body = response.read(16 * 1024 * 1024 + 1)
    if len(body) > 16 * 1024 * 1024 or len(body) < 20:
        raise ValueError('Candidate exceeds review size bound or is empty')
    magic, version, size = struct.unpack_from('<4sII', body)
    if magic != b'glTF' or version != 2 or size != len(body):
        raise ValueError('Not a complete GLB 2.0 file')
    length, kind = struct.unpack_from('<II', body, 12)
    if kind != 0x4e4f534a or 20 + length > len(body):
        raise ValueError('Invalid GLB JSON chunk')
    model = json.loads(body[20:20+length])
    # A review source must be self-contained. No network resources from the GLB.
    for resource in model.get('buffers', []) + model.get('images', []):
        if resource.get('uri') and not resource['uri'].startswith('data:'):
            raise ValueError('External resource URI in candidate')
    (out / 'source.glb').write_bytes(body)
    record = {
        'status': 'CANDIDATE_ONLY_NOT_RUNTIME', 'merge_allowed': False,
        'source_url': SOURCE, 'download_url': DOWNLOAD,
        'creator': '3D Assets (3dassets.dev)',
        'license': 'CC0-1.0',
        'license_url': 'https://creativecommons.org/publicdomain/zero/1.0/',
        'attribution_required': False,
        'license_evidence': 'Publisher source-page search index states CC0-1.0 and allows modification/redistribution. Live HTML returned 503; preserve this limitation until live verification.',
        'downloaded_at': dt.datetime.now(dt.timezone.utc).isoformat(),
        'sha256': hashlib.sha256(body).hexdigest(), 'bytes': len(body),
        'generator': model.get('asset', {}).get('generator'),
        'asset_extras': model.get('asset', {}).get('extras'),
        'mesh_count': len(model.get('meshes', [])),
        'skin_count': len(model.get('skins', [])),
        'animation_names': [clip.get('name') for clip in model.get('animations', [])],
        'material_count': len(model.get('materials', [])),
        'materials': [m.get('name') for m in model.get('materials', [])],
        'uv_primitives': sum('TEXCOORD_0' in p.get('attributes', {}) for m in model.get('meshes', []) for p in m.get('primitives', [])),
        'extensions_used': model.get('extensionsUsed', []),
        'web_observer_validation': 'NOT_RUN', 'android_performance': 'NOT_MEASURED',
    }
    (out / 'source-record.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
