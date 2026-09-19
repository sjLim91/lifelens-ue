#include "Core/LLLifeLensGameMode.h"
#include "World/LLWorldDirector.h"
#include "World/LLWorldObstacleCollisionProxyActor.h"
#include "World/LLWorldSpatialContract.h"
#include "WorldPresentation/LLDynamicEnvironmentPresentationActor.h"
#include "WorldPresentation/LLWaterPresentationActor.h"
#if !PLATFORM_ANDROID
#include "WorldPresentation/LLPCGGroundCoverPresentationActor.h"
#endif
#include "UI/LLSocialObserverHUD.h"
#include "UI/LLObserverPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

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

    if (GetWorld())
    {
        GetWorld()->SpawnActor<ALLWorldDirector>(ALLWorldDirector::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        GetWorld()->SpawnActor<ALLWorldObstacleCollisionProxyActor>(
            ALLWorldObstacleCollisionProxyActor::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
        GetWorld()->SpawnActor<ALLDynamicEnvironmentPresentationActor>(
            ALLDynamicEnvironmentPresentationActor::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
        GetWorld()->SpawnActor<ALLWaterPresentationActor>(
            ALLWaterPresentationActor::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
#if !PLATFORM_ANDROID
        GetWorld()->SpawnActor<ALLPCGGroundCoverPresentationActor>(
            ALLPCGGroundCoverPresentationActor::StaticClass(),
            FVector::ZeroVector,
            FRotator::ZeroRotator);
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

    ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(), CameraLocation, CameraRotation);

    if (!Camera)
    {
        return;
    }

    if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
    {
        CameraComponent->SetFieldOfView(CameraFOVDegrees);
    }

    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        PlayerController->SetViewTarget(Camera);
    }
}
