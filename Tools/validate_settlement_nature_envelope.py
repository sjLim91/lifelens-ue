#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "NEW GAME spawn is only a world-entry coordinate",
    "CoreClearRadiusUU = 360.0f",
    "ActivityRadiusUU = 1350.0f",
    "CoreZoneCanopyKeep = 0.30f",
    "CoreZoneUndergrowthKeep = 0.48f",
    "bDynamicObserverCanopyVisibility = true",
    "bClearInitialSightlineCanopy = false",
    "DynamicCanopyHideRadiusUU = 220.0f",
    "DynamicCanopyRestoreRadiusUU = 300.0f",
):
    assert token in header, f"authority-derived readability contract missing: {token}"

# Legacy radius fields remain serialized for old configs, but must not regain
# runtime authority over ambient nature or resource presentation.
ambient_start = cpp.index("float ALLWorldPresentationActor::AmbientDressingKeepFactor")
ambient_end = cpp.index("bool ALLWorldPresentationActor::CaptureInitialViewOrigin", ambient_start)
ambient_block = cpp[ambient_start:ambient_end]
assert "return FacilityDressingKeepFactor(LocationUU, Layer);" in ambient_block
for forbidden in ("CachedSettlementReferenceUU", "CoreClearRadiusUU", "ActivityRadiusUU"):
    assert forbidden not in ambient_block, f"spawn-centred ambient authority returned: {forbidden}"

sight_start = cpp.index("float ALLWorldPresentationActor::InitialSightlineKeepFactor")
sight_end = cpp.index("float ALLWorldPresentationActor::ResourcePatchScaleFactor", sight_start)
sight_block = cpp[sight_start:sight_end]
assert "return 1.0f;" in sight_block
assert "CachedSettlementReferenceUU" not in sight_block
assert "InitialViewOriginUU" not in sight_block

resource_start = cpp.index("float ALLWorldPresentationActor::ResourcePatchScaleFactor")
resource_end = cpp.index("FVector ALLWorldPresentationActor::ChunkOriginUU", resource_start)
resource_scale_block = cpp[resource_start:resource_end]
for forbidden in ("CachedSettlementReferenceUU", "CoreClearRadiusUU", "ActivityRadiusUU"):
    assert forbidden not in resource_scale_block, f"spawn-centred resource authority returned: {forbidden}"
assert "CachedFacilityReadabilityCentersUU" in resource_scale_block
assert "LLTerrainPresentationContract::FacilityFlattenRadiusUU" in resource_scale_block
assert "LLTerrainPresentationContract::FacilityBlendEndRadiusUU" in resource_scale_block

# Resource patches remain independent of ambient decorative thinning.
build_chunk_start = cpp.index("void ALLWorldPresentationActor::BuildChunkDressing")
resource_patch_start = cpp.index(
    "for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)",
    build_chunk_start,
)
resource_patch_end = cpp.index(
    "// Decorative ecology is budgeted only after every authoritative obstacle",
    resource_patch_start,
)
resource_block = cpp[resource_patch_start:resource_patch_end]
assert "AmbientDressingKeepFactor" not in resource_block
assert "Component->AddInstance" in resource_block

assert "spawnEnvelope=off" in cpp

print("LifeLens authority-derived settlement nature envelope: PASS")
