#include "WorldPresentation/LLWorldPresentationActor.h"

#include "Characters/LLResidentCharacter.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Simulation/LLCivilizationReadTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLWorldGenerationReadTypes.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "World/LLWorldSpatialContract.h"

namespace
{
    uint32 MixHash(uint32 Seed, uint32 Value)
    {
        Seed ^= Value + 0x9E3779B9u + (Seed << 6) + (Seed >> 2);
        return Seed;
    }

    uint32 ChunkHash(int64 WorldSeed, int32 GenerationVersion, int32 ChunkX, int32 ChunkY)
    {
        uint32 Hash = static_cast<uint32>(WorldSeed & 0xFFFFFFFF);
        Hash = MixHash(Hash, static_cast<uint32>((WorldSeed >> 32) & 0xFFFFFFFF));
        Hash = MixHash(Hash, static_cast<uint32>(GenerationVersion));
        Hash = MixHash(Hash, static_cast<uint32>(ChunkX));
        Hash = MixHash(Hash, static_cast<uint32>(ChunkY));
        return Hash;
    }

    float HashUnit(uint32& State)
    {
        State = MixHash(State, 0x2545F491u);
        return static_cast<float>(State % 1000000u) / 1000000.0f;
    }

    int32 ScaledCount(float Factor, int32 MaxCount)
    {
        return FMath::Clamp(FMath::RoundToInt(FMath::Clamp(Factor, 0.0f, 1.0f) * MaxCount), 0, MaxCount);
    }

    uint32 PresentationSeed(int64 Seed)
    {
        const uint64 Word = static_cast<uint64>(Seed);
        uint32 Hash = static_cast<uint32>(Word & 0xFFFFFFFFu);
        Hash = MixHash(Hash, static_cast<uint32>((Word >> 32) & 0xFFFFFFFFu));
        return Hash != 0 ? Hash : 0x4C4C5043u; // LLPC
    }
}

