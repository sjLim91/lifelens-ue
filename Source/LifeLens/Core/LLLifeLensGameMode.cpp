#include "Core/LLLifeLensGameMode.h"
#include "World/LLWorldDirector.h"
#include "World/LLWorldObstacleCollisionProxyActor.h"
#include "World/LLWorldSpatialContract.h"
#include "WorldPresentation/LLDynamicEnvironmentPresentationActor.h"
#include "WorldPresentation/LLWorldPresentationActor.h"
#include "WorldPresentation/LLWaterPresentationActor.h"
#if !PLATFORM_ANDROID
#include "WorldPresentation/LLPCGGroundCoverPresentationActor.h"
#include "WorldPresentation/LLDesktopTerrainPresentationActor.h"
#endif
#include "UI/LLSocialObserverHUD.h"
#include "UI/LLObserverPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
template <typename TActor>
TActor* EnsureSingletonWorldActor(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    for (TActorIterator<TActor> It(World); It; ++It)
    {
        if (IsValid(*It))
        {
            return *It;
        }
    }

    return World->SpawnActor<TActor>(
        TActor::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator);
}
}

ALLLifeLensGameMode::ALLLifeLensGameMode()
{
    DefaultPawnClass = nullptr;
    HUDClass = ALLSocialObserverHUD::StaticClass();
    PlayerControllerClass = ALLObserverPlayerController::StaticClass();

    static ConstructorHelpers::FObjectFinder<UStaticMesh> FloorMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (FloorMeshFinder.Succeeded())
    {
        RuntimeFloorMesh = FloorMeshFinder.Object;
    }
}

void ALLLifeLensGameMode::BeginPlay()
{
    Super::BeginPlay();

    SpawnRuntimeFloor();

    if (UWorld* World = GetWorld())
    {
        // Runtime support actors are singletons per world. Authored maps may
        // already contain one of these; clean/generated maps may contain none.
        // Always reuse the authored instance instead of stacking a second
        // WorldDirector, collision proxy, terrain, water or weather projection.
        EnsureSingletonWorldActor<ALLWorldDirector>(World);
        EnsureSingletonWorldActor<ALLWorldObstacleCollisionProxyActor>(World);
        EnsureSingletonWorldActor<ALLWorldPresentationActor>(World);
        EnsureSingletonWorldActor<ALLDynamicEnvironmentPresentationActor>(World);
        EnsureSingletonWorldActor<ALLWaterPresentationActor>(World);
#if !PLATFORM_ANDROID
        EnsureSingletonWorldActor<ALLPCGGroundCoverPresentationActor>(World);
        EnsureSingletonWorldActor<ALLDesktopTerrainPresentationActor>(World);
#endif
    }

    SpawnObserverCamera();
}

void ALLLifeLensGameMode::SpawnRuntimeFloor()
{
    if (!GetWorld() || !RuntimeFloorMesh)
    {
        return;
    }

    AStaticMeshActor* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), FVector(0.0f, 0.0f, -55.0f), FRotator::ZeroRotator);
    if (!Floor)
    {
        return;
    }

    UStaticMeshComponent* FloorComponent = Floor->GetStaticMeshComponent();
    FloorComponent->SetMobility(EComponentMobility::Movable);
    FloorComponent->SetStaticMesh(RuntimeFloorMesh);
    FloorComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    FloorComponent->SetVisibility(false, true);
    FloorComponent->SetHiddenInGame(true, true);
    FloorComponent->SetCastShadow(false);

    // This is now collision-only bootstrap support. Visible terrain/ground belongs
    // to WorldPresentation, so the Engine Cube must not leak through underneath
    // photoreal materials as a giant prototype slab. Collision still covers the
    // complete authoritative selected start chunk; hiding the mesh does not
    // change simulation coordinates or movement authority.
    Floor->SetActorScale3D(FVector(
        LLWorldSpatialContract::BootstrapFloorCubeScale,
        LLWorldSpatialContract::BootstrapFloorCubeScale,
        0.1f));
}

void ALLLifeLensGameMode::SpawnObserverCamera()
{
    if (!GetWorld())
    {
        return;
    }

    // Keep the authoritative selected start region centred, but enter from a
    // higher, steeper observer angle so generated tree canopies do not sit
    // between the initial camera and the founders. Framing values are product
    // presentation tuning in DefaultGame.ini, not Core spatial authority.
    const float CameraDistanceChunks = FMath::Max(0.5f, ObserverCameraDistanceChunks);
    const float CameraHeightChunks = FMath::Max(0.5f, ObserverCameraHeightChunks);
    const float CameraDistanceUU = LLWorldSpatialContract::ChunkSpanUU * CameraDistanceChunks;
    const float CameraHeightUU = LLWorldSpatialContract::ChunkSpanUU * CameraHeightChunks;
    const float TargetHeightUU = FMath::Max(0.0f, ObserverCameraTargetHeightUU);
    const float CameraFOVDegrees = FMath::Clamp(ObserverCameraFOVDegrees, 30.0f, 90.0f);

    const FVector CameraLocation(0.0f, -CameraDistanceUU, CameraHeightUU);
    const FVector CameraTarget(0.0f, 0.0f, TargetHeightUU);
    const FRotator CameraRotation = (CameraTarget - CameraLocation).Rotation();

    ObserverCamera = GetWorld()->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(), CameraLocation, CameraRotation);

    if (!ObserverCamera)
    {
        UE_LOG(LogTemp, Error,
            TEXT("LifeLens observer camera spawn failed; controller recovery cannot establish a world view."));
        return;
    }

    // Stable runtime identity for the controller's self-healing view path.
    // This is presentation-only metadata and never changes world authority.
    ObserverCamera->Tags.AddUnique(FName(TEXT("LifeLens.ObserverCamera")));

    if (UCameraComponent* CameraComponent = ObserverCamera->GetCameraComponent())
    {
        CameraComponent->SetFieldOfView(CameraFOVDegrees);
    }

    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        PlayerController->SetViewTarget(ObserverCamera);
    }
}
