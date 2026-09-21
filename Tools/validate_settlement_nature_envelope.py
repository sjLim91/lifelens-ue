#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "CoreClearRadiusUU = 360.0f",
    "ActivityRadiusUU = 1350.0f",
    "CoreZoneCanopyKeep = 0.30f",
    "CoreZoneUndergrowthKeep = 0.48f",
    "bDynamicObserverCanopyVisibility = true",
    "DynamicCanopyHideRadiusUU = 220.0f",
    "DynamicCanopyRestoreRadiusUU = 300.0f",
):
    assert token in header, f"settlement nature/readability contract missing: {token}"

# The opening landscape must not return to the old nearly-whole-chunk shaved
# envelope. The authoritative start chunk is 3200 UU wide in presentation.
for forbidden in (
    "CoreClearRadiusUU = 520.0f",
    "ActivityRadiusUU = 2200.0f",
    "CoreZoneCanopyKeep = 0.18f",
    "CoreZoneUndergrowthKeep = 0.32f",
):
    assert forbidden not in header, f"over-cleared settlement baseline returned: {forbidden}"

for token in (
    "AmbientDressingKeepFactor",
    "FacilityDressingKeepFactor",
    "UpdateDynamicObserverCanopyVisibility",
    "RegisterDynamicCanopyInstance",
    "Authoritative resource patches are never hidden",
):
    assert token in header or token in cpp, f"readability safety path missing: {token}"

# Resource patches remain independent of decorative thinning.
resource_start = cpp.index(
    "for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)"
)
resource_end = cpp.index(
    "// Decorative ecology is budgeted only after every authoritative obstacle",
    resource_start,
)
resource_block = cpp[resource_start:resource_end]
assert "AmbientDressingKeepFactor" not in resource_block
assert "Component->AddInstance" in resource_block

print("LifeLens settlement nature envelope: PASS")
