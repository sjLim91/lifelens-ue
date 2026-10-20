"""Run every shared source and behavioral Web regression once; no screenshots."""
import argparse
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "web"
EXTRAS = ("runtime-resilience/continuity.mjs", "cognitive-agent/shadow-ui.mjs", "procedural-environment/lifecycle.mjs",
          "procedural-environment/grounding-wildlife.mjs", "procedural-environment/native-wildlife.mjs", "characters/export-review.mjs")
def regression_entries(tests):
    entries = set(tests.glob("*/run.mjs"))
    entries.update(tests.glob("*/integration.mjs"))
    entries.update(tests / p for p in EXTRAS if (tests / p).exists())
    return sorted(entries)


def run(entries):
    for entry in entries:
        print(f"CI Web: {entry.relative_to(WEB)}", flush=True)
        subprocess.run(["node", str(entry)], cwd=WEB, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("regressions",))
    parser.parse_args()
    entries = regression_entries(WEB / "tests")
    if not entries:
        raise RuntimeError("No Web regression entrypoints found")
    run(entries)


if __name__ == "__main__":
    main()
