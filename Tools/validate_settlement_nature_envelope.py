#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "Initial spawn is only the world-entry coordinate",
    "FacilityDressingKeepFactor",
    "UpdateDynamicObserverCanopyVisibility",
    "RegisterDynamicCanopyInstance",
    "Authoritative resource patches are never hidden",
):
    assert token in header or token in cpp, f"authority-derived readability contract missing: {token}"

# The legacy serialized radius fields may remain for config compatibility, but
# runtime nature thinning must not read them as a spawn-centered living zone.
ambient_start = cpp.index("float ALLWorldPresentationActor::AmbientDressingKeepFactor")
ambient_end = cpp.index("bool ALLWorldPresentationActor::CaptureInitialViewOrigin", ambient_start)
ambient_block = cpp[ambient_start:ambient_end]
assert "return FacilityDressingKeepFactor(LocationUU, Layer);" in ambient_block
assert "CachedSettlementReferenceUU" not in ambient_block
assert "CoreClearRadiusUU" not in ambient_block
assert "ActivityRadiusUU" not in ambient_block

sight_start = cpp.index("float ALLWorldPresentationActor::InitialSightlineKeepFactor")
sight_end = cpp.index("float ALLWorldPresentationActor::ResourcePatchScaleFactor", sight_start)
sight_block = cpp[sight_start:sight_end]
assert "return 1.0f;" in sight_block
assert "CachedSettlementReferenceUU" not in sight_block
assert "InitialViewOriginUU" not in sight_block

resource_start = cpp.index("float ALLWorldPresentationActor::ResourcePatchScaleFactor")
resource_end = cpp.index("FVector ALLWorldPresentationActor::ChunkOriginUU", resource_start)
resource_scale_block = cpp[resource_start:resource_end]
assert "CachedSettlementReferenceUU" not in resource_scale_block
assert "CoreClearRadiusUU" not in resource_scale_block
assert "ActivityRadiusUU" not in resource_scale_block
assert "CachedFacilityReadabilityCentersUU" in resource_scale_block

# Authoritative resource patches remain independent of ambient decorative
# thinning. They are always instantiated and only their visual scale may change.
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
assert "Component->AddInstance" in resource_block

print("LifeLens authority-derived settlement nature envelope: PASS")
