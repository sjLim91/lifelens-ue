#include "Core/LLLifeLensGameMode.h"
#include "World/LLWorldDirector.h"
#include "World/LLWorldSpatialContract.h"
#include "UI/LLObserverHUD.h"
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
    HUDClass = ALLObserverHUD::StaticClass();
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

    // This is only the bootstrap visual/collision ground. It must nevertheless
    // cover the complete authoritative selected start chunk; clamping Core
    // targets to a smaller debug plane would corrupt simulation/presentation
    // agreement. World Visual may replace this actor later.
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

    // The selected authoritative start chunk is mapped to Unreal presentation
    // origin by LLWorldDirector. Frame that region using the shared spatial
    // contract instead of the legacy 1,400 UU bootstrap-floor dimensions.
    const FVector CameraLocation(
        0.0f,
        -LLWorldSpatialContract::ObserverCameraDistanceUU,
        LLWorldSpatialContract::ObserverCameraHeightUU);
    const FVector CameraTarget(
        0.0f,
        0.0f,
        LLWorldSpatialContract::ObserverCameraTargetHeightUU);
    const FRotator CameraRotation = (CameraTarget - CameraLocation).Rotation();

    ACameraActor* Camera = GetWorld()->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(), CameraLocation, CameraRotation);

    if (!Camera)
    {
        return;
    }

    if (UCameraComponent* CameraComponent = Camera->GetCameraComponent())
    {
        CameraComponent->SetFieldOfView(LLWorldSpatialContract::ObserverCameraFOVDegrees);
    }

    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        PlayerController->SetViewTarget(Camera);
    }
}
