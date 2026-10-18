"""Stage only the pinned source derivatives; preserve the established tiger."""
import hashlib
import html
import json
from pathlib import Path
import shutil
import struct

ROOT = Path(__file__).resolve().parents[2]
REVIEW = ROOT / 'docs/asset-reviews/native-wildlife'
DEST = ROOT / 'web/public/vendor/wildlife'
MARKER = '\n\n<!-- LifeLens native wildlife four species -->\n'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    pins = json.loads((ROOT / 'Tools/assets/native-wildlife-pins.json').read_text())
    report = json.loads((REVIEW / 'derived/lods.json').read_text())
    assert {r['species'] for r in report['reports']} == set(pins)
    credits = ['# 토끼·사슴·늑대·담수어 원본과 변경 내역', '']
    staged = []
    for row in report['reports']:
        species = row['species']
        pin = pins[species]
        original = REVIEW / species / pin['uid']
        assert digest(original / 'source.glb') == pin['sha256']
        receipt = json.loads((original / 'receipt.json').read_text())
        assert receipt['license'] == 'CC-BY-4.0' and receipt['sha256'] == pin['sha256']
        credits.append(f'- “{receipt["name"]}” by {pin["author"]}. [Original]({pin["original"]}), [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). Source SHA-256: `{pin["sha256"]}`. Retrieved {receipt["retrievedAt"]}. Public redistribution: {receipt["downloadUrl"]}.')
        for item in row['files']:
            source = REVIEW / 'derived' / item['file']
            assert digest(source) == item['sha256'] and source.stat().st_size == item['bytes']
            raw = source.read_bytes()
            assert struct.unpack_from('<4sII', raw) == (b'glTF', 2, len(raw))
            size = struct.unpack_from('<I', raw, 12)[0]
            doc = json.loads(raw[20:20 + size])
            contract = doc['asset']['extras']['lifelensWildlife']
            assert contract['source_sha256'] == pin['sha256'] and contract['species'] == species
            assert contract['tier'] in ('near', 'mid', 'far') and contract['license'] == 'CC-BY-4.0'
            assert not doc['asset']['extras'].get('lifelensInspectionOnly')
            if contract['tier'] == 'far':
                assert not doc.get('skins') and not doc.get('animations')
            else:
                assert doc.get('skins') and len(doc.get('animations', [])) >= (2 if species == 'FreshwaterFish' else 7)
            DEST.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, DEST / item['file'])
            staged.append({'species': species, **item})
    credits += ['', 'Public dataset: AI2 Objaverse (ODC-By 1.0). Deitke et al., Objaverse: A Universe of Annotated 3D Objects, 2022, https://objaverse.allenai.org/.', '',
        'LifeLens adaptations: source-preserving metric/forward normalization; native 29/50/90/19-joint rigs; original deer locomotion and trout swimming; authored hare/wolf locomotion and behavior poses; source foot trajectories; brown deer PBR pigments; actual reduced mid meshes; source-pose far batches with colors sampled from original UV textures. The female deer omits the separate source antler surface. No primitive replacement body or paid generation.', '',
        'Trout texture: Clker-Free-Vector-Images, [Trout, Fish, Rainbow Trout](https://pixabay.com/vectors/trout-fish-rainbow-trout-animal-294469/), published April 4, 2014. Pixabay [Terms §4](https://pixabay.com/service/terms/) classify content published before January 9, 2019 as CC0. Original image reference is retained in the model author’s source metadata.', '',
        'Validation receipts are recorded separately. Browser viewports do not establish physical Android FPS.']
    base = (DEST / 'CREDITS.md').read_text().split(MARKER)[0]
    text = base + MARKER + '\n'.join(credits) + '\n'
    (DEST / 'CREDITS.md').write_text(text)
    page = '<!doctype html><html lang="ko"><meta charset="utf-8"><meta name="viewport" content="width=device-width"><title>LifeLens 동물 자산 출처</title><style>body{max-width:56rem;margin:2rem auto;padding:1rem;font:16px/1.6 system-ui}pre{white-space:pre-wrap;overflow-wrap:anywhere}</style><h1>동물 3D 자산 출처</h1><p><a href="https://creativecommons.org/licenses/by/4.0/">CC BY 4.0</a> · <a href="https://creativecommons.org/publicdomain/zero/1.0/">CC0</a></p><pre>' + html.escape(text) + '</pre></html>\n'
    (DEST / 'CREDITS.html').write_text(page)
    (DEST / 'native-wildlife-receipt.json').write_text(json.dumps({'status': 'SOURCE_DERIVED_RUNTIME_CANDIDATES', 'sourcePins': pins, 'files': staged, 'android': 'NOT_MEASURED'}, indent=2) + '\n')
    (REVIEW / 'derived/CREDITS.md').write_text('\n'.join(credits) + '\n')
    print('STAGED_NATIVE_WILDLIFE', len(staged), 'files; tiger GLBs unchanged')


if __name__ == '__main__':
    main()
