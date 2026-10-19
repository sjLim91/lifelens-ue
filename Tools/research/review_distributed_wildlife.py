"""Review public CC0/CC-BY originals for the four remaining Core species.

No paid provider, authenticated download, or viewer extraction. Original bytes
are only fetched for explicitly selected, individually licensed object IDs.
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

ROOT = Path('docs/asset-reviews/native-wildlife')
CONFIG = Path('Tools/assets/native-wildlife-selection.json')
DATASET = 'https://huggingface.co/datasets/allenai/objaverse'
CATEGORIES = {
    'Hare': {'hare', 'rabbit', 'bunny'},
    'Deer': {'deer', 'elk', 'stag', 'reindeer'},
    'Wolf': {'wolf'},
    'FreshwaterFish': {'fish', 'trout', 'salmon', 'carp', 'perch', 'bass', 'pike'},
}
EXCLUDED = re.compile(r"\b(ripped|noai|no-ai|non[ -]?commercial|personal use only|editorial only|pokemon|fortnite|minecraft|warcraft|witcher|skyrim|anthro|furry|taxidermy|not my mesh)\b", re.I)
LICENSES = {
    'https://creativecommons.org/licenses/by/4.0': 'CC-BY-4.0',
    'https://creativecommons.org/publicdomain/zero/1.0': 'CC0-1.0',
}


def read(url, limit=32 * 1024 * 1024):
    req = urllib.request.Request(url, headers={'User-Agent': 'LifeLens-public-wildlife-review/1'})
    with urllib.request.urlopen(req, timeout=60) as response:
        if response.status != 200:
            raise ValueError(f'Unexpected public response: {response.status}')
        result = response.read(limit + 1)
    if len(result) > limit:
        raise ValueError('Review download budget exceeded')
    return result


def license_of(item):
    lic = item.get('license', {})
    if isinstance(lic, str):
        # Objaverse preserves Sketchfab's by / cc0 license slug.
        lic = {'url': {'by': 'https://creativecommons.org/licenses/by/4.0/',
                       'cc0': 'https://creativecommons.org/publicdomain/zero/1.0/'}.get(lic, '')}
    url = lic.get('url', '').replace('http://', 'https://').rstrip('/')
    return LICENSES.get(url), url


def eligible(item):
    name = license_of(item)[0]
    author = item.get('user')
    creator = (author.get('displayName') or author.get('username')) if isinstance(author, dict) else None
    text = item.get('name', '') + ' ' + item.get('description', '') + ' ' + json.dumps(item.get('tags', []))
    return bool(name and creator and not EXCLUDED.search(text))


def inspect_glb(raw):
    if len(raw) < 28 or struct.unpack_from('<4sII', raw) != (b'glTF', 2, len(raw)):
        raise ValueError('Invalid GLB header')
    size, kind = struct.unpack_from('<II', raw, 12)
    if kind != 0x4e4f534a or size > len(raw) - 20:
        raise ValueError('Invalid GLB JSON')
    doc = json.loads(raw[20:20 + size])
    if any(b.get('uri') for b in doc.get('buffers', [])):
        raise ValueError('External buffers require independent provenance')
    if any(i.get('uri') and not i['uri'].startswith('data:') for i in doc.get('images', [])):
        raise ValueError('External images require independent provenance')
    return doc


def main():
    ROOT.mkdir(parents=True, exist_ok=True)
    config = json.loads(CONFIG.read_text())
    revision = config['datasetRevision']
    base = DATASET + '/resolve/' + revision + '/'
    def zipped(name):
        return json.loads(gzip.decompress(read(base + name)))
    paths = zipped('object-paths.json.gz')
    lvis = zipped('lvis-annotations.json.gz')
    species_uids = {}
    for species, categories in CATEGORIES.items():
        found = [uid for key, values in lvis.items()
                 if re.sub(r'_?\([^)]*\)', '', key.lower()).replace('_', ' ') in categories
                 for uid in values]
        species_uids[species] = list(dict.fromkeys(config['selected'].get(species, []) + found))[:48]
    uids = set(uid for values in species_uids.values() for uid in values if uid in paths)
    parts = sorted({paths[uid].split('/')[1] for uid in uids})
    annotations = {}
    def metadata(part):
        batch = zipped('metadata/' + part + '.json.gz')
        return {uid: batch[uid] for uid in uids if uid in batch}
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        for batch in pool.map(metadata, parts):
            annotations.update(batch)
    report = {'status': 'SOURCE_REVIEW_ONLY', 'runtimeAdmission': False,
              'datasetRevision': revision, 'dataset': DATASET,
              'datasetLicense': 'ODC-By-1.0', 'species': {}, 'selectedUnavailable': []}
    selected_count = 0
    for species, values in species_uids.items():
        candidates = []
        for uid in values:
            item = annotations.get(uid, {})
            license_name, license_url = license_of(item)
            entry = {'uid': uid, 'name': item.get('name'), 'author': item.get('user'),
                     'license': license_name, 'licenseUrl': license_url,
                     'original': item.get('viewerUrl') or 'https://sketchfab.com/3d-models/' + uid,
                     'description': item.get('description'), 'faceCount': item.get('faceCount'),
                     'vertexCount': item.get('vertexCount'), 'animationCount': item.get('animationCount'),
                     'allowedForReview': eligible(item)}
            candidates.append(entry)
            if uid not in config['selected'].get(species, []):
                continue
            if not eligible(item) or uid not in paths:
                report['selectedUnavailable'].append({'species': species, 'uid': uid,
                    'reason': 'Unavailable in pinned redistribution or per-object provenance rejected'})
                continue
            out = ROOT / species / uid
            out.mkdir(parents=True, exist_ok=True)
            prior = json.loads((out / 'receipt.json').read_text()) if (out / 'receipt.json').is_file() else {}
            raw = (out / 'source.glb').read_bytes() if (out / 'source.glb').is_file() else b''
            reuse = prior.get('datasetRevision') == revision and prior.get('sha256') == hashlib.sha256(raw).hexdigest()
            if not reuse:
                raw = read(base + paths[uid])
            gltf = inspect_glb(raw)
            (out / 'source.glb').write_bytes(raw)
            receipt = {**entry, 'purpose': 'CANDIDATE_REVIEW_NOT_RUNTIME', 'runtimeAdmission': False,
                       'retrievedAt': prior['retrievedAt'] if reuse else datetime.datetime.now(datetime.timezone.utc).isoformat(),
                       'datasetRevision': revision, 'downloadUrl': base + paths[uid],
                       'sha256': hashlib.sha256(raw).hexdigest(), 'bytes': len(raw),
                       'originalMetadata': item, 'asset': gltf.get('asset'),
                       'skins': [{'name': s.get('name'), 'joints': len(s.get('joints', []))} for s in gltf.get('skins', [])],
                       'animations': [{'name': a.get('name'), 'channels': len(a.get('channels', []))} for a in gltf.get('animations', [])]}
            (out / 'receipt.json').write_text(json.dumps(receipt, ensure_ascii=False, indent=2) + '\n')
            author = item['user'].get('displayName') or item['user']['username']
            (out / 'CREDITS.md').write_text(
                f'"{item["name"]}" by {author}. Original: {entry["original"]}. License: {license_url}/.\n\n'
                f'Public redistribution: {DATASET} (ODC-By 1.0), Deitke et al., Objaverse, 2022, https://objaverse.allenai.org/.\n\n'
                'Unmodified source preserved for review. No runtime admission, protected viewer extraction or paid generation.\n')
            selected_count += 1
            print('SOURCE_RECEIPT', species, uid, receipt['sha256'], receipt['skins'], receipt['animations'])
        candidates.sort(key=lambda x: (not x['allowedForReview'], not bool(x['animationCount']), x['name'] or ''))
        report['species'][species] = candidates
        print('CANDIDATE_SUMMARY', species, json.dumps([{k: c[k] for k in ('uid', 'name', 'license', 'animationCount', 'allowedForReview')} for c in candidates], ensure_ascii=False))
    report['selectedSourceCount'] = selected_count
    (ROOT / 'candidates.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n')


if __name__ == '__main__':
    main()
