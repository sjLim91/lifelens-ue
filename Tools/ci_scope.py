"""Select bounded CI work from the actual Git diff, without third-party actions."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
CORE_PREFIX = "Source/LifeLensCore/"


def select_scope(paths, full=False):
    """Only necessary Core and Web code checks; no image capture selection."""
    paths = set(paths)
    web = full or any(p.startswith(("web/", "Clients/Web/")) or (
        p.startswith(CORE_PREFIX) and "/tests/" not in p and "/harness/" not in p) for p in paths)
    core = full or any(p.startswith(CORE_PREFIX) for p in paths)
    long_run = full or any(
        p.startswith((CORE_PREFIX + "src/", CORE_PREFIX + "include/"))
        and not Path(p).name.startswith("WebClientBridge") for p in paths)
    if any(p in {"Tools/verify_sleep_longrun.py", "Tools/test_verify_sleep_longrun.py",
                 "tasks/WEATHER_SLEEP_ERA_LONGRUN.json"} for p in paths):
        core = long_run = True
    if any(p.startswith(".github/workflows/") or p in {"Tools/ci_scope.py", "Tools/ci_web.py"}
           or p.startswith("Tools/validate_web_")
           or p == "Tools/prepare_web_character_assets.py" for p in paths):
        web = True
    return {"web": web, "core": core, "long_run": long_run}


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
    print(json.dumps(scope))


if __name__ == "__main__":
    main()
