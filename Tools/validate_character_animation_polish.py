from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.h").read_text(encoding="utf-8")

for token in (
    "Pebble_Round_1.Pebble_Round_1",
    "SM_LL_dead_tree_trunk.SM_LL_dead_tree_trunk",
    "DiggingStickMesh",
    "PrimitiveStoneFinder",
    "PrimitiveStickFinder",
):
    assert token in cpp or token in header, f"missing visible primitive held-tool token: {token}"

assert "SharpFlakeMesh = nullptr;" not in cpp
assert "StoneCuttingToolMesh = nullptr;" not in cpp
assert "DesiredMesh = DiggingStickMesh.Get();" in cpp

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

# A seated clip is valid only when presentation has a real seat-support
# affordance. Current parenting/social directives do not carry that contract,
# so they must not synthesize sitting in empty space.
parenting_start = cpp.index("case ELLCoreContextActionKind::Parenting:")
social_start = cpp.index("case ELLCoreContextActionKind::Social:", parenting_start)
parenting_block = cpp[parenting_start:social_start]
assert "ELLResidentContextMotion::SeatedCare" not in parenting_block
assert "ELLResidentContextMotion::SeatedQuiet" not in parenting_block
assert "No seat-supporting affordance" in parenting_block
assert parenting_block.count("return ELLResidentContextMotion::Learn;") >= 3

social_end = cpp.index("default:", social_start)
social_block = cpp[social_start:social_end]
assert "ELLResidentContextMotion::SeatedCare" not in social_block
assert "return ELLResidentContextMotion::Talk;" in social_block

world_director_context = (
    root / "Source/LifeLens/World/LLWorldDirectorContextActions.cpp"
).read_text(encoding="utf-8")
parenting_gate_start = world_director_context.index("case ELLCoreContextActionKind::Parenting:")
parenting_gate_end = world_director_context.index("case ELLCoreContextActionKind::Civilization:", parenting_gate_start)
parenting_gate_block = world_director_context[parenting_gate_start:parenting_gate_end]
assert "WorkAtTarget = ELLResidentWorkPresentationMode::Interact;" in parenting_gate_block, (
    "parenting must open the at-target legacy presentation gate so the authoritative "
    "ParentingAction can be refined by ResolveContextMotion"
)

print("LifeLens character animation truth/polish: PASS")
