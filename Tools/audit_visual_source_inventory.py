#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
production_roots = (
    root / "Source" / "LifeLens",
    root / "Source" / "LifeLensCore" / "include",
    root / "Source" / "LifeLensCore" / "src",
)
patterns = {
    "engine_basic_shape": "/Engine/BasicShapes/",
    "basic_shape_material": "BasicShapeMaterial",
    "engine_emissive_material": "EmissiveMeshMaterial",
    "force_visible": "SetVisibility(true",
    "force_unhidden": "SetHiddenInGame(false",
}
suffixes = {".h", ".hpp", ".cpp", ".c", ".cs"}

files = []
for base in production_roots:
    if base.exists():
        files.extend(
            path for path in base.rglob("*")
            if path.is_file() and path.suffix.lower() in suffixes
        )
files = sorted(set(files))

hits = []
counts = {key: 0 for key in patterns}
for path in files:
    text = path.read_text(encoding="utf-8", errors="strict")
    rel = path.relative_to(root).as_posix()
    for line_no, line in enumerate(text.splitlines(), 1):
        for key, token in patterns.items():
            if token in line:
                counts[key] += 1
                hits.append((rel, line_no, key, line.strip()))

print(f"LifeLens visual source inventory: files={len(files)} hits={len(hits)}")
for rel, line_no, key, line in hits:
    print(f"VISUAL_AUDIT|{key}|{rel}:{line_no}|{line}")
for key in patterns:
    print(f"VISUAL_AUDIT_COUNT|{key}|{counts[key]}")
