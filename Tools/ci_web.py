"""Run each Web regression entrypoint once; capture only selected scenes."""
import argparse
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
WEB = ROOT / "web"
EXTRAS = ("runtime-resilience/continuity.mjs", "cognitive-agent/shadow-ui.mjs", "procedural-environment/lifecycle.mjs",
          "procedural-environment/grounding-wildlife.mjs", "characters/export-review.mjs")
CAPTURES = {
    "environment": ("procedural-environment/browser-review.mjs", "procedural-environment/wildlife-browser-review.mjs"),
    "consequences": ("environment-consequences/browser-review.mjs",),
    "characters": ("procedural-motion/browser-review.mjs",),
    "facilities": ("facility-construction/browser-review.mjs",),
    "lifecycle": ("resident-lifecycle/browser-review.mjs",),
    "social": ("social-events/browser-review.mjs",),
    "production": ("production-logistics/browser-review.mjs",),
    "settlement": ("settlement-trade/browser-review.mjs",),
}


def regression_entries(tests):
    entries = set(tests.glob("*/run.mjs"))
    entries.update(tests.glob("*/integration.mjs"))
    entries.update(tests / p for p in EXTRAS if (tests / p).exists())
    return sorted(entries)


def capture_entries(tests, screens):
    unknown = set(screens) - CAPTURES.keys()
    if unknown:
        raise ValueError(f"Unknown capture groups: {sorted(unknown)}")
    entries = set()
    for screen in screens:
        primary, *optional = CAPTURES[screen]
        required = tests / primary
        if not required.exists():
            raise FileNotFoundError(f"Required {screen} capture is missing: {primary}")
        entries.add(required)
        # Only the future wildlife extension is optional. Existing scene QA
        # cannot silently disappear when a PR deletes or moves its script.
        entries.update(tests / p for p in optional if (tests / p).exists())
    return sorted(entries)


def run(entries):
    for entry in entries:
        print(f"CI Web: {entry.relative_to(WEB)}", flush=True)
        subprocess.run(["node", str(entry)], cwd=WEB, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("regressions", "captures"))
    parser.add_argument("--scope", default="ci-scope.json")
    args = parser.parse_args()
    entries = regression_entries(WEB / "tests") if args.mode == "regressions" else capture_entries(
        WEB / "tests", json.loads(Path(args.scope).read_text())["screens"])
    if args.mode == "regressions" and not entries:
        raise RuntimeError("No Web regression entrypoints found")
    run(entries)


if __name__ == "__main__":
    main()
