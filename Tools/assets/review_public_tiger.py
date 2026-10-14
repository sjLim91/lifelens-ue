"""Acquire an openly published CC BY candidate for review, never for runtime admission."""
from __future__ import annotations

import datetime as dt
import hashlib
import json
import pathlib
import struct
import sys
import urllib.request

SOURCE = 'https://www.sloyd.ai/free-3d-models/model/tiger-7m4ap2k1'
DOWNLOAD = 'https://storage.googleapis.com/ai-services-quality/jobs/7m4ap2k1.glb'


def main():
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(DOWNLOAD, headers={'User-Agent': 'LifeLens-source-review/1.0'})
    with urllib.request.urlopen(request, timeout=30) as response:
        body = response.read(64 * 1024 * 1024 + 1)
    if len(body) > 64 * 1024 * 1024 or len(body) < 20:
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
        'creator': 'TopoForge',
        'license': 'CC-BY-4.0',
        'license_url': 'https://creativecommons.org/licenses/by/4.0/',
        'attribution_required': True,
        'attribution': '"Tiger" by TopoForge, generated with Sloyd, licensed under CC BY 4.0.',
        'license_evidence': 'Live publisher page verified 2026-10-14: commercial use, modification and redistribution allowed with attribution. Published 2025-08-21.',
        'modifications': 'None to source.glb; Blender import and neutral review staging only.',
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
    (out / 'CREDITS.md').write_text(
        '# Tiger source attribution\n\n'
        '"Tiger" by TopoForge, generated with Sloyd, licensed under CC BY 4.0.\n\n'
        + 'Source: ' + SOURCE + '\n\n'
        + 'License: https://creativecommons.org/licenses/by/4.0/\n\n'
        + 'Changes: source.glb is unmodified; Blender review adds camera, lighting and floor. '
        + 'This is a candidate, not an accepted LifeLens runtime asset.\n')
    (out / 'source-record.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
