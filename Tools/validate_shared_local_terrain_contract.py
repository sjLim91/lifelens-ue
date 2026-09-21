#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
contract = (root / 'Source/LifeLens/WorldPresentation/LLTerrainPresentationContract.h').read_text(encoding='utf-8')
world_cpp = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp').read_text(encoding='utf-8')
world_h = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h').read_text(encoding='utf-8')
desktop_cpp = (root / 'Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp').read_text(encoding='utf-8')
desktop_h = (root / 'Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.h').read_text(encoding='utf-8')
water_cpp = (root / 'Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp').read_text(encoding='utf-8')
water_h = (root / 'Source/LifeLens/WorldPresentation/LLWaterPresentationActor.h').read_text(encoding='utf-8')
bridge_cpp = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')

for token in (
    'LocalReliefAmplitudeUU = 220.0f',
    'SettlementFlattenRadiusUU = 650.0f',
    'SettlementBlendBandUU = 900.0f',
    'FacilityFlattenRadiusUU = 340.0f',
    'FacilityBlendEndRadiusUU = 900.0f',
    'DesktopSurfaceLiftUU = 1.0f',
    'FMath::Lerp(CenterSurface, CornerSurface, 0.72f)',
    'ReliefBlend(',
    'LocalSurfaceZUU(',
    'RegionalPreviewRadiusChunks = 8',
    'RegionalInnerFlatRingChunks = 0',
    'RegionalReliefAmplitudeUU = 8000.0f',
    'RegionalSurfaceZUU(',
):
    assert token in contract, f'missing shared local terrain contract token: {token}'

# Local materialized relief must stay within the current resident presentation
# grounding budget, while regional preview needs substantially larger macro relief.
motion_h = (root / 'Source/LifeLens/Characters/LLResidentMotionComponent.h').read_text(encoding='utf-8')
assert 'MaxVisualGroundLiftUU = 220.0f' in motion_h
assert 'LocalReliefAmplitudeUU = 220.0f' in contract
assert 'RegionalReliefAmplitudeUU = 8000.0f' in contract

# Local and regional visual surface consumers must call shared implementations.
assert 'LLTerrainPresentationContract::LocalSurfaceZUU' in world_cpp
assert 'LLTerrainPresentationContract::LocalSurfaceZUU' in desktop_cpp
assert 'LLTerrainPresentationContract::LocalSurfaceZUU' in water_cpp
assert 'LLTerrainPresentationContract::RegionalSurfaceZUU' in world_cpp
assert 'LLTerrainPresentationContract::RegionalSurfaceZUU' in water_cpp

# Old independently tunable height contracts are deliberately removed.
for obsolete in (
    'TerrainReliefFlattenRadiusUU',
    'TerrainReliefBlendBandUU',
):
    assert obsolete not in world_h, f'world terrain tuning drift reintroduced: {obsolete}'
for obsolete in (
    'TerrainReliefAmplitudeUU',
    'SettlementFlattenRadiusUU',
    'SettlementBlendBandUU',
    'FacilityFlattenRadiusUU',
    'FacilityBlendBandUU',
    'SurfaceLiftUU',
):
    assert obsolete not in desktop_h, f'desktop terrain tuning drift reintroduced: {obsolete}'
assert 'TerrainReliefAmplitudeUU' not in water_h
assert 'WaterSurfaceZForChunk' not in water_cpp
assert 'WaterSurfaceZForChunk' not in water_h

# Facility readability and surface flattening share one radius contract too.
assert 'FacilityClearRadiusUU = 340.0f' not in world_h
assert 'FacilityActivityRadiusUU = 900.0f' not in world_h
assert 'LLTerrainPresentationContract::FacilityFlattenRadiusUU' in world_cpp
assert 'LLTerrainPresentationContract::FacilityBlendEndRadiusUU' in world_cpp

# Water must evaluate the exact center/downstream grid position, not one chunk
# center height, and must be able to resolve an unmaterialized downstream point
# from the deterministic regional terrain preview.
for token in (
    'WaterSurfaceZForGrid',
    'GridX - World.InitialCenterGridX',
    'GridY - World.InitialCenterGridY',
    'GetTerrainPresentationObservation',
    'GetTerrainPreviewObservation',
    'bMaterializedTerrain',
    'LLTerrainPresentationContract::RegionalSurfaceZUU',
):
    assert token in water_cpp, f'missing terrain-attached water projection: {token}'

preview_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPreviewObservation')
preview_end = bridge_cpp.index('ULLCoreBridgeSubsystem::GetTerrainPresentationObservation', preview_start)
preview_block = bridge_cpp[preview_start:preview_end]
assert 'FillTerrainPresentationObservation' in preview_block
assert 'materializeNaturalChunk(' not in preview_block
assert 'generatedNaturalChunks.push' not in preview_block
assert 'GetTerrainPreviewObservation' in bridge_h

# Facility construction/migration changes the shared flattening surface. Even
# unchanged hydrology must therefore invalidate/rebuild the water projection.
signature_start = water_cpp.index('uint32 SurfaceWaterSignature(')
signature_end = water_cpp.index('void ConfigurePresentationOnlyWater', signature_start)
signature_block = water_cpp[signature_start:signature_end]
for token in (
    'Civilization.Facilities.Num()',
    'Facility.FacilityId',
    'Facility.GridX',
    'Facility.GridY',
):
    assert token in signature_block, f'water signature misses terrain-shaping input: {token}'

print('LifeLens shared local terrain/water contract: PASS')
