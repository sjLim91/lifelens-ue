#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
read_types = (root / 'Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h').read_text(encoding='utf-8')
bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
bridge_cpp = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
runtime_cpp = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp').read_text(encoding='utf-8')

for token in (
    'FLLCoreWorldAddressObservation',
    'PlanetId',
    'SurfaceRegionX',
    'SurfaceRegionY',
    'SurfaceRegionId',
    'SurfaceRegionSeed',
    'ChunkX',
    'ChunkY',
):
    assert token in read_types, f'missing World v2 address token: {token}'

assert 'GetWorldAddressForChunk' in bridge_h
address_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetWorldAddressForChunk')
address_end = bridge_cpp.index(
    'FLLCoreWorldGenerationObservation ULLCoreBridgeSubsystem::GetWorldGenerationObservation',
    address_start)
address_body = bridge_cpp[address_start:address_end]
for token in (
    'derivePrimaryPlanetIdentity',
    'deriveSurfaceRegionIdentityForChunk',
    '{ChunkX, ChunkY}',
):
    assert token in address_body, f'world address must derive from authoritative hierarchy: {token}'
for forbidden in ('materializeNaturalChunk(', 'generatedNaturalChunks.push', 'resourceNodes'):
    assert forbidden not in address_body, f'world address lookup must stay read-only: {forbidden}'

assert 'GetTerrainPreviewObservationsAroundChunk' in bridge_h
preview_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPreviewObservationsAroundChunk')
preview_end = bridge_cpp.index('bool ULLCoreBridgeSubsystem::GetTerrainPreviewObservation', preview_start)
preview_body = bridge_cpp[preview_start:preview_end]
for token in (
    'CenterChunkX',
    'CenterChunkY',
    'FMath::Clamp(RadiusChunks, 1, 16)',
    'FillTerrainPresentationObservation',
    'CachedTerrainPreviewCenterChunkX',
    'CachedTerrainPreviewCenterChunkY',
):
    assert token in preview_body, f'missing centered terrain-preview contract: {token}'
for forbidden in ('initialStartRegionCoord', 'materializeNaturalChunk(', 'generatedNaturalChunks.push', 'resourceNodes'):
    assert forbidden not in preview_body, f'generic terrain preview must stay spawn-independent/read-only: {forbidden}'

legacy_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetRegionalTerrainPreviewObservations')
legacy_end = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPreviewObservationsAroundChunk', legacy_start)
legacy_body = bridge_cpp[legacy_start:legacy_end]
assert 'GetTerrainPreviewObservationsAroundChunk' in legacy_body
assert 'initialStartRegionCoord' in legacy_body

materialized_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetMaterializedNaturalChunkObservations')
materialized_end = bridge_cpp.index('bool ULLCoreBridgeSubsystem::GetNaturalChunkObservation', materialized_start)
materialized_body = bridge_cpp[materialized_start:materialized_end]
assert 'for (const lifelens::GeneratedNaturalChunk& Chunk : World.generatedNaturalChunks)' in materialized_body
assert 'MaterializedChunkCount' not in materialized_body

for token in (
    'CachedTerrainPreview',
    'CachedTerrainPreviewWorldSeed',
    'CachedTerrainPreviewGenerationVersion',
    'CachedTerrainPreviewCenterChunkX',
    'CachedTerrainPreviewCenterChunkY',
    'CachedTerrainPreviewRadiusChunks',
):
    assert token in bridge_h, f'missing generic terrain-preview cache token: {token}'
    assert token in runtime_cpp or token in bridge_cpp

for stale in ('CachedRegionalTerrainStartChunkX', 'CachedRegionalTerrainStartChunkY'):
    assert stale not in bridge_h
    assert stale not in bridge_cpp
    assert stale not in runtime_cpp

print('World v2 contract foundation structural validation: PASS')
