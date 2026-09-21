#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
stream_h = (root / 'Source/LifeLens/World/LLWorldStreamingSubsystem.h').read_text(encoding='utf-8')
stream_cpp = (root / 'Source/LifeLens/World/LLWorldStreamingSubsystem.cpp').read_text(encoding='utf-8')
spatial = (root / 'Source/LifeLens/World/LLWorldSpatialContract.h').read_text(encoding='utf-8')
camera_h = (root / 'Source/LifeLens/UI/LLObserverPlayerController.h').read_text(encoding='utf-8')
camera_cpp = (root / 'Source/LifeLens/UI/LLObserverPlayerController.cpp').read_text(encoding='utf-8')
world_h = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h').read_text(encoding='utf-8')
world_cpp = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp').read_text(encoding='utf-8')
bridge_cpp = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
default_game = (root / 'Config/DefaultGame.ini').read_text(encoding='utf-8')

for token in (
    'ULLWorldStreamingSubsystem',
    'SetObserverPresentationTarget',
    'ResolveObserverCenterChunk',
    'bHasObserverPresentationTarget',
):
    assert token in stream_h or token in stream_cpp, f'missing streaming interest token: {token}'

# ObserverInterest must remain presentation-only. It must not mutate Core world
# materialization or create resources/facilities.
for forbidden in (
    'materializeNaturalChunk(',
    'generatedNaturalChunks.push',
    'resourceNodes.push',
    'facilities.push',
    'storageSites.push',
):
    assert forbidden not in stream_h
    assert forbidden not in stream_cpp

for token in (
    'LogicalChunkForPresentationLocation',
    'PresentationChunkOffsetForAxis',
    'PresentationLocationForLogicalChunk',
):
    assert token in spatial, f'missing presentation/logical coordinate helper: {token}'

for token in (
    '#include "World/LLWorldStreamingSubsystem.h"',
    'SetObserverPresentationTarget(CurrentOrbitTarget)',
):
    assert token in camera_cpp, f'camera is not publishing ObserverInterest: {token}'

assert 'ManualPanMaxRadiusChunks' not in camera_h
assert 'ManualPanMaxRadiusChunks' not in default_game

pan_start = camera_cpp.index('void ALLObserverPlayerController::PanByScreenDelta')
pan_end = camera_cpp.index('void ALLObserverPlayerController::ZoomByScale', pan_start)
pan_body = camera_cpp[pan_start:pan_end]
assert 'DesiredOrbitTarget = ProposedTarget' in pan_body
assert 'GetClampedToMaxSize(MaxPanRadiusUU)' not in pan_body

for token in (
    'ResolveObserverCenterChunk',
    'BuiltObserverCenterChunkX',
    'BuiltObserverCenterChunkY',
    'GetTerrainPreviewObservationsAroundChunk(',
    'ObserverCenterChunk.X',
    'ObserverCenterChunk.Y',
):
    assert token in world_h or token in world_cpp, f'missing observer-centered presentation token: {token}'

refresh_start = world_cpp.index('void ALLWorldPresentationActor::RefreshFromCore')
refresh_body = world_cpp[refresh_start:]
assert 'GetTerrainPreviewObservationsAroundChunk(' in refresh_body
assert 'GetRegionalTerrainPreviewObservations(' not in refresh_body

# Simulation/local dressing authority still comes only from explicit materialized
# chunks; scrolling the camera may only move the read-only regional preview.
assert 'GetMaterializedNaturalChunkObservations()' in refresh_body
materialized_start = bridge_cpp.index('ULLCoreBridgeSubsystem::GetMaterializedNaturalChunkObservations')
materialized_end = bridge_cpp.index('bool ULLCoreBridgeSubsystem::GetNaturalChunkObservation', materialized_start)
materialized_body = bridge_cpp[materialized_start:materialized_end]
assert 'World.generatedNaturalChunks' in materialized_body
assert 'materializeNaturalChunk(' not in materialized_body

ground_start = world_cpp.index('void ALLWorldPresentationActor::BuildGround')
ground_end = world_cpp.index('void ALLWorldPresentationActor::BuildChunkGround', ground_start)
ground_body = world_cpp[ground_start:ground_end]
for token in (
    'ObserverCenterChunk',
    'ObserverCenterUU',
    'Ground->SetRelativeLocation',
    'FarGround->SetRelativeLocation',
):
    assert token in ground_body, f'world underlay must follow observer center: {token}'

far_start = world_cpp.index('void ALLWorldPresentationActor::BuildFarEnvironment')
far_end = world_cpp.index('void ALLWorldPresentationActor::BuildChunkDressing', far_start)
far_body = world_cpp[far_start:far_end]
for token in (
    'ObserverCenterChunk',
    'ObserverCenterUU',
    'ObserverCenterChunk.X + OffsetX',
    'ObserverCenterChunk.Y + OffsetY',
):
    assert token in far_body, f'far environment must follow observer center: {token}'

regional_start = world_cpp.index('void ALLWorldPresentationActor::BuildRegionalTerrainPreview')
regional_end = world_cpp.index('void ALLWorldPresentationActor::BuildFarEnvironment', regional_start)
regional_body = world_cpp[regional_start:regional_end]
assert 'MaterializedCoords.Contains(Coord)' in regional_body
assert 'if (Ring == 0)' not in regional_body

print('World v2 observer streaming-interest structural validation: PASS')