ALLWorldPresentationActor::ALLWorldPresentationActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.5f;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GrassMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_Grass.MI_Ground_Grass"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> DryMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_DryEarth.MI_Ground_DryEarth"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TransitionMatFinder(TEXT("/Game/Environment/Materials/MI_Ground_Transition.MI_Ground_Transition"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FacilitySurfaceMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FacilityAccentMatFinder(TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));

    GroundMesh = CubeFinder.Succeeded() ? CubeFinder.Object : nullptr;
    GroundGrass = GrassMatFinder.Succeeded() ? GrassMatFinder.Object : nullptr;
    GroundDry = DryMatFinder.Succeeded() ? DryMatFinder.Object : nullptr;
    GroundTransition = TransitionMatFinder.Succeeded() ? TransitionMatFinder.Object : nullptr;
    FacilitySurfaceMaterial = FacilitySurfaceMatFinder.Succeeded() ? FacilitySurfaceMatFinder.Object : nullptr;
    FacilityAccentMaterial = FacilityAccentMatFinder.Succeeded() ? FacilityAccentMatFinder.Object : nullptr;

    // Production local-view art only uses the curated photoreal catalogue.
    // Missing approved assets are omitted rather than silently regressing to
    // obvious prototype geometry. Stylized assets remain available in Content
    // for explicit bootstrap/LOD/developer modes, not as the normal hero path.
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreeFirSapling(
        TEXT("/Game/Environment/Photoreal/PolyHaven/fir_sapling/SM_LL_fir_sapling.SM_LL_fir_sapling"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreePineSapling(
        TEXT("/Game/Environment/Photoreal/PolyHaven/pine_sapling_small/SM_LL_pine_sapling_small.SM_LL_pine_sapling_small"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreeFirB(
        TEXT("/Game/Environment/Photoreal/PolyHaven/fir_sapling/fir_sapling_1k/StaticMeshes/fir_sapling_b.fir_sapling_b"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreeFirC(
        TEXT("/Game/Environment/Photoreal/PolyHaven/fir_sapling/fir_sapling_1k/StaticMeshes/fir_sapling_c.fir_sapling_c"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreePineB(
        TEXT("/Game/Environment/Photoreal/PolyHaven/pine_sapling_small/pine_sapling_small_1k/StaticMeshes/pine_sapling_small_b.pine_sapling_small_b"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoTreePineC(
        TEXT("/Game/Environment/Photoreal/PolyHaven/pine_sapling_small/pine_sapling_small_1k/StaticMeshes/pine_sapling_small_c.pine_sapling_small_c"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoBoulder(
        TEXT("/Game/Environment/Photoreal/PolyHaven/boulder_01/SM_LL_boulder_01.SM_LL_boulder_01"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrubA(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_02/SM_LL_shrub_02.SM_LL_shrub_02"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrubB(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_03/SM_LL_shrub_03.SM_LL_shrub_03"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrub02B(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_02/shrub_02_1k/StaticMeshes/shrub_02_b.shrub_02_b"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrub02C(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_02/shrub_02_1k/StaticMeshes/shrub_02_c.shrub_02_c"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrub03B(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_03/shrub_03_1k/StaticMeshes/shrub_03_b.shrub_03_b"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoShrub03C(
        TEXT("/Game/Environment/Photoreal/PolyHaven/shrub_03/shrub_03_1k/StaticMeshes/shrub_03_c.shrub_03_c"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoGroundCover(
        TEXT("/Game/Environment/Photoreal/PolyHaven/weed_plant_02/SM_LL_weed_plant_02.SM_LL_weed_plant_02"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoGroundCoverB(
        TEXT("/Game/Environment/Photoreal/PolyHaven/weed_plant_02/weed_plant_02_1k/StaticMeshes/weed_plant_02_b_LOD0.weed_plant_02_b_LOD0"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoGroundCoverC(
        TEXT("/Game/Environment/Photoreal/PolyHaven/weed_plant_02/weed_plant_02_1k/StaticMeshes/weed_plant_02_c_LOD0.weed_plant_02_c_LOD0"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoGroundCoverD(
        TEXT("/Game/Environment/Photoreal/PolyHaven/weed_plant_02/weed_plant_02_1k/StaticMeshes/weed_plant_02_d_LOD0.weed_plant_02_d_LOD0"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoFirePit(
        TEXT("/Game/Environment/Photoreal/PolyHaven/stone_fire_pit/SM_LL_stone_fire_pit.SM_LL_stone_fire_pit"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoStorageBasket(
        TEXT("/Game/Environment/Photoreal/PolyHaven/wicker_basket_01/SM_LL_wicker_basket_01.SM_LL_wicker_basket_01"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoWoodenAxe(
        TEXT("/Game/Environment/Photoreal/PolyHaven/wooden_axe/SM_LL_wooden_axe.SM_LL_wooden_axe"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PhotoStructureLog(
        TEXT("/Game/Environment/Photoreal/PolyHaven/dead_tree_trunk/SM_LL_dead_tree_trunk.SM_LL_dead_tree_trunk"));

    if (PhotoTreeFirSapling.Succeeded()) { TreeMeshes.Add(PhotoTreeFirSapling.Object); }
    if (PhotoTreePineSapling.Succeeded()) { TreeMeshes.Add(PhotoTreePineSapling.Object); }
    if (PhotoTreeFirB.Succeeded()) { TreeMeshes.Add(PhotoTreeFirB.Object); }
    if (PhotoTreeFirC.Succeeded()) { TreeMeshes.Add(PhotoTreeFirC.Object); }
    if (PhotoTreePineB.Succeeded()) { TreeMeshes.Add(PhotoTreePineB.Object); }
    if (PhotoTreePineC.Succeeded()) { TreeMeshes.Add(PhotoTreePineC.Object); }
    if (PhotoShrubA.Succeeded()) { ShrubMeshes.Add(PhotoShrubA.Object); }
    if (PhotoShrubB.Succeeded()) { ShrubMeshes.Add(PhotoShrubB.Object); }
    if (PhotoShrub02B.Succeeded()) { ShrubMeshes.Add(PhotoShrub02B.Object); }
    if (PhotoShrub02C.Succeeded()) { ShrubMeshes.Add(PhotoShrub02C.Object); }
    if (PhotoShrub03B.Succeeded()) { ShrubMeshes.Add(PhotoShrub03B.Object); }
    if (PhotoShrub03C.Succeeded()) { ShrubMeshes.Add(PhotoShrub03C.Object); }
    if (PhotoGroundCover.Succeeded()) { GrassMeshes.Add(PhotoGroundCover.Object); }
    if (PhotoGroundCoverB.Succeeded()) { GrassMeshes.Add(PhotoGroundCoverB.Object); }
    if (PhotoGroundCoverC.Succeeded()) { GrassMeshes.Add(PhotoGroundCoverC.Object); }
    if (PhotoGroundCoverD.Succeeded()) { GrassMeshes.Add(PhotoGroundCoverD.Object); }
    if (PhotoBoulder.Succeeded()) { RockMeshes.Add(PhotoBoulder.Object); }

    UE_LOG(LogTemp, Log,
        TEXT("LLWorldPresentation production natural art: trees=%d shrubs=%d groundCover=%d rocks=%d"),
        TreeMeshes.Num(),
        ShrubMeshes.Num(),
        GrassMeshes.Num(),
        RockMeshes.Num());

    Ground = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GeneratedGround"));
    Ground->SetupAttachment(Root);
    Ground->SetMobility(EComponentMobility::Movable);
    Ground->SetStaticMesh(GroundMesh);
    Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Ground->SetCanEverAffectNavigation(false);

    FarGround = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FarVisualGround"));
    FarGround->SetupAttachment(Root);
    FarGround->SetMobility(EComponentMobility::Movable);
    FarGround->SetStaticMesh(GroundMesh);
    FarGround->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FarGround->SetCanEverAffectNavigation(false);
    FarGround->SetCastShadow(false);

    GroundGrassTileInstances = AddInstancedComponent(
        TEXT("GroundGrassTiles"),
        GroundMesh,
        TreeCullStartUU,
        TreeCullEndUU,
        false);
    GroundDryTileInstances = AddInstancedComponent(
        TEXT("GroundDryTiles"),
        GroundMesh,
        TreeCullStartUU,
        TreeCullEndUU,
        false);
    GroundTransitionTileInstances = AddInstancedComponent(
        TEXT("GroundTransitionTiles"),
        GroundMesh,
        TreeCullStartUU,
        TreeCullEndUU,
        false);

    if (GroundGrassTileInstances && GroundGrass)
    {
        GroundGrassTileInstances->SetMaterial(0, GroundGrass);
    }
    if (GroundDryTileInstances && GroundDry)
    {
        GroundDryTileInstances->SetMaterial(0, GroundDry);
    }
    if (GroundTransitionTileInstances && GroundTransition)
    {
        GroundTransitionTileInstances->SetMaterial(0, GroundTransition);
    }

    for (int32 Index = 0; Index < TreeMeshes.Num(); ++Index)
    {
        TreeInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Trees_%d"), Index), TreeMeshes[Index], TreeCullStartUU, TreeCullEndUU, true));
        FarTreeInstances.Add(AddInstancedComponent(
            *FString::Printf(TEXT("FarTrees_%d"), Index),
            TreeMeshes[Index],
            FarDressingCullStartUU,
            FarDressingCullEndUU,
            false));
    }
    for (int32 Index = 0; Index < ShrubMeshes.Num(); ++Index)
    {
        ShrubInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Shrubs_%d"), Index), ShrubMeshes[Index], SmallCullStartUU, SmallCullEndUU, false));
    }
    for (int32 Index = 0; Index < GrassMeshes.Num(); ++Index)
    {
        GrassInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Grass_%d"), Index), GrassMeshes[Index], SmallCullStartUU, SmallCullEndUU, false));
    }
    for (int32 Index = 0; Index < RockMeshes.Num(); ++Index)
    {
        RockInstances.Add(AddInstancedComponent(*FString::Printf(TEXT("Rocks_%d"), Index), RockMeshes[Index], SmallCullStartUU, TreeCullEndUU, false));
        FarRockInstances.Add(AddInstancedComponent(
            *FString::Printf(TEXT("FarRocks_%d"), Index),
            RockMeshes[Index],
            FarDressingCullStartUU,
            FarDressingCullEndUU,
            false));
    }

    FacilityFoundationInstances = AddInstancedComponent(TEXT("FacilityFoundations"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityPostInstances = AddInstancedComponent(TEXT("FacilityPosts"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityRoofInstances = AddInstancedComponent(TEXT("FacilityRoofs"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityCargoInstances = AddInstancedComponent(TEXT("FacilityCargo"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, true);
    FacilityAccentInstances = AddInstancedComponent(TEXT("FacilityAccents"), GroundMesh, FacilityCullStartUU, FacilityCullEndUU, false);

    if (PhotoFirePit.Succeeded())
    {
        PhotorealFirePitInstances = AddInstancedComponent(
            TEXT("PhotorealFirePits"), PhotoFirePit.Object,
            FacilityCullStartUU, FacilityCullEndUU, true);
    }
    if (PhotoStorageBasket.Succeeded())
    {
        PhotorealStorageBasketInstances = AddInstancedComponent(
            TEXT("PhotorealStorageBaskets"), PhotoStorageBasket.Object,
            FacilityCullStartUU, FacilityCullEndUU, true);
    }
    if (PhotoWoodenAxe.Succeeded())
    {
        PhotorealWorkToolInstances = AddInstancedComponent(
            TEXT("PhotorealWorkTools"), PhotoWoodenAxe.Object,
            FacilityCullStartUU, FacilityCullEndUU, true);
    }
    if (PhotoStructureLog.Succeeded())
    {
        PhotorealStructureLogInstances = AddInstancedComponent(
            TEXT("PhotorealStructureLogs"), PhotoStructureLog.Object,
            FacilityCullStartUU, FacilityCullEndUU, true);
    }

    UE_LOG(LogTemp, Log,
        TEXT("LLWorldPresentation approved facility art: firepit=%d basket=%d workTool=%d structureLog=%d deterministicSeeds=1"),
        PhotorealFirePitInstances ? 1 : 0,
        PhotorealStorageBasketInstances ? 1 : 0,
        PhotorealWorkToolInstances ? 1 : 0,
        PhotorealStructureLogInstances ? 1 : 0);
}

UHierarchicalInstancedStaticMeshComponent* ALLWorldPresentationActor::AddInstancedComponent(
    const TCHAR* Name, UStaticMesh* Mesh, float CullStartUU, float CullEndUU, bool bCastShadow)
{
    UHierarchicalInstancedStaticMeshComponent* Component =
        CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(Name);
    Component->SetupAttachment(GetRootComponent());
    Component->SetStaticMesh(Mesh);
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(false);
    Component->SetCastShadow(bCastShadow);
    Component->InstanceStartCullDistance = static_cast<int32>(CullStartUU);
    Component->InstanceEndCullDistance = static_cast<int32>(CullEndUU);
    return Component;
}

void ALLWorldPresentationActor::AddPhotorealStructureLog(
    const FVector& CenterUU,
    const FVector& DirectionUU,
    float LengthUU,
    float DiameterUU)
{
    if (!PhotorealStructureLogInstances || LengthUU <= KINDA_SMALL_NUMBER || DiameterUU <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    UStaticMesh* Mesh = PhotorealStructureLogInstances->GetStaticMesh();
    if (!Mesh)
    {
        return;
    }

    const FVector NativeSize = Mesh->GetBounds().BoxExtent * 2.0f;
    if (NativeSize.X <= KINDA_SMALL_NUMBER
        || NativeSize.Y <= KINDA_SMALL_NUMBER
        || NativeSize.Z <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    int32 LongAxis = 0;
    if (NativeSize.Y > NativeSize.X && NativeSize.Y >= NativeSize.Z)
    {
        LongAxis = 1;
    }
    else if (NativeSize.Z > NativeSize.X && NativeSize.Z > NativeSize.Y)
    {
        LongAxis = 2;
    }

    FVector Scale(
        DiameterUU / NativeSize.X,
        DiameterUU / NativeSize.Y,
        DiameterUU / NativeSize.Z);
    FVector LocalLongAxis = FVector::ForwardVector;
    if (LongAxis == 0)
    {
        Scale.X = LengthUU / NativeSize.X;
    }
    else if (LongAxis == 1)
    {
        LocalLongAxis = FVector::RightVector;
        Scale.Y = LengthUU / NativeSize.Y;
    }
    else
    {
        LocalLongAxis = FVector::UpVector;
        Scale.Z = LengthUU / NativeSize.Z;
    }

    const FVector TargetDirection = DirectionUU.GetSafeNormal();
    if (TargetDirection.IsNearlyZero())
    {
        return;
    }

    const FQuat Alignment = FQuat::FindBetweenNormals(LocalLongAxis, TargetDirection);
    PhotorealStructureLogInstances->AddInstance(
        FTransform(Alignment, CenterUU, Scale));
}

void ALLWorldPresentationActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyFacilityMaterialPalette();
    RefreshFromCore(true);
    UpdateDynamicObserverCanopyVisibility();
}

void ALLWorldPresentationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshAccumulator += DeltaSeconds;
    if (RefreshAccumulator >= RefreshIntervalSeconds)
    {
        RefreshAccumulator = 0.0f;
        RefreshFromCore(false);
    }

    // Camera readability is presentation state and must follow the observer,
    // not the much slower Core/world refresh cadence.
    UpdateDynamicObserverCanopyVisibility();
}

void ALLWorldPresentationActor::ClearInstances()
{
    if (GroundGrassTileInstances) { GroundGrassTileInstances->ClearInstances(); }
    if (GroundDryTileInstances) { GroundDryTileInstances->ClearInstances(); }
    if (GroundTransitionTileInstances) { GroundTransitionTileInstances->ClearInstances(); }

    for (UHierarchicalInstancedStaticMeshComponent* Component : TreeInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : ShrubInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : GrassInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : RockInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : FarTreeInstances) { if (Component) { Component->ClearInstances(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : FarRockInstances) { if (Component) { Component->ClearInstances(); } }
    PlacedTrees = 0;
    PlacedShrubs = 0;
    PlacedGrass = 0;
    PlacedRocks = 0;
    SuppressedDressing = 0;
    SightlineCleared = 0;
    DynamicCanopySuppressed = 0;
    DynamicCanopyInstances.Reset();
}

void ALLWorldPresentationActor::ClearFacilityInstances()
{
    if (FacilityFoundationInstances) { FacilityFoundationInstances->ClearInstances(); }
    if (FacilityPostInstances) { FacilityPostInstances->ClearInstances(); }
    if (FacilityRoofInstances) { FacilityRoofInstances->ClearInstances(); }
    if (FacilityCargoInstances) { FacilityCargoInstances->ClearInstances(); }
    if (FacilityAccentInstances) { FacilityAccentInstances->ClearInstances(); }
    if (PhotorealFirePitInstances) { PhotorealFirePitInstances->ClearInstances(); }
    if (PhotorealStorageBasketInstances) { PhotorealStorageBasketInstances->ClearInstances(); }
    if (PhotorealStructureLogInstances) { PhotorealStructureLogInstances->ClearInstances(); }
    if (PhotorealWorkToolInstances) { PhotorealWorkToolInstances->ClearInstances(); }
}

void ALLWorldPresentationActor::ApplyFacilityMaterialPalette()
{
    const FName ColorParameter(TEXT("Color"));
    if (FacilitySurfaceMaterial)
    {
        FacilityFoundationMaterial = UMaterialInstanceDynamic::Create(FacilitySurfaceMaterial, this);
        FacilityPostMaterial = UMaterialInstanceDynamic::Create(FacilitySurfaceMaterial, this);
        FacilityRoofMaterial = UMaterialInstanceDynamic::Create(FacilitySurfaceMaterial, this);
        FacilityCargoMaterial = UMaterialInstanceDynamic::Create(FacilitySurfaceMaterial, this);

        if (FacilityFoundationMaterial)
        {
            FacilityFoundationMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(0.24f, 0.22f, 0.19f, 1.0f));
            if (FacilityFoundationInstances) { FacilityFoundationInstances->SetMaterial(0, FacilityFoundationMaterial); }
        }
        if (FacilityPostMaterial)
        {
            FacilityPostMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(0.34f, 0.20f, 0.10f, 1.0f));
            if (FacilityPostInstances) { FacilityPostInstances->SetMaterial(0, FacilityPostMaterial); }
        }
        if (FacilityRoofMaterial)
        {
            FacilityRoofMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(0.39f, 0.31f, 0.19f, 1.0f));
            if (FacilityRoofInstances) { FacilityRoofInstances->SetMaterial(0, FacilityRoofMaterial); }
        }
        if (FacilityCargoMaterial)
        {
            FacilityCargoMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(0.47f, 0.32f, 0.16f, 1.0f));
            if (FacilityCargoInstances) { FacilityCargoInstances->SetMaterial(0, FacilityCargoMaterial); }
        }
    }

    if (FacilityAccentMaterial)
    {
        FacilityAccentDynamicMaterial = UMaterialInstanceDynamic::Create(FacilityAccentMaterial, this);
        if (FacilityAccentDynamicMaterial)
        {
            FacilityAccentDynamicMaterial->SetVectorParameterValue(ColorParameter, FLinearColor(1.0f, 0.20f, 0.025f, 1.0f));
            if (FacilityAccentInstances) { FacilityAccentInstances->SetMaterial(0, FacilityAccentDynamicMaterial); }
        }
    }
}

FVector2D ALLWorldPresentationActor::SettlementReferenceUU(const FLLCoreWorldGenerationObservation& World) const
{
    // Resource patches convert with `(GridX - InitialCenterGridX) * CellSize`,
    // so the Core start-region centre maps to the presentation origin by
    // construction. Deriving it keeps the envelope tied to `InitialCenterGrid`
    // instead of a hard-coded world origin.
    const FVector ChunkOffset = ChunkOriginUU(World, World.InitialChunkX, World.InitialChunkY);
    return FVector2D(ChunkOffset.X, ChunkOffset.Y);
}

float ALLWorldPresentationActor::FacilityDressingKeepFactor(
    const FVector2D& LocationUU,
    ELLDressingLayer Layer) const
{
    if (Layer == ELLDressingLayer::GroundDetail
        || CachedFacilityReadabilityCentersUU.Num() == 0)
    {
        return 1.0f;
    }

    const float ClearRadius = FMath::Max(0.0f, FacilityClearRadiusUU);
    const float ActivityRadius = FMath::Max(ClearRadius, FacilityActivityRadiusUU);
    if (ActivityRadius <= KINDA_SMALL_NUMBER)
    {
        return 1.0f;
    }

    float NearestDistanceSq = TNumericLimits<float>::Max();
    for (const FVector2D& FacilityCenter : CachedFacilityReadabilityCentersUU)
    {
        NearestDistanceSq = FMath::Min(
            NearestDistanceSq,
            FVector2D::DistSquared(LocationUU, FacilityCenter));
    }
    const float Distance = FMath::Sqrt(NearestDistanceSq);
    if (Distance >= ActivityRadius)
    {
        return 1.0f;
    }

    const bool bCanopy = Layer == ELLDressingLayer::Canopy;
    // Local facility envelopes are intentionally softer than the original
    // settlement core so an expanding town still feels embedded in nature.
    const float BaseCoreKeep = FMath::Clamp(
        bCanopy ? CoreZoneCanopyKeep : CoreZoneUndergrowthKeep,
        0.0f,
        1.0f);
    const float LocalCoreKeep = FMath::Clamp(
        BaseCoreKeep + (bCanopy ? 0.06f : 0.10f),
        0.0f,
        1.0f);
    if (Distance <= ClearRadius)
    {
        return LocalCoreKeep;
    }

    const float Band = FMath::Max(ActivityRadius - ClearRadius, KINDA_SMALL_NUMBER);
    const float Progress = FMath::Clamp((Distance - ClearRadius) / Band, 0.0f, 1.0f);
    const float Exponent = FMath::Max(
        1.0f,
        bCanopy ? CanopyRecoveryExponent : UndergrowthRecoveryExponent);
    return FMath::Lerp(
        LocalCoreKeep,
        1.0f,
        FMath::Pow(Progress, Exponent));
}

float ALLWorldPresentationActor::AmbientDressingKeepFactor(const FVector2D& LocationUU, ELLDressingLayer Layer) const
{
    // Ground detail is low enough that it never hides a resident.
    if (Layer == ELLDressingLayer::GroundDetail) { return 1.0f; }

    const float CoreRadius = FMath::Max(0.0f, CoreClearRadiusUU);
    const float ActivityRadius = FMath::Max(CoreRadius, ActivityRadiusUU);
    const float Distance = (LocationUU - CachedSettlementReferenceUU).Size();

    const bool bCanopy = Layer == ELLDressingLayer::Canopy;
    const float CoreKeep = FMath::Clamp(bCanopy ? CoreZoneCanopyKeep : CoreZoneUndergrowthKeep, 0.0f, 1.0f);
    float SettlementKeep = 1.0f;
    if (ActivityRadius > KINDA_SMALL_NUMBER && Distance < ActivityRadius)
    {
        if (Distance <= CoreRadius)
        {
            SettlementKeep = CoreKeep;
        }
        else
        {
            // Activity zone: restore density with distance, canopy last.
            const float Band = FMath::Max(ActivityRadius - CoreRadius, KINDA_SMALL_NUMBER);
            const float Progress = FMath::Clamp((Distance - CoreRadius) / Band, 0.0f, 1.0f);
            const float Exponent = FMath::Max(1.0f, bCanopy ? CanopyRecoveryExponent : UndergrowthRecoveryExponent);
            SettlementKeep = FMath::Lerp(CoreKeep, 1.0f, FMath::Pow(Progress, Exponent));
        }
    }

    return FMath::Min(
        SettlementKeep,
        FacilityDressingKeepFactor(LocationUU, Layer));
}

bool ALLWorldPresentationActor::CaptureInitialViewOrigin()
{
    if (bInitialViewCaptured) { return true; }
    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
    if (!CameraManager) { return false; }   // game mode has not placed the camera yet

    // Read only. The observer camera pose and its Config tuning belong to
    // another lane; this never writes to either.
    const FVector CameraLocation = CameraManager->GetCameraLocation();
    if (CameraLocation.ContainsNaN()) { return false; }

    InitialViewOriginUU = FVector2D(CameraLocation.X, CameraLocation.Y);
    bInitialViewCaptured = true;
    return true;
}

void ALLWorldPresentationActor::RegisterDynamicCanopyInstance(
    UHierarchicalInstancedStaticMeshComponent* Component,
    int32 InstanceIndex,
    const FTransform& BaseTransform)
{
    if (!Component || InstanceIndex == INDEX_NONE)
    {
        return;
    }

    FDynamicCanopyInstance Record;
    Record.Component = Component;
    Record.InstanceIndex = InstanceIndex;
    Record.BaseTransform = BaseTransform;
    DynamicCanopyInstances.Add(MoveTemp(Record));
}

void ALLWorldPresentationActor::UpdateDynamicObserverCanopyVisibility()
{
    if (DynamicCanopyInstances.Num() == 0)
    {
        DynamicCanopySuppressed = 0;
        return;
    }

    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
    const APlayerCameraManager* CameraManager = Controller ? Controller->PlayerCameraManager : nullptr;
    if (!World || !CameraManager)
    {
        return;
    }

    const FVector CameraLocation3D = CameraManager->GetCameraLocation();
    const FVector2D CameraLocation(CameraLocation3D.X, CameraLocation3D.Y);

    TArray<FVector2D> VisibilityTargets;
    for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
    {
        const ALLResidentCharacter* Resident = *It;
        if (!Resident || Resident->IsHidden())
        {
            continue;
        }
        const FVector Location = Resident->GetActorLocation();
        VisibilityTargets.Add(FVector2D(Location.X, Location.Y));
    }

    VisibilityTargets.Sort([CameraLocation](const FVector2D& A, const FVector2D& B)
    {
        return FVector2D::DistSquared(A, CameraLocation) < FVector2D::DistSquared(B, CameraLocation);
    });
    if (VisibilityTargets.Num() > MaxDynamicVisibilityTargets)
    {
        VisibilityTargets.SetNum(FMath::Max(1, MaxDynamicVisibilityTargets));
    }

    TSet<UHierarchicalInstancedStaticMeshComponent*> DirtyComponents;
    int32 SuppressedNow = 0;

    for (FDynamicCanopyInstance& Record : DynamicCanopyInstances)
    {
        UHierarchicalInstancedStaticMeshComponent* Component = Record.Component.Get();
        if (!Component || Record.InstanceIndex < 0 || Record.InstanceIndex >= Component->GetInstanceCount())
        {
            continue;
        }

        bool bShouldSuppress = false;
        if (bDynamicObserverCanopyVisibility && VisibilityTargets.Num() > 0)
        {
            const FVector TreeWorld3D = Component->GetComponentTransform().TransformPosition(Record.BaseTransform.GetLocation());
            const FVector2D TreeWorld(TreeWorld3D.X, TreeWorld3D.Y);
            const float CorridorRadius = Record.bSuppressed
                ? FMath::Max(DynamicCanopyHideRadiusUU, DynamicCanopyRestoreRadiusUU)
                : FMath::Max(1.0f, DynamicCanopyHideRadiusUU);
            const float CorridorRadiusSq = FMath::Square(CorridorRadius);

            for (const FVector2D& Target : VisibilityTargets)
            {
                const FVector2D Segment = Target - CameraLocation;
                const float SegmentLengthSq = Segment.SizeSquared();
                if (SegmentLengthSq <= KINDA_SMALL_NUMBER)
                {
                    continue;
                }

                const float Projection = FVector2D::DotProduct(TreeWorld - CameraLocation, Segment) / SegmentLengthSq;
                // Only canopy truly between camera and resident may collapse;
                // trees behind either endpoint remain untouched.
                if (Projection <= 0.03f || Projection >= 0.97f)
                {
                    continue;
                }

                const FVector2D Closest = CameraLocation + Segment * Projection;
                if (FVector2D::DistSquared(TreeWorld, Closest) <= CorridorRadiusSq)
                {
                    bShouldSuppress = true;
                    break;
                }
            }
        }

        if (bShouldSuppress != Record.bSuppressed)
        {
            FTransform Updated = Record.BaseTransform;
            if (bShouldSuppress)
            {
                Updated.SetScale3D(Record.BaseTransform.GetScale3D()
                    * FMath::Clamp(DynamicCanopyHiddenScale, 0.001f, 0.20f));
            }
            Component->UpdateInstanceTransform(Record.InstanceIndex, Updated, false, false, true);
            Record.bSuppressed = bShouldSuppress;
            DirtyComponents.Add(Component);
        }

        if (Record.bSuppressed)
        {
            ++SuppressedNow;
        }
    }

    for (UHierarchicalInstancedStaticMeshComponent* Component : DirtyComponents)
    {
        if (Component)
        {
            Component->MarkRenderStateDirty();
        }
    }
    DynamicCanopySuppressed = SuppressedNow;
}

float ALLWorldPresentationActor::InitialSightlineKeepFactor(const FVector2D& LocationUU) const
{
    if (!bClearInitialSightlineCanopy || !bInitialViewCaptured) { return 1.0f; }

    const FVector2D Axis = CachedSettlementReferenceUU - InitialViewOriginUU;
    const float AxisLength = Axis.Size();
    if (AxisLength <= KINDA_SMALL_NUMBER) { return 1.0f; }
    const FVector2D AxisDirection = Axis / AxisLength;

    const FVector2D ToPoint = LocationUU - InitialViewOriginUU;
    const float Along = FVector2D::DotProduct(ToPoint, AxisDirection);
    // Only what stands between the camera and the settlement can occlude it.
    if (Along <= 0.0f || Along >= AxisLength) { return 1.0f; }

    const float Lateral = FMath::Abs(FVector2D::CrossProduct(ToPoint, AxisDirection));
    const float InnerHalfAngle = FMath::DegreesToRadians(FMath::Max(0.0f, InitialSightlineHalfAngleDegrees));
    const float OuterHalfAngle = InnerHalfAngle
        + FMath::DegreesToRadians(FMath::Max(0.0f, InitialSightlineEdgeFalloffDegrees));

    // The cone widens with distance so the cleared wedge stays a constant
    // angular slice of the opening view.
    const float InnerWidth = Along * FMath::Tan(InnerHalfAngle);
    const float OuterWidth = Along * FMath::Tan(OuterHalfAngle);
    const float CentreKeep = FMath::Clamp(InitialSightlineCanopyKeep, 0.0f, 1.0f);

    if (Lateral <= InnerWidth) { return CentreKeep; }
    if (Lateral >= OuterWidth || OuterWidth - InnerWidth <= KINDA_SMALL_NUMBER) { return 1.0f; }
    const float EdgeProgress = (Lateral - InnerWidth) / (OuterWidth - InnerWidth);
    return FMath::Lerp(CentreKeep, 1.0f, EdgeProgress);
}

float ALLWorldPresentationActor::ResourcePatchScaleFactor(const FVector2D& LocationUU) const
{
    // An authoritative resource is never removed for readability; inside the
    // settlement or immediately beside a real facility it is only drawn smaller.
    const float Distance = (LocationUU - CachedSettlementReferenceUU).Size();
    float Scale = 1.0f;
    if (Distance <= FMath::Max(0.0f, CoreClearRadiusUU))
    {
        Scale = FMath::Clamp(CoreZoneResourceScale, 0.1f, 1.0f);
    }
    else if (Distance <= FMath::Max(CoreClearRadiusUU, ActivityRadiusUU))
    {
        Scale = FMath::Clamp(ActivityZoneResourceScale, 0.1f, 1.0f);
    }

    if (CachedFacilityReadabilityCentersUU.Num() > 0)
    {
        float NearestDistanceSq = TNumericLimits<float>::Max();
        for (const FVector2D& FacilityCenter : CachedFacilityReadabilityCentersUU)
        {
            NearestDistanceSq = FMath::Min(
                NearestDistanceSq,
                FVector2D::DistSquared(LocationUU, FacilityCenter));
        }
        const float FacilityDistance = FMath::Sqrt(NearestDistanceSq);
        if (FacilityDistance <= FMath::Max(0.0f, FacilityClearRadiusUU))
        {
            Scale = FMath::Min(
                Scale,
                FMath::Clamp(CoreZoneResourceScale + 0.15f, 0.1f, 1.0f));
        }
        else if (FacilityDistance <= FMath::Max(FacilityClearRadiusUU, FacilityActivityRadiusUU))
        {
            Scale = FMath::Min(
                Scale,
                FMath::Clamp(ActivityZoneResourceScale, 0.1f, 1.0f));
        }
    }

    return Scale;
}

FVector ALLWorldPresentationActor::ChunkOriginUU(const FLLCoreWorldGenerationObservation& World, int32 ChunkX, int32 ChunkY) const
{
    const float OffsetX = static_cast<float>(ChunkX - World.InitialChunkX) * LLWorldSpatialContract::ChunkSpanUU;
    const float OffsetY = static_cast<float>(ChunkY - World.InitialChunkY) * LLWorldSpatialContract::ChunkSpanUU;
    return FVector(OffsetX, OffsetY, 0.0f);
}

float ALLWorldPresentationActor::TerrainReliefBlend(const FVector2D& LocationUU) const
{
    const float Start = FMath::Max(0.0f, TerrainReliefFlattenRadiusUU);
    const float End = Start + FMath::Max(100.0f, TerrainReliefBlendBandUU);
    const float SettlementDistance = (LocationUU - CachedSettlementReferenceUU).Size();
    float Blend = FMath::SmoothStep(
        0.0f,
        1.0f,
        FMath::Clamp((SettlementDistance - Start) / FMath::Max(End - Start, 1.0f), 0.0f, 1.0f));

    for (const FVector2D& FacilityCenter : CachedFacilityReadabilityCentersUU)
    {
        const float Distance = (LocationUU - FacilityCenter).Size();
        const float LocalStart = FMath::Max(0.0f, FacilityClearRadiusUU);
        const float LocalEnd = FMath::Max(LocalStart + 100.0f, FacilityActivityRadiusUU);
        const float FacilityBlend = FMath::SmoothStep(
            0.0f,
            1.0f,
            FMath::Clamp((Distance - LocalStart) / FMath::Max(LocalEnd - LocalStart, 1.0f), 0.0f, 1.0f));
        Blend = FMath::Min(Blend, FacilityBlend);
    }
    return FMath::Clamp(Blend, 0.0f, 1.0f);
}

float ALLWorldPresentationActor::TerrainSurfaceZUU(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreTerrainPresentationObservation& Terrain,
    const FVector2D& LocationUU) const
{
    if (!Terrain.bAvailable || TerrainReliefAmplitudeUU <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    const float Blend = TerrainReliefBlend(LocationUU);
    if (Blend <= KINDA_SMALL_NUMBER)
    {
        return 0.0f;
    }

    auto ElevationOffset = [&](float Elevation01)
    {
        // Preserve the flat locomotion/collision baseline and add hills only.
        // This avoids hiding visual terrain beneath the collision-only bootstrap
        // floor while still making macro elevation readable at observer range.
        return FMath::Max(
            0.0f,
            Elevation01 - World.InitialChunk.Elevation)
            * FMath::Max(0.0f, TerrainReliefAmplitudeUU);
    };

    const FVector ChunkCenter3D = ChunkOriginUU(World, Terrain.ChunkX, Terrain.ChunkY);
    const FVector2D ChunkCenter(ChunkCenter3D.X, ChunkCenter3D.Y);
    const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
    const float U = FMath::Clamp((LocationUU.X - (ChunkCenter.X - Half))
        / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f), 0.0f, 1.0f);
    const float V = FMath::Clamp((LocationUU.Y - (ChunkCenter.Y - Half))
        / FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f), 0.0f, 1.0f);

    const float South = FMath::Lerp(
        ElevationOffset(Terrain.SouthWestElevation01),
        ElevationOffset(Terrain.SouthEastElevation01),
        U);
    const float North = FMath::Lerp(
        ElevationOffset(Terrain.NorthWestElevation01),
        ElevationOffset(Terrain.NorthEastElevation01),
        U);
    const float CornerSurface = FMath::Lerp(South, North, V);
    const float CenterSurface = ElevationOffset(Terrain.CenterElevation01);

    // Center truth stabilizes the tile while seam-compatible corners drive most
    // of the local shape.
    return FMath::Lerp(CenterSurface, CornerSurface, 0.72f) * Blend;
}

FRotator ALLWorldPresentationActor::TerrainTileRotation(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreTerrainPresentationObservation& Terrain,
    const FVector2D& CenterUU) const
{
    const float Half = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
    const float WestZ = TerrainSurfaceZUU(
        World, Terrain, CenterUU + FVector2D(-Half, 0.0f));
    const float EastZ = TerrainSurfaceZUU(
        World, Terrain, CenterUU + FVector2D(Half, 0.0f));
    const float SouthZ = TerrainSurfaceZUU(
        World, Terrain, CenterUU + FVector2D(0.0f, -Half));
    const float NorthZ = TerrainSurfaceZUU(
        World, Terrain, CenterUU + FVector2D(0.0f, Half));

    const float Pitch = FMath::Clamp(
        -FMath::RadiansToDegrees(FMath::Atan2(
            EastZ - WestZ,
            FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f))),
        -TerrainMaxTiltDegrees,
        TerrainMaxTiltDegrees);
    const float Roll = FMath::Clamp(
        FMath::RadiansToDegrees(FMath::Atan2(
            NorthZ - SouthZ,
            FMath::Max(LLWorldSpatialContract::ChunkSpanUU, 1.0f))),
        -TerrainMaxTiltDegrees,
        TerrainMaxTiltDegrees);
    return FRotator(Pitch, 0.0f, Roll);
}

UMaterialInterface* ALLWorldPresentationActor::GroundMaterialForChunk(const FLLCoreNaturalChunkObservation& Chunk) const
{
    const FString Surface = Chunk.Surface.ToString().ToLower();
    const FString Biome = Chunk.Biome.ToString().ToLower();
    const bool bDry = Chunk.Moisture < 0.33f
        || Surface.Contains(TEXT("sand")) || Surface.Contains(TEXT("rock")) || Surface.Contains(TEXT("dirt"))
        || Biome.Contains(TEXT("desert")) || Biome.Contains(TEXT("arid"));
    const bool bLush = Chunk.Moisture > 0.6f && Chunk.FertilityPotential > 0.45f;
    if (bDry && GroundDry) { return GroundDry; }
    if (bLush && GroundGrass) { return GroundGrass; }
    return GroundTransition ? GroundTransition.Get() : GroundGrass.Get();
}

void ALLWorldPresentationActor::BuildGround(const FLLCoreWorldGenerationObservation& World)
{
    if (!Ground || !GroundMesh) { return; }

    const int32 Rings = FMath::Clamp(
        FMath::CeilToInt(FMath::Sqrt(static_cast<float>(
            FMath::Max(1, World.MaterializedChunkCount)))),
        1,
        9);
    const float ActiveSpanUU =
        LLWorldSpatialContract::ChunkSpanUU * (2.0f * Rings + 1.0f);
    const float FarSpanUU = FMath::Max(
        ActiveSpanUU * FMath::Max(2.0f, FarGroundActiveSpanMultiplier),
        LLWorldSpatialContract::ChunkSpanUU
            * FMath::Max(16.0f, FarGroundMinSpanChunks));

    const float Thickness = 20.0f;
    Ground->SetRelativeLocation(FVector(0.0f, 0.0f, -Thickness * 0.5f));
    Ground->SetRelativeScale3D(FVector(
        ActiveSpanUU / LLWorldSpatialContract::EngineCubeSideUU,
        ActiveSpanUU / LLWorldSpatialContract::EngineCubeSideUU,
        Thickness / LLWorldSpatialContract::EngineCubeSideUU));

    UMaterialInterface* Material = GroundMaterialForChunk(World.InitialChunk);
    if (Material)
    {
        Ground->SetMaterial(0, Material);
    }

    if (FarGround)
    {
        // The far visual surface sits just under the authoritative/materialized
        // presentation surface. Its overlap removes the visible square edge
        // without adding collision or simulation authority outside Core chunks.
        const float FarThickness = 18.0f;
        FarGround->SetRelativeLocation(FVector(
            0.0f,
            0.0f,
            -FarThickness * 0.5f - FMath::Max(0.0f, FarGroundDropUU)));
        FarGround->SetRelativeScale3D(FVector(
            FarSpanUU / LLWorldSpatialContract::EngineCubeSideUU,
            FarSpanUU / LLWorldSpatialContract::EngineCubeSideUU,
            FarThickness / LLWorldSpatialContract::EngineCubeSideUU));
        if (Material)
        {
            FarGround->SetMaterial(0, Material);
        }
    }

    BuildFarEnvironment(World, ActiveSpanUU, FarSpanUU);
}

void ALLWorldPresentationActor::BuildChunkGround(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreNaturalChunkObservation& Chunk,
    const FLLCoreTerrainPresentationObservation& Terrain)
{
    if (!Chunk.bMaterialized || !GroundMesh)
    {
        return;
    }

    UMaterialInterface* Material = GroundMaterialForChunk(Chunk);
    UHierarchicalInstancedStaticMeshComponent* Target = nullptr;
    if (Material == GroundDry)
    {
        Target = GroundDryTileInstances;
    }
    else if (Material == GroundGrass)
    {
        Target = GroundGrassTileInstances;
    }
    else
    {
        Target = GroundTransitionTileInstances
            ? GroundTransitionTileInstances.Get()
            : GroundGrassTileInstances.Get();
    }

    if (!Target)
    {
        return;
    }

    const FVector ChunkOrigin = ChunkOriginUU(World, Chunk.ChunkX, Chunk.ChunkY);
    const FVector2D ChunkCenterUU(ChunkOrigin.X, ChunkOrigin.Y);
    const float TerrainZ = TerrainSurfaceZUU(World, Terrain, ChunkCenterUU);
    const FRotator TerrainRotation = TerrainTileRotation(World, Terrain, ChunkCenterUU);
    constexpr float TileThicknessUU = 8.0f;
    constexpr float SurfaceLiftUU = 0.35f;
    const float SpanScale =
        (LLWorldSpatialContract::ChunkSpanUU
            / LLWorldSpatialContract::EngineCubeSideUU)
        * 1.0125f;
    const float HeightScale =
        TileThicknessUU / LLWorldSpatialContract::EngineCubeSideUU;

    // Slight XY overlap removes precision cracks between adjacent tiles. The top
    // face sits 0.35 UU above the broad continuity ground, avoiding z-fighting.
    Target->AddInstance(FTransform(
        TerrainRotation,
        ChunkOrigin + FVector(
            0.0f,
            0.0f,
            TerrainZ - TileThicknessUU * 0.5f + SurfaceLiftUU),
        FVector(SpanScale, SpanScale, HeightScale)));
}

void ALLWorldPresentationActor::BuildFarEnvironment(
    const FLLCoreWorldGenerationObservation& World,
    float ActiveGroundSpanUU,
    float FarGroundSpanUU)
{
    if (FarGroundSpanUU <= ActiveGroundSpanUU)
    {
        return;
    }

    const float Fertility = FMath::Clamp(
        World.InitialChunk.FertilityPotential,
        0.0f,
        1.0f);
    const float Moisture = FMath::Clamp(
        World.InitialChunk.Moisture,
        0.0f,
        1.0f);
    const float Traversal = FMath::Clamp(
        World.InitialChunk.TraversalEase,
        0.0f,
        1.0f);

    const float TreeSuitability = FMath::Clamp(
        Fertility * (0.35f + 0.65f * Moisture),
        0.0f,
        1.0f);
    const float RockSuitability = FMath::Clamp(
        (1.0f - Fertility) * 0.7f + (1.0f - Traversal) * 0.3f,
        0.0f,
        1.0f);

    const int32 TreeCount = FMath::Clamp(
        FMath::RoundToInt(TreeSuitability * MaxFarTreeInstances),
        0,
        MaxFarTreeInstances);
    const int32 RockCount = FMath::Clamp(
        FMath::RoundToInt((0.25f + 0.75f * RockSuitability) * MaxFarRockInstances),
        0,
        MaxFarRockInstances);

    const float InnerRadius = FMath::Max(
        ActiveGroundSpanUU * 0.58f,
        LLWorldSpatialContract::ChunkSpanUU * 2.0f);
    const float OuterRadius = FMath::Max(
        InnerRadius + LLWorldSpatialContract::ChunkSpanUU,
        FarGroundSpanUU * FMath::Clamp(
            FarDressingOuterRadiusFraction,
            0.2f,
            0.48f));

    uint32 State = ChunkHash(
        World.WorldSeed,
        World.GenerationVersion,
        World.InitialChunkX ^ 0x5A5A,
        World.InitialChunkY ^ 0xA5A5);

    auto PlaceRing = [&](TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Components,
                         int32 Count,
                         float MinScale,
                         float MaxScale,
                         float HeightScale)
    {
        if (Components.Num() == 0 || Count <= 0)
        {
            return;
        }

        for (int32 Index = 0; Index < Count; ++Index)
        {
            const float Angle = HashUnit(State) * 2.0f * PI;
            // sqrt keeps a broad but visually denser outer horizon belt.
            const float RadiusT = FMath::Sqrt(HashUnit(State));
            const float Radius = FMath::Lerp(InnerRadius, OuterRadius, RadiusT);
            const float Yaw = HashUnit(State) * 360.0f;
            const float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(State));
            const int32 Slot =
                static_cast<int32>(HashUnit(State) * Components.Num())
                % Components.Num();

            if (UHierarchicalInstancedStaticMeshComponent* Component = Components[Slot])
            {
                const FVector Location(
                    FMath::Cos(Angle) * Radius,
                    FMath::Sin(Angle) * Radius,
                    -FMath::Max(0.0f, FarGroundDropUU));
                Component->AddInstance(FTransform(
                    FRotator(0.0f, Yaw, 0.0f),
                    Location,
                    FVector(Scale, Scale, Scale * HeightScale)));
            }
        }
    };

    // These are backdrop silhouettes only: no shadow, collision, nav or Core
    // resource identity. Real nearby dressing still comes exclusively from
    // materialized authoritative chunks.
    PlaceRing(FarTreeInstances, TreeCount, 0.75f, 1.45f, 1.15f);
    PlaceRing(FarRockInstances, RockCount, 0.65f, 1.55f, 0.85f);
}

void ALLWorldPresentationActor::BuildChunkDressing(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreNaturalChunkObservation& Chunk,
    const FLLCoreTerrainPresentationObservation& Terrain)
{
    if (!Chunk.bMaterialized) { return; }
    const FVector ChunkOrigin = ChunkOriginUU(World, Chunk.ChunkX, Chunk.ChunkY);
    const float HalfSpan = LLWorldSpatialContract::ChunkSpanUU * 0.5f;
    uint32 State = Chunk.VisualSeed != 0
        ? PresentationSeed(Chunk.VisualSeed)
        : ChunkHash(World.WorldSeed, World.GenerationVersion, Chunk.ChunkX, Chunk.ChunkY);

    const float Fertility = FMath::Clamp(Chunk.FertilityPotential, 0.0f, 1.0f);
    const float Moisture = FMath::Clamp(Chunk.Moisture, 0.0f, 1.0f);
    const float Traversal = FMath::Clamp(Chunk.TraversalEase, 0.0f, 1.0f);
    const int32 TreeCount = ScaledCount(Fertility * Moisture, MaxTreesPerChunk);
    const int32 ShrubCount = ScaledCount(Fertility * 0.8f + Moisture * 0.2f, MaxShrubsPerChunk);
    const int32 GrassCount = ScaledCount(Fertility * 0.6f + Moisture * 0.4f, MaxGrassPerChunk);
    const int32 RockCount = ScaledCount((1.0f - Fertility) * 0.7f + (1.0f - Traversal) * 0.3f, MaxRocksPerChunk);

    auto Place = [&](TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>& Components,
                     int32 Count, int32& Placed, int32 MaxTotal,
                     float MinScale, float MaxScale, float TiltDegrees,
                     ELLDressingLayer Layer)
    {
        if (Components.Num() == 0) { return; }
        for (int32 Index = 0; Index < Count && Placed < MaxTotal; ++Index)
        {
            const float X = (HashUnit(State) * 2.0f - 1.0f) * HalfSpan;
            const float Y = (HashUnit(State) * 2.0f - 1.0f) * HalfSpan;
            const float Yaw = HashUnit(State) * 360.0f;
            const float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(State));
            const float TiltPitch = (HashUnit(State) * 2.0f - 1.0f) * TiltDegrees;
            const float TiltRoll = (HashUnit(State) * 2.0f - 1.0f) * TiltDegrees;
            const float HeightJitter = FMath::Lerp(0.92f, 1.12f, HashUnit(State));
            const int32 Slot = static_cast<int32>(HashUnit(State) * Components.Num()) % Components.Num();
            const float KeepRoll = HashUnit(State);
            const FVector2D Location2D(
                ChunkOrigin.X + X,
                ChunkOrigin.Y + Y);
            const FVector Location(
                Location2D.X,
                Location2D.Y,
                TerrainSurfaceZUU(World, Terrain, Location2D));
            float KeepFactor = AmbientDressingKeepFactor(Location2D, Layer);
            if (Layer == ELLDressingLayer::Canopy && !bDynamicObserverCanopyVisibility)
            {
                // Legacy opening-view fallback. The default dynamic path keeps
                // the canopy instance so it can be restored when the camera moves.
                const float SightlineKeep = InitialSightlineKeepFactor(Location2D);
                if (SightlineKeep < KeepFactor)
                {
                    ++SightlineCleared;
                    KeepFactor = SightlineKeep;
                }
            }
            if (KeepRoll >= KeepFactor)
            {
                ++SuppressedDressing;
                continue;
            }
            if (UHierarchicalInstancedStaticMeshComponent* Component = Components[Slot])
            {
                const FTransform InstanceTransform(
                    FRotator(TiltPitch, Yaw, TiltRoll), Location, FVector(Scale, Scale, Scale * HeightJitter));
                const int32 InstanceIndex = Component->AddInstance(InstanceTransform);
                if (Layer == ELLDressingLayer::Canopy)
                {
                    // Only ambient canopy enters the reversible sightline set.
                    // Authoritative resource-patch trees are added later through
                    // a separate path and are never registered here.
                    RegisterDynamicCanopyInstance(Component, InstanceIndex, InstanceTransform);
                }
                ++Placed;
            }
        }
    };

    Place(TreeInstances, TreeCount, PlacedTrees, MaxTreeInstances, 0.85f, 1.6f, 3.0f, ELLDressingLayer::Canopy);
    Place(ShrubInstances, ShrubCount, PlacedShrubs, MaxShrubInstances, 0.7f, 1.5f, 5.0f, ELLDressingLayer::Undergrowth);
    Place(GrassInstances, GrassCount, PlacedGrass, MaxGrassInstances, 0.7f, 1.7f, 4.0f, ELLDressingLayer::Undergrowth);
    Place(RockInstances, RockCount, PlacedRocks, MaxRockInstances, 0.7f, 1.8f, 8.0f, ELLDressingLayer::GroundDetail);

    for (const FLLCoreNaturalResourcePatchObservation& Patch : Chunk.ResourcePatches)
    {
        const FString Material = Patch.Material.ToString().ToLower();
        TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>>* Target = nullptr;
        int32* PlacedCounter = nullptr;
        int32 MaxTotal = 0;
        float MinScale = 1.0f;
        float MaxScale = 1.3f;

        if (Material.Contains(TEXT("wood")) || Material.Contains(TEXT("timber")) || Material.Contains(TEXT("tree")))
        {
            Target = &TreeInstances; PlacedCounter = &PlacedTrees; MaxTotal = MaxTreeInstances; MinScale = 1.1f; MaxScale = 1.7f;
        }
        else if (Material.Contains(TEXT("stone")) || Material.Contains(TEXT("rock")) || Material.Contains(TEXT("flint")))
        {
            Target = &RockInstances; PlacedCounter = &PlacedRocks; MaxTotal = MaxRockInstances; MinScale = 1.0f; MaxScale = 1.9f;
        }
        else if (Material.Contains(TEXT("berry")) || Material.Contains(TEXT("plant"))
            || Material.Contains(TEXT("fiber")) || Material.Contains(TEXT("food")))
        {
            Target = &ShrubInstances; PlacedCounter = &PlacedShrubs; MaxTotal = MaxShrubInstances; MinScale = 0.9f; MaxScale = 1.5f;
        }

        if (!Target || Target->Num() == 0 || !PlacedCounter || *PlacedCounter >= MaxTotal) { continue; }
        const float PatchX = static_cast<float>(Patch.GridX - World.InitialCenterGridX) * LLWorldSpatialContract::GridCellSizeUU;
        const float PatchY = static_cast<float>(Patch.GridY - World.InitialCenterGridY) * LLWorldSpatialContract::GridCellSizeUU;
        const float Quantity01 = Patch.MaxQuantity > 0
            ? FMath::Clamp(
                static_cast<float>(Patch.CurrentQuantity)
                    / static_cast<float>(Patch.MaxQuantity),
                0.0f,
                1.0f)
            : (Patch.CurrentQuantity > 0 ? 1.0f : 0.0f);
        const float VisibleDensity = FMath::Clamp(
            Patch.VisualDensity,
            0.0f,
            1.0f)
            * FMath::Sqrt(Quantity01);
        const int32 PatchInstances = Quantity01 <= KINDA_SMALL_NUMBER
            ? 0
            : FMath::Clamp(
                FMath::RoundToInt(VisibleDensity * 6.0f) + 1,
                1,
                8);
        uint32 PatchState = Patch.VisualSeed != 0
            ? PresentationSeed(Patch.VisualSeed)
            : MixHash(State, static_cast<uint32>(
                static_cast<uint64>(Patch.ResourceNodeId) & 0xFFFFFFFFu));
        for (int32 Index = 0; Index < PatchInstances && *PlacedCounter < MaxTotal; ++Index)
        {
            const float SpreadX = (HashUnit(PatchState) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            const float SpreadY = (HashUnit(PatchState) * 2.0f - 1.0f) * LLWorldSpatialContract::GridCellSizeUU;
            float Scale = FMath::Lerp(MinScale, MaxScale, HashUnit(PatchState));
            Scale *= FMath::Lerp(0.62f, 1.0f, FMath::Sqrt(Quantity01));
            const float Yaw = HashUnit(PatchState) * 360.0f;
            const int32 Slot = static_cast<int32>(HashUnit(PatchState) * Target->Num()) % Target->Num();
            const FVector2D PatchLocation(PatchX + SpreadX, PatchY + SpreadY);
            Scale *= ResourcePatchScaleFactor(PatchLocation);
            if (UHierarchicalInstancedStaticMeshComponent* Component = (*Target)[Slot])
            {
                Component->AddInstance(FTransform(
                    FRotator(0.0f, Yaw, 0.0f),
                    FVector(
                        PatchLocation.X,
                        PatchLocation.Y,
                        TerrainSurfaceZUU(World, Terrain, PatchLocation)),
                    FVector(Scale, Scale, Scale)));
                ++(*PlacedCounter);
            }
        }
    }
}

uint32 ALLWorldPresentationActor::ResourceQuantitySignature(
    const FLLCoreCivilizationWorldObservation& Civilization) const
{
    uint32 Hash = 0x52455331u;
    Hash = MixHash(Hash, static_cast<uint32>(Civilization.ResourceNodeCount));
    for (const FLLCoreCivilizationResourceObservation& Resource : Civilization.Resources)
    {
        const uint64 Id = static_cast<uint64>(Resource.ResourceNodeId);
        Hash = MixHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Resource.Quantity)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Resource.MaxQuantity)));
    }
    return Hash;
}

uint32 ALLWorldPresentationActor::FacilityLayoutSignature(
    const FLLCoreCivilizationWorldObservation& Civilization) const
{
    uint32 Hash = 0x4C41594Fu;
    Hash = MixHash(Hash, static_cast<uint32>(Civilization.FacilityCount));
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 Id = static_cast<uint64>(Facility.FacilityId);
        Hash = MixHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.Kind));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridX));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridY));
    }
    return Hash;
}

void ALLWorldPresentationActor::RefreshFacilityReadabilityReferences(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreCivilizationWorldObservation& Civilization)
{
    CachedFacilityReadabilityCentersUU.Reset();
    CachedFacilityReadabilityCentersUU.Reserve(Civilization.Facilities.Num());

    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const float X = static_cast<float>(
            Facility.GridX - World.InitialCenterGridX)
            * LLWorldSpatialContract::GridCellSizeUU;
        const float Y = static_cast<float>(
            Facility.GridY - World.InitialCenterGridY)
            * LLWorldSpatialContract::GridCellSizeUU;
        CachedFacilityReadabilityCentersUU.Add(FVector2D(X, Y));
    }
}

uint32 ALLWorldPresentationActor::FacilitySignature(const FLLCoreCivilizationWorldObservation& Civilization) const
{
    uint32 Hash = 0x46414331u;
    Hash = MixHash(Hash, static_cast<uint32>(Civilization.FacilityCount));
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const uint64 Id = static_cast<uint64>(Facility.FacilityId);
        Hash = MixHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.Kind));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.State));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridX));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.GridY));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::RoundToInt(Facility.WorkProgress * 1000.0f)));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.RequiredMaterialUnits));
        Hash = MixHash(Hash, static_cast<uint32>(Facility.DeliveredMaterialUnits));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::RoundToInt(FMath::Clamp(Facility.Durability, 0.0f, 1.0f) * 1000.0f)));
        Hash = MixHash(Hash, Facility.bActive ? 1u : 0u);
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Facility.FuelUnits)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Facility.CharcoalUnits)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Facility.OreUnits)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Facility.MetalUnits)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::RoundToInt(FMath::Max(0.0f, Facility.HeatLevel) * 100.0f)));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Facility.BurnMinutesRemaining)));
        Hash = MixHash(Hash, Facility.bLit ? 1u : 0u);
    }

    // Primitive storage visuals depend on linked authoritative storage totals.
    // Include storage truth in the presentation signature so adding/removing
    // inventory refreshes cargo without touching Core state.
    for (const FLLCoreCivilizationStorageObservation& Storage : Civilization.Storages)
    {
        const uint64 Id = static_cast<uint64>(Storage.StorageId);
        Hash = MixHash(Hash, static_cast<uint32>(Id & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>((Id >> 32) & 0xFFFFFFFFu));
        Hash = MixHash(Hash, static_cast<uint32>(FMath::Max(0, Storage.TotalUnits)));
    }
    return Hash;
}

