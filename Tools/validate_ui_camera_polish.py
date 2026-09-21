from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/UI/LLObserverPlayerController.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/UI/LLObserverPlayerController.h").read_text(encoding="utf-8")
polish = (root / "Source/LifeLens/UI/LLObserverMobilePolish.cpp").read_text(encoding="utf-8")
game_mode_cpp = (root / "Source/LifeLens/Core/LLLifeLensGameMode.cpp").read_text(encoding="utf-8")
game_mode_header = (root / "Source/LifeLens/Core/LLLifeLensGameMode.h").read_text(encoding="utf-8")
bridge_header = (root / "Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp").read_text(encoding="utf-8")
lifecycle_header = (root / "Source/LifeLens/UI/LLLifecycleEventOverlay.h").read_text(encoding="utf-8")
lifecycle_cpp = (root / "Source/LifeLens/UI/LLLifecycleEventOverlay.cpp").read_text(encoding="utf-8")
default_game = (root / "Config/DefaultGame.ini").read_text(encoding="utf-8")
observation_header = (root / "Source/LifeLens/UI/LLObservationSubsystem.h").read_text(encoding="utf-8")
observation_cpp = (root / "Source/LifeLens/UI/LLObservationSubsystem.cpp").read_text(encoding="utf-8")

for token in (
    "ObservedResidentFollowSmoothingSpeed",
    "FocusTransitionSmoothingSpeed",
    "OverviewTransitionSmoothingSpeed",
    "bReturningToWorldOverview",
    "FMath::VInterpTo",
):
    assert token in cpp or token in header, f"missing camera polish token: {token}"

for token in (
    "ManualPanMaxRadiusChunks = 3.0f",
    "LLWorldSpatialContract::ChunkSpanUU",
    "ProposedTarget",
    "WorldOverviewTarget",
    "Offset.GetClampedToMaxSize(MaxPanRadiusUU)",
):
    assert token in cpp or token in header, f"manual pan world-boundary guard missing: {token}"
assert "ManualPanMaxRadiusChunks=3.000000" in default_game

pan_start = cpp.index("void ALLObserverPlayerController::PanByScreenDelta")
pan_end = cpp.index("void ALLObserverPlayerController::ZoomByScale", pan_start)
pan_block = cpp[pan_start:pan_end]
assert "DesiredOrbitTarget += " not in pan_block, (
    "manual pan must not remain unbounded after the regional presentation boundary guard"
)
assert "if (bWorldOverviewCaptured)" in pan_block

for token in (
    "LinearFade * LinearFade * (3.0f - 2.0f * LinearFade)",
    "const float EaseOut",
    "FMath::Square(1.0f - T)",
):
    assert token in polish, f"missing HUD easing token: {token}"

for token in (
    "GetHitResultUnderCursor",
    "GetHitResultUnderFinger",
    "HandleDetailScrollWheel",
    "ObserveResident",
):
    assert token in cpp, f"observer input contract regressed: {token}"

# The initial observer view must be recoverable. A transient bad view target
# must not strand the product on a sky-only frame.
for token in (
    "LifeLens.ObserverCamera",
    "GetObserverCamera()",
    "SetViewTarget(Camera)",
    "TActorIterator<ACameraActor>",
    "observer view recovered",
):
    assert token in cpp or token in game_mode_cpp or token in game_mode_header, (
        f"observer camera recovery contract regressed: {token}"
    )

assert "ObserverCamera = GetWorld()->SpawnActor<ACameraActor>" in game_mode_cpp
assert "TObjectPtr<ACameraActor> ObserverCamera;" in game_mode_header

for token in (
    "bFrameFreshSurfaceWaterInInitialView",
    "GetMaterializedSurfaceWaterPresentationObservations",
    "Water.bFreshSurfaceWater",
    "InitialFreshWaterFocusWeight",
    "InitialFreshWaterMaxFocusOffsetChunks",
    "InitialFreshWaterDistanceBoostPerChunk",
    "InitialFreshWaterMaxDistanceBoostChunks",
    "LifeLens initial observer framing",
):
    assert token in game_mode_cpp or token in game_mode_header, (
        f"initial freshwater framing contract regressed: {token}"
    )

# Lifecycle transition caches must reset when the authoritative Core runtime is
# replaced, even if the new world has the same seed or simulation minute.
for token in (
    "GetRuntimeGeneration() const",
    "RuntimeGeneration = 0",
):
    assert token in bridge_header, f"missing Core runtime generation contract: {token}"
