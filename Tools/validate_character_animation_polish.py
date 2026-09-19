from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.h").read_text(encoding="utf-8")

for token in (
    "ResolveResidentLoopVariation",
    "HashCombine",
    "SetPlayRate",
    "SetPosition",
    "Sitting_Idle_Loop",
    "SeatedIdleAnimation",
    "ELLResidentContextMotion::SeatedQuiet",
):
    assert token in cpp or token in header, f"missing character animation polish token: {token}"

# Movement authority and blend-space speed mapping must remain intact.
for token in (
    "SingleNode->SetBlendSpacePosition",
    "ResolveTravelMotion",
    "HasReachedMovementTarget",
    "IsResidentPerformingPhysicalAction",
):
    assert token in cpp, f"missing authoritative motion contract: {token}"

assert "OutPlayRate = FMath::Lerp(0.96f, 1.04f, RateUnit);" in cpp
assert "return ELLResidentContextMotion::SeatedQuiet;" in cpp
print("LifeLens character animation polish v1: PASS")
