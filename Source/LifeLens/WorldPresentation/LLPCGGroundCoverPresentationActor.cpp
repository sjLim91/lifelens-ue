#include "WorldPresentation/LLPCGGroundCoverPresentationActor.h"

#include "Components/SceneComponent.h"
#include "Engine/GameInstance.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"

#if !PLATFORM_ANDROID
#include "PCGComponent.h"
#include "PCGGraph.h"
#endif

ALLPCGGroundCoverPresentationActor::ALLPCGGroundCoverPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

#if !PLATFORM_ANDROID
    UPCGComponent* PCG =
        CreateDefaultSubobject<UPCGComponent>(TEXT("GroundCoverPCG"));
    RuntimePCGComponent = PCG;

    static ConstructorHelpers::FObjectFinder<UPCGGraph> GraphFinder(
        TEXT("/Game/Environment/PCG/PCG_LL_GroundCover.PCG_LL_GroundCover"));
    if (GraphFinder.Succeeded())
    {
        GroundCoverGraphAsset = GraphFinder.Object;
        PCG->SetGraph(GraphFinder.Object);
    }
#endif
}

void ALLPCGGroundCoverPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    TryGenerateFromCore();
}

void ALLPCGGroundCoverPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bGenerated)
    {
        return;
    }

    RetryAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (RetryAccumulator < RetryIntervalSeconds)
    {
        return;
    }

    RetryAccumulator = 0.0f;
    TryGenerateFromCore();
}

void ALLPCGGroundCoverPresentationActor::TryGenerateFromCore()
{
#if PLATFORM_ANDROID
    SetActorTickEnabled(false);
    return;
#else
    UGameInstance* GameInstance = GetGameInstance();
    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    UPCGComponent* PCG = Cast<UPCGComponent>(RuntimePCGComponent);
    UPCGGraph* Graph = Cast<UPCGGraph>(GroundCoverGraphAsset);

    if (!Bridge || !Bridge->IsCoreRunning() || !PCG || !Graph)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation World =
        Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        return;
    }

    FLLCoreNaturalChunkObservation InitialChunk;
    if (!Bridge->GetNaturalChunkObservation(
            World.InitialChunkX,
            World.InitialChunkY,
            InitialChunk)
        || !InitialChunk.bMaterialized)
    {
        return;
    }

    const int64 VisualSeed = InitialChunk.VisualSeed != 0
        ? InitialChunk.VisualSeed
        : World.WorldSeed;
    if (bGenerated && LastGeneratedVisualSeed == VisualSeed)
    {
        return;
    }

    uint64 SeedWord = static_cast<uint64>(VisualSeed);
    uint32 Seed32 = static_cast<uint32>(SeedWord & 0xFFFFFFFFu)
        ^ static_cast<uint32>((SeedWord >> 32) & 0xFFFFFFFFu);
    Seed32 &= 0x7FFFFFFFu;
    if (Seed32 == 0)
    {
        Seed32 = 0x4C4C5043u & 0x7FFFFFFFu;
    }

    PCG->Seed = static_cast<int32>(Seed32);
    PCG->SetGraph(Graph);
    PCG->GenerateLocal(true);

    LastGeneratedVisualSeed = VisualSeed;
    bGenerated = true;
    SetActorTickEnabled(false);

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens desktop PCG ground cover generated: chunk=(%d,%d) visualSeed=%lld pcgSeed=%u"),
        World.InitialChunkX,
        World.InitialChunkY,
        static_cast<long long>(VisualSeed),
        Seed32);
#endif
}
