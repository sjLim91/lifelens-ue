#include "WorldPresentation/LLDesktopTerrainPresentationActor.h"

#if !PLATFORM_ANDROID

#include "Engine/GameInstance.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "World/LLWorldSpatialContract.h"

namespace
{
uint32 MixTerrainHash(uint32 Seed, uint32 Value)
{
    Seed ^= Value + 0x9E3779B9u + (Seed << 6) + (Seed >> 2);
    return Seed;
}
}

ALLDesktopTerrainPresentationActor::ALLDesktopTerrainPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    TerrainMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SmoothTerrain"));
    SetRootComponent(TerrainMesh);
    TerrainMesh->SetMobility(EComponentMobility::Movable);
    TerrainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TerrainMesh->SetCanEverAffectNavigation(false);
    TerrainMesh->SetCastShadow(true);
    TerrainMesh->bUseAsyncCooking = true;

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Grass(
        TEXT("/Game/Environment/Materials/MI_Ground_Grass.MI_Ground_Grass"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Dry(
        TEXT("/Game/Environment/Materials/MI_Ground_DryEarth.MI_Ground_DryEarth"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Transition(
        TEXT("/Game/Environment/Materials/MI_Ground_Transition.MI_Ground_Transition"));

    GroundGrass = Grass.Succeeded() ? Grass.Object : nullptr;
    GroundDry = Dry.Succeeded() ? Dry.Object : nullptr;
    GroundTransition = Transition.Succeeded() ? Transition.Object : nullptr;
}

void ALLDesktopTerrainPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshFromCore(true);
}

void ALLDesktopTerrainPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    RefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);
    if (RefreshAccumulator < RefreshIntervalSeconds)
    {
        return;
    }
    RefreshAccumulator = 0.0f;
    RefreshFromCore(false);
}

uint32 ALLDesktopTerrainPresentationActor::BuildSignature(
    const TArray<FLLCoreTerrainPresentationObservation>& Terrains) const
{
    uint32 Hash = 0x54455252u; // TERR
    for (const FLLCoreTerrainPresentationObservation& Terrain : Terrains)
    {
        Hash = MixTerrainHash(Hash, static_cast<uint32>(Terrain.ChunkX));
        Hash = MixTerrainHash(Hash, static_cast<uint32>(Terrain.ChunkY));
        Hash = MixTerrainHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(Terrain.CenterElevation01 * 100000.0f)));
        Hash = MixTerrainHash(
            Hash,
            static_cast<uint32>(FMath::RoundToInt(Terrain.Relief01 * 100000.0f)));
    }
    return Hash;
}

float ALLDesktopTerrainPresentationActor::ReliefBlend(
    const FVector2D& LocationUU,
    const TArray<FVector2D>& FacilityCentersUU) const
{
    const float SettlementStart = FMath::Max(0.0f, SettlementFlattenRadiusUU);
    const float SettlementEnd =
        SettlementStart + FMath::Max(100.0f, SettlementBlendBandUU);
    const float SettlementAlpha = FMath::Clamp(
        static_cast<float>(
            (LocationUU.Size() - static_cast<double>(SettlementStart))
            / static_cast<double>(FMath::Max(SettlementEnd - SettlementStart, 1.0f))),
        0.0f,
        1.0f);
    float Blend = SettlementAlpha * SettlementAlpha * (3.0f - 2.0f * SettlementAlpha);

    for (const FVector2D& FacilityCenter : FacilityCentersUU)
    {
        const double Distance = FVector2D::Distance(LocationUU, FacilityCenter);
        const float Start = FMath::Max(0.0f, FacilityFlattenRadiusUU);
        const float End = Start + FMath::Max(50.0f, FacilityBlendBandUU);
        const float FacilityAlpha = FMath::Clamp(
            static_cast<float>(
                (Distance - static_cast<double>(Start))
                / static_cast<double>(FMath::Max(End - Start, 1.0f))),
            0.0f,
            1.0f);
        const float FacilityBlend =
            FacilityAlpha * FacilityAlpha * (3.0f - 2.0f * FacilityAlpha);
        Blend = FMath::Min(Blend, FacilityBlend);
    }

    return FMath::Clamp(Blend, 0.0f, 1.0f);
}

