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
    global SOURCE, DOWNLOAD
    alternative = len(sys.argv) > 2 and sys.argv[2] == 'ferocious'
    creator = 'a Sloyd creator' if alternative else 'TopoForge'
    title = 'A Ferocious Tiger Figurine With A Muscular Body' if alternative else 'Tiger'
    if alternative:
        SOURCE = 'https://www.sloyd.ai/free-3d-models/model/a-ferocious-tiger-figurine-with-a-muscular-body-inzht46x'
        DOWNLOAD = 'https://storage.googleapis.com/ai-services-quality/jobs/inzht46x.glb'
    out = pathlib.Path(sys.argv[1])
    out.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(DOWNLOAD, headers={'User-Agent': 'LifeLens-source-review/1.0'})
    with urllib.request.urlopen(request, timeout=30) as response:
        body = response.read(64 * 1024 * 1024 + 1)
    if len(body) > 64 * 1024 * 1024 or len(body) < 20:
        raise ValueError('Candidate exceeds review size bound or is empty')
    expected = ('b62a99cbe6d41e2f669e4c717e52f7c24f448623479d73ada1508dd7db4953e8'
                if alternative else 'f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be')
    if hashlib.sha256(body).hexdigest() != expected:
        raise ValueError('Publisher bytes differ from the inspected source; stop for review')
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
        'creator': creator,
        'license': 'CC-BY-4.0',
        'license_url': 'https://creativecommons.org/licenses/by/4.0/',
        'attribution_required': True,
        'attribution': f'"{title}" by {creator}, generated with Sloyd, licensed under CC BY 4.0.',
        'license_evidence': 'Live publisher page verified 2026-10-14: commercial use, modification and redistribution allowed with attribution.',
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
        + record['attribution'] + '\n\n'
        + 'Source: ' + SOURCE + '\n\n'
        + 'License: https://creativecommons.org/licenses/by/4.0/\n\n'
        + 'Changes: source.glb is unmodified; Blender review adds camera, lighting and floor. '
        + 'This is a candidate, not an accepted LifeLens runtime asset.\n')
    (out / 'source-record.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
