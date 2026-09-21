#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

# World v2 invariant: spawn/start region is a coordinate anchor, not a settlement.
# Natural terrain/dressing/resource presentation must not be cleared merely
# because it is close to the initial spawn.
for forbidden in (
    "CoreClearRadiusUU",
    "ActivityRadiusUU",
    "CachedSettlementReferenceUU",
    "SettlementReferenceUU(",
    "InitialSightlineKeepFactor",
    "CaptureInitialViewOrigin",
    "bClearInitialSightlineCanopy",
    "InitialSightlineHalfAngleDegrees",
    "InitialSightlineEdgeFalloffDegrees",
    "InitialSightlineCanopyKeep",
):
    assert forbidden not in header
    assert forbidden not in cpp, f"spawn-centered nature/readability legacy returned: {forbidden}"

# Readability is now facility-based or reversible observer-canopy suppression.
for token in (
    "bDynamicObserverCanopyVisibility = true",
    "DynamicCanopyHideRadiusUU = 220.0f",
    "DynamicCanopyRestoreRadiusUU = 300.0f",
    "FacilityDressingKeepFactor",
    "UpdateDynamicObserverCanopyVisibility",
    "RegisterDynamicCanopyInstance",
    "LLTerrainPresentationContract::FacilityFlattenRadiusUU",
    "LLTerrainPresentationContract::FacilityBlendEndRadiusUU",
):
    assert token in header or token in cpp, f"facility/dynamic readability contract missing: {token}"

ambient_start = cpp.index("float ALLWorldPresentationActor::AmbientDressingKeepFactor")
ambient_end = cpp.index("void ALLWorldPresentationActor::RegisterDynamicCanopyInstance", ambient_start)
ambient_block = cpp[ambient_start:ambient_end]
assert "return FacilityDressingKeepFactor(LocationUU, Layer);" in ambient_block
assert "Distance" not in ambient_block

resource_start = cpp.index("float ALLWorldPresentationActor::ResourcePatchScaleFactor")
resource_end = cpp.index("FVector ALLWorldPresentationActor::ChunkOriginUU", resource_start)
resource_scale_block = cpp[resource_start:resource_end]
assert "CachedFacilityReadabilityCentersUU" in resource_scale_block
assert "initial spawn" in resource_scale_block
assert "FacilityFlattenRadiusUU" in resource_scale_block

# Authoritative resource patches remain independent of decorative thinning.
build_chunk_start = cpp.index("void ALLWorldPresentationActor::BuildChunkDressing")
patch_start = cpp.index(
    "for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)",
    build_chunk_start,
)
patch_end = cpp.index(
    "// Decorative ecology is budgeted only after every authoritative obstacle",
    patch_start,
)
resource_block = cpp[patch_start:patch_end]
assert "AmbientDressingKeepFactor" not in resource_block
assert "Component->AddInstance" in resource_block

print("LifeLens spawn-independent nature envelope: PASS")