void ALLWorldPresentationActor::BuildFacilities(
    const FLLCoreWorldGenerationObservation& World,
    const FLLCoreCivilizationWorldObservation& Civilization,
    bool bNightPresentation)
{
    for (const FLLCoreCivilizationFacilityObservation& Facility : Civilization.Facilities)
    {
        const float X = static_cast<float>(Facility.GridX - World.InitialCenterGridX) * LLWorldSpatialContract::GridCellSizeUU;
        const float Y = static_cast<float>(Facility.GridY - World.InitialCenterGridY) * LLWorldSpatialContract::GridCellSizeUU;
        const FVector Base(X, Y, 0.0f);
        const float MaterialProgress = Facility.RequiredMaterialUnits > 0
            ? FMath::Clamp(static_cast<float>(Facility.DeliveredMaterialUnits) / static_cast<float>(Facility.RequiredMaterialUnits), 0.0f, 1.0f)
            : 0.0f;
        const float WorkProgress = FMath::Clamp(Facility.WorkProgress, 0.0f, 1.0f);
        // Structural completion and runtime activity are different truths.
        // An Operational facility stays fully built even while temporarily
        // inactive; activity only affects the work/fire presentation.
        const bool bStructurallyComplete = Facility.State == ELLCoreFacilityState::Operational;
        const bool bPlanned = Facility.State == ELLCoreFacilityState::Planned;
        const bool bRuined = Facility.State == ELLCoreFacilityState::Ruined;
        const float Durability = FMath::Clamp(Facility.Durability, 0.0f, 1.0f);
        const float BuildProgress = bStructurallyComplete ? 1.0f : FMath::Max(MaterialProgress, WorkProgress);

        // Ruins stay visible as low, scattered debris instead of disappearing.
        // This is deliberately generic: Core owns the Ruined state; Presentation
        // only projects that state and never decides whether a facility failed.
        if (bRuined)
        {
            if (FacilityFoundationInstances)
            {
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator(0.0f, 18.0f, 8.0f),
                    Base + FVector(-42.0f, -18.0f, 9.0f),
                    FVector(0.82f, 0.42f, 0.12f)));
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator(0.0f, -27.0f, -6.0f),
                    Base + FVector(48.0f, 24.0f, 7.0f),
                    FVector(0.68f, 0.34f, 0.10f)));
            }
            if (FacilityPostInstances)
            {
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator(0.0f, 36.0f, 78.0f),
                    Base + FVector(12.0f, -34.0f, 18.0f),
                    FVector(0.12f, 0.12f, 0.92f)));
            }
            if (FacilityCargoInstances)
            {
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(11.0f, 62.0f, 5.0f),
                    Base + FVector(-64.0f, 52.0f, 13.0f),
                    FVector(0.46f, 0.22f, 0.16f)));
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(-8.0f, 121.0f, -4.0f),
                    Base + FVector(58.0f, -54.0f, 11.0f),
                    FVector(0.40f, 0.20f, 0.14f)));
            }
            continue;
        }

        // Operational damage should be legible before the facility becomes a
        // full ruin. Project a small amount of loose debris from authoritative
        // durability without changing collision, navigation or Core state.
        if (bStructurallyComplete && Durability < 0.78f && FacilityCargoInstances)
        {
            const float Damage01 = 1.0f - Durability;
            const int32 DebrisCount = FMath::Clamp(
                FMath::CeilToInt(Damage01 * 4.0f),
                1,
                3);
            const float FacilityPhase = static_cast<float>(
                FMath::Abs(static_cast<int64>(Facility.FacilityId)) % 360);
            for (int32 Index = 0; Index < DebrisCount; ++Index)
            {
                const float AngleDegrees = FMath::Fmod(
                    FacilityPhase + 121.0f * static_cast<float>(Index),
                    360.0f);
                const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
                const float Radius = 82.0f + 18.0f * static_cast<float>(Index);
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(
                        5.0f + 4.0f * static_cast<float>(Index),
                        AngleDegrees,
                        (Index % 2 == 0) ? 8.0f : -7.0f),
                    Base + FVector(
                        FMath::Cos(AngleRadians) * Radius,
                        FMath::Sin(AngleRadians) * Radius,
                        10.0f + 3.0f * static_cast<float>(Index)),
                    FVector(
                        FMath::Lerp(0.22f, 0.42f, Damage01),
                        0.15f + 0.03f * static_cast<float>(Index),
                        0.10f + 0.02f * static_cast<float>(Index))));
            }
        }

        // Planned facilities should read as a staked-out work site rather than
        // a miniature finished building. Core owns the Planned state and grid
        // location; Presentation only projects that truth into a low footprint.
        if (bPlanned && FacilityPostInstances)
        {
            FVector2D HalfExtent(76.0f, 52.0f);
            switch (Facility.Kind)
            {
                case ELLCoreFacilityKind::PrimitiveStorage:
                    HalfExtent = FVector2D(92.0f, 64.0f);
                    break;
                case ELLCoreFacilityKind::WorkSurface:
                    HalfExtent = FVector2D(72.0f, 40.0f);
                    break;
                case ELLCoreFacilityKind::SleepingPlace:
                    HalfExtent = FVector2D(80.0f, 44.0f);
                    break;
                case ELLCoreFacilityKind::Shelter:
                    HalfExtent = FVector2D(98.0f, 76.0f);
                    break;
                case ELLCoreFacilityKind::Furnace:
                    HalfExtent = FVector2D(66.0f, 58.0f);
                    break;
                case ELLCoreFacilityKind::FirePit:
                    HalfExtent = FVector2D(54.0f, 54.0f);
                    break;
                default:
                    break;
            }

            const FVector2D Stakes[4] = {
                FVector2D(-HalfExtent.X, -HalfExtent.Y),
                FVector2D(HalfExtent.X, -HalfExtent.Y),
                FVector2D(-HalfExtent.X, HalfExtent.Y),
                FVector2D(HalfExtent.X, HalfExtent.Y)
            };
            for (int32 StakeIndex = 0; StakeIndex < 4; ++StakeIndex)
            {
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(
                        Stakes[StakeIndex].X,
                        Stakes[StakeIndex].Y,
                        16.0f),
                    FVector(0.08f, 0.08f, 0.30f)));
            }
        }

        if (Facility.Kind == ELLCoreFacilityKind::PrimitiveStorage)
        {
            if (bStructurallyComplete && PhotorealStructureLogInstances)
            {
                AddPhotorealStructureLog(Base + FVector(0.0f, -60.0f, 12.0f), FVector::ForwardVector, 210.0f, 18.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,  60.0f, 12.0f), FVector::ForwardVector, 210.0f, 18.0f);
                AddPhotorealStructureLog(Base + FVector(-90.0f, 0.0f, 12.0f), FVector::RightVector, 140.0f, 18.0f);
                AddPhotorealStructureLog(Base + FVector( 90.0f, 0.0f, 12.0f), FVector::RightVector, 140.0f, 18.0f);
                const FVector2D CompletePostOffsets[4] = {
                    FVector2D(-90.0f, -60.0f), FVector2D(90.0f, -60.0f),
                    FVector2D(-90.0f,  60.0f), FVector2D(90.0f,  60.0f)};
                for (const FVector2D& Offset : CompletePostOffsets)
                {
                    AddPhotorealStructureLog(Base + FVector(Offset.X, Offset.Y, 62.0f), FVector::UpVector, 112.0f, 18.0f);
                }
                AddPhotorealStructureLog(Base + FVector(0.0f, -60.0f, 118.0f), FVector::ForwardVector, 210.0f, 16.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,  60.0f, 118.0f), FVector::ForwardVector, 210.0f, 16.0f);
                AddPhotorealStructureLog(Base + FVector(-90.0f, 0.0f, 118.0f), FVector::RightVector, 140.0f, 16.0f);
                AddPhotorealStructureLog(Base + FVector( 90.0f, 0.0f, 118.0f), FVector::RightVector, 140.0f, 16.0f);
            }

            int32 LinkedStoredUnits = 0;
            if (Facility.LinkedStorageId != 0)
            {
                for (const FLLCoreCivilizationStorageObservation& Storage : Civilization.Storages)
                {
                    if (Storage.StorageId == Facility.LinkedStorageId)
                    {
                        LinkedStoredUnits = FMath::Max(0, Storage.TotalUnits);
                        break;
                    }
                }
            }

            if (!bStructurallyComplete && FacilityFoundationInstances)
            {
                const float PlannedScale = Facility.State == ELLCoreFacilityState::Planned ? 0.72f : 1.0f;
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(0.0f, 0.0f, 6.0f),FVector(2.2f * PlannedScale, 1.6f * PlannedScale, 0.12f)));
            }
            const FVector2D PostOffsets[4] = {
                FVector2D(-90.0f, -60.0f), FVector2D(90.0f, -60.0f),
                FVector2D(-90.0f, 60.0f), FVector2D(90.0f, 60.0f)};
            const int32 PostCount = bStructurallyComplete ? 0 : FMath::Clamp(FMath::CeilToInt(BuildProgress * 4.0f), 0, 4);
            for (int32 Index = 0; Index < PostCount; ++Index)
            {
                if (!FacilityPostInstances) { break; }
                const float HeightFactor = bStructurallyComplete ? 1.0f : FMath::Clamp(0.35f + 0.65f * BuildProgress, 0.35f, 1.0f);
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(PostOffsets[Index].X, PostOffsets[Index].Y, 55.0f * HeightFactor),
                    FVector(0.14f, 0.14f, 1.1f * HeightFactor)));
            }
            // Construction still shows delivered build material. Once the
            // storage is operational, approved CC0 baskets replace fake cargo
            // cubes. If that approved art is unavailable, do not invent extra
            // completed-storage cargo visuals.
            if (bStructurallyComplete && PhotorealStorageBasketInstances)
            {
                const int32 BasketCount = FMath::Clamp(
                    FMath::CeilToInt(static_cast<float>(LinkedStoredUnits) / 2.0f),
                    0,
                    4);
                for (int32 Index = 0; Index < BasketCount; ++Index)
                {
                    const int32 Column = Index % 2;
                    const int32 Row = Index / 2;
                    PhotorealStorageBasketInstances->AddInstance(FTransform(
                        FRotator(0.0f, 18.0f + 71.0f * static_cast<float>(Index), 0.0f),
                        Base + FVector(
                            -52.0f + Column * 104.0f,
                            -30.0f + Row * 66.0f,
                            0.0f),
                        FVector(0.92f)));
                }
            }

            const int32 CargoCount = bStructurallyComplete
                ? 0
                : FMath::Clamp(FMath::CeilToInt(MaterialProgress * 6.0f), 0, 6);
            for (int32 Index = 0; Index < CargoCount; ++Index)
            {
                if (!FacilityCargoInstances) { break; }
                const int32 Column = Index % 4;
                const int32 Row = Index / 4;
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(0.0f, (Index % 2 == 0) ? 0.0f : 90.0f, 0.0f),
                    Base + FVector(
                        -78.0f + Column * 52.0f,
                        -26.0f + Row * 52.0f,
                        24.0f),
                    FVector(0.45f, 0.28f, 0.25f)));
            }
            if (!bStructurallyComplete && WorkProgress >= 0.65f && FacilityRoofInstances)
            {
                const float RoofScale = bStructurallyComplete ? 1.0f : FMath::Clamp((WorkProgress - 0.65f) / 0.35f, 0.25f, 1.0f);
                FacilityRoofInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(0.0f, 0.0f, 118.0f),FVector(2.15f * RoofScale, 1.55f, 0.12f)));
            }
            continue;
        }

        if (Facility.Kind == ELLCoreFacilityKind::WorkSurface)
        {
            if (bStructurallyComplete && PhotorealStructureLogInstances)
            {
                const FVector2D CompleteLegOffsets[4] = {
                    FVector2D(-68.0f, -32.0f), FVector2D(68.0f, -32.0f),
                    FVector2D(-68.0f,  32.0f), FVector2D(68.0f,  32.0f)};
                for (const FVector2D& Offset : CompleteLegOffsets)
                {
                    AddPhotorealStructureLog(Base + FVector(Offset.X, Offset.Y, 36.0f), FVector::UpVector, 64.0f, 14.0f);
                }
                AddPhotorealStructureLog(Base + FVector(0.0f, -30.0f, 72.0f), FVector::ForwardVector, 172.0f, 16.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,   0.0f, 74.0f), FVector::ForwardVector, 172.0f, 16.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,  30.0f, 72.0f), FVector::ForwardVector, 172.0f, 16.0f);
            }

            // Construction visibly grows from delivered
            // materials to legs to a usable top. Durability subtly sags the top.
            const float Integrity = FMath::Lerp(0.72f, 1.0f, Durability);
            const float DamageTilt = (1.0f - Durability) * 11.0f;

            if (!bStructurallyComplete && FacilityFoundationInstances)
            {
                const float FootprintScale = Facility.State == ELLCoreFacilityState::Planned ? 0.72f : 1.0f;
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(0.0f, 0.0f, 5.0f),
                    FVector(1.75f * FootprintScale, 0.95f * FootprintScale, 0.08f)));
            }

            const FVector2D LegOffsets[4] = {
                FVector2D(-68.0f, -32.0f), FVector2D(68.0f, -32.0f),
                FVector2D(-68.0f, 32.0f), FVector2D(68.0f, 32.0f)};
            const int32 LegCount = bStructurallyComplete
                ? 0
                : FMath::Clamp(FMath::CeilToInt(BuildProgress * 4.0f), 0, 4);
            for (int32 Index = 0; Index < LegCount; ++Index)
            {
                if (!FacilityPostInstances) { break; }
                const float Height = 0.62f * Integrity;
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator(0.0f, 0.0f, (Index >= 2 ? 1.0f : -1.0f) * DamageTilt * 0.25f),
                    Base + FVector(LegOffsets[Index].X, LegOffsets[Index].Y, 34.0f * Integrity),
                    FVector(0.12f, 0.12f, Height)));
            }

            if (!bStructurallyComplete && WorkProgress >= 0.45f && FacilityRoofInstances)
            {
                const float TopScale = bStructurallyComplete
                    ? 1.0f
                    : FMath::Clamp((WorkProgress - 0.45f) / 0.55f, 0.25f, 1.0f);
                FacilityRoofInstances->AddInstance(FTransform(
                    FRotator(0.0f, 0.0f, DamageTilt),
                    Base + FVector(0.0f, 0.0f, 72.0f * Integrity),
                    FVector(1.65f * TopScale, 0.82f, 0.12f * Integrity)));
            }

            if (bStructurallyComplete && PhotorealWorkToolInstances)
            {
                PhotorealWorkToolInstances->AddInstance(FTransform(
                    FRotator(0.0f, 18.0f, 82.0f),
                    Base + FVector(18.0f, 0.0f, 84.0f * Integrity),
                    FVector(0.92f)));
            }

            const int32 LooseMaterialCount = bStructurallyComplete
                ? 2
                : FMath::Clamp(FMath::CeilToInt(MaterialProgress * 3.0f), 0, 3);
            for (int32 Index = 0; Index < LooseMaterialCount; ++Index)
            {
                if (!FacilityCargoInstances) { break; }
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(0.0f, 18.0f + Index * 43.0f, 0.0f),
                    Base + FVector(-44.0f + Index * 42.0f, 58.0f, 17.0f),
                    FVector(0.40f, 0.14f, 0.12f)));
            }
            continue;
        }

        if (Facility.Kind == ELLCoreFacilityKind::SleepingPlace)
        {
            // Primitive bedding: a raised frame plus layered fiber bed. The bed
            // remains visually low-tech and is clearly distinct from a WorkSurface.
            const float Integrity = FMath::Lerp(0.70f, 1.0f, Durability);
            const float DamageTilt = (1.0f - Durability) * 8.0f;

            if (FacilityFoundationInstances)
            {
                const float FrameScale = Facility.State == ELLCoreFacilityState::Planned ? 0.70f : 1.0f;
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(0.0f, 0.0f, 9.0f),
                    FVector(1.85f * FrameScale, 0.92f * FrameScale, 0.12f)));
            }

            const int32 RailCount = bStructurallyComplete
                ? 2
                : FMath::Clamp(FMath::CeilToInt(BuildProgress * 2.0f), 0, 2);
            for (int32 Index = 0; Index < RailCount; ++Index)
            {
                if (!FacilityPostInstances) { break; }
                const float Side = Index == 0 ? -1.0f : 1.0f;
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator(0.0f, 0.0f, Side * DamageTilt * 0.35f),
                    Base + FVector(0.0f, Side * 42.0f, 24.0f),
                    FVector(1.72f, 0.11f, 0.16f * Integrity)));
            }

            if ((bStructurallyComplete || MaterialProgress >= 0.55f) && FacilityCargoInstances)
            {
                const float BeddingScale = bStructurallyComplete
                    ? 1.0f
                    : FMath::Clamp((MaterialProgress - 0.55f) / 0.45f, 0.30f, 1.0f);
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(0.0f, 0.0f, DamageTilt),
                    Base + FVector(0.0f, 0.0f, 31.0f * Integrity),
                    FVector(1.58f * BeddingScale, 0.72f, 0.16f)));
                if (bStructurallyComplete)
                {
                    FacilityCargoInstances->AddInstance(FTransform(
                        FRotator(0.0f, 0.0f, DamageTilt * 0.65f),
                        Base + FVector(-58.0f, 0.0f, 47.0f * Integrity),
                        FVector(0.42f, 0.64f, 0.12f)));
                }
            }
            continue;
        }

        if (Facility.Kind == ELLCoreFacilityKind::Shelter)
        {
            if (bStructurallyComplete && PhotorealStructureLogInstances)
            {
                AddPhotorealStructureLog(Base + FVector(0.0f, -70.0f, 12.0f), FVector::ForwardVector, 220.0f, 20.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,  70.0f, 12.0f), FVector::ForwardVector, 220.0f, 20.0f);
                AddPhotorealStructureLog(Base + FVector(-92.0f, 0.0f, 12.0f), FVector::RightVector, 160.0f, 20.0f);
                AddPhotorealStructureLog(Base + FVector( 92.0f, 0.0f, 12.0f), FVector::RightVector, 160.0f, 20.0f);
                const FVector2D CompleteShelterPosts[4] = {
                    FVector2D(-92.0f, -70.0f), FVector2D(92.0f, -70.0f),
                    FVector2D(-92.0f,  70.0f), FVector2D(92.0f,  70.0f)};
                for (const FVector2D& Offset : CompleteShelterPosts)
                {
                    AddPhotorealStructureLog(Base + FVector(Offset.X, Offset.Y, 84.0f), FVector::UpVector, 150.0f, 20.0f);
                }
                AddPhotorealStructureLog(Base + FVector(0.0f, -70.0f, 156.0f), FVector::ForwardVector, 220.0f, 18.0f);
                AddPhotorealStructureLog(Base + FVector(0.0f,  70.0f, 156.0f), FVector::ForwardVector, 220.0f, 18.0f);
                for (int32 RafterIndex = -2; RafterIndex <= 2; ++RafterIndex)
                {
                    AddPhotorealStructureLog(
                        Base + FVector(static_cast<float>(RafterIndex) * 44.0f, 0.0f, 160.0f),
                        FVector::RightVector,
                        160.0f,
                        15.0f);
                }
            }

            // Partial projects show posts first,
            // then roof coverage. Wear is projected as roof sag/tilt.
            const float Integrity = FMath::Lerp(0.68f, 1.0f, Durability);
            const float DamageTilt = (1.0f - Durability) * 13.0f;

            if (!bStructurallyComplete && FacilityFoundationInstances)
            {
                const float PlannedScale = Facility.State == ELLCoreFacilityState::Planned ? 0.72f : 1.0f;
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(0.0f, 0.0f, 5.0f),
                    FVector(2.35f * PlannedScale, 1.85f * PlannedScale, 0.08f)));
            }

            const FVector2D PostOffsets[4] = {
                FVector2D(-92.0f, -70.0f), FVector2D(92.0f, -70.0f),
                FVector2D(-92.0f, 70.0f), FVector2D(92.0f, 70.0f)};
            const int32 PostCount = bStructurallyComplete
                ? 0
                : FMath::Clamp(FMath::CeilToInt(BuildProgress * 4.0f), 0, 4);
            for (int32 Index = 0; Index < PostCount; ++Index)
            {
                if (!FacilityPostInstances) { break; }
                const float LeanDirection = (Index % 2 == 0) ? -1.0f : 1.0f;
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator(LeanDirection * DamageTilt * 0.25f, 0.0f, LeanDirection * DamageTilt * 0.15f),
                    Base + FVector(PostOffsets[Index].X, PostOffsets[Index].Y, 78.0f * Integrity),
                    FVector(0.14f, 0.14f, 1.48f * Integrity)));
            }

            if (!bStructurallyComplete && WorkProgress >= 0.58f && FacilityRoofInstances)
            {
                const float RoofProgress = bStructurallyComplete
                    ? 1.0f
                    : FMath::Clamp((WorkProgress - 0.58f) / 0.42f, 0.25f, 1.0f);
                FacilityRoofInstances->AddInstance(FTransform(
                    FRotator(0.0f, 0.0f, DamageTilt),
                    Base + FVector(0.0f, 0.0f, 156.0f * Integrity),
                    FVector(2.25f * RoofProgress, 1.75f, 0.13f * Integrity)));
            }

            if (!bStructurallyComplete && MaterialProgress >= 0.70f && FacilityCargoInstances)
            {
                const float WallProgress = bStructurallyComplete
                    ? 1.0f
                    : FMath::Clamp((MaterialProgress - 0.70f) / 0.30f, 0.25f, 1.0f);
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(0.0f, 74.0f, 76.0f * Integrity),
                    FVector(2.02f * WallProgress, 0.10f, 1.20f * Integrity)));
            }
            continue;
        }

        if (Facility.Kind == ELLCoreFacilityKind::Furnace)
        {
            // Android-safe visual-only furnace. Chamber/charge/output are derived
            // solely from the authoritative facility read DTO.
            const float Structure = bStructurallyComplete ? 1.0f : FMath::Clamp(BuildProgress, 0.0f, 1.0f);
            if (FacilityFoundationInstances && Structure > 0.0f)
            {
                FacilityFoundationInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(0.0f, 0.0f, 8.0f),FVector(1.55f * Structure, 1.35f * Structure, 0.16f)));
            }
            const FVector2D WallOffsets[3] = { FVector2D(-58.0f,0.0f), FVector2D(58.0f,0.0f), FVector2D(0.0f,52.0f) };
            const int32 WallCount = bStructurallyComplete ? 3 : FMath::Clamp(FMath::CeilToInt(Structure * 3.0f),0,3);
            for (int32 Index=0; Index<WallCount; ++Index)
            {
                if (!FacilityPostInstances) { break; }
                const bool bSide = Index < 2;
                FacilityPostInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(WallOffsets[Index].X, WallOffsets[Index].Y, 62.0f),
                    bSide ? FVector(0.22f,1.15f,1.05f) : FVector(1.15f,0.22f,1.05f)));
            }
            if ((bStructurallyComplete || WorkProgress>=0.70f) && FacilityRoofInstances)
            {
                FacilityRoofInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(0.0f,0.0f,126.0f),FVector(1.30f,1.10f,0.16f)));
                FacilityRoofInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,Base + FVector(42.0f,28.0f,184.0f),FVector(0.34f,0.34f,0.95f)));
            }

            const int32 OreCount = FMath::Clamp(Facility.OreUnits,0,3);
            for (int32 Index=0; Index<OreCount; ++Index)
            {
                if (!FacilityCargoInstances) { break; }
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(0.0f,static_cast<float>(Index)*31.0f,0.0f),
                    Base + FVector(-24.0f+Index*24.0f,-28.0f,28.0f),FVector(0.26f,0.22f,0.18f)));
            }
            if (!Facility.bLit)
            {
                const int32 MetalCount = FMath::Clamp(Facility.MetalUnits,0,3);
                for (int32 Index=0; Index<MetalCount; ++Index)
                {
                    if (!FacilityCargoInstances) { break; }
                    FacilityCargoInstances->AddInstance(FTransform(
                        FRotator(0.0f,0.0f,0.0f),
                        Base + FVector(-36.0f+Index*36.0f,-72.0f,18.0f),FVector(0.30f,0.16f,0.10f)));
                }
            }
            if (Facility.bLit && FacilityAccentInstances)
            {
                const float FlameHeight = FMath::Lerp(0.45f,0.85f,FMath::Clamp(Facility.HeatLevel,0.0f,1.0f));
                FacilityAccentInstances->AddInstance(FTransform(
                    FRotator(0.0f,45.0f,0.0f),Base + FVector(0.0f,-34.0f,52.0f),FVector(0.20f,0.16f,FlameHeight)));
                FacilityAccentInstances->AddInstance(FTransform(
                    FRotator(0.0f,135.0f,0.0f),Base + FVector(8.0f,-32.0f,46.0f),FVector(0.14f,0.13f,FlameHeight*0.72f)));

                if (bNightPresentation)
                {
                    // Android-safe emissive pool: no dynamic lights or shadows,
                    // just a low, collision-free accent that keeps an active
                    // furnace readable against the dark ground.
                    FacilityAccentInstances->AddInstance(FTransform(
                        FRotator::ZeroRotator,
                        Base + FVector(0.0f, -22.0f, 10.0f),
                        FVector(0.95f, 0.70f, 0.025f)));
                    FacilityAccentInstances->AddInstance(FTransform(
                        FRotator(0.0f, 45.0f, 0.0f),
                        Base + FVector(0.0f, -22.0f, 12.0f),
                        FVector(0.62f, 0.62f, 0.020f)));
                }
            }
            continue;
        }

        if (Facility.Kind != ELLCoreFacilityKind::FirePit) { continue; }

        if (bStructurallyComplete && PhotorealFirePitInstances)
        {
            PhotorealFirePitInstances->AddInstance(FTransform(
                FRotator::ZeroRotator,
                Base,
                FVector(1.0f)));
        }

        // Construction may use reviewed lightweight progress pieces, but an
        // operational production firepit never falls back to the old Engine
        // cube ring if the approved photoreal hero asset is unavailable.
        const int32 StoneCount = bStructurallyComplete
            ? 0
            : FMath::Clamp(FMath::CeilToInt(BuildProgress * 8.0f), 0, 8);
        for (int32 Index = 0; Index < StoneCount; ++Index)
        {
            if (!FacilityFoundationInstances) { break; }
            const float AngleDegrees = static_cast<float>(Index) * 45.0f;
            const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
            const FVector Offset(FMath::Cos(AngleRadians) * 68.0f,FMath::Sin(AngleRadians) * 68.0f,13.0f);
            FacilityFoundationInstances->AddInstance(FTransform(
                FRotator(0.0f, AngleDegrees, 0.0f),Base + Offset,FVector(0.48f, 0.28f, 0.20f)));
        }
        // No completed Engine-cube hearth fallback: the imported CC0 asset
        // above is the only hero structure for an operational firepit.
        const int32 LogCount = bStructurallyComplete ? FMath::Clamp(Facility.FuelUnits, 0, 3)
            : FMath::Clamp(FMath::CeilToInt(MaterialProgress * 2.0f), 0, 2);
        for (int32 Index = 0; Index < LogCount; ++Index)
        {
            if (!FacilityCargoInstances) { break; }
            FacilityCargoInstances->AddInstance(FTransform(
                FRotator(0.0f, Index % 2 == 0 ? 45.0f : 135.0f, 0.0f),
                Base + FVector(0.0f, 0.0f, 24.0f + Index * 7.0f),FVector(0.85f, 0.16f, 0.14f)));
        }
        if (!Facility.bLit)
        {
            const int32 CharcoalCount = FMath::Clamp(Facility.CharcoalUnits, 0, 4);
            for (int32 Index = 0; Index < CharcoalCount; ++Index)
            {
                if (!FacilityCargoInstances) { break; }
                const float OffsetX = (Index % 2 == 0) ? -20.0f : 20.0f;
                const float OffsetY = (Index < 2) ? -14.0f : 14.0f;
                FacilityCargoInstances->AddInstance(FTransform(
                    FRotator(0.0f, static_cast<float>(Index) * 37.0f, 0.0f),
                    Base + FVector(OffsetX, OffsetY, 18.0f),FVector(0.24f, 0.20f, 0.12f)));
            }
        }
        if (Facility.bLit && FacilityAccentInstances)
        {
            FacilityAccentInstances->AddInstance(FTransform(
                FRotator(0.0f, 45.0f, 0.0f),Base + FVector(0.0f, 0.0f, 55.0f),FVector(0.24f, 0.18f, 0.70f)));
            FacilityAccentInstances->AddInstance(FTransform(
                FRotator(0.0f, 135.0f, 0.0f),Base + FVector(0.0f, 0.0f, 48.0f),FVector(0.18f, 0.16f, 0.50f)));

            if (bNightPresentation)
            {
                FacilityAccentInstances->AddInstance(FTransform(
                    FRotator::ZeroRotator,
                    Base + FVector(0.0f, 0.0f, 9.0f),
                    FVector(0.88f, 0.88f, 0.022f)));
                FacilityAccentInstances->AddInstance(FTransform(
                    FRotator(0.0f, 45.0f, 0.0f),
                    Base + FVector(0.0f, 0.0f, 11.0f),
                    FVector(0.56f, 0.56f, 0.018f)));
            }
        }
    }
}

