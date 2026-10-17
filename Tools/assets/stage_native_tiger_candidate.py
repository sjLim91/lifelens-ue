"""Stage only the hash-verified native CC BY candidate; not quality acceptance."""
from pathlib import Path
import hashlib
import html
import json
import shutil
import struct

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'docs/asset-reviews/amil-tiger'
DEST = ROOT / 'web/public/vendor/wildlife'
PINS = {
    'fc2c7fd74dd04e39935803b884c77258': '6e152d1d98c6149d4241c292bf21cbc9e8ab16f81a456ea1515d174f7f7d1e40',
    '733afd855f5d4c378f041cc89e231770': '4a6e97f3130d5f525e52e958cfcd687ee0ba1224eca18cff1a070b4765caec28',
}


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    report = json.loads((SOURCE / 'native-authoring.json').read_text())
    assert report['status'] != 'INSPECTION_ONLY', 'Inspection derivatives cannot enter the Web application'
    assert report['sourceHashes'] == list(PINS.values())
    for uid, sha in PINS.items():
        assert digest(ROOT / 'docs/asset-reviews/distributed-tigers' / uid / 'source.glb') == sha
    receipts = report['receipts']
    assert [r['file'] for r in receipts] == ['tiger-rigged.glb', 'tiger-mid.glb', 'tiger-far.glb']
    for tier, receipt in zip(['near', 'mid', 'far'], receipts):
        path = SOURCE / receipt['file']
        raw = path.read_bytes()
        assert digest(path) == receipt['sha256'] and len(raw) == receipt['bytes']
        assert raw[:4] == b'glTF' and struct.unpack_from('<I', raw, 8)[0] == len(raw)
        size = struct.unpack_from('<I', raw, 12)[0]
        doc = json.loads(raw[20:20 + size])
        extras = doc['asset']['extras']
        contract = extras['lifelensTiger']
        assert not extras.get('lifelensInspectionOnly')
        assert contract['version'] == 2 and contract['tier'] == tier
        assert contract['source_sha256'] == list(PINS.values())[0]
        assert contract['donor_sha256'] == list(PINS.values())[1]
        assert contract['license'] == 'CC-BY-4.0' and contract['status'] != 'INSPECTION_ONLY'
        if tier == 'far':
            assert contract['render_mode'] == 'instanced-static' and contract['bone_count'] == 0
            assert not doc.get('skins') and not doc.get('animations')
            assert len(doc['scenes']) == 3 and contract['triangles'] <= 1100
        else:
            assert len(doc['skins'][0]['joints']) == 47
            assert len(doc['animations']) == 7 and len(contract['paws']) == 4
            assert contract['triangles'] <= (5000 if tier == 'near' else 3500)
    DEST.mkdir(parents=True, exist_ok=True)
    for receipt in receipts:
        shutil.copyfile(SOURCE / receipt['file'], DEST / receipt['file'])
    shutil.copyfile(SOURCE / 'CREDITS.md', DEST / 'CREDITS.md')
    text = (SOURCE / 'CREDITS.md').read_text()
    page = '<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>LifeLens 3D 자산 출처</title><style>body{max-width:54rem;margin:2rem auto;padding:1rem;font:16px/1.6 system-ui}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h1>호랑이 3D 자산 출처</h1><p><a href="https://creativecommons.org/licenses/by/4.0/">CC BY 4.0</a> · <a href="https://sketchfab.com/3d-models/fc2c7fd74dd04e39935803b884c77258">Amil — Running Tiger</a> · <a href="https://sketchfab.com/3d-models/733afd855f5d4c378f041cc89e231770">Unity (shehab house) — 보행 원본</a></p><pre>' + html.escape(text) + '</pre></html>\n'
    (DEST / 'CREDITS.html').write_text(page)
    (DEST / 'receipt.json').write_text(json.dumps({'status': report['status'], 'sourcePins': report['sourcePins'], 'receipts': receipts, 'visualQuality': 'REQUIRES_ACTUAL_OBSERVER_REVIEW', 'android': 'NOT_MEASURED', 'mergeAllowed': False}, indent=2) + '\n')
    print('STAGED_HASH_VERIFIED_NATIVE_CANDIDATE; Observer and physical-device acceptance remain pending')


if __name__ == '__main__':
    main()
