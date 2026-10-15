"""Stage only the pinned-source derived Draft candidate, never download substitutes."""
import hashlib
import json
from pathlib import Path
import shutil
import struct

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'docs/asset-reviews/tiger-topoforge'
DEST = ROOT / 'web/public/vendor/wildlife'
SOURCE_HASH = 'f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be'


def main():
    if hashlib.sha256((SOURCE / 'source.glb').read_bytes()).hexdigest() != SOURCE_HASH:
        raise ValueError('Original source hash mismatch; inspection reconstructions are forbidden')
    DEST.mkdir(parents=True, exist_ok=True)
    receipts = []
    for name, tier in [('tiger-rigged.glb', 'near'), ('tiger-mid.glb', 'mid')]:
        data = (SOURCE / name).read_bytes()
        magic, version, length = struct.unpack_from('<4sII', data)
        size, kind = struct.unpack_from('<II', data, 12)
        assert magic == b'glTF' and version == 2 and length == len(data) and kind == 0x4e4f534a
        doc = json.loads(data[20:20+size])
        contract = doc['asset']['extras']['lifelensTiger']
        assert contract['source_sha256'] == SOURCE_HASH and contract['tier'] == tier
        assert len(doc['skins'][0]['joints']) >= 24 and len(doc['animations']) == 7
        assert not any(item.get('uri') for item in doc.get('images', [])+doc.get('buffers', []))
        shutil.copyfile(SOURCE / name, DEST / name)
        receipts.append(dict(file=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), **contract))
    shutil.copyfile(SOURCE / 'CREDITS.md', DEST / 'CREDITS.md')
    (DEST / 'receipt.json').write_text(json.dumps(receipts, indent=2)+'\n')
    (DEST / 'CREDITS.html').write_text('''<!doctype html><html lang="ko"><meta charset="utf-8">
<meta name="viewport" content="width=device-width"><title>LifeLens 3D 자산 출처</title>
<body><h1>3D 자산 출처</h1><p>“Tiger” by TopoForge, generated with Sloyd,
licensed under <a href="https://creativecommons.org/licenses/by/4.0/">CC BY 4.0</a>.</p>
<p><a href="https://www.sloyd.ai/free-3d-models/model/tiger-7m4ap2k1">원본 모델</a></p>
<p>LifeLens 수정: 전시 받침 제거, 작은 분리 조각 정리, 자세·크기 조정,
스켈레톤·스킨 가중치·행동 애니메이션 추가, 텍스처 축소 및 LOD 제작.</p>
<p>원본 취득 시각·해시: <a href="CREDITS.md">상세 출처</a>.
배포 파일 해시: <a href="receipt.json">자산 기록</a>.</p></body></html>''')


if __name__ == '__main__':
    main()