float ALLDesktopTerrainPresentationActor::SurfaceZUU(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreTerrainPresentationObservation& Terrain,
    const FVector2D& LocationUU,
    const TArray<FVector2D>& FacilityCentersUU) const
{
    if (!Terrain.bAvailable || TerrainReliefAmplitudeUU <= KINDA_SMALL_NUMBER)
    {
        return SurfaceLiftUU;
    }

    const FVector2D ChunkCenter(
        static_cast<float>(Terrain.ChunkX - World.InitialChunkX)
            * LLWorldSpatialContract::ChunkSpanUU,
        static_cast<float>(Terrain.ChunkY - World.InitialChunkY)
            * LLWorldSpatialContract::ChunkSpanUU);
    const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
    const float U = FMath::Clamp(
        (LocationUU.X - (ChunkCenter.X - Half))
            / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
        0.0f, 1.0f);
    const float V = FMath::Clamp(
        (LocationUU.Y - (ChunkCenter.Y - Half))
            / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f),
        0.0f, 1.0f);

    auto RelativeHeight = [&](float Elevation01)
    {
        return FMath::Max(0.0f, Elevation01 - World.InitialChunk.Elevation)
            * FMath::Max(0.0f, TerrainReliefAmplitudeUU);
    };

    const float South = FMath::Lerp(
        RelativeHeight(Terrain.SouthWestElevation01),
        RelativeHeight(Terrain.SouthEastElevation01),
        U);
    const float North = FMath::Lerp(
        RelativeHeight(Terrain.NorthWestElevation01),
        RelativeHeight(Terrain.NorthEastElevation01),
        U);
    const float CornerSurface = FMath::Lerp(South, North, V);
    const float CenterSurface = RelativeHeight(Terrain.CenterElevation01);

    // Keep the desktop procedural surface on the same height contract used by
    // WorldPresentation when it places trees, rocks and resources. The old
    // center-weighted surface could diverge from dressing Z and leave objects
    // visibly floating or buried.
    const float SharedSurface =
        FMath::Lerp(CenterSurface, CornerSurface, 0.72f)
        * ReliefBlend(LocationUU, FacilityCentersUU);

    return SurfaceLiftUU + SharedSurface;
}

UMaterialInterface* ALLDesktopTerrainPresentationActor::MaterialForChunk(
    const FLLCoreNaturalChunkObservation& /*Chunk*/) const
{
    // Recovery baseline: avoid hard per-chunk material boundaries on the
    // procedural overlay. One failing dry/transition material section otherwise
    // appears as a giant engine-grey polygon. Biome variation will return via a
    // verified blended material rather than section-wide swaps.
    if (GroundGrass) { return GroundGrass; }
    if (GroundTransition) { return GroundTransition; }
    return GroundDry.Get();
}

