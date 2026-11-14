#!/usr/bin/env python3
"""Verify deployed bytes against the published gh-pages Git tree and runtime manifest."""
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import urllib.request

expected = sys.argv[1]
assert re.fullmatch(r'[0-9a-f]{40}', expected)
out = Path(sys.argv[2]); out.mkdir(parents=True, exist_ok=True)
base = 'https://sjlim91.github.io/lifelens-ue/'
repo = 'repos/sjLim91/lifelens-ue'
def api(path):
    return json.loads(subprocess.check_output(['gh', 'api', repo + path]))
pages = api('/branches/gh-pages')['commit']['sha']
tree = api('/git/trees/' + pages + '?recursive=1')
assert not tree.get('truncated')
blobs = {e['path']: e['sha'] for e in tree['tree'] if e['type'] == 'blob'}
checks = []
def fetch(path):
    url = base + path + '?m8-proof=' + expected
    req = urllib.request.Request(url, headers={'Cache-Control': 'no-cache'})
    with urllib.request.urlopen(req, timeout=60) as response:
        assert response.status == 200, (path, response.status)
        body = response.read()
        git_hash = hashlib.sha1(b'blob ' + str(len(body)).encode() + b'\0' + body).hexdigest()
        assert git_hash == blobs[path], ('public bytes differ from pinned gh-pages tree', path, pages)
        checks.append({'path': path, 'status': response.status, 'bytes': len(body),
                       'sha256': hashlib.sha256(body).hexdigest(), 'gitBlob': git_hash,
                       'matchesPublishedTree': True})
    target = out / path
    target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(body)
    return body
index = fetch('index.html').decode()
launch = fetch('launch.html').decode()
assert '라이프렌즈 미리보기' in launch and 'lifelens-bootstrap' in index
commit = fetch('runtime/RUNTIME_COMMIT.txt').decode().strip()
assert commit == expected, ('wrong runtime source', commit, expected)
manifest = fetch('runtime/SHA256SUMS.txt').decode()
for name in ['lifelens_core.js', 'lifelens_core.wasm']:
    body = fetch('runtime/' + name)
    expected_hash = next(line.split()[0] for line in manifest.splitlines() if line.split()[-1].endswith(name))
    assert hashlib.sha256(body).hexdigest() == expected_hash
assets = sorted(set(re.findall(r'(?:src|href)="\./(assets/[^\"]+\.(?:js|css))"', index)))
assert any(p.endswith('.js') for p in assets)
for asset in assets:
    body = fetch(asset)
    if asset.endswith('.js'):
        assert '발명과 제작 지식' in body.decode(), 'deployed bundle missing M8 invention UI'
proof = {'validatedHead': '8dcdcbdb1ff0a99085913c804855fd7f4509eba1',
         'runtimeSource': commit, 'base': base, 'pagesCommit': pages,
         'checkedAtUtc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
         'manifestVerified': True, 'assets': checks}
(out / 'public-proof.json').write_text(json.dumps(proof, ensure_ascii=False, indent=2) + '\n')
print('PASS public app/launcher/assets/runtime manifest and pinned gh-pages byte hashes; runtime=' + commit)
