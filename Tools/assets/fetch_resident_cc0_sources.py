"""Acquire pinned CC0 graphical assets only; never import MakeHuman AGPL code."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'docs/asset-reviews/resident-makehuman/source'
OFFICIAL = ('makehumancommunity/makehuman', 'a8bc2d54ff0ac92e78ff71431b1023eda42bf482')
BUNDLED = ('s20220526/makehuman-assets', '8cf9645b975a98eea056b140df11a1d278da0d10')


def get(url, limit=24 * 1024 * 1024):
    request = urllib.request.Request(url, headers={'User-Agent': 'LifeLens-CC0-source-review'})
    with urllib.request.urlopen(request, timeout=90) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError(f'Source exceeds bounded download: {url}')
    return data


def git_hash(data):
    return hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    prior_path = OUT / 'receipts.json'
    prior = {item['file']: item for item in json.loads(prior_path.read_text())['sources']} if prior_path.is_file() else {}
    inputs = [
        ('official', 'LICENSE.md'), ('official', 'LICENSE.ASSETS.md'),
        ('official', 'makehuman/data/3dobjs/base.obj'),
        ('official', 'makehuman/data/rigs/default.mhskel'),
        ('official', 'makehuman/data/rigs/default_weights.mhw'),
        ('official', 'makehuman/data/eyes/high-poly/high-poly.obj'),
        ('official', 'makehuman/data/eyes/high-poly/high-poly.mhclo'),
        ('official', 'makehuman/data/eyes/materials/brown_eye.png'),
        ('assets', 'README.md'), ('assets', 'LICENSE.txt'),
    ]
    for sex, hairstyle in [('female', 'ponytail01'), ('male', 'short02')]:
        for target in [f'universal-{sex}-young-averagemuscle-averageweight', f'caucasian-{sex}-young']:
            inputs.append(('official', f'makehuman/data/targets/macrodetails/{target}.target'))
        inputs.append(('assets', f'base/skins/textures/young_lightskinned_{sex}_diffuse.png'))
        for suffix in ['.obj', '.mhclo', '_diffuse.png']:
            inputs.append(('assets', f'base/hair/{hairstyle}/{hairstyle}{suffix}'))
    if '--verify-local' in sys.argv:
        expected_files = {f'{label}/{path}' for label, path in inputs}
        assert set(prior) == expected_files, 'Incomplete pinned source receipt'
        for name, receipt in prior.items():
            path = (OUT / name).resolve()
            assert path.is_relative_to(OUT.resolve()), 'Source receipt escapes its directory'
            data = path.read_bytes()
            assert len(data) == receipt['bytes'], name
            assert hashlib.sha256(data).hexdigest() == receipt['sha256'], name
            assert receipt['license'] == 'CC0-1.0', name
            pointer = receipt.get('lfsPointer')
            if pointer:
                assert pointer['sha256'] == receipt['sha256'] and pointer['bytes'] == len(data), name
            else:
                assert git_hash(data) == receipt['gitBlob'], name
        assert 'released under CC0' in (OUT/'official/LICENSE.md').read_text()
        assert 'Creative Commons CC0' in (OUT/'assets/README.md').read_text()
        print(json.dumps({'status':'VERIFIED_PINNED_LOCAL_CC0_SOURCES','assets':len(prior)}))
        return
    trees = {}
    for label, (repo, pin) in [('official', OFFICIAL), ('assets', BUNDLED)]:
        tree = json.loads(get(f'https://api.github.com/repos/{repo}/git/trees/{pin}?recursive=1', 12 * 1024 * 1024))
        assert not tree.get('truncated'), 'Cannot verify sources against an incomplete pinned tree'
        trees[label] = {item['path']: item['sha'] for item in tree['tree'] if item['type'] == 'blob'}

    def acquire(item):
        label, path = item
        repo, pin = OFFICIAL if label == 'official' else BUNDLED
        expected = trees[label][path]
        raw_url = f'https://raw.githubusercontent.com/{repo}/{pin}/{path}'
        dest = OUT / label / path
        previous = prior.get(str(dest.relative_to(OUT)))
        if previous and previous['gitBlob'] == expected and dest.is_file() and hashlib.sha256(dest.read_bytes()).hexdigest() == previous['sha256']:
            return previous
        raw = get(raw_url)
        assert git_hash(raw) == expected, f'Pinned Git blob mismatch: {path}'
        url, source_pointer = raw_url, None
        if raw.startswith(b'version https://git-lfs.github.com/spec/v1'):
            pointer = raw.decode('ascii')
            sha = re.search(r'^oid sha256:([0-9a-f]{64})$', pointer, re.M).group(1)
            size = int(re.search(r'^size (\d+)$', pointer, re.M).group(1))
            url = f'https://media.githubusercontent.com/media/{repo}/{pin}/{path}'
            data = get(url)
            assert len(data) == size and hashlib.sha256(data).hexdigest() == sha
            source_pointer = {'gitBlob': expected, 'sha256': sha, 'bytes': size}
        else:
            data = raw
        if path.endswith('.png'):
            assert data.startswith(b'\x89PNG\r\n\x1a\n'), f'Expected original PNG, not HTML or an LFS pointer: {path}'
        dest.parent.mkdir(parents=True, exist_ok=True)
        if not dest.is_file() or dest.read_bytes() != data:
            dest.write_bytes(data)
        metadata = []
        if path.endswith(('.obj', '.mhclo', '.mhskel')):
            metadata = [line for line in data[:20000].decode('utf-8', errors='replace').splitlines()
                        if re.search(r'author|copyright|license|no.?ai|non.?commercial', line, re.I)][:20]
        return {'file': str(dest.relative_to(OUT)), 'sourceUrl': f'https://github.com/{repo}/blob/{pin}/{path}',
                'downloadUrl': url, 'creator': 'MakeHuman team; individual asset headers retained',
                'license': 'CC0-1.0', 'requiredAttribution': 'None; provenance and courtesy credit retained',
                'retrievedAt': datetime.now(timezone.utc).isoformat(), 'gitBlob': expected,
                'sha256': hashlib.sha256(data).hexdigest(), 'bytes': len(data),
                'lfsPointer': source_pointer, 'originalAuthorshipHeaders': metadata}

    with ThreadPoolExecutor(max_workers=4) as pool:
        receipts = list(pool.map(acquire, inputs))
    for label, path, snippet in [('official', 'LICENSE.md', 'released under CC0'), ('assets', 'README.md', 'Creative Commons CC0')]:
        assert snippet in (OUT / label / path).read_text(), 'Pinned graphical-asset license grant changed'
    report = {'status': 'SOURCE_ONLY_REQUIRES_NATIVE_RETARGET_AND_OBSERVER_VALIDATION',
              'sources': receipts, 'softwareCodeImported': False, 'paidServices': False, 'mergeAllowed': False}
    (OUT / 'receipts.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'status': report['status'], 'assets': len(receipts), 'bytes': sum(r['bytes'] for r in receipts)}))


if __name__ == '__main__':
    main()

