#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
bridge_cpp = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
bridge_runtime_cpp = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp').read_text(encoding='utf-8')
world_h = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h').read_text(encoding='utf-8')
world_cpp = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp').read_text(encoding='utf-8')

assert 'GetRegionalTerrainPreviewObservations' in bridge_h
assert 'GetRegionalTerrainPreviewObservations' in bridge_cpp
assert 'FMath::Clamp(RadiusChunks, 1, 16)' in bridge_cpp
assert 'deriveMacroRegionFacts(Identity, Center)' in bridge_cpp

preview_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetRegionalTerrainPreviewObservations')
preview_end = bridge_cpp.index('bool ULLCoreBridgeSubsystem::GetTerrainPresentationObservation', preview_start)
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

for token in [
    'RegionalTerrainTileInstances',
    'BuildRegionalTerrainPreview',
    'RegionalTerrainSurfaceZUU',
    'RegionalTerrainTileRotation',
    'RegionalTerrainReliefAmplitudeUU',
    'GetRegionalTerrainPreviewObservations',
]:
    assert token in world_h or token in world_cpp, f'missing regional terrain presentation token: {token}'

assert '(Elevation01 - World.InitialChunk.Elevation) * Amplitude' in world_cpp, (
    'regional relief must preserve signed valleys as well as mountains'
)
assert 'MaterializedCoords.Contains(Coord)' in world_cpp, (
    'regional preview must not duplicate authoritative materialized surfaces'
)
assert 'BuildFarEnvironment(World, ActiveSpanUU, FarSpanUU, RegionalTerrains)' in world_cpp
assert 'RegionalTerrainSurfaceZUU(' in world_cpp
assert 'EffectiveFarGroundDropUU' in world_cpp, (
    'flat fallback must remain beneath signed regional valleys'
)

print('Regional terrain preview structural validation: PASS')
