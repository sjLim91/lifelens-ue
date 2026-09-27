#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

facility_header = (
    ROOT / "Source/LifeLensCore/include/lifelens/Facility.h"
).read_text(encoding="utf-8")
facility_layer = (
    ROOT / "web/src/render/facility-layer.ts"
).read_text(encoding="utf-8")
trace_layer = (
    ROOT / "web/src/render/human-trace-layer.ts"
).read_text(encoding="utf-8")
world_scene = (
    ROOT / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")
trace_ui = (
    ROOT / "web/src/ui/human-traces.tsx"
).read_text(encoding="utf-8")
ground_detail = (
    ROOT / "web/src/render/ground-detail-layer.ts"
).read_text(encoding="utf-8")
vegetation = (
    ROOT / "web/src/render/vegetation-layer.ts"
).read_text(encoding="utf-8")

enum_match = re.search(
    r"enum\s+class\s+FacilityKind(?:\s*:\s*[^\{]+)?\s*\{(.*?)\}\s*;",
    facility_header,
    flags=re.S,
)
assert enum_match, "FacilityKind enum missing"
facility_kinds: set[str] = set()
for raw in enum_match.group(1).split(","):
    token = raw.strip().split("=", 1)[0].strip()
    name = re.match(r"([A-Za-z_]\w*)", token)
    if name:
        facility_kinds.add(name.group(1))

implemented = set(re.findall(r"case\s+'([A-Za-z_]\w*)'\s*:", facility_layer))
missing = sorted(facility_kinds - implemented)
assert not missing, (
    "Web 3D 시설 외형 구현 누락: " + ", ".join(missing)
)

for token in (
    "visibleHumanTraces(window)",
    "trace.kind === 'Facility'",
    "trace.facilityKind",
    "trace.state",
    "trace.progress01",
    "trace.deliveredMaterialUnits",
    "trace.requiredMaterialUnits",
    "trace.lit",
    "createTerrainElevationSampler(window)",
    "buildStorage(",
    "buildFirePit(",
    "buildWorkSurface(",
    "buildSleepingPlace(",
    "buildShelter(",
    "buildFurnace(",
    "pickTrace(raycaster",
):
    assert token in facility_layer, f"facility presentation contract missing: {token}"

# Visual variety must remain deterministic presentation, never invented durable state.
assert "Math.random(" not in facility_layer
assert "worldSeed" in facility_layer
assert "trace.id" in facility_layer

# Facility rings are only a selection halo now, never the structure itself.
assert "trace.kind !== 'Facility' || trace.id === selectedId" in trace_layer
assert "Facilities themselves are rendered" in trace_layer

for token in (
    "import { FacilityLayer } from './facility-layer';",
    "private readonly facilityLayer = new FacilityLayer();",
    "this.scene.add(this.facilityLayer.group);",
    "this.facilityLayer.setTerrain(window);",
    "this.facilityLayer.pickTrace(this.raycaster)",
    "this.facilityLayer.dispose();",
):
    assert token in world_scene, f"world scene missing facility layer wiring: {token}"

assert "시설은 실제 구조물로 표시" in trace_ui

for source_name, source in (
    ("ground detail", ground_detail),
    ("vegetation", vegetation),
):
    assert "facilityPresentationFootprints(window)" in source, (
        f"{source_name} must derive clearing only from authoritative facilities"
    )
    assert "outsideFacilityFootprints(" in source, (
        f"{source_name} must not clip through visible facilities"
    )
    assert "initialStartRegion" not in source, (
        f"{source_name} must not treat NEW GAME spawn as a living zone"
    )

print("LifeLens Web visible facility structures: PASS")
