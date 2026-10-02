#!/usr/bin/env python3
"""Download the approved CC0 character assets, verify pinned Git blobs, stage for Vite."""
import hashlib
import json
import struct
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'web/public/vendor/characters'
ASSETS = [
    ('character.glb', 'programasweights/avatar', 'ddd5fc34a445bcded3cf9836607aaeebc19a5c78', 'public/assets/character.glb', 'b3fd79533fdb9fcedd077744f7e120920eb6cc97'),
    ('ual1.glb', 'Seyamalam/blood-league-kickoff', 'aa02a4e6d8337a0604d2da131bcbbeb1f01badf0', 'public/assets/vendor/quaternius/universal-animation-library.glb', '4fccf561b9b2ef73f611efe21981ef8739080065'),
    ('ual2.glb', 'richardanaya/metaverse-avatar', '84fd636910bf713099010efbab7f3c84550f4bcb', 'anims/UAL2_Standard.glb', 'dc684c2a664927964307e8eb7b27b0000ebf6a18'),
]

def git_digest(data):
    return hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest()

def download(url):
    with urllib.request.urlopen(url, timeout=40) as response:
        data = response.read(20 * 1024 * 1024 + 1)
    if len(data) > 20 * 1024 * 1024:
        raise ValueError('asset exceeds download budget')
    return data

def glb_document(data):
    magic, version, length = struct.unpack_from('<4sII', data)
    if magic != b'glTF' or version != 2 or length != len(data):
        raise ValueError('invalid GLB header')
    size, kind = struct.unpack_from('<II', data, 12)
    if kind != 0x4E4F534A or size + 20 > len(data):
        raise ValueError('invalid GLB JSON chunk')
    doc = json.loads(data[20:20 + size])
    if any(b.get('uri') for b in doc.get('buffers', [])):
        raise ValueError('external buffers are not admitted')
    if any(i.get('uri') for i in doc.get('images', [])):
        raise ValueError('external textures are not admitted')
    return doc

def main():
    DEST.mkdir(parents=True, exist_ok=True)
    receipts = []
    for name, repo, commit, source_path, expected in ASSETS:
        target = DEST / name
        url = f'https://raw.githubusercontent.com/{repo}/{commit}/{source_path}'
        data = target.read_bytes() if target.exists() else b''
        if git_digest(data) != expected:
            data = download(url)
        if git_digest(data) != expected:
            raise ValueError(f'{name}: pinned Git blob mismatch')
        doc = glb_document(data)
        temp = target.with_suffix('.tmp')
        temp.write_bytes(data)
        temp.replace(target)
        record = dict(file=name, source=url, license='CC0-1.0', gitBlob=expected,
                      bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                      animations=[a.get('name', '') for a in doc.get('animations', [])],
                      nodes=[n.get('name', '') for n in doc.get('nodes', [])])
        receipts.append(record)
        print(json.dumps(record), flush=True)
    (DEST / 'receipt.json').write_text(json.dumps(receipts, indent=2) + '\n')
    (DEST / 'LICENSE.txt').write_text(
        'Character and animation assets by Quaternius. CC0 1.0 Universal.\n'
        'https://creativecommons.org/publicdomain/zero/1.0/\n'
        'https://quaternius.com/packs/universalbasecharacters.html\n'
        'https://quaternius.com/packs/universalanimationlibrary.html\n'
        'https://quaternius.com/packs/universalanimationlibrary2.html\n'
        'Pinned mirrors and verified hashes: receipt.json.\n')

if __name__ == '__main__':
    main()