void ALLDesktopTerrainPresentationActor::RefreshFromCore(bool bForce)
{
    UGameInstance* GameInstance = GetGameInstance();
    ULLCoreBridgeSubsystem* Bridge =
        GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Bridge || !Bridge->IsCoreRunning() || !TerrainMesh)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation World =
        Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion)
    {
        return;
    }

    const TArray<FLLCoreTerrainPresentationObservation> Terrains =
        Bridge->GetMaterializedTerrainPresentationObservations();
    const TArray<FLLCoreNaturalChunkObservation> Chunks =
        Bridge->GetMaterializedNaturalChunkObservations();
    if (Terrains.Num() == 0 || Chunks.Num() == 0)
    {
        return;
    }

    uint32 Signature = BuildSignature(Terrains);
    const FLLCoreCivilizationWorldObservation Civilization =
        Bridge->GetCivilizationWorldObservation(0);
    Signature = MixTerrainHash(Signature, static_cast<uint32>(Civilization.FacilityCount));
    // Terrain flattening depends on the actual facility coordinates, not merely
    // how many facilities exist. Same-count construction/migration must rebuild
    // the visual surface around the new authoritative locations.
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 FacilityId = static_cast<uint64>(Facility.FacilityId);
        Signature = MixTerrainHash(
            Signature,
            static_cast<uint32>(FacilityId & 0xFFFFFFFFu));
        Signature = MixTerrainHash(
            Signature,
            static_cast<uint32>((FacilityId >> 32) & 0xFFFFFFFFu));
        Signature = MixTerrainHash(Signature, static_cast<uint32>(Facility.GridX));
        Signature = MixTerrainHash(Signature, static_cast<uint32>(Facility.GridY));
    }
    if (!bForce && bBuiltOnce && Signature == LastSignature)
    {
        return;
    }

    TMap<FIntPoint, FLLCoreNaturalChunkObservation> ChunkByCoord;
    for (const FLLCoreNaturalChunkObservation& Chunk : Chunks)
    {
        ChunkByCoord.Add(FIntPoint(Chunk.ChunkX, Chunk.ChunkY), Chunk);
    }

    TArray<FVector2D> FacilityCentersUU;
    FacilityCentersUU.Reserve(Civilization.Facilities.Num());
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        FacilityCentersUU.Add(FVector2D(
            static_cast<float>(Facility.GridX - World.InitialCenterGridX)
                * LLWorldSpatialContract::GridCellSizeUU,
            static_cast<float>(Facility.GridY - World.InitialCenterGridY)
                * LLWorldSpatialContract::GridCellSizeUU));
    }

    TerrainMesh->ClearAllMeshSections();

    const int32 Steps = FMath::Clamp(SubdivisionsPerChunk, 2, 24);
    int32 SectionIndex = 0;
    for (const FLLCoreTerrainPresentationObservation& Terrain : Terrains)
    {
        const FLLCoreNaturalChunkObservation* Chunk =
            ChunkByCoord.Find(FIntPoint(Terrain.ChunkX, Terrain.ChunkY));
        if (!Chunk || !Chunk->bMaterialized)
        {
            continue;
        }

        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UV0;
        TArray<FColor> VertexColors;
        TArray<FProcMeshTangent> Tangents;

        const int32 VerticesPerSide = Steps + 1;
        Vertices.Reserve(VerticesPerSide * VerticesPerSide);
        UV0.Reserve(VerticesPerSide * VerticesPerSide);
        VertexColors.Reserve(VerticesPerSide * VerticesPerSide);

        const FVector2D ChunkCenter(
            static_cast<float>(Terrain.ChunkX - World.InitialChunkX)
                * LLWorldSpatialContract::ChunkSpanUU,
            static_cast<float>(Terrain.ChunkY - World.InitialChunkY)
                * LLWorldSpatialContract::ChunkSpanUU);
        const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;

        for (int32 Y = 0; Y <= Steps; ++Y)
        {
            const float V = static_cast<float>(Y) / static_cast<float>(Steps);
            for (int32 X = 0; X <= Steps; ++X)
            {
                const float U = static_cast<float>(X) / static_cast<float>(Steps);
                const FVector2D Location(
                    FMath::Lerp(ChunkCenter.X - Half, ChunkCenter.X + Half, U),
                    FMath::Lerp(ChunkCenter.Y - Half, ChunkCenter.Y + Half, V));

                Vertices.Add(FVector(
                    Location.X,
                    Location.Y,
                    SurfaceZUU(World, Terrain, Location, FacilityCentersUU)));
                UV0.Add(FVector2D(U * 4.0f, V * 4.0f));
                VertexColors.Add(FColor::White);
            }
        }

        Triangles.Reserve(Steps * Steps * 6);
        for (int32 Y = 0; Y < Steps; ++Y)
        {
            for (int32 X = 0; X < Steps; ++X)
            {
                const int32 I0 = Y * VerticesPerSide + X;
                const int32 I1 = I0 + 1;
                const int32 I2 = I0 + VerticesPerSide;
                const int32 I3 = I2 + 1;

                Triangles.Add(I0); Triangles.Add(I2); Triangles.Add(I1);
                Triangles.Add(I1); Triangles.Add(I2); Triangles.Add(I3);
            }
        }

        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
            Vertices,
            Triangles,
            UV0,
            Normals,
            Tangents);

        TerrainMesh->CreateMeshSection(
            SectionIndex,
            Vertices,
            Triangles,
            Normals,
            UV0,
            VertexColors,
            Tangents,
            false);

        if (UMaterialInterface* Material = MaterialForChunk(*Chunk))
        {
            TerrainMesh->SetMaterial(SectionIndex, Material);
        }
        ++SectionIndex;
    }

    LastSignature = Signature;
    bBuiltOnce = true;

    UE_LOG(LogTemp, Log,
        TEXT("LifeLens desktop smooth terrain: sections=%d chunks=%d subdivisions=%d facilities=%d"),
        SectionIndex,
        Terrains.Num(),
        Steps,
        Civilization.Facilities.Num());
}

#else

ALLDesktopTerrainPresentationActor::ALLDesktopTerrainPresentationActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ALLDesktopTerrainPresentationActor::BeginPlay()
{
    Super::BeginPlay();
}

void ALLDesktopTerrainPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

#endif
