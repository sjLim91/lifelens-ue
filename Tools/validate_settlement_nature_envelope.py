#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "CoreZoneCanopyKeep = 0.30f",
    "CoreZoneUndergrowthKeep = 0.48f",
    "bDynamicObserverCanopyVisibility = true",
    "bClearInitialSightlineCanopy = false",
    "DynamicCanopyHideRadiusUU = 220.0f",
    "DynamicCanopyRestoreRadiusUU = 300.0f",
):
    assert token in header, f"settlement nature/readability contract missing: {token}"

# NEW GAME's initial region is not a living-zone authority. There must be no
# permanent start-centred ambient clearing contract and the legacy initial
# sightline wedge must not be enabled by default.
for forbidden in (
    "CoreClearRadiusUU",
    "ActivityRadiusUU",
    "bClearInitialSightlineCanopy = true",
    "CoreZoneCanopyKeep = 0.18f",
    "CoreZoneUndergrowthKeep = 0.32f",
):
    assert forbidden not in header, f"over-cleared settlement baseline returned: {forbidden}"

for token in (
    "AmbientDressingKeepFactor",
    "FacilityDressingKeepFactor",
    "return FacilityDressingKeepFactor(LocationUU, Layer);",
    "spawnEnvelope=off",
    "UpdateDynamicObserverCanopyVisibility",
    "RegisterDynamicCanopyInstance",
    "Authoritative resource patches are never hidden",
):
    assert token in header or token in cpp, f"readability safety path missing: {token}"

# Resource patches remain independent of decorative thinning.
# Anchor after BuildChunkDressing so the similarly named signature/hash loop
# earlier in the translation unit cannot produce a false positive.
build_chunk_start = cpp.index(
    "void ALLWorldPresentationActor::BuildChunkDressing"
)
resource_start = cpp.index(
    "for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)",
    build_chunk_start,
)
resource_end = cpp.index(
    "// Decorative ecology is budgeted only after every authoritative obstacle",
    resource_start,
)
resource_block = cpp[resource_start:resource_end]
assert "AmbientDressingKeepFactor" not in resource_block
assert "CachedSettlementReferenceUU" not in resource_block
assert "Component->AddInstance" in resource_block

print("LifeLens settlement nature envelope: PASS")
