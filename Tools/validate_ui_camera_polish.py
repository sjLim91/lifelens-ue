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

for token in (
    "ObservedResidentFollowSmoothingSpeed",
    "FocusTransitionSmoothingSpeed",
    "OverviewTransitionSmoothingSpeed",
    "bReturningToWorldOverview",
    "FMath::VInterpTo",
):
    assert token in cpp or token in header, f"missing camera polish token: {token}"

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

print("LifeLens UI/camera resilience polish: PASS")
