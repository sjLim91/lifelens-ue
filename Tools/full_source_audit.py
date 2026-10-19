#!/usr/bin/env python3
"""Read-only source inventory, cross-reference and causal-boundary audit.

This scanner is diagnostic, not a claim that CI passing proves emergence.
It never changes Core rules, files, save data, or browser runtime.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

TEXT_EXT = {".cpp", ".h", ".hpp", ".c", ".ts", ".tsx", ".js", ".mjs",
            ".py", ".json", ".md", ".yml", ".yaml", ".css", ".html",
            ".svg", ".txt", ".cmake", ".webmanifest"}
SKIP_PARTS = {".git", "node_modules", "__pycache__", "build", "dist"}
IMPORT_RE = re.compile(r"""(?:from\s*|import\s*)['"](\.[^'"]+)['"]""")
LOCAL_INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.MULTILINE)
MARKER_RE = re.compile(r"\b(TODO|FIXME|XXX|HACK)\b")


def tracked_files(root):
    try:
        raw = subprocess.check_output(["git", "ls-files", "-z"], cwd=root)
        names = [Path(s.decode("utf-8")) for s in raw.split(b"\0") if s]
    except (OSError, subprocess.CalledProcessError):
        names = [p.relative_to(root) for p in root.rglob("*") if p.is_file()]
    return sorted(p for p in names if not SKIP_PARTS.intersection(p.parts)
                  and (root / p).is_file())


def code_line(content, needle):
    for line, text in enumerate(content.splitlines(), 1):
        if needle in text:
            return line
    return None


def local_typescript_target(root, origin, module):
    base = origin.parent / module
    choices = [base, *[Path(str(base) + ext) for ext in
                        (".ts", ".tsx", ".js", ".mjs", ".json", ".css")],
               *[base / ("index" + ext) for ext in (".ts", ".tsx", ".js")]]
    if base.suffix in (".js", ".mjs"):
        choices.extend([base.with_suffix(".ts"), base.with_suffix(".tsx")])
    return next((p for p in choices if (root / p).is_file()), None)


def collect(root):
    files = tracked_files(root)
    entries, contents, unresolved, markers = [], {}, [], []
    for name in files:
        full = root / name
        data = full.read_bytes()
        item = {"path": name.as_posix(), "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "text": name.suffix in TEXT_EXT}
        if name.suffix in TEXT_EXT:
            try:
                content = data.decode("utf-8")
            except UnicodeDecodeError:
                item["utf8"] = False
            else:
                item["utf8"] = True
                item["lines"] = len(content.splitlines())
                contents[name.as_posix()] = content
                if name.suffix in (".ts", ".tsx", ".mjs", ".js") and "web/src/" in item["path"]:
                    for module in IMPORT_RE.findall(content):
                        if local_typescript_target(root, name, module) is None:
                            unresolved.append({"path": item["path"],
                                               "module": module})
                if item["path"].startswith("Source/LifeLensCore/") and name.suffix in (".h", ".cpp"):
                    for include in LOCAL_INCLUDE_RE.findall(content):
                        if include.startswith("lifelens/"):
                            where = root / "Source/LifeLensCore/include" / include
                        else:
                            where = full.parent / include
                        if not where.is_file():
                            unresolved.append({"path": item["path"],
                                               "include": include})
                if item["path"].startswith(("Source/", "web/src/", "Tools/")):
                    for line, text in enumerate(content.splitlines(), 1):
                        if MARKER_RE.search(text):
                            markers.append({"path": item["path"], "line": line,
                                            "kind": MARKER_RE.search(text).group(1)})
        entries.append(item)
    return entries, contents, unresolved, markers


def findings_for(contents):
    findings = []
    def add(code, priority, state, description, path, needle, validation):
        src = contents.get(path, "")
        if needle not in src:
            return
        findings.append({"code": code, "priority": priority, "state": state,
                         "description": description, "path": path,
                         "line": code_line(src, needle), "validation": validation})

    sim = "Source/LifeLensCore/src/Simulation.cpp"
    sim_h = "Source/LifeLensCore/include/lifelens/Simulation.h"
    social = "Source/LifeLensCore/include/lifelens/SocialUtility.h"
    bridge = "Source/LifeLensCore/src/WebClientBridge.cpp"
    bridge_h = contents.get("Source/LifeLensCore/include/lifelens/WebClientBridge.h", "")
    binds = contents.get("Source/LifeLensCore/wasm/LifeLensWebBindings.cpp", "")
    js_types = contents.get("web/src/runtime/core-types.ts", "")
    if "Acceptance only records an input; it does not execute or bias goals yet" in contents.get(sim_h, ""):
        add("COG-01", "P1", "source-confirmed capability gap",
            "Accepted model decisions are journal inputs rather than actions or goal bias.",
            sim_h, "Acceptance only records an input",
            "Trace accepted decision -> chosen action with real resident and outcome counters.")
    if "acceptCognitiveProposal(" in contents.get(sim, "") and (
            "acceptCognitiveProposal" not in bridge_h or
            "acceptCognitiveProposal" not in binds or
            "acceptCognitiveProposal" not in js_types):
        add("COG-02", "P1", "source-confirmed integration gap",
            "Web/WASM public runtime does not expose the Core cognition submission entrypoint.",
            sim, "bool Simulation::acceptCognitiveProposal(",
            "Verify an authorized Web-local-model proposal reaches a Core acceptance journal.")
    chooser = re.search(r"inline SocialUtilityDecision chooseSocialUtilityDecision\([\s\S]*?\n\}", contents.get(social, ""))
    if chooser and "candidate.alive" not in chooser.group(0):
        add("SOC-01", "P1", "source-confirmed candidate-selection defect",
            "Global social utility chooser may rank dead residents; downstream execution rejects them.",
            social, "for (const auto& candidate : world.characters)",
            "Add a dead-target regression on a live production NEW GAME, including valid alternative.")
    earth = contents.get(bridge, "")
    if '"snapshotPersistence\\":false' in earth or '\\"snapshotPersistence\\":false' in earth:
        add("EARTH-01", "P1", "source-confirmed feature gap",
            "Geographic origin mapping is not persisted in the Core world snapshot.",
            bridge, 'snapshotPersistence',
            "Earth new game -> save/load -> same region/coordinate and resident/seed identity.")
    if '"terrainGeographicallyConditioned\\":false' in earth or '\\"terrainGeographicallyConditioned\\":false' in earth:
        add("EARTH-02", "P1", "source-confirmed feature gap",
            "Earth origin does not condition local Core terrain and climate.",
            bridge, 'terrainGeographicallyConditioned',
            "Compare two geographically distinct starts with truthful terrain/climate provenance.")
    wildlife = "Source/LifeLensCore/include/lifelens/WildlifeEcology.h"
    if "Not a pathfinder or geography-aware migration yet" in contents.get(wildlife, ""):
        add("ECO-01", "P2", "source-confirmed feature gap",
            "Wildlife movement is confined to a procedural chunk, without terrain-aware migration.",
            wildlife, "Not a pathfinder or geography-aware migration yet",
            "Verify physical obstacle/water handling, multi-chunk travel and save/replay determinism.")
    if "for(int i=0;i<minutes;++i) step();" in contents.get(sim, ""):
        add("PERF-01", "P2", "scalability risk; not benchmarked",
            "Fast-forward runs one full Core step per simulated minute.",
            sim, "for(int i=0;i<minutes;++i) step();",
            "Profile 1/100/1000 residents at 1/100/365/1000 days before optimizing.")
    if "createWildlifeGeometry" in contents.get("web/src/render/wildlife-layer.ts", ""):
        add("VIS-01", "P2", "visual-acceptance gap",
            "Main wildlife scene uses procedural geometry; separately developed GLB assets need final QA.",
            "web/src/render/wildlife-layer.ts", "createWildlifeGeometry",
            "Compare actual Web Observer close-ups, animation retarget and mobile frame budget.")
    return findings


def audit(root):
    entries, content, unresolved, markers = collect(root)
    cmake = content.get("Source/LifeLensCore/CMakeLists.txt", "")
    registered = set(re.findall(r"\blifelens_add_test\(\s*(test_[a-zA-Z0-9_]+)\s*\)", cmake))
    present = {p.stem for p in (root / "Source/LifeLensCore/tests").glob("test_*.cpp")}
    unregistered = sorted(present - registered)
    missing = sorted(registered - present)
    findings = findings_for(content)
    for entry in unresolved:
        findings.append({"code": "REF-01", "priority": "P1", "state": "static reference; verify",
                         "description": "Unresolved local import/include",
                         **entry, "validation": "Confirm build resolver/alias or correct the path."})
    for name in unregistered:
        findings.append({"code": "TEST-01", "priority": "P2", "state": "static registration gap",
                         "description": "Core test file absent from CTest registry",
                         "path": "Source/LifeLensCore/tests/" + name + ".cpp",
                         "validation": "Register test or document intentional exclusion."})
    for name in missing:
        findings.append({"code": "TEST-02", "priority": "P1", "state": "static registration gap",
                         "description": "CTest references a missing test source",
                         "path": "Source/LifeLensCore/tests/" + name + ".cpp",
                         "validation": "Correct test source/registration."})
    findings.sort(key=lambda v: (v["priority"], v.get("path", ""), v["code"]))
    hotspots = sorted((x for x in entries if x.get("lines")), key=lambda x: x["lines"], reverse=True)[:25]
    total = {"files": len(entries), "bytes": sum(x["bytes"] for x in entries),
             "text_files": sum(x.get("utf8") is True for x in entries),
             "source_files": sum(x["path"].startswith(("Source/LifeLensCore/", "web/src/"))
                                 for x in entries),
             "core_registered_tests": len(registered),
             "core_test_sources": len(present),
             "unresolved_imports": len(unresolved),
             "review_markers": len(markers),
             "findings": len(findings)}
    return {"method": "static full tracked-file inventory; feature/gap findings require runtime verification",
            "totals": total, "findings": findings, "hotspots": hotspots,
            "markers": markers, "files": entries}


def write_report(result, output):
    output.mkdir(parents=True, exist_ok=True)
    (output / "source-inventory.json").write_text(
        json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    t = result["totals"]
    lines = ["# LifeLens full source static-audit evidence", "",
             "Diagnostic only. This does not certify code behavior, simulation causality or model quality.",
             "", "## Scope", "",
             "| Measure | Count |", "|---|---:|",
             *[f"| {k} | {v} |" for k, v in t.items()], "",
             "## Findings (observed source facts vs dynamic risks)", ""]
    for x in result["findings"]:
        lines += [f"### {x['priority']} {x['code']}: {x['description']}",
                  f"- State: {x['state']}",
                  f"- Evidence: \`{x.get('path', '?')}:{x.get('line', '?')}\`",
                  f"- Verification: {x['validation']}", ""]
    lines += ["## Largest source files", "", "| File | Lines | Bytes |",
              "|---|---:|---:|"]
    lines += [f"| \`{x['path']}\` | {x['lines']} | {x['bytes']} |" for x in result["hotspots"]]
    lines += ["", "## Constraints", "",
              "- All source files are inventoried, but static inspection alone is not an execution test.",
              "- No Core rules/weights, world state, snapshots, assets, or runtime files are mutated.",
              "- P1/P2 are review priorities, not a claim of runtime regression or an automatic merge blocker.",
              "- Behavioral claims require scenario, seed, observed outcome and exact commit evidence.", ""]
    (output / "SOURCE_AUDIT.md").write_text("\n".join(lines), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--out", type=Path, default=Path("full-source-audit"))
    args = parser.parse_args()
    result = audit(args.root.resolve())
    write_report(result, args.out)
    print(json.dumps({"totals": result["totals"],
                      "findings": result["findings"]}, ensure_ascii=False, indent=2))
    if result["totals"]["files"] == 0:
        raise SystemExit("No files audited")


if __name__ == "__main__":
    main()
