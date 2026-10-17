"""Review CC-licensed objects from AI2's public redistribution, never viewer data.

No Sketchfab download endpoint, account, token or protected CDN is used.
Dataset metadata and each object's license are checked before its bytes are read.
"""
import concurrent.futures
import datetime
import gzip
import hashlib
import json
from pathlib import Path
import re
import struct
import urllib.request

ROOT = Path('docs/asset-reviews/distributed-tigers')
ROOT.mkdir(parents=True, exist_ok=True)
BASE = 'https://huggingface.co/datasets/allenai/objaverse/resolve/'
KNOWN = 'fc2c7fd74dd04e39935803b884c77258'

def read(url, limit=32 * 1024 * 1024):
    req = urllib.request.Request(url, headers={'User-Agent': 'LifeLens-free-asset-review/1'})
    with urllib.request.urlopen(req, timeout=60) as response:
        if response.status != 200:
            raise ValueError(f'Non-public response: {response.status}')
        data = response.read(limit + 1)
        if len(data) > limit:
            raise ValueError('Review size budget exceeded')
        return data

revision = json.loads(read('https://huggingface.co/api/datasets/allenai/objaverse', 512000))['sha']
base = BASE + revision + '/'
def zipped(name):
    return json.loads(gzip.decompress(read(base + name)))
paths = zipped('object-paths.json.gz')
lvis = zipped('lvis-annotations.json.gz')
uids = list(dict.fromkeys([KNOWN] + [uid for key, values in lvis.items() if key.lower() in ('tiger', 'tiger_cat') for uid in values]))
uids = [uid for uid in uids if uid in paths][:24]
parts = sorted({paths[uid].split('/')[1] for uid in uids})
def metadata(part):
    return zipped('metadata/' + part + '.json.gz')
annotations = {}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
    for batch in pool.map(metadata, parts):
        annotations.update({uid: batch[uid] for uid in uids if uid in batch})
allowed = []
report = []
for uid in uids:
    item = annotations.get(uid, {})
    lic = item.get('license', {})
    license_url = lic.get('url', '')
    permitted = license_url.rstrip('/') in ('https://creativecommons.org/licenses/by/4.0', 'http://creativecommons.org/licenses/by/4.0', 'https://creativecommons.org/publicdomain/zero/1.0', 'http://creativecommons.org/publicdomain/zero/1.0')
    title = item.get('name', '')
    text = title + ' ' + item.get('description', '') + ' ' + json.dumps(item.get('tags', []))
    excluded = bool(re.search(r'\b(tank|warrior|anthro|ripped|noai|no-ai|noncommercial)\b', text, re.I))
    entry = {'uid': uid, 'name': title, 'license': lic, 'author': item.get('user'),
             'source': item.get('viewerUrl') or 'https://sketchfab.com/3d-models/' + uid,
             'description': item.get('description'), 'vertexCount': item.get('vertexCount'),
             'faceCount': item.get('faceCount'), 'animationCount': item.get('animationCount'),
             'allowedForReview': permitted and not excluded}
    report.append(entry)
    if permitted and not excluded:
        allowed.append((uid, item))
allowed.sort(key=lambda pair: (pair[0] != KNOWN, not bool(pair[1].get('animationCount')), -min(pair[1].get('vertexCount', 0), 100000)))
for uid, item in allowed[:3]:
    out = ROOT / uid
    out.mkdir(exist_ok=True)
    raw = read(base + paths[uid])
    if struct.unpack_from('<4sII', raw) != (b'glTF', 2, len(raw)):
        raise ValueError('Invalid self-contained GLB')
    size, kind = struct.unpack_from('<II', raw, 12)
    if kind != 0x4e4f534a:
        raise ValueError('Invalid GLB JSON')
    gltf = json.loads(raw[20:20 + size])
    if any(buffer.get('uri') for buffer in gltf.get('buffers', [])) or any(image.get('uri', '').startswith(('http:', 'https:')) for image in gltf.get('images', [])):
        raise ValueError('External asset dependencies require separate license review')
    (out / 'source.glb').write_bytes(raw)
    receipt = {'purpose': 'CANDIDATE_REVIEW_NOT_RUNTIME', 'runtimeAdmission': False,
               'retrievedAt': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'uid': uid, 'originalMetadata': item, 'datasetRevision': revision,
               'downloadUrl': base + paths[uid], 'sha256': hashlib.sha256(raw).hexdigest(),
               'bytes': len(raw), 'datasetLicense': 'ODC-By-1.0', 'dataset': 'https://huggingface.co/datasets/allenai/objaverse',
               'asset': gltf.get('asset'),
               'skins': [{'name': s.get('name'), 'joints': len(s.get('joints', []))} for s in gltf.get('skins', [])],
               'animations': [{'name': a.get('name'), 'channels': len(a.get('channels', [])), 'extras': a.get('extras')} for a in gltf.get('animations', [])]}
    (out / 'receipt.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n')
    creator = item.get('user', {}).get('displayName') or item.get('user', {}).get('username')
    credit = f'"{item.get("name")}" by {creator}. Original: {item.get("viewerUrl")}. License: {item["license"]["url"]}.\n'
    credit += 'Obtained from the openly redistributed AI2 Objaverse dataset (ODC-By 1.0), not a protected viewer or authenticated download.\n'
    credit += 'AI2 dataset: Deitke et al., Objaverse: A Universe of Annotated 3D Objects, 2022, https://objaverse.allenai.org/.\n'
    credit += 'No runtime admission. Unmodified original GLB retained. Review renders only.\n'
    (out / 'CREDITS.md').write_text(credit)
    print('PUBLIC_TIGER_CANDIDATE', json.dumps({key: receipt[key] for key in ('uid', 'bytes', 'sha256', 'skins', 'animations')}))
(ROOT / 'candidates.json').write_text(json.dumps({'status': 'REVIEW_ONLY', 'datasetRevision': revision, 'candidates': report}, ensure_ascii=False, indent=2) + '\n')
