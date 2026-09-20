from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CPP = (ROOT / "Source/LifeLens/UI/LLObserverPlayerController.cpp").read_text(encoding="utf-8")

def require(token: str) -> None:
    if token not in CPP:
        raise AssertionError(f"mobile detail tap/scroll contract missing: {token}")

for token in (
    "BeginDetailScrollDrag",
    "bHUDDetailScrollTouchActive = true;",
    "bCrossedScrollThreshold",
    "(Current1 - TouchStart1).Size() > TouchDragThresholdPixels()",
    "ObserverHUD->UpdateDetailScrollDrag",
    "&& !bTouchGesture",
    "&& !bTouchHadSecondFinger",
    "&& bReleaseStayedWithinTapThreshold",
    "ApplyTap(ReleasePosition, ExactHit);",
):
    require(token)

if "&& !bWasHUDDetailScrollTouchActive" in CPP:
    raise AssertionError(
        "LEVEL 2 content taps are still suppressed merely because the touch began in the scroll viewport"
    )

press_start = CPP.index("void ALLObserverPlayerController::HandleTouchPressed")
release_start = CPP.index("void ALLObserverPlayerController::HandleTouchReleased", press_start)
press_block = CPP[press_start:release_start]
candidate_start = press_block.index("BeginDetailScrollDrag")
candidate_block = press_block[candidate_start:candidate_start + 700]
if "bTouchGesture = true;" in candidate_block:
    raise AssertionError(
        "detail scroll candidate is still promoted to a gesture before crossing the drag threshold"
    )

print("LifeLens mobile detail tap-vs-scroll validation: PASS")
