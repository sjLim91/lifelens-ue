from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/Characters/LLResidentMotionComponent.h").read_text(encoding="utf-8")
appearance_header = (root / "Source/LifeLens/Characters/LLResidentAppearanceComponent.h").read_text(encoding="utf-8")
appearance_cpp = (root / "Source/LifeLens/Characters/LLResidentAppearanceComponent.cpp").read_text(encoding="utf-8")
presentation_cpp = (root / "Source/LifeLens/Characters/LLResidentPresentationComponent.cpp").read_text(encoding="utf-8")

world_presentation_cpp = (
    root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp"
).read_text(encoding="utf-8")

android_game = (root / "Config/Android/AndroidGame.ini").read_text(encoding="utf-8")

# Hard runtime character/motion references must be backed by real cooked assets.
# ConstructorHelpers makes these production dependencies, not optional editor art.
required_character_assets = (
    "Content/Characters/Quaternius/UBC/Male/Superhero_Male_FullBody/SkeletalMeshes/Superhero_Male_FullBody.uasset",
    "Content/Characters/Quaternius/UBC/Female/Superhero_Female_FullBody/SkeletalMeshes/Superhero_Female_FullBody.uasset",
    "Content/Characters/Quaternius/UBC/HeadOnly/Male/LL_Superhero_Male_HeadOnly/SkeletalMeshes/LL_Superhero_Male_HeadOnly.uasset",
    "Content/Characters/Quaternius/UBC/HeadOnly/Female/LL_Superhero_Female_HeadOnly/SkeletalMeshes/LL_Superhero_Female_HeadOnly.uasset",
    "Content/Characters/Quaternius/MCO/Peasant/Male/Male_Peasant/SkeletalMeshes/Male_Peasant.uasset",
    "Content/Characters/Quaternius/MCO/Peasant/Female/Female_Peasant/SkeletalMeshes/Female_Peasant.uasset",
    "Content/Characters/Quaternius/UAL/BS_ResidentLocomotion.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Talking_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Interact.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/PickUp_Table.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Fixing_Kneeling.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sword_Attack.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Push_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Idle_Torch_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Crouch_Idle_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Enter.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Idle_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Talking_Loop.uasset",
    "Content/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes/Sitting_Exit.uasset",
    "Content/Environment/Photoreal/PolyHaven/wicker_basket_01/SM_LL_wicker_basket_01.uasset",
    "Content/Environment/Photoreal/PolyHaven/wooden_bowl_01/SM_LL_wooden_bowl_01.uasset",
    "Content/Environment/Quaternius/StylizedNature/Pebble_Round_1/StaticMeshes/Pebble_Round_1.uasset",
    "Content/Environment/Photoreal/PolyHaven/dead_tree_trunk/SM_LL_dead_tree_trunk.uasset",
)
for rel in required_character_assets:
    asset = root / rel
    assert asset.is_file(), f"missing runtime character/motion asset: {rel}"
    assert asset.stat().st_size > 1024, f"runtime character/motion asset looks empty/pointer-only: {rel}"

for forbidden_cook_exclusion in (
    '+DirectoriesToNeverCook=(Path="/Game/Characters")',
    '+DirectoriesToNeverCook=(Path="/Game/Characters/Quaternius")',
    '+DirectoriesToNeverCook=(Path="/Game/Environment/Photoreal/PolyHaven")',
    '+DirectoriesToNeverCook=(Path="/Game/Environment/Quaternius")',
):
    assert forbidden_cook_exclusion not in android_game, (
        f"Android cook would strip required resident assets: {forbidden_cook_exclusion}"
    )


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

# Terrain relief is presentation-only on every platform. The resident capsule
# stays on the flat authoritative locomotion plane, while the rendered human
# body/ring follows the actual visible local surface. Android local HISM tiles
# therefore expose query-only WorldStatic geometry without blocking Pawn.
for token in (
    "UpdateVisualSurfaceGrounding",
    "LineTraceSingleByChannel",
    "ECC_WorldStatic",
    "Hit.ImpactPoint.Z - PhysicalGroundZ",
    "MaxVisualGroundLiftUU",
):
    assert token in cpp or token in header, f"missing visual terrain grounding token: {token}"

