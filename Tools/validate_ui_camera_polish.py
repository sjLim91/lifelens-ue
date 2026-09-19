from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/UI/LLObserverPlayerController.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/UI/LLObserverPlayerController.h").read_text(encoding="utf-8")
polish = (root / "Source/LifeLens/UI/LLObserverMobilePolish.cpp").read_text(encoding="utf-8")

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

print("LifeLens UI/camera polish v1: PASS")
