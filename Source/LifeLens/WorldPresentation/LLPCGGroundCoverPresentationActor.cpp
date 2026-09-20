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

    // Keep a low-frequency desktop watch alive after the first generation.
    // A loaded/new world can change while this actor survives; bGenerated must
    // not permanently freeze presentation on the first seed it observed.
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
    UPCGComponent* PCG = Cast<UPCGComponent>(RuntimePCGComponent);

    auto ClearStaleGroundCover = [this, PCG]()
    {
        if (PCG && bGenerated)
        {
            // Generated PCG components are presentation-only. When Core
            // authority disappears or the feature is disabled, remove them
            // instead of leaving vegetation from the previous runtime visible.
            PCG->CleanupLocal(true, false);
        }

        LastGeneratedVisualSeed = 0;
        LastGeneratedWorldSeed = 0;
        LastGeneratedGenerationVersion = -1;
        LastGeneratedChunkX = 0;
        LastGeneratedChunkY = 0;
        bGenerated = false;
    };

    if (!bEnableGroundCoverPCG)
    {
        ClearStaleGroundCover();
        SetActorTickEnabled(false);
        return;
    }

    UGameInstance* GameInstance = GetGameInstance();
    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    UPCGGraph* Graph = Cast<UPCGGraph>(GroundCoverGraphAsset);

    if (!Bridge || !Bridge->IsCoreRunning() || !PCG || !Graph)
    {
        ClearStaleGroundCover();
        return;
    }

    const FLLCoreWorldGenerationObservation World =
        Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        ClearStaleGroundCover();
        return;
    }

    FLLCoreNaturalChunkObservation InitialChunk;
    if (!Bridge->GetNaturalChunkObservation(
            World.InitialChunkX,
            World.InitialChunkY,
            InitialChunk)
        || !InitialChunk.bMaterialized)
    {
        ClearStaleGroundCover();
        return;
    }

    const int64 VisualSeed = InitialChunk.VisualSeed != 0
        ? InitialChunk.VisualSeed
        : World.WorldSeed;
    const bool bSameGeneration =
        bGenerated
        && LastGeneratedVisualSeed == VisualSeed
        && LastGeneratedWorldSeed == World.WorldSeed
        && LastGeneratedGenerationVersion == World.GenerationVersion
        && LastGeneratedChunkX == World.InitialChunkX
        && LastGeneratedChunkY == World.InitialChunkY;
    if (bSameGeneration)
    {
        return;
    }

    // Force generation replaces the previous PCG projection for a changed
    // authoritative world/seed; explicit stale cleanup above covers the
    // no-authority path where generation never runs again.
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
    LastGeneratedWorldSeed = World.WorldSeed;
    LastGeneratedGenerationVersion = World.GenerationVersion;
    LastGeneratedChunkX = World.InitialChunkX;
    LastGeneratedChunkY = World.InitialChunkY;
    bGenerated = true;

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens desktop PCG ground cover generated: chunk=(%d,%d) visualSeed=%lld pcgSeed=%u"),
        World.InitialChunkX,
        World.InitialChunkY,
        static_cast<long long>(VisualSeed),
        Seed32);
#endif
}
