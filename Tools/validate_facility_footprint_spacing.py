#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


facility = read("Source/LifeLensCore/include/lifelens/Facility.h")

enum_match = re.search(
    r"enum\s+class\s+FacilityKind(?:\s*:\s*[^\{]+)?\s*\{(.*?)\}\s*;",
    facility,
    flags=re.S,
)
assert enum_match, "FacilityKind enum missing"
kinds: set[str] = set()
for raw in enum_match.group(1).split(","):
    item = raw.strip().split("=", 1)[0].strip()
    match = re.match(r"([A-Za-z_]\w*)", item)
    if match:
        kinds.add(match.group(1))

radius_match = re.search(
    r"inline\s+int\s+facilityFootprintRadiusGrid\([^)]*\)\s*\{(.*?)\n\}",
    facility,
    flags=re.S,
)
assert radius_match, "facilityFootprintRadiusGrid missing"
registered = set(
    re.findall(r"case\s+FacilityKind::([A-Za-z_]\w*)\s*:", radius_match.group(1))
)
missing = sorted(kinds - registered)
assert not missing, "시설 Footprint 반경 등록 누락: " + ", ".join(missing)

for token in (
    "facilityPairSafetyClearanceGrid",
    "facilityMinimumCenterDistanceGrid",
    "facilityFootprintsConflict",
    "dx*dx+dy*dy<minimum*minimum",
):
    assert token in facility, f"facility footprint authority missing: {token}"

# Heat/sleep separation is intentionally stricter than ordinary circulation.
assert "return 4;" in facility
assert "return 2;" in facility
assert "return 1;" in facility

placement_files = {
    "settlement": read("Source/LifeLensCore/include/lifelens/SettlementProgression.h"),
    "storage": read("Source/LifeLensCore/include/lifelens/PrimitiveStorageProgression.h"),
    "fire": read("Source/LifeLensCore/include/lifelens/PrimitiveFireProgression.h"),
    "furnace": read("Source/LifeLensCore/include/lifelens/PrimitiveSmeltingProgression.h"),
}

for name, source in placement_files.items():
    assert "facilityFootprintsConflict(" in source, (
        f"{name} placement must reject overlapping facility footprints"
    )
    assert "manhattan(facility.pos,pos)<=2" not in source, (
        f"{name} placement regressed to point-only two-cell spacing"
    )
    assert "footprintRadius" in source, (
        f"{name} placement must keep the whole footprint clear of resources/sanitation"
    )

settlement = placement_files["settlement"]
assert "facilityMinimumCenterDistanceGrid(planned,facility.kind)" in settlement
assert "distance-safeDistance" in settlement
assert "std::array<GridPos,32>" in settlement

for name in ("storage", "fire", "furnace"):
    assert "std::array<GridPos,24>" in placement_files[name], (
        f"{name} search radius must include farther non-overlapping candidates"
    )

tests = read("Source/LifeLensCore/tests/test_facility_authority.cpp")
for token in (
    "facilityMinimumCenterDistanceGrid",
    "facilityFootprintsConflict",
    "FacilityKind::Shelter,{0,0}",
    "FacilityKind::FirePit,{0,0}",
    "FacilityKind::WorkSurface,{0,0}",
):
    assert token in tests, f"facility spacing regression test missing: {token}"

settlement_test = read("Source/LifeLensCore/tests/test_settlement_autonomy.cpp")
assert "!facilityFootprintsConflict(" in settlement_test

furnace_test = read("Source/LifeLensCore/tests/test_primitive_furnace_smelting.cpp")
assert "!facilityFootprintsConflict(" in furnace_test

print("LifeLens facility footprint spacing contract: PASS")
