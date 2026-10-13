"""Select bounded CI work from the actual Git diff, without third-party actions."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SCREENS = ("environment", "consequences", "characters", "facilities",
           "lifecycle", "social", "production", "settlement")
SCREEN_PREFIXES = {
    "environment": ("web/src/render/terrain-", "web/src/render/water-", "web/src/render/vegetation-",
                    "web/src/render/procedural-", "web/src/render/plant-", "web/src/render/tree-",
                    "web/src/render/ground-detail-", "web/src/render/natural-resource-", "web/src/render/wildlife-",
                    "web/tests/procedural-environment/"),
    "consequences": ("web/src/render/environment-", "web/src/render/weather-", "web/src/render/surface-consequence-",
                     "web/tests/environment-consequences/"),
    "characters": ("web/src/render/resident-", "web/src/character-", "web/public/vendor/characters/",
                   "web/tests/characters/", "web/tests/procedural-motion/", "web/tests/resident-motion/",
                   "Tools/prepare_web_character_assets.py"),
    "facilities": ("web/src/render/facility-", "web/tests/facility-construction/"),
    "lifecycle": ("web/src/render/resident-life-", "web/tests/resident-lifecycle/"),
    "social": ("web/src/render/social-", "web/src/render/resident-social-", "web/tests/social-events/"),
    "production": ("web/src/render/resident-props", "web/src/render/resident-production-",
                   "web/src/render/production-material-", "web/src/render/facility-production-",
                   "web/src/render/stored-goods-", "web/tests/production-logistics/"),
    "settlement": ("web/src/render/settlement-", "web/tests/settlement-trade/"),
}
CORE_PREFIX = "Source/LifeLensCore/"


def select_scope(paths, full=False):
    paths = set(paths)
    selected = set(SCREENS if full else ())
    web = full or any(p.startswith(("web/", "Clients/Web/")) or (
        p.startswith(CORE_PREFIX) and "/tests/" not in p and "/harness/" not in p) for p in paths)
    core = full or any(p.startswith(CORE_PREFIX) for p in paths)
    long_run = full or any(
        p.startswith((CORE_PREFIX + "src/", CORE_PREFIX + "include/"))
        and not Path(p).name.startswith("WebClientBridge") for p in paths)
    if any(p in {"Tools/verify_sleep_longrun.py", "Tools/test_verify_sleep_longrun.py",
                 "tasks/WEATHER_SLEEP_ERA_LONGRUN.json"} for p in paths):
        core = long_run = True
    for path in paths:
        if path.startswith(".github/workflows/") or path in {"Tools/ci_scope.py", "Tools/ci_web.py"}:
            web = True
            selected.update(SCREENS)
        if path.startswith("Tools/validate_web_") or path == "Tools/prepare_web_character_assets.py":
            web = True
            selected.update(SCREENS)
        # A shared runtime, dependency or surface affects every scene. Fail
        # conservatively for unclassified render files, never silently omit QA.
        if path.startswith(("web/src/runtime/", "web/src/observer/")) or path in {
            "web/package.json", "web/package-lock.json", "web/tsconfig.json", "web/vite.config.ts",
            "web/src/styles.css", "web/src/App.tsx", "web/src/render/world-scene.ts",
            "web/src/render/world-renderer.ts", "web/src/render/procedural-asset-kit.ts",
            "web/src/render/resident-grounding.ts", "web/src/render/terrain-geometry.ts",
        }:
            selected.update(SCREENS)
        matched = False
        for screen, prefixes in SCREEN_PREFIXES.items():
            if path.startswith(prefixes):
                selected.add(screen)
                matched = True
        if path.startswith("web/src/render/") and not matched:
            selected.update(SCREENS)
        if path.startswith(CORE_PREFIX) and "/tests/" not in path:
            filename = Path(path).name
            if filename.startswith(("Continuous", "Hydrology", "Natural", "WorldGenesis", "Macro", "Wildlife")):
                selected.update(("environment", "consequences", "facilities"))
            elif filename.startswith(("Civilization", "Facility", "Primitive")):
                selected.update(("facilities", "production", "settlement"))
            elif filename.startswith(("Character", "Genetics", "Birth", "Pregnancy", "Genealogy", "Family")):
                selected.update(("characters", "lifecycle", "social"))
            elif filename in {"World.h", "Simulation.cpp", "WebClientBridge.cpp"}:
                selected.update(SCREENS)
    return {"web": web, "core": core, "long_run": long_run,
            "screens": [s for s in SCREENS if s in selected]}


def changed_paths(event, event_name, root=ROOT):
    if event_name in {"schedule", "workflow_dispatch"}:
        return [], True
    if event_name == "pull_request":
        base, head = event["pull_request"]["base"]["sha"], event["pull_request"]["head"]["sha"]
        comparison = f"{base}...{head}"
    elif event_name == "push":
        base, head = event.get("before", ""), event.get("after", "")
        if not base or set(base) == {"0"}:
            return [], True
        comparison = f"{base}..{head}"
    else:
        raise ValueError(f"Unsupported CI event: {event_name}")
    # Missing commits must fail the selector, not masquerade as no changes.
    # Treat a rename as removal + addition so both old and new consumers are
    # covered. Git's default rename detection reports only the new pathname.
    result = subprocess.run(["git", "diff", "--no-renames", "--name-only", "-z", comparison],
                            cwd=root, check=True, capture_output=True)
    return [p for p in result.stdout.decode("utf-8").split("\0") if p], False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--event", default=os.environ.get("GITHUB_EVENT_PATH"))
    parser.add_argument("--event-name", default=os.environ.get("GITHUB_EVENT_NAME"))
    parser.add_argument("--output", default="ci-scope.json")
    args = parser.parse_args()
    paths, full = changed_paths(json.loads(Path(args.event).read_text()), args.event_name)
    scope = select_scope(paths, full)
    Path(args.output).write_text(json.dumps(scope, indent=2) + "\n")
    if os.environ.get("GITHUB_OUTPUT"):
        with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
            for key in ("web", "core", "long_run"):
                output.write(f"{key}={str(scope[key]).lower()}\n")
            output.write(f"screens={json.dumps(scope['screens'], separators=(',', ':'))}\n")
    print(json.dumps(scope))


if __name__ == "__main__":
    main()
