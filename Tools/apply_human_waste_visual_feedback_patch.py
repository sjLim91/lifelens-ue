from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def replace_once(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8")
    if old not in text:
        raise SystemExit(f"missing patch anchor in {path}: {old[:120]!r}")
    if text.count(old) != 1:
        raise SystemExit(f"patch anchor is not unique in {path}: {old[:120]!r}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


visualizer_h = r'''#pragma once

#include "CoreMinimal.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "LLEnvironmentalResidueVisualizerComponent.generated.h"

class ULLCoreBridgeSubsystem;
struct FLLCoreEnvironmentObservation;

/**
 * Read-only presentation of authoritative environmental residues.
 *
 * One HISM component represents many residue records.  It never creates,
 * mutates, cleans or acknowledges simulation state; it only rebuilds from the
 * Core Bridge read DTO.  Per-instance custom data reserves four floats for
 * presentation materials: intensity, normalized amount, normalized radius,
 * and normalized age.
 */
UCLASS(ClassGroup=(LifeLens), meta=(BlueprintSpawnableComponent))
class LIFELENS_API ULLEnvironmentalResidueVisualizerComponent
    : public UHierarchicalInstancedStaticMeshComponent
{
    GENERATED_BODY()

public:
    ULLEnvironmentalResidueVisualizerComponent();

    void RefreshFromCore(
        const ULLCoreBridgeSubsystem& CoreBridge,
        float CoreGridCellSizeUU,
        bool bForce = false);

    UFUNCTION(BlueprintPure, Category="LifeLens|World|Environment")
    int32 GetVisualInstanceCount() const { return GetInstanceCount(); }

private:
    uint32 BuildVisualSignature(const FLLCoreEnvironmentObservation& Environment) const;
    FVector ResolveSurfaceLocation(int32 GridX, int32 GridY, float CoreGridCellSizeUU) const;

    uint32 LastVisualSignature = 0;
    bool bHasVisualSignature = false;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="1", ClampMax="512"))
    int32 MaxResidueInstances = 128;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0.0"))
    float SurfaceOffsetUU = 1.5f;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0"))
    int32 StartCullDistanceUU = 2500;

    UPROPERTY(EditAnywhere, Category="LifeLens|Environment|Visual", meta=(ClampMin="0"))
    int32 EndCullDistanceUU = 12000;
};
'''

visualizer_cpp = r'''#include "World/LLEnvironmentalResidueVisualizerComponent.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLEnvironmentReadTypes.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float BasicCubeSizeUU = 100.0f;
constexpr int32 ResidueCustomDataFloats = 4;

uint32 MixVisualHash(uint32 Seed, uint32 Value)
{
    return HashCombine(Seed, Value);
}
}

ULLEnvironmentalResidueVisualizerComponent::ULLEnvironmentalResidueVisualizerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetCastShadow(false);
    SetCanEverAffectNavigation(false);
    SetMobility(EComponentMobility::Movable);
    NumCustomDataFloats = ResidueCustomDataFloats;

    // Cube is intentionally the dependency-free fallback already used by the
    // project smoke world.  It is flattened into a ground marker here.  A
    // presentation material/mesh can replace it later without changing Core.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        SetStaticMesh(CubeFinder.Object);
    }
}

uint32 ULLEnvironmentalResidueVisualizerComponent::BuildVisualSignature(
    const FLLCoreEnvironmentObservation& Environment) const
{
    uint32 Hash = GetTypeHash(Environment.TotalResidues);
    Hash = MixVisualHash(Hash, GetTypeHash(Environment.Residues.Num()));

    for (const FLLCoreEnvironmentalResidueObservation& Residue : Environment.Residues)
    {
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.ResidueId));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.GridX));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.GridY));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.RadiusTiles));
        Hash = MixVisualHash(Hash, GetTypeHash(Residue.AgeMinutes));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(Residue.Amount * 1000.0f)));
        Hash = MixVisualHash(Hash, GetTypeHash(FMath::RoundToInt(Residue.Intensity * 10000.0f)));
    }
    return Hash;
}

FVector ULLEnvironmentalResidueVisualizerComponent::ResolveSurfaceLocation(
    int32 GridX,
    int32 GridY,
    float CoreGridCellSizeUU) const
{
    const AActor* Owner = GetOwner();
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    FVector Location = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
    Location.X += static_cast<float>(GridX) * CellSize;
    Location.Y += static_cast<float>(GridY) * CellSize;

    UWorld* World = GetWorld();
    if (!World)
    {
        Location.Z += SurfaceOffsetUU;
        return Location;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(LLEnvironmentResidueSurface), false, Owner);
    FHitResult Hit;
    const FVector TraceStart(Location.X, Location.Y, Location.Z + 2500.0f);
    const FVector TraceEnd(Location.X, Location.Y, Location.Z - 2500.0f);
    if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
    {
        Location.Z = Hit.ImpactPoint.Z + SurfaceOffsetUU;
    }
    else
    {
        Location.Z += SurfaceOffsetUU;
    }
    return Location;
}

void ULLEnvironmentalResidueVisualizerComponent::RefreshFromCore(
    const ULLCoreBridgeSubsystem& CoreBridge,
    float CoreGridCellSizeUU,
    bool bForce)
{
    SetCullDistances(
        FMath::Max(0, StartCullDistanceUU),
        FMath::Max(StartCullDistanceUU, EndCullDistanceUU));

    const FLLCoreEnvironmentObservation Environment =
        CoreBridge.GetEnvironmentObservation(FMath::Max(1, MaxResidueInstances));
    const uint32 Signature = BuildVisualSignature(Environment);
    if (!bForce && bHasVisualSignature && Signature == LastVisualSignature)
    {
        return;
    }

    LastVisualSignature = Signature;
    bHasVisualSignature = true;
    ClearInstances();

    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    for (const FLLCoreEnvironmentalResidueObservation& Residue : Environment.Residues)
    {
        if (Residue.Kind != ELLCoreEnvironmentalResidueKind::HumanWaste)
        {
            continue;
        }

        const float Intensity = FMath::Clamp(Residue.Intensity, 0.0f, 1.0f);
        const float Amount = FMath::Max(0.0f, Residue.Amount);
        const float Radius = static_cast<float>(FMath::Clamp(Residue.RadiusTiles, 1, 16));
        const float AmountNorm = FMath::Clamp(
            FMath::Log2(1.0f + Amount) / FMath::Log2(101.0f), 0.0f, 1.0f);
        const float RadiusNorm = Radius / 16.0f;
        const float AgeNorm = FMath::Clamp(
            static_cast<float>(FMath::Max(0, Residue.AgeMinutes)) / 1440.0f,
            0.0f,
            1.0f);

        // Footprint is intentionally smaller than the exposure radius.  Radius,
        // intensity and accumulated amount still make open waste visually larger
        // than a contained DugPit record without pretending the waste disappeared.
        const float FootprintCells = FMath::Clamp(0.30f + Radius * 0.18f, 0.45f, 1.40f);
        const float AmountScale = 0.90f + AmountNorm * 0.35f;
        const float IntensityScale = 0.78f + Intensity * 0.22f;
        const float DiameterUU = CellSize * FootprintCells * AmountScale * IntensityScale;
        const float XYScale = FMath::Max(0.04f, DiameterUU / BasicCubeSizeUU);
        const float ZScale = FMath::Lerp(0.010f, 0.035f, Intensity);

        const FVector WorldLocation = ResolveSurfaceLocation(
            Residue.GridX, Residue.GridY, CellSize);
        const FTransform InstanceTransform(
            FRotator::ZeroRotator,
            WorldLocation,
            FVector(XYScale, XYScale, ZScale));

        const int32 InstanceIndex = AddInstance(InstanceTransform, true);
        if (InstanceIndex == INDEX_NONE)
        {
            continue;
        }

        SetCustomDataValue(InstanceIndex, 0, Intensity, false);
        SetCustomDataValue(InstanceIndex, 1, AmountNorm, false);
        SetCustomDataValue(InstanceIndex, 2, RadiusNorm, false);
        SetCustomDataValue(InstanceIndex, 3, AgeNorm, false);
    }

    MarkRenderStateDirty();
}
'''

validator = r'''from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def text(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="utf-8")


def require(blob: str, token: str, where: str) -> None:
    if token not in blob:
        raise SystemExit(f"missing {token!r} in {where}")


h = text("Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h")
cpp = text("Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp")
director_h = text("Source/LifeLens/World/LLWorldDirector.h")
director_cpp = text("Source/LifeLens/World/LLWorldDirector.cpp")
read_types = text("Source/LifeLens/Simulation/LLEnvironmentReadTypes.h")
doc = text("docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md")

for token in [
    "UHierarchicalInstancedStaticMeshComponent",
    "MaxResidueInstances = 128",
    "RefreshFromCore",
    "GetVisualInstanceCount",
]:
    require(h, token, "residue visualizer header")

for token in [
    "GetEnvironmentObservation",
    "ClearInstances()",
    "AddInstance(InstanceTransform, true)",
    "SetCustomDataValue",
    "LineTraceSingleByChannel",
    "ELLCoreEnvironmentalResidueKind::HumanWaste",
]:
    require(cpp, token, "residue visualizer implementation")

for forbidden in ["deposit(", "containHumanWasteAt(", "CompleteResidentPhysicalAction("]:
    if forbidden in cpp:
        raise SystemExit(f"presentation must not mutate simulation: found {forbidden!r}")

for token in ["ResidueId", "GridX", "GridY", "Amount", "Intensity", "RadiusTiles"]:
    require(read_types, token, "environment read DTO")

for token in [
    "EnvironmentalResidueVisualizer",
    "EnvironmentalVisualRefreshIntervalSeconds",
    "RefreshFromCore(*CoreBridge",
]:
    require(director_h + director_cpp, token, "WorldDirector integration")

for token in [
    "0.42",
    "0.16",
    "radius 3",
    "radius 1",
    "HISM",
]:
    require(doc, token, "environment visual feedback canonical doc")

print("Environmental visual feedback structural validation: PASS")
'''

(ROOT / "Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.h").write_text(
    visualizer_h, encoding="utf-8")
(ROOT / "Source/LifeLens/World/LLEnvironmentalResidueVisualizerComponent.cpp").write_text(
    visualizer_cpp, encoding="utf-8")
(ROOT / "Tools/validate_environment_visual_feedback.py").write_text(validator, encoding="utf-8")

header = ROOT / "Source/LifeLens/World/LLWorldDirector.h"
replace_once(
    header,
    "class ULLCoreBridgeSubsystem;\n",
    "class ULLCoreBridgeSubsystem;\nclass USceneComponent;\nclass ULLEnvironmentalResidueVisualizerComponent;\n",
)
replace_once(
    header,
    "    UFUNCTION(BlueprintPure, Category=\"LifeLens|World\")\n    bool IsResidentUsingEmergencyFallback(FGuid ResidentId) const;\n",
    "    UFUNCTION(BlueprintPure, Category=\"LifeLens|World\")\n    bool IsResidentUsingEmergencyFallback(FGuid ResidentId) const;\n\n"
    "    UFUNCTION(BlueprintPure, Category=\"LifeLens|World|Environment\")\n"
    "    int32 GetEnvironmentalResidueVisualCount() const;\n",
)
replace_once(
    header,
    "    UPROPERTY()\n    TObjectPtr<ULLSimulationSubsystem> Simulation;\n",
    "    UPROPERTY(VisibleAnywhere, Category=\"LifeLens|World\")\n"
    "    TObjectPtr<USceneComponent> SceneRoot;\n\n"
    "    UPROPERTY(VisibleAnywhere, Category=\"LifeLens|World|Environment\")\n"
    "    TObjectPtr<ULLEnvironmentalResidueVisualizerComponent> EnvironmentalResidueVisualizer;\n\n"
    "    UPROPERTY()\n    TObjectPtr<ULLSimulationSubsystem> Simulation;\n",
)
replace_once(
    header,
    "    float SimulationClockAccumulator = 0.0f;\n",
    "    float SimulationClockAccumulator = 0.0f;\n"
    "    float EnvironmentalVisualRefreshAccumulator = 0.0f;\n\n"
    "    UPROPERTY(EditAnywhere, Category=\"LifeLens|Environment|Visual\", meta=(ClampMin=\"0.05\"))\n"
    "    float EnvironmentalVisualRefreshIntervalSeconds = 0.25f;\n",
)

cpp = ROOT / "Source/LifeLens/World/LLWorldDirector.cpp"
replace_once(
    cpp,
    "#include \"World/LLActivityAnchor.h\"\n",
    "#include \"World/LLActivityAnchor.h\"\n"
    "#include \"World/LLEnvironmentalResidueVisualizerComponent.h\"\n",
)
replace_once(
    cpp,
    "#include \"EngineUtils.h\"\n",
    "#include \"EngineUtils.h\"\n#include \"Components/SceneComponent.h\"\n",
)
replace_once(
    cpp,
    "ALLWorldDirector::ALLWorldDirector()\n{\n    PrimaryActorTick.bCanEverTick = true;\n}\n",
    "ALLWorldDirector::ALLWorldDirector()\n{\n"
    "    PrimaryActorTick.bCanEverTick = true;\n\n"
    "    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT(\"SceneRoot\"));\n"
    "    SetRootComponent(SceneRoot);\n\n"
    "    EnvironmentalResidueVisualizer = CreateDefaultSubobject<ULLEnvironmentalResidueVisualizerComponent>(\n"
    "        TEXT(\"EnvironmentalResidueVisualizer\"));\n"
    "    EnvironmentalResidueVisualizer->SetupAttachment(SceneRoot);\n"
    "}\n",
)
replace_once(
    cpp,
    "    CollectActivityAnchors();\n    SpawnResidents();\n}\n\nvoid ALLWorldDirector::Tick",
    "    CollectActivityAnchors();\n    SpawnResidents();\n"
    "    if (EnvironmentalResidueVisualizer)\n"
    "    {\n"
    "        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, true);\n"
    "    }\n"
    "}\n\nvoid ALLWorldDirector::Tick",
)
replace_once(
    cpp,
    "    for (auto& Pair : RuntimeStates)\n    {\n        if (!FindResidentActor(Pair.Key))\n        {\n            ReleasePhysicalReservation(Pair.Key, Pair.Value);\n        }\n    }\n}\n\nALLResidentCharacter* ALLWorldDirector::FindResidentActor",
    "    for (auto& Pair : RuntimeStates)\n"
    "    {\n"
    "        if (!FindResidentActor(Pair.Key))\n"
    "        {\n"
    "            ReleasePhysicalReservation(Pair.Key, Pair.Value);\n"
    "        }\n"
    "    }\n\n"
    "    EnvironmentalVisualRefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);\n"
    "    const float VisualRefreshInterval = FMath::Max(0.05f, EnvironmentalVisualRefreshIntervalSeconds);\n"
    "    if (EnvironmentalResidueVisualizer\n"
    "        && EnvironmentalVisualRefreshAccumulator >= VisualRefreshInterval)\n"
    "    {\n"
    "        EnvironmentalVisualRefreshAccumulator = FMath::Fmod(\n"
    "            EnvironmentalVisualRefreshAccumulator, VisualRefreshInterval);\n"
    "        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, false);\n"
    "    }\n"
    "}\n\nALLResidentCharacter* ALLWorldDirector::FindResidentActor",
)
replace_once(
    cpp,
    "bool ALLWorldDirector::IsResidentUsingEmergencyFallback(FGuid ResidentId) const\n{\n    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);\n    return Runtime && Runtime->bUsingEmergencyFallback;\n}\n",
    "bool ALLWorldDirector::IsResidentUsingEmergencyFallback(FGuid ResidentId) const\n"
    "{\n"
    "    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);\n"
    "    return Runtime && Runtime->bUsingEmergencyFallback;\n"
    "}\n\n"
    "int32 ALLWorldDirector::GetEnvironmentalResidueVisualCount() const\n"
    "{\n"
    "    return EnvironmentalResidueVisualizer\n"
    "        ? EnvironmentalResidueVisualizer->GetVisualInstanceCount()\n"
    "        : 0;\n"
    "}\n",
)

preflight = ROOT / ".github/workflows/preflight.yml"
replace_once(
    preflight,
    "      - name: Validate primitive latrine progression\n        run: python Tools/validate_primitive_latrine.py\n\n",
    "      - name: Validate primitive latrine progression\n"
    "        run: python Tools/validate_primitive_latrine.py\n\n"
    "      - name: Validate environmental visual feedback\n"
    "        run: python Tools/validate_environment_visual_feedback.py\n\n",
)

doc_path = ROOT / "docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md"
doc = doc_path.read_text(encoding="utf-8")
marker = "## HumanWaste runtime presentation baseline — 2026-09-15"
if marker not in doc:
    doc += r'''

---

## HumanWaste runtime presentation baseline — 2026-09-15

PR #80 이후 HumanWaste 표현은 **site kind를 별도 시각 권위로 복제하지 않고 residue read DTO 자체**를 따른다.

현재 authoritative deposit profile:
- open `DesignatedArea`: intensity `0.42`, radius 3 tiles.
- `DugPit`: intensity `0.16`, radius 1 tile.
- 기존 open waste가 DugPit 완성으로 containment될 때도 amount를 삭제하지 않고 intensity/radius만 줄인다.

따라서 Presentation은 `DesignatedArea`/`DugPit` 이름을 보고 임의 효과를 만들지 않는다.
같은 `FLLCoreEnvironmentalResidueObservation`의 `GridPos / amount / intensity / radius / age`를 읽고 결과를 표현한다.
이렇게 하면 DugPit의 작은 footprint가 실제 Core containment 결과에서 자연스럽게 나온다.

v1 런타임 baseline:
- `ALLWorldDirector`에 단일 `ULLEnvironmentalResidueVisualizerComponent`를 둔다.
- visualizer는 HISM(Hierarchical Instanced Static Mesh) 하나로 최대 128개 residue marker를 표현한다.
- residue마다 Actor/Niagara를 생성하지 않는다.
- Core Grid XY를 World 좌표로 변환하고 WorldStatic surface trace로 실제 지면 Z를 찾는다.
- marker footprint는 observed radius/intensity/amount를 반영하며, 노출 반경 전체를 오염 mesh로 덮는 방식은 사용하지 않는다.
- per-instance custom data 4개를 예약한다: `intensity`, normalized `amount`, normalized `radius`, normalized `age`.
- 현재 dependency-free fallback은 engine Cube를 매우 얇게 flatten한 ground marker다. 이후 다겸 Presentation lane에서 material/mesh를 교체해도 Core/read contract는 그대로 유지한다.
- visualizer는 signature가 바뀐 경우에만 instance set을 rebuild하고, 기본 0.25초 주기로 read-only refresh한다.
- HISM cull distance를 사용하고 최대 visual residue 수를 제한하여 Android baseline 비용을 통제한다.
- Save/Load 후에는 저장된 authoritative residue state를 `BeginPlay`에서 강제 rebuild하므로 visual-only save state를 추가하지 않는다.

금지 사항은 그대로 유지한다:
- visualizer가 `deposit`, containment, hygiene burden, facility state를 직접 수정하지 않는다.
- visual marker 수/크기를 simulation truth로 다시 읽지 않는다.
- 128개 cap을 넘는 경우에도 Core의 `TotalResidues`는 그대로이며, presentation cap은 simulation 삭제를 의미하지 않는다.
'''
    doc_path.write_text(doc, encoding="utf-8")

print("HumanWaste visual feedback patch applied")
