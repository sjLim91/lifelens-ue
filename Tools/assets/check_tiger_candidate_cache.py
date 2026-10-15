"""Reuse byte-identical build inputs/outputs, never cache a quality conclusion."""
import hashlib
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def digest_matches(path, expected):
    target = ROOT / path
    return target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == expected


def main():
    lock = json.loads((ROOT / 'Tools/assets/tiger-candidate-cache.json').read_text())
    models = all(digest_matches(path, expected) for path, expected in lock['files'].items())
    models = models and (ROOT / 'docs/asset-reviews/tiger-topoforge/tiger-rigged.blend').is_file()
    models = models and (ROOT / 'docs/asset-reviews/tiger-topoforge/inspection-positions.json').is_file()
    try:
        core_tree = subprocess.check_output(['git', 'rev-parse', 'HEAD:Source/LifeLensCore'], cwd=ROOT, text=True).strip()
    except subprocess.CalledProcessError:
        core_tree = ''
    fixture = core_tree == lock['core_tree'] and digest_matches(lock['fixture'], lock['fixture_sha256'])
    result = {'rebuild_models': str(not models).lower(), 'rebuild_fixture': str(not fixture).lower()}
    output = os.environ.get('GITHUB_OUTPUT')
    if output:
        with open(output, 'a') as handle:
            for key, value in result.items():
                handle.write(f'{key}={value}\n')
    print(json.dumps({**result, 'quality_accepted': False, 'web_checks_and_captures': 'ALWAYS_RUN'}))


if __name__ == '__main__':
    main()