grounding_start = cpp.index("void ULLResidentMotionComponent::UpdateVisualSurfaceGrounding")
grounding_end = cpp.index("void ULLResidentMotionComponent::UpdateBodyOrientation", grounding_start)
grounding_block = cpp[grounding_start:grounding_end]
assert "#if PLATFORM_ANDROID" not in grounding_block
assert "SetPresentationGroundOffsetUU(0.0f)" not in grounding_block

for token in (
    "EnableVisualGroundQuery",
    "SetCollisionEnabled(ECollisionEnabled::QueryOnly)",
    "SetCollisionResponseToAllChannels(ECR_Ignore)",
    "SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block)",
    "EnableVisualGroundQuery(GroundGrassTileInstances)",
    "EnableVisualGroundQuery(GroundDryTileInstances)",
    "EnableVisualGroundQuery(GroundTransitionTileInstances)",
):
    assert token in world_presentation_cpp, (
        f"Android local terrain grounding query contract missing: {token}"
    )

# Regional/far terrain remains presentation-only and query-free; resident
# physical routing never leaves materialized local chunks.
query_start = world_presentation_cpp.index("auto EnableVisualGroundQuery")
query_end = world_presentation_cpp.index("#endif", query_start)
query_block = world_presentation_cpp[query_start:query_end]
assert "RegionalTerrainTileInstances" not in query_block

for token in (
    "GetPresentationGroundOffsetUU",
    "SetPresentationGroundOffsetUU",
    "PresentationGroundOffsetUU",
):
    assert token in appearance_header, f"missing appearance ground-offset contract: {token}"

assert "PresentationGroundOffsetUU + MeshHeight * BodyScaleZ" in appearance_cpp
assert "-FeetOffset + PresentationGroundOffsetUU" in appearance_header
assert "-FeetOffset + PresentationGroundOffsetUU" in appearance_cpp
assert "GroundOffset = Appearance" in presentation_cpp
assert "-Feet + GroundOffset + RingThickness" in presentation_cpp
assert "Appearance->GetVisualTopOffset() + LabelAboveHead" in presentation_cpp

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

resident_cpp = (
    root / "Source/LifeLens/Characters/LLResidentCharacter.cpp"
).read_text(encoding="utf-8")
resident_header = (
    root / "Source/LifeLens/Characters/LLResidentCharacter.h"
).read_text(encoding="utf-8")

# High observer speeds scale resident Tick delta. A route must consume the
# resulting distance budget across multiple Core-grid waypoints instead of
# discarding all remaining movement after one waypoint per render frame.
for token in (
    "MaxMovementSegmentsPerTick = 8",
    "RemainingMoveDistance",
    "SegmentBudget",
    "while (bHasMovementTarget",
    "SegmentsProcessed < SegmentBudget",
    "RequestedStepDistance",
    "RemainingMoveDistance - RequestedStepDistance",
    "if (bIntermediateWaypoint)",
    "++MovementWaypointIndex;",
    "continue;",
    "ForwardHit.bBlockingHit",
    "ActualSegmentMove <= 0.5f",
):
    assert token in resident_cpp or token in resident_header, (
        f"high-speed resident route budget missing: {token}"
    )

tick_start = resident_cpp.index("void ALLResidentCharacter::Tick(float DeltaSeconds)")
tick_end = resident_cpp.index("void ALLResidentCharacter::BindResident", tick_start)
tick_block = resident_cpp[tick_start:tick_end]
assert "FMath::VInterpConstantTo" not in tick_block, (
    "single-target interpolation would discard distance budget at route waypoints"
)
assert "MaxMovementSegmentsPerTick" in resident_header

print("LifeLens character animation truth/polish: PASS")