void ALLWorldPresentationActor::RefreshFromCore(bool bForce)
{
    const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    ULLCoreBridgeSubsystem* Bridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Bridge) { return; }

    const FLLCoreWorldGenerationObservation World = Bridge->GetWorldGenerationObservation();
    if (!World.bAvailable || !World.bHasInitialStartRegion) { return; }
    const FLLCoreCivilizationWorldObservation Civilization = Bridge->GetCivilizationWorldObservation(0);
    const FLLCoreTimeObservation Time = Bridge->GetTimeObservation();
    const bool bNightPresentation = Time.bIsNight || Time.Daylight01 < 0.22f;

    // The observer camera is spawned by the game mode, which can run after this
    // actor's BeginPlay. The first build may therefore miss it; the next refresh
    // picks it up and rebuilds the dressing once.
    const bool bSightlinePending = !bDynamicObserverCanopyVisibility
        && bClearInitialSightlineCanopy && !bInitialViewCaptured;
    CaptureInitialViewOrigin();

    const uint32 CurrentFacilityLayoutSignature =
        FacilityLayoutSignature(Civilization);
    const bool bFacilityLayoutChanged =
        bForce || CurrentFacilityLayoutSignature != BuiltFacilityLayoutSignature;
    if (bFacilityLayoutChanged)
    {
        BuiltFacilityLayoutSignature = CurrentFacilityLayoutSignature;
        RefreshFacilityReadabilityReferences(World, Civilization);
    }

    const uint32 CurrentResourceQuantitySignature =
        ResourceQuantitySignature(Civilization);
    const bool bResourceQuantityChanged =
        bForce
        || CurrentResourceQuantitySignature != BuiltResourceQuantitySignature;

    const bool bNaturalChanged = bForce
        || World.WorldSeed != BuiltWorldSeed
        || World.GenerationVersion != BuiltGenerationVersion
        || World.MaterializedChunkCount != BuiltChunkCount
        || bFacilityLayoutChanged
        || bResourceQuantityChanged
        || (bSightlinePending && bInitialViewCaptured);

    uint32 CurrentFacilitySignature = FacilitySignature(Civilization);
    // Rebuild facility accent instances only when the coarse day/night state
    // changes; minute-by-minute time does not churn the HISM presentation.
    CurrentFacilitySignature = MixHash(
        CurrentFacilitySignature,
        bNightPresentation ? 0x4E494748u : 0x44415900u);
    const bool bFacilitiesChanged = bForce || !bBuiltFacilityPresentation || CurrentFacilitySignature != BuiltFacilitySignature;
    if (!bNaturalChanged && !bFacilitiesChanged) { return; }

    if (bNaturalChanged)
    {
        // Measured from the Core start-region centre, not the world origin.
        CachedSettlementReferenceUU = SettlementReferenceUU(World);
        BuiltWorldSeed = World.WorldSeed;
        BuiltGenerationVersion = World.GenerationVersion;
        BuiltChunkCount = World.MaterializedChunkCount;
        BuiltResourceQuantitySignature = CurrentResourceQuantitySignature;
        ClearInstances();
        BuildGround(World);

        const TArray<FLLCoreNaturalChunkObservation> MaterializedChunks =
            Bridge->GetMaterializedNaturalChunkObservations();
        for (const FLLCoreNaturalChunkObservation& Chunk : MaterializedChunks)
        {
            if (Chunk.bMaterialized)
            {
                FLLCoreTerrainPresentationObservation Terrain;
                if (!Bridge->GetTerrainPresentationObservation(
                    Chunk.ChunkX,
                    Chunk.ChunkY,
                    Terrain))
                {
                    Terrain.bAvailable = false;
                    Terrain.ChunkX = Chunk.ChunkX;
                    Terrain.ChunkY = Chunk.ChunkY;
                    Terrain.CenterElevation01 = Chunk.Elevation;
                }
                BuildChunkGround(World, Chunk, Terrain);
                BuildChunkDressing(World, Chunk, Terrain);
            }
        }
    }

    if (bFacilitiesChanged)
    {
        BuiltFacilitySignature = CurrentFacilitySignature;
        bBuiltFacilityPresentation = true;
        ClearFacilityInstances();
        BuildFacilities(World, Civilization, bNightPresentation);
    }

    const int32 GroundTileCount =
        (GroundGrassTileInstances ? GroundGrassTileInstances->GetInstanceCount() : 0)
        + (GroundDryTileInstances ? GroundDryTileInstances->GetInstanceCount() : 0)
        + (GroundTransitionTileInstances ? GroundTransitionTileInstances->GetInstanceCount() : 0);

    int32 TreeInstanceCount = 0;
    int32 ShrubInstanceCount = 0;
    int32 GrassInstanceCount = 0;
    int32 RockInstanceCount = 0;
    for (UHierarchicalInstancedStaticMeshComponent* Component : TreeInstances) { if (Component) { TreeInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : ShrubInstances) { if (Component) { ShrubInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : GrassInstances) { if (Component) { GrassInstanceCount += Component->GetInstanceCount(); } }
    for (UHierarchicalInstancedStaticMeshComponent* Component : RockInstances) { if (Component) { RockInstanceCount += Component->GetInstanceCount(); } }

    const int32 FacilityInstanceCount =
        (FacilityFoundationInstances ? FacilityFoundationInstances->GetInstanceCount() : 0)
        + (FacilityPostInstances ? FacilityPostInstances->GetInstanceCount() : 0)
        + (FacilityRoofInstances ? FacilityRoofInstances->GetInstanceCount() : 0)
        + (FacilityCargoInstances ? FacilityCargoInstances->GetInstanceCount() : 0)
        + (FacilityAccentInstances ? FacilityAccentInstances->GetInstanceCount() : 0)
        + (PhotorealFirePitInstances ? PhotorealFirePitInstances->GetInstanceCount() : 0)
        + (PhotorealStorageBasketInstances ? PhotorealStorageBasketInstances->GetInstanceCount() : 0)
        + (PhotorealWorkToolInstances ? PhotorealWorkToolInstances->GetInstanceCount() : 0);

    UE_LOG(LogTemp, Log,
        TEXT("LLWorldPresentation seed=%lld gen=%d chunks=%d groundTiles=%d natural=%d/%d/%d/%d facilities=%d facilityInstances=%d thinned=%d sightline=%d/%d dynamicCanopy=%d core=%.0f activity=%.0f ground=%s farGround=%s"),
        World.WorldSeed, World.GenerationVersion, World.MaterializedChunkCount,
        GroundTileCount,
        TreeInstanceCount, ShrubInstanceCount, GrassInstanceCount, RockInstanceCount,
        Civilization.FacilityCount, FacilityInstanceCount,
        SuppressedDressing, SightlineCleared, bInitialViewCaptured ? 1 : 0, DynamicCanopySuppressed,
        CoreClearRadiusUU, ActivityRadiusUU,
        (Ground && Ground->GetStaticMesh()) ? TEXT("yes") : TEXT("no"),
        (FarGround && FarGround->GetStaticMesh()) ? TEXT("yes") : TEXT("no"));
}