assert "++RuntimeGeneration;" in bridge_cpp
assert "LastObservedRuntimeGeneration" in lifecycle_header
for token in (
    "Bridge->GetRuntimeGeneration()",
    "RuntimeGeneration != LastObservedRuntimeGeneration",
    "ResetObservationState();",
    "LastObservedRuntimeGeneration = RuntimeGeneration;",
):
    assert token in lifecycle_cpp, f"lifecycle runtime reset contract missing: {token}"

for token in (
    "const bool bSameWorldFocus",
    "Existing.bHasWorldFocus == bHasWorldFocus",
    "Existing.WorldFocus.Equals(WorldFocus, 1.0f)",
    "Existing.SubjectResidentId == SubjectResidentId",
    "Existing.RelatedResidentId == RelatedResidentId",
    "Existing.SimulationMinute == SimulationMinute",
    "const bool bSameEvent",
):
    assert token in lifecycle_cpp, f"lifecycle notice identity dedupe missing: {token}"

push_start = lifecycle_cpp.index("void ULLLifecycleEventOverlay::PushNotice")
push_end = lifecycle_cpp.index("void ULLLifecycleEventOverlay::RefreshNoticeWidgets", push_start)
push_block = lifecycle_cpp[push_start:push_end]
assert "if (Existing.Text == Text)" not in push_block, (
    "lifecycle notices must not dedupe distinct events by text alone"
)

# Observer selection may survive a save load only when the selected stable ID
# still exists in the replacement Core runtime. Unrelated-world IDs must close.
assert "LastObservedCoreRuntimeGeneration" in header
for token in (
    "Bridge->GetRuntimeGeneration()",
    "RuntimeGeneration != LastObservedCoreRuntimeGeneration",
    "Bridge->GetResidentObservation(",
    "Observation->ClearObservedResident();",
    "RestoreWorldOverview();",
):
    assert token in cpp, f"observer runtime selection validation missing: {token}"

# Deceased residents retain a stable history/genealogy identity without
# leaving the observer in physical Quick/Detail or following a destroyed actor.
for token in (
    "ObserveHistoricalResident",
    "SetLevel(ELLObservationLevel::World)",
):
    assert token in observation_header or token in observation_cpp, (
        f"historical observation contract missing: {token}"
    )

historical_start = observation_cpp.index(
    "void ULLObservationSubsystem::ObserveHistoricalResident"
)
historical_end = observation_cpp.index(
    "void ULLObservationSubsystem::ClearObservedResident",
    historical_start,
)
historical_block = observation_cpp[historical_start:historical_end]
assert "ObservedResidentId = ResidentId" in historical_block
assert "OnObservedResidentChanged.Broadcast" in historical_block
assert "SetLevel(ELLObservationLevel::World)" in historical_block

step_start = observation_cpp.index("void ULLObservationSubsystem::StepBack")
step_end = observation_cpp.index("void ULLObservationSubsystem::SetLevel", step_start)
step_block = observation_cpp[step_start:step_end]
assert "case ELLObservationLevel::World:" in step_block
assert "if (ObservedResidentId.IsValid())" in step_block
assert "ClearObservedResident();" in step_block

sync_start = cpp.index("void ALLObserverPlayerController::SyncObservedResidentSelection")
sync_end = cpp.index("void ALLObserverPlayerController::UpdateObservedResidentFocus", sync_start)
sync_block = cpp[sync_start:sync_end]
for token in (
    "Simulation->IsCoreAuthoritativeRuntime()",
    "Simulation->FindResidentById(",
    "Bridge->GetResidentObservation(",
    "!CoreResident.bAlive",
    "Observation->ObserveHistoricalResident(",
    "Observation->GetObservationLevel() == ELLObservationLevel::World",
    "RestoreWorldOverview();",
):
    assert token in sync_block, f"deceased observer cleanup missing: {token}"

# Do not erase the stable deceased identity merely because the physical actor
# left the living projection; lifecycle/history UI still consumes that ID.
death_branch = sync_block[
    sync_block.index("!CoreResident.bAlive"):
    sync_block.index("// Selection can originate outside this controller")
]
assert "Observation->ClearObservedResident();" not in death_branch

print("LifeLens UI/camera resilience polish: PASS")
