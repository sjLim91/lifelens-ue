#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
bridge_cpp = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
bridge_runtime_cpp = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp').read_text(encoding='utf-8')
world_h = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h').read_text(encoding='utf-8')
world_cpp = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp').read_text(encoding='utf-8')
terrain_contract = (root / 'Source/LifeLens/WorldPresentation/LLTerrainPresentationContract.h').read_text(encoding='utf-8')

assert 'GetRegionalTerrainPreviewObservations' in bridge_h
assert 'GetRegionalTerrainPreviewObservations' in bridge_cpp
assert 'FMath::Clamp(RadiusChunks, 1, 16)' in bridge_cpp
assert 'deriveMacroRegionFacts(Identity, Center)' in bridge_cpp

preview_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetRegionalTerrainPreviewObservations')
preview_end = bridge_cpp.index('bool ULLCoreBridgeSubsystem::GetTerrainPreviewObservation', preview_start)
preview_body = bridge_cpp[preview_start:preview_end]
assert 'materializeNaturalChunk' not in preview_body, (
    'regional terrain preview must never materialize simulation chunks'
)
assert 'resourceNodes' not in preview_body
assert 'generatedNaturalChunks.push' not in preview_body

for token in [
    'CachedRegionalTerrainPreview',
    'CachedRegionalTerrainWorldSeed',
    'CachedRegionalTerrainGenerationVersion',
    'CachedRegionalTerrainStartChunkX',
    'CachedRegionalTerrainStartChunkY',
    'CachedRegionalTerrainRadiusChunks',
]:
    assert token in bridge_h, f'missing regional terrain cache key: {token}'

assert 'const bool bCacheHit' in preview_body
assert 'if (bCacheHit)' in preview_body
assert 'return CachedRegionalTerrainPreview;' in preview_body
assert 'CachedRegionalTerrainPreview = Result;' in preview_body
assert 'CachedRegionalTerrainPreview.Reset();' in bridge_runtime_cpp
assert 'CachedRegionalTerrainRadiusChunks = -1;' in bridge_runtime_cpp

single_preview_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPreviewObservation')
single_preview_end = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPresentationObservation', single_preview_start)
single_preview_body = bridge_cpp[single_preview_start:single_preview_end]
for token in (
    'FillTerrainPresentationObservation',
    'World.genesisIdentity()',
    '{ChunkX, ChunkY}',
):
    assert token in single_preview_body, f'missing single terrain preview token: {token}'
for forbidden in (
    'materializeNaturalChunk(',
    'generatedNaturalChunks.push',
    'resourceNodes',
):
    assert forbidden not in single_preview_body, (
        f'single-coordinate terrain preview must remain read-only: {forbidden}'
    )

for token in [
    'RegionalTerrainTileInstances',
    'BuildRegionalTerrainPreview',
    'RegionalTerrainSurfaceZUU',
    'RegionalTerrainTileRotation',
    'LLTerrainPresentationContract::RegionalSurfaceZUU',
    'GetRegionalTerrainPreviewObservations',
]:
    assert token in world_h or token in world_cpp, f'missing regional terrain presentation token: {token}'

for token in (
    'RegionalPreviewRadiusChunks = 8',
    'RegionalInnerFlatRingChunks = 0',
    'RegionalReliefAmplitudeUU = 8000.0f',
    'const float Delta =',
    'if (Ring <= InnerRing)',
    'FMath::Max(0.0f, Delta)',
    '* LocalReliefAmplitudeUU',
    'return Delta * Amplitude',
):
    assert token in terrain_contract, f'missing shared regional terrain contract token: {token}'
assert 'LLTerrainPresentationContract::RegionalSurfaceZUU' in world_cpp
assert 'MaterializedCoords.Contains(Coord)' in world_cpp, (
    'regional preview must not duplicate authoritative materialized surfaces'
)
assert 'if (Ring == 0)' in world_cpp, (
    'regional preview must reserve only the local origin ring itself'
)
assert 'if (Ring <= InnerFlatRing)' not in world_cpp, (
    'ring 1 must render seam-compatible relief instead of falling back to the flat underlay'
)
assert 'BuildFarEnvironment(World, ActiveSpanUU, FarSpanUU, RegionalTerrains)' in world_cpp
assert 'RegionalTerrainSurfaceZUU(' in world_cpp
assert 'EffectiveFarGroundDropUU' in world_cpp, (
    'flat fallback must remain beneath signed regional valleys'
)

print('Regional terrain preview structural validation: PASS')
