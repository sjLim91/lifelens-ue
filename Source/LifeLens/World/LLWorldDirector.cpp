#include "World/LLWorldDirector.h"
#include "World/LLActivityAnchor.h"
#include "World/LLEnvironmentalResidueVisualizerComponent.h"
#include "Characters/LLResidentCharacter.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "Algo/Reverse.h"

namespace
{
    struct FLLAStarOpenNode
    {
        FIntPoint Cell = FIntPoint::ZeroValue;
        int32 G = 0;
        int32 F = 0;
    };

    int32 LLGridOctileHeuristic(const FIntPoint& A, const FIntPoint& B)
    {
        const int32 Dx = FMath::Abs(A.X - B.X);
        const int32 Dy = FMath::Abs(A.Y - B.Y);
        const int32 Diagonal = FMath::Min(Dx, Dy);
        const int32 Straight = FMath::Max(Dx, Dy) - Diagonal;
        return Diagonal * 14 + Straight * 10;
    }
}

ALLWorldDirector::ALLWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    EnvironmentalResidueVisualizer = CreateDefaultSubobject<ULLEnvironmentalResidueVisualizerComponent>(
        TEXT("EnvironmentalResidueVisualizer"));
    EnvironmentalResidueVisualizer->SetupAttachment(SceneRoot);
}

void ALLWorldDirector::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    CoreBridge = GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    if (!Simulation || !CoreBridge)
    {
        return;
    }

    if (!Simulation->LoadGame())
    {
        Simulation->NewGame();
    }

    CoreBridge->SetExternalPhysicalExecutionEnabled(true);
    RefreshCorePresentationOrigin();
    CollectActivityAnchors();
    SpawnResidents();
    if (EnvironmentalResidueVisualizer)
    {
        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, true,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
    }
}

void ALLWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Simulation || !CoreBridge)
    {
        return;
    }

    CoreBridge->SetExternalPhysicalExecutionEnabled(true);

    const float RealDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
    const float SpeedMultiplier = FMath::Max(0.0f, Simulation->GetSimulationSpeedMultiplier());

    bool bAdvancedSimulation = false;
    if (SpeedMultiplier > KINDA_SMALL_NUMBER)
    {
        // Core time consumes scaled real time, while visual refresh below stays
        // on the render clock. This keeps Pause/1x/4x/16x/64x from creating a
        // second simulation truth in Presentation.
        SimulationClockAccumulator += RealDeltaSeconds * SpeedMultiplier;
        const float StepSeconds = FMath::Max(0.01f, RealSecondsPerSimulationMinute);
        const int32 StepBudget = FMath::Max(1, MaxSimulationMinutesPerFrame);
        int32 StepsThisFrame = 0;
        while (SimulationClockAccumulator >= StepSeconds && StepsThisFrame < StepBudget)
        {
            SimulationClockAccumulator -= StepSeconds;
            Simulation->AdvanceSimulationMinutes(1);
            ++StepsThisFrame;
            bAdvancedSimulation = true;

            if ((Simulation->GetSimulationMinute() % 60) == 0)
            {
                Simulation->SaveGame();
            }
        }
    }

    if (bAdvancedSimulation)
    {
        CollectActivityAnchors();
        SpawnResidents();
    }

    // Physical presentation follows the same Observer speed so authoritative
    // external actions do not become a real-time bottleneck at high speed.
    // Pausing disables resident movement ticks without stopping the render/UI
    // clock. Core itself remains frozen because no simulation minutes advance.
    const bool bPaused = SpeedMultiplier <= KINDA_SMALL_NUMBER;
    const float SimulationDeltaSeconds = RealDeltaSeconds * SpeedMultiplier;
    for (ALLResidentCharacter* Character : SpawnedResidents)
    {
        if (IsValid(Character))
        {
            Character->SetActorTickEnabled(!bPaused);
            Character->CustomTimeDilation = bPaused ? 1.0f : SpeedMultiplier;
            UpdateResident(*Character, SimulationDeltaSeconds);
        }
    }

    for (auto& Pair : RuntimeStates)
    {
        if (!FindResidentActor(Pair.Key))
        {
            ReleasePhysicalReservation(Pair.Key, Pair.Value);
        }
    }

    EnvironmentalVisualRefreshAccumulator += RealDeltaSeconds;
    const float VisualRefreshInterval = FMath::Max(0.05f, EnvironmentalVisualRefreshIntervalSeconds);
    if (EnvironmentalResidueVisualizer
        && EnvironmentalVisualRefreshAccumulator >= VisualRefreshInterval)
    {
        EnvironmentalVisualRefreshAccumulator = FMath::Fmod(
            EnvironmentalVisualRefreshAccumulator, VisualRefreshInterval);
        EnvironmentalResidueVisualizer->RefreshFromCore(*CoreBridge, CoreGridCellSizeUU, false,
            CorePresentationOriginGrid.X, CorePresentationOriginGrid.Y);
    }
}

ALLResidentCharacter* ALLWorldDirector::FindResidentActor(FGuid ResidentId) const
{
    for (ALLResidentCharacter* Character : SpawnedResidents)
    {
        if (IsValid(Character) && Character->GetResidentId() == ResidentId)
        {
            return Character;
        }
    }
    return nullptr;
}

ELLActionIntent ALLWorldDirector::GetResidentIntent(FGuid ResidentId) const
{
    const ALLResidentCharacter* Character = FindResidentActor(ResidentId);
    return Character ? Character->GetCurrentIntent() : ELLActionIntent::Idle;
}

ELLWorldAffordanceTier ALLWorldDirector::GetResidentAffordanceTier(FGuid ResidentId) const
{
    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);
    return Runtime ? Runtime->ActiveAffordanceTier : ELLWorldAffordanceTier::Unavailable;
}

bool ALLWorldDirector::IsResidentUsingEmergencyFallback(FGuid ResidentId) const
{
    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);
    return Runtime
        && Runtime->ActiveAffordanceTier == ELLWorldAffordanceTier::Emergency;
}

bool ALLWorldDirector::IsResidentPerformingPhysicalAction(FGuid ResidentId) const
{
    const FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId);
    return Runtime
        && Runtime->bPerformingAction
        && Runtime->LastActivityKind == ELLCoreObservedActivityKind::Physical;
}

int32 ALLWorldDirector::GetEnvironmentalResidueVisualCount() const
{
    return EnvironmentalResidueVisualizer
        ? EnvironmentalResidueVisualizer->GetVisualInstanceCount()
        : 0;
}

void ALLWorldDirector::RefreshCorePresentationOrigin()
{
    CorePresentationOriginGrid = FIntPoint::ZeroValue;
    if (!CoreBridge)
    {
        return;
    }

    const FLLCoreWorldGenerationObservation Genesis = CoreBridge->GetWorldGenerationObservation();
    if (Genesis.bAvailable && Genesis.bHasInitialStartRegion)
    {
        CorePresentationOriginGrid = FIntPoint(
            Genesis.InitialCenterGridX,
            Genesis.InitialCenterGridY);
    }
}

FVector ALLWorldDirector::CoreGridToWorldCellCenter(
    const FIntPoint& Grid,
    float WorldZ) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    return GetActorLocation()
        + FVector(
            static_cast<float>(Grid.X - CorePresentationOriginGrid.X) * CellSize,
            static_cast<float>(Grid.Y - CorePresentationOriginGrid.Y) * CellSize,
            WorldZ - GetActorLocation().Z);
}

TArray<FVector> ALLWorldDirector::BuildLocalAStarPath(
    const FVector& StartLocation,
    const FVector& TargetLocation) const
{
    TArray<FVector> Result;
    if (!CoreBridge)
    {
        return Result;
    }

    const FIntPoint Start = WorldLocationToCoreGrid(StartLocation);
    const FIntPoint Goal = WorldLocationToCoreGrid(TargetLocation);
    if (Start == Goal)
    {
        Result.Add(TargetLocation);
        return Result;
    }

    TSet<FIntPoint> Blocked;
    TSet<FIntPoint> Walkable;

    const int32 ChunkSpan =
        FMath::Max(1, LLWorldSpatialContract::ChunkSpanGridCells);
    const TArray<FLLCoreNaturalChunkObservation> Chunks =
        CoreBridge->GetMaterializedNaturalChunkObservations();
    for (const FLLCoreNaturalChunkObservation& Chunk : Chunks)
    {
        // Physical locomotion may only traverse materialized non-ocean surface.
        // Presentation can draw continuity beyond that area, but residents need
        // an authoritative support surface underneath every routed grid cell.
        if (Chunk.bMaterialized && Chunk.Surface != FName(TEXT("Ocean")))
        {
            const int32 ChunkMinX = Chunk.ChunkX * ChunkSpan;
            const int32 ChunkMinY = Chunk.ChunkY * ChunkSpan;
            for (int32 Y = ChunkMinY; Y < ChunkMinY + ChunkSpan; ++Y)
            {
                for (int32 X = ChunkMinX; X < ChunkMinX + ChunkSpan; ++X)
                {
                    Walkable.Add(FIntPoint(X, Y));
                }
            }
        }

        for (const FLLCoreNaturalObstacleObservation& Obstacle : Chunk.PhysicalObstacles)
        {
            Blocked.Add(FIntPoint(Obstacle.GridX, Obstacle.GridY));
        }
    }

    // Marine water is physical routing truth at Local Surface scale. Ocean is
    // completely blocked. Coast uses the same deterministic marine-neighbour
    // orientation exposed to WaterPresentation, so A* will not route residents
    // across the visible marine half of a coastal chunk.
    for (const FLLCoreSurfaceWaterPresentationObservation& Water :
        CoreBridge->GetMaterializedSurfaceWaterPresentationObservations())
    {
        if (!Water.bAvailable)
        {
            continue;
        }

        const int32 MinX = Water.ChunkX * ChunkSpan;
        const int32 MinY = Water.ChunkY * ChunkSpan;
        if (Water.SurfaceKind == ELLCoreSurfaceWaterKind::Ocean)
        {
            for (int32 Y = MinY; Y < MinY + ChunkSpan; ++Y)
            {
                for (int32 X = MinX; X < MinX + ChunkSpan; ++X)
                {
                    Blocked.Add(FIntPoint(X, Y));
                }
            }
            continue;
        }

        if (Water.SurfaceKind != ELLCoreSurfaceWaterKind::Coast
            || !Water.bHasMarineNeighbour)
        {
            continue;
        }

        FVector2D MarineDirection(
            static_cast<float>(Water.MarineCenterGridX - Water.CenterGridX),
            static_cast<float>(Water.MarineCenterGridY - Water.CenterGridY));
        if (!MarineDirection.Normalize())
        {
            continue;
        }

        // WaterPresentation places its shoreline at 4% of half-chunk toward
        // the marine neighbour. Mirror that boundary in grid routing.
        const float ShoreOffsetCells = static_cast<float>(ChunkSpan) * 0.02f;
        for (int32 Y = MinY; Y < MinY + ChunkSpan; ++Y)
        {
            for (int32 X = MinX; X < MinX + ChunkSpan; ++X)
            {
                const FVector2D CellDelta(
                    static_cast<float>(X - Water.CenterGridX) + 0.5f,
                    static_cast<float>(Y - Water.CenterGridY) + 0.5f);
                if (FVector2D::DotProduct(CellDelta, MarineDirection)
                    >= ShoreOffsetCells)
                {
                    Blocked.Add(FIntPoint(X, Y));
                }
            }
        }
    }

    // Constructed facilities occupy real Core grid sites. Route around other
    // facilities, while allowing the actual goal cell so a resident can reach
    // the facility it is currently using.
    // Environmental blockers are hard physical truth. Remember whether the
    // requested goal itself is inside a tree/rock or visible marine surface
    // before adding facilities, because the actual facility goal is the only
    // blocker type that may be intentionally entered.
    const bool bGoalEnvironmentBlocked = Blocked.Contains(Goal);

    const FLLCoreCivilizationWorldObservation Civilization =
        CoreBridge->GetCivilizationWorldObservation(0);
    for (const FLLCoreCivilizationFacilityObservation& Facility :
        Civilization.Facilities)
    {
        Blocked.Add(FIntPoint(Facility.GridX, Facility.GridY));
    }

    Blocked.Remove(Start);
    if (bGoalEnvironmentBlocked
        || !Walkable.Contains(Start)
        || !Walkable.Contains(Goal))
    {
        UE_LOG(LogTemp, Verbose,
            TEXT("LifeLens local A* rejected unsupported/blocked endpoint: start=(%d,%d) goal=(%d,%d) startWalk=%d goalWalk=%d hardGoal=%d"),
            Start.X, Start.Y, Goal.X, Goal.Y,
            Walkable.Contains(Start) ? 1 : 0,
            Walkable.Contains(Goal) ? 1 : 0,
            bGoalEnvironmentBlocked ? 1 : 0);
        return Result;
    }

    // Facilities are valid action destinations, so the selected goal facility
    // may be entered while all other facility cells remain obstacles.
    Blocked.Remove(Goal);

    const int32 Margin = FMath::Clamp(LocalAStarMarginCells, 2, 64);
    const int32 MinX = FMath::Min(Start.X, Goal.X) - Margin;
    const int32 MaxX = FMath::Max(Start.X, Goal.X) + Margin;
    const int32 MinY = FMath::Min(Start.Y, Goal.Y) - Margin;
    const int32 MaxY = FMath::Max(Start.Y, Goal.Y) + Margin;

    TArray<FLLAStarOpenNode> Open;
    TSet<FIntPoint> Closed;
    TMap<FIntPoint, int32> GScore;
    TMap<FIntPoint, FIntPoint> CameFrom;

    Open.Add({Start, 0, LLGridOctileHeuristic(Start, Goal)});
    GScore.Add(Start, 0);

    static const FIntPoint Directions[8] = {
        FIntPoint(1, 0), FIntPoint(-1, 0),
        FIntPoint(0, 1), FIntPoint(0, -1),
        FIntPoint(1, 1), FIntPoint(1, -1),
        FIntPoint(-1, 1), FIntPoint(-1, -1)
    };

    int32 Expanded = 0;
    const int32 ExpansionBudget =
        FMath::Clamp(MaxLocalAStarExpandedNodes, 128, 32768);

    while (Open.Num() > 0 && Expanded < ExpansionBudget)
    {
        int32 BestIndex = 0;
        for (int32 Index = 1; Index < Open.Num(); ++Index)
        {
            const FLLAStarOpenNode& Candidate = Open[Index];
            const FLLAStarOpenNode& Best = Open[BestIndex];
            if (Candidate.F < Best.F
                || (Candidate.F == Best.F && Candidate.G < Best.G)
                || (Candidate.F == Best.F && Candidate.G == Best.G
                    && (Candidate.Cell.X < Best.Cell.X
                        || (Candidate.Cell.X == Best.Cell.X
                            && Candidate.Cell.Y < Best.Cell.Y))))
            {
                BestIndex = Index;
            }
        }

        const FLLAStarOpenNode Current = Open[BestIndex];
        Open.RemoveAtSwap(BestIndex, 1, EAllowShrinking::No);

        const int32* BestKnownG = GScore.Find(Current.Cell);
        if (!BestKnownG || Current.G != *BestKnownG || Closed.Contains(Current.Cell))
        {
            continue;
        }

        if (Current.Cell == Goal)
        {
            TArray<FIntPoint> Cells;
            FIntPoint Cursor = Goal;
            while (Cursor != Start)
            {
                Cells.Add(Cursor);
                const FIntPoint* Parent = CameFrom.Find(Cursor);
                if (!Parent)
                {
                    Cells.Reset();
                    break;
                }
                Cursor = *Parent;
            }

            Algo::Reverse(Cells);
            Result.Reserve(Cells.Num() + 1);
            for (const FIntPoint& Cell : Cells)
            {
                Result.Add(CoreGridToWorldCellCenter(Cell, StartLocation.Z));
            }
            if (Result.Num() == 0
                || FVector::DistSquared2D(Result.Last(), TargetLocation)
                    > FMath::Square(1.0f))
            {
                Result.Add(TargetLocation);
            }
            return Result;
        }

        Closed.Add(Current.Cell);
        ++Expanded;

        for (const FIntPoint& Direction : Directions)
        {
            const FIntPoint Next = Current.Cell + Direction;
            if (Next.X < MinX || Next.X > MaxX
                || Next.Y < MinY || Next.Y > MaxY
                || !Walkable.Contains(Next)
                || Blocked.Contains(Next)
                || Closed.Contains(Next))
            {
                continue;
            }

            const bool bDiagonal = Direction.X != 0 && Direction.Y != 0;
            if (bDiagonal)
            {
                // No diagonal corner cutting between two blocked obstacle cells.
                const FIntPoint SideX =
                    Current.Cell + FIntPoint(Direction.X, 0);
                const FIntPoint SideY =
                    Current.Cell + FIntPoint(0, Direction.Y);
                if (!Walkable.Contains(SideX)
                    || !Walkable.Contains(SideY)
                    || Blocked.Contains(SideX)
                    || Blocked.Contains(SideY))
                {
                    continue;
                }
            }

            const int32 TentativeG = Current.G + (bDiagonal ? 14 : 10);
            const int32* ExistingG = GScore.Find(Next);
            if (ExistingG && TentativeG >= *ExistingG)
            {
                continue;
            }

            CameFrom.Add(Next, Current.Cell);
            GScore.Add(Next, TentativeG);
            Open.Add({
                Next,
                TentativeG,
                TentativeG + LLGridOctileHeuristic(Next, Goal)
            });
        }
    }

    UE_LOG(LogTemp, Verbose,
        TEXT("LifeLens local A* fallback to direct sweep: start=(%d,%d) goal=(%d,%d) expanded=%d blocked=%d"),
        Start.X, Start.Y, Goal.X, Goal.Y, Expanded, Blocked.Num());
    return Result;
}

void ALLWorldDirector::MoveResidentToward(
    ALLResidentCharacter& Character,
    const FVector& DesiredLocation)
{
    if (Character.IsMovingToward(DesiredLocation))
    {
        return;
    }

    FLLResidentRuntimeState* Runtime =
        RuntimeStates.Find(Character.GetResidentId());
    const UWorld* World = GetWorld();
    const double NowSeconds = World ? static_cast<double>(World->GetTimeSeconds()) : 0.0;
    const float SameFailedTargetToleranceUU =
        FMath::Max(1.0f, CoreGridCellSizeUU * 0.50f);

    if (Runtime
        && Runtime->bHasFailedRouteTarget
        && FVector::DistSquared2D(
            Runtime->LastFailedRouteTarget,
            DesiredLocation)
            <= FMath::Square(SameFailedTargetToleranceUU)
        && NowSeconds < Runtime->NextRouteRetryWorldSeconds)
    {
        return;
    }

    const TArray<FVector> Path =
        BuildLocalAStarPath(Character.GetActorLocation(), DesiredLocation);
    if (Path.Num() > 0)
    {
        if (Runtime)
        {
            Runtime->bHasFailedRouteTarget = false;
            Runtime->NextRouteRetryWorldSeconds = 0.0;
        }
        Character.SetMovementPath(Path, DesiredLocation);
    }
    else
    {
        // Do not bypass the authoritative route mask with a direct sweep.
        // Water presentation is collision-free and non-materialized chunks have
        // no support surface, so a "fallback" straight line could walk through
        // visible sea or off the physical world. An unreachable Core target is
        // left pending for re-resolution instead of violating world geometry.
        Character.ClearMovementTarget();
        if (Runtime)
        {
            Runtime->bHasFailedRouteTarget = true;
            Runtime->LastFailedRouteTarget = DesiredLocation;
            Runtime->NextRouteRetryWorldSeconds =
                NowSeconds + static_cast<double>(
                    FMath::Max(0.05f, FailedRouteRetrySeconds));
        }
        UE_LOG(LogTemp, Verbose,
            TEXT("LifeLens resident route unavailable; movement held at %s toward %s"),
            *Character.GetActorLocation().ToCompactString(),
            *DesiredLocation.ToCompactString());
    }
}

void ALLWorldDirector::CollectActivityAnchors()
{
    ActivityAnchors.Reset();
    for (TActorIterator<ALLActivityAnchor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It))
        {
            ActivityAnchors.Add(*It);
        }
    }
}

void ALLWorldDirector::SpawnResidents()
{
    if (!Simulation || !CoreBridge || !GetWorld())
    {
        return;
    }

    SpawnedResidents.RemoveAll([](const TObjectPtr<ALLResidentCharacter>& Character)
    {
        return !IsValid(Character.Get());
    });

    const TArray<FLLResidentData> Residents = Simulation->GetResidents();
    TSet<FGuid> ProjectedResidentIds;
    ProjectedResidentIds.Reserve(Residents.Num());

    for (int32 Index = 0; Index < Residents.Num(); ++Index)
    {
        const FLLResidentData& Resident = Residents[Index];
        if (!Resident.ResidentId.IsValid())
        {
            continue;
        }

        ProjectedResidentIds.Add(Resident.ResidentId);

        if (ALLResidentCharacter* Existing = FindResidentActor(Resident.ResidentId))
        {
            Existing->BindResident(Resident);
            RuntimeStates.FindOrAdd(Resident.ResidentId);
            continue;
        }

        int32 GridX = 0;
        int32 GridY = 0;
        CoreBridge->GetResidentRuntimeGridPosition(Resident.ResidentId, GridX, GridY);
        const FVector SpawnLocation = CoreGridToWorldSpawnLocation(GridX, GridY, Index);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ALLResidentCharacter* Character = GetWorld()->SpawnActor<ALLResidentCharacter>(
            ALLResidentCharacter::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
        if (!Character)
        {
            continue;
        }

        Character->BindResident(Resident);
        SpawnedResidents.Add(Character);
        RuntimeStates.FindOrAdd(Resident.ResidentId);
    }

    for (int32 Index = SpawnedResidents.Num() - 1; Index >= 0; --Index)
    {
        ALLResidentCharacter* Character = SpawnedResidents[Index].Get();
        if (!IsValid(Character))
        {
            SpawnedResidents.RemoveAtSwap(Index);
            continue;
        }

        const FGuid ResidentId = Character->GetResidentId();
        if (ProjectedResidentIds.Contains(ResidentId))
        {
            continue;
        }

        if (FLLResidentRuntimeState* Runtime = RuntimeStates.Find(ResidentId))
        {
            ReleasePhysicalReservation(ResidentId, *Runtime);
        }
        RuntimeStates.Remove(ResidentId);
        Character->Destroy();
        SpawnedResidents.RemoveAtSwap(Index);
    }

    for (auto It = RuntimeStates.CreateIterator(); It; ++It)
    {
        if (!ProjectedResidentIds.Contains(It.Key()))
        {
            ReleasePhysicalReservation(It.Key(), It.Value());
            It.RemoveCurrent();
        }
    }
}

void ALLWorldDirector::UpdateResident(ALLResidentCharacter& Character, float DeltaSeconds)
{
    FLLResidentRuntimeState* Runtime = RuntimeStates.Find(Character.GetResidentId());
    if (!Runtime || !CoreBridge)
    {
        return;
    }

    FLLCoreActionDirective PendingDirective;
    if (CoreBridge->GetResidentPendingContextDirective(Character.GetResidentId(), PendingDirective))
    {
        ApplyPendingContextDirective(Character, *Runtime, PendingDirective, DeltaSeconds);
        return;
    }

    Runtime->ActiveContextActionToken = 0;
    Runtime->ContextUseElapsedSeconds = 0.0f;

    FLLCoreActionDirective Directive;
    if (!CoreBridge->GetResidentActionDirective(Character.GetResidentId(), Directive))
    {
        ReleasePhysicalReservation(Character.GetResidentId(), *Runtime);
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
        Runtime->bPerformingAction = false;
        return;
    }

    ApplyCoreDirective(Character, *Runtime, Directive, DeltaSeconds);
}

void ALLWorldDirector::ApplyCoreDirective(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    const FLLCoreActionDirective& Directive,
    float DeltaSeconds)
{
    const bool bDirectiveChanged =
        !Runtime.bInitialized
        || Runtime.LastActivityKind != Directive.ActivityKind
        || Runtime.LastPhysicalIntent != Directive.PhysicalIntent
        || Runtime.LastSocialIntent != Directive.SocialIntent
        || Runtime.LastTargetId != Directive.TargetResidentId;

    if (bDirectiveChanged)
    {
        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
        Runtime.bInitialized = true;
        Runtime.bPerformingAction = false;
        Runtime.PhysicalUseElapsedSeconds = 0.0f;
        Runtime.LastActivityKind = Directive.ActivityKind;
        Runtime.LastPhysicalIntent = Directive.PhysicalIntent;
        Runtime.LastSocialIntent = Directive.SocialIntent;
        Runtime.LastTargetId = Directive.TargetResidentId;
        Character.ClearMovementTarget();
    }

    if (!Directive.bAlive || Directive.ActivityKind == ELLCoreObservedActivityKind::Idle)
    {
        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
        Runtime.bPerformingAction = false;
        Character.ClearMovementTarget();
        Character.SetCurrentIntent(ELLActionIntent::Idle);
        return;
    }

    if (Directive.ActivityKind == ELLCoreObservedActivityKind::Physical)
    {
        const ELLActionIntent Intent = ToPresentationIntent(Directive.PhysicalIntent);
        Character.SetCurrentIntent(Intent);

        if (Intent == ELLActionIntent::Idle)
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        ALLActivityAnchor* Anchor = EnsurePhysicalReservation(Character, Runtime, Intent);

        // A Core-owned designated sanitation area is a Primitive-tier affordance.
        // Preferred/Primitive authored anchors may win; Natural anchors may not
        // mask an authoritative designated site that should be selected first.
        if (Anchor && Intent == ELLActionIntent::Toilet
            && static_cast<uint8>(Anchor->AffordanceTier)
                > static_cast<uint8>(ELLWorldAffordanceTier::Primitive))
        {
            int32 GridX = 0;
            int32 GridY = 0;
            bool bDesignated = false;
            int64 SiteId = 0;
            if (CoreBridge->GetSanitationUseTarget(
                    Character.GetResidentId(), GridX, GridY, bDesignated, SiteId)
                && bDesignated)
            {
                ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
                Anchor = nullptr;
            }
        }

        FTransform UseTransform = FTransform::Identity;
        const bool bUsesAnchor = Anchor != nullptr;

        if (bUsesAnchor)
        {
            UseTransform = Anchor->GetUseTransform();
        }
        else if (!EnsureEmergencyFallback(Character, Runtime, Intent, UseTransform))
        {
            Runtime.bPerformingAction = false;
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
            Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
            Character.ClearMovementTarget();
            return;
        }

        const FVector DesiredLocation = UseTransform.GetLocation();
        const double DistanceSquared = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bCoreSanitationGridTarget =
            Intent == ELLActionIntent::Toilet
            && (Runtime.bUsingEmergencyFallback || Runtime.bUsingDesignatedSanitationSite);
        const float ArrivalRadius = bCoreSanitationGridTarget
            ? FMath::Max(1.0f, FMath::Min(45.0f, CoreGridCellSizeUU * 0.45f))
            : 110.0f;
        const bool bAtUsePoint = DistanceSquared <= FMath::Square(ArrivalRadius);

        if (!bAtUsePoint)
        {
            Runtime.bPerformingAction = false;
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
            MoveResidentToward(Character, DesiredLocation);
            return;
        }

        Character.ClearMovementTarget();
        if (bUsesAnchor && !Anchor->MarkInUse(Character.GetResidentId()))
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            return;
        }

        Runtime.bPerformingAction = true;
        const FRotator UseRotation = UseTransform.GetRotation().Rotator();
        Character.SetActorRotation(FRotator(0.0f, UseRotation.Yaw, 0.0f));

        Runtime.PhysicalUseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
        const int32 DurationTicks = FMath::Max(
            1,
            CoreBridge->GetPhysicalActionDurationTicks(
                Directive.PhysicalIntent,
                Runtime.bUsingEmergencyFallback,
                Runtime.bUsingDesignatedSanitationSite));
        const float RequiredUseSeconds = FMath::Max(
            0.1f,
            static_cast<float>(DurationTicks) * FMath::Max(0.1f, RealSecondsPerSimulationMinute));

        if (Runtime.PhysicalUseElapsedSeconds < RequiredUseSeconds)
        {
            return;
        }

        const bool bEmergencyFallback = Runtime.bUsingEmergencyFallback;
        const int64 SanitationSiteId = Runtime.bUsingDesignatedSanitationSite
            ? Runtime.CoreSanitationSiteId
            : 0;
        const FIntPoint ResolvedGrid = WorldLocationToCoreGrid(Character.GetActorLocation());
        const bool bAcknowledged = CoreBridge->CompleteResidentPhysicalAction(
            Character.GetResidentId(),
            bEmergencyFallback,
            ResolvedGrid.X,
            ResolvedGrid.Y,
            SanitationSiteId);

        if (bAcknowledged)
        {
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            Character.SetCurrentIntent(ELLActionIntent::Idle);
        }
        else if (Runtime.bUsingDesignatedSanitationSite)
        {
            // The Core site may have been invalidated while the resident moved.
            // Release the stale target; the next tick re-resolves through Core
            // and naturally falls back to another valid tier if necessary.
            ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
        }
        else
        {
            Runtime.PhysicalUseElapsedSeconds = 0.0f;
        }
        return;
    }

    ReleasePhysicalReservation(Character.GetResidentId(), Runtime);

    if (Directive.ActivityKind == ELLCoreObservedActivityKind::Social)
    {
        Character.SetCurrentIntent(ELLActionIntent::Socialize);

        ALLResidentCharacter* Target = FindResidentActor(Directive.TargetResidentId);
        if (!Target || Directive.SocialIntent == ELLCoreSocialIntent::None)
        {
            Runtime.bPerformingAction = false;
            Character.ClearMovementTarget();
            return;
        }

        const FVector DesiredLocation = ResolveSocialTargetLocation(Character, *Target, Directive.SocialIntent);
        const double DistanceToDesired = FVector::DistSquared2D(Character.GetActorLocation(), DesiredLocation);
        const bool bAtDesiredLocation = DistanceToDesired <= FMath::Square(110.0);

        if (!bAtDesiredLocation)
        {
            Runtime.bPerformingAction = false;
            MoveResidentToward(Character, DesiredLocation);
        }
        else
        {
            Runtime.bPerformingAction = true;
            Character.ClearMovementTarget();

            if (Directive.SocialIntent != ELLCoreSocialIntent::Avoid)
            {
                FVector LookDirection = Target->GetActorLocation() - Character.GetActorLocation();
                LookDirection.Z = 0.0f;
                if (!LookDirection.IsNearlyZero())
                {
                    Character.SetActorRotation(FRotator(0.0f, LookDirection.Rotation().Yaw, 0.0f));
                }
            }
        }
    }
}

ELLActionIntent ALLWorldDirector::ToPresentationIntent(ELLCorePhysicalIntent Intent) const
{
    switch (Intent)
    {
        case ELLCorePhysicalIntent::Eat: return ELLActionIntent::Eat;
        case ELLCorePhysicalIntent::Drink: return ELLActionIntent::Drink;
        case ELLCorePhysicalIntent::Sleep: return ELLActionIntent::Sleep;
        case ELLCorePhysicalIntent::Toilet: return ELLActionIntent::Toilet;
        case ELLCorePhysicalIntent::Hygiene: return ELLActionIntent::Hygiene;
        case ELLCorePhysicalIntent::None:
        default:
            return ELLActionIntent::Idle;
    }
}

ALLActivityAnchor* ALLWorldDirector::FindBestUsableAnchor(
    const ALLResidentCharacter& Character,
    ELLActionIntent Intent,
    ELLWorldAffordanceTier& OutTier) const
{
    ALLActivityAnchor* BestAnchor = nullptr;
    ELLWorldAffordanceTier BestTier = ELLWorldAffordanceTier::Unavailable;
    double BestDistanceSquared = TNumericLimits<double>::Max();
    FString BestPath;

    for (ALLActivityAnchor* Anchor : ActivityAnchors)
    {
        if (!IsValid(Anchor)
            || !Anchor->SupportsIntent(Intent)
            || !Anchor->CanBeUsedBy(Character.GetResidentId())
            || Anchor->AffordanceTier == ELLWorldAffordanceTier::Unavailable)
        {
            continue;
        }

        const ELLWorldAffordanceTier CandidateTier = Anchor->AffordanceTier;
        const double DistanceSquared = FVector::DistSquared2D(
            Character.GetActorLocation(), Anchor->GetUseLocation());
        const FString AnchorPath = Anchor->GetPathName();

        const uint8 CandidateTierValue = static_cast<uint8>(CandidateTier);
        const uint8 BestTierValue = static_cast<uint8>(BestTier);
        const bool bBetterTier = CandidateTierValue < BestTierValue;
        const bool bSameTier = CandidateTier == BestTier;
        const bool bCloser = bSameTier && DistanceSquared + KINDA_SMALL_NUMBER < BestDistanceSquared;
        const bool bStableTieBreak =
            bSameTier
            && FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared)
            && (BestAnchor == nullptr || AnchorPath.Compare(BestPath, ESearchCase::CaseSensitive) < 0);

        if (bBetterTier || bCloser || bStableTieBreak)
        {
            BestAnchor = Anchor;
            BestTier = CandidateTier;
            BestDistanceSquared = DistanceSquared;
            BestPath = AnchorPath;
        }
    }

    OutTier = BestTier;
    return BestAnchor;
}

ALLActivityAnchor* ALLWorldDirector::EnsurePhysicalReservation(
    ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    ELLActionIntent Intent)
{
    if (Runtime.ReservedAnchor.IsValid())
    {
        ALLActivityAnchor* Existing = Runtime.ReservedAnchor.Get();
        if (Runtime.ReservedIntent == Intent
            && Existing->SupportsIntent(Intent)
            && Existing->IsClaimedBy(Character.GetResidentId())
            && Existing->CanBeUsedBy(Character.GetResidentId()))
        {
            Runtime.ActiveAffordanceTier = Existing->AffordanceTier;
            Runtime.bUsingEmergencyFallback = false;
            Runtime.bUsingDesignatedSanitationSite = false;
            Runtime.CoreSanitationSiteId = 0;
            return Existing;
        }

        ReleasePhysicalReservation(Character.GetResidentId(), Runtime);
    }

    ELLWorldAffordanceTier CandidateTier = ELLWorldAffordanceTier::Unavailable;
    ALLActivityAnchor* Candidate = FindBestUsableAnchor(Character, Intent, CandidateTier);
    if (!Candidate || !Candidate->TryReserve(Character.GetResidentId()))
    {
        return nullptr;
    }

    Runtime.ReservedAnchor = Candidate;
    Runtime.ReservedIntent = Intent;
    Runtime.ActiveAffordanceTier = CandidateTier;
    Runtime.bUsingEmergencyFallback = false;
    Runtime.bUsingDesignatedSanitationSite = false;
    Runtime.CoreSanitationSiteId = 0;
    Runtime.EmergencyUseTransform = FTransform::Identity;
    Runtime.PhysicalUseElapsedSeconds = 0.0f;
    return Candidate;
}

bool ALLWorldDirector::EnsureEmergencyFallback(
    const ALLResidentCharacter& Character,
    FLLResidentRuntimeState& Runtime,
    ELLActionIntent Intent,
    FTransform& OutUseTransform) const
{
    if (!SupportsEmergencyFallback(Intent))
    {
        Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
        Runtime.bUsingEmergencyFallback = false;
        Runtime.bUsingDesignatedSanitationSite = false;
        Runtime.CoreSanitationSiteId = 0;
        return false;
    }

    const bool bHasCachedTarget = Runtime.ReservedIntent == Intent
        && (Intent == ELLActionIntent::Toilet
            ? (Runtime.bUsingEmergencyFallback || Runtime.bUsingDesignatedSanitationSite)
            : Runtime.bUsingEmergencyFallback);

    if (!bHasCachedTarget)
    {
        Runtime.ReservedIntent = Intent;
        Runtime.bUsingEmergencyFallback = false;
        Runtime.bUsingDesignatedSanitationSite = false;
        Runtime.CoreSanitationSiteId = 0;

        if (Intent == ELLActionIntent::Toilet)
        {
            int32 RecommendedGridX = 0;
            int32 RecommendedGridY = 0;
            bool bDesignatedSite = false;
            int64 SanitationSiteId = 0;
            if (!CoreBridge || !CoreBridge->GetSanitationUseTarget(
                    Character.GetResidentId(),
                    RecommendedGridX,
                    RecommendedGridY,
                    bDesignatedSite,
                    SanitationSiteId))
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
                Runtime.EmergencyUseTransform = FTransform::Identity;
                Runtime.PhysicalUseElapsedSeconds = 0.0f;
                return false;
            }

            Runtime.bUsingDesignatedSanitationSite = bDesignatedSite;
            Runtime.bUsingEmergencyFallback = !bDesignatedSite;
            Runtime.CoreSanitationSiteId = bDesignatedSite ? SanitationSiteId : 0;
            Runtime.ActiveAffordanceTier = bDesignatedSite
                ? ELLWorldAffordanceTier::Primitive
                : ELLWorldAffordanceTier::Emergency;

            const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
            FVector RecommendedLocation = GetActorLocation()
                + FVector(
                    static_cast<float>(RecommendedGridX - CorePresentationOriginGrid.X) * CellSize,
                    static_cast<float>(RecommendedGridY - CorePresentationOriginGrid.Y) * CellSize,
                    0.0f);
            RecommendedLocation.Z = Character.GetActorLocation().Z;

            FVector FacingDirection = RecommendedLocation - Character.GetActorLocation();
            FacingDirection.Z = 0.0f;
            const FRotator RecommendedRotation = FacingDirection.IsNearlyZero()
                ? Character.GetActorRotation()
                : FacingDirection.Rotation();

            Runtime.EmergencyUseTransform = FTransform(
                RecommendedRotation,
                RecommendedLocation,
                FVector::OneVector);
        }
        else if (Intent == ELLActionIntent::Sleep)
        {
            int32 SleepGridX = 0;
            int32 SleepGridY = 0;
            int64 SleepFacilityId = 0;
            if (CoreBridge && CoreBridge->GetSettlementSleepUseTarget(
                    Character.GetResidentId(),
                    SleepGridX,
                    SleepGridY,
                    SleepFacilityId))
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Primitive;
                Runtime.bUsingEmergencyFallback = true;

                const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
                FVector SleepLocation = GetActorLocation()
                    + FVector(
                        static_cast<float>(SleepGridX - CorePresentationOriginGrid.X) * CellSize,
                        static_cast<float>(SleepGridY - CorePresentationOriginGrid.Y) * CellSize,
                        0.0f);
                SleepLocation.Z = Character.GetActorLocation().Z;

                FVector FacingDirection = SleepLocation - Character.GetActorLocation();
                FacingDirection.Z = 0.0f;
                const FRotator SleepRotation = FacingDirection.IsNearlyZero()
                    ? Character.GetActorRotation()
                    : FacingDirection.Rotation();

                Runtime.EmergencyUseTransform = FTransform(
                    SleepRotation,
                    SleepLocation,
                    FVector::OneVector);
            }
            else
            {
                Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Emergency;
                Runtime.bUsingEmergencyFallback = true;
                Runtime.EmergencyUseTransform = ResolveEmergencyFallbackTransform(Character, Intent);
            }
        }
        else
        {
            Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Emergency;
            Runtime.bUsingEmergencyFallback = true;
            Runtime.EmergencyUseTransform = ResolveEmergencyFallbackTransform(Character, Intent);
        }

        Runtime.PhysicalUseElapsedSeconds = 0.0f;
    }

    OutUseTransform = Runtime.EmergencyUseTransform;
    return true;
}

bool ALLWorldDirector::SupportsEmergencyFallback(ELLActionIntent Intent) const
{
    switch (Intent)
    {
        case ELLActionIntent::Eat:
        case ELLActionIntent::Drink:
        case ELLActionIntent::Sleep:
        case ELLActionIntent::Toilet:
        case ELLActionIntent::Hygiene:
            return true;
        case ELLActionIntent::Idle:
        case ELLActionIntent::Socialize:
        case ELLActionIntent::HaveFun:
        default:
            return false;
    }
}

FTransform ALLWorldDirector::ResolveEmergencyFallbackTransform(
    const ALLResidentCharacter& Character,
    ELLActionIntent Intent) const
{
    const FVector Location = Character.GetActorLocation();
    const FRotator Rotation = Character.GetActorRotation();
    return FTransform(Rotation, Location, FVector::OneVector);
}

FIntPoint ALLWorldDirector::WorldLocationToCoreGrid(const FVector& WorldLocation) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const FVector Relative = WorldLocation - GetActorLocation();
    return FIntPoint(
        CorePresentationOriginGrid.X + FMath::RoundToInt(Relative.X / CellSize),
        CorePresentationOriginGrid.Y + FMath::RoundToInt(Relative.Y / CellSize));
}

FVector ALLWorldDirector::CoreGridToWorldSpawnLocation(
    int32 GridX,
    int32 GridY,
    int32 PresentationSlot) const
{
    const float CellSize = FMath::Max(1.0f, CoreGridCellSizeUU);
    const float MaxSubCellRadius = CellSize * 0.42f;

    FVector SubCellOffset = FVector::ZeroVector;
    const int32 SafeSlot = FMath::Max(0, PresentationSlot);
    if (SafeSlot < 4)
    {
        const float XSign = (SafeSlot % 2) == 0 ? -1.0f : 1.0f;
        const float YSign = SafeSlot < 2 ? -1.0f : 1.0f;
        SubCellOffset = FVector(
            XSign * MaxSubCellRadius,
            YSign * MaxSubCellRadius,
            0.0f);
    }
    else
    {
        const int32 RingSlot = (SafeSlot - 4) % 8;
        const int32 Ring = ((SafeSlot - 4) / 8) + 1;
        const float AngleDegrees = static_cast<float>(RingSlot) * 45.0f
            + ((Ring % 2) == 1 ? 22.5f : 0.0f);
        const float AngleRadians = FMath::DegreesToRadians(AngleDegrees);
        const float RadiusScale = FMath::Min(1.0f, 0.55f + static_cast<float>(Ring - 1) * 0.12f);
        const float Radius = MaxSubCellRadius * RadiusScale;
        SubCellOffset = FVector(
            FMath::Cos(AngleRadians) * Radius,
            FMath::Sin(AngleRadians) * Radius,
            0.0f);
    }

    return GetActorLocation()
        + FVector(
            static_cast<float>(GridX - CorePresentationOriginGrid.X) * CellSize,
            static_cast<float>(GridY - CorePresentationOriginGrid.Y) * CellSize,
            90.0f)
        + SubCellOffset;
}

void ALLWorldDirector::ReleasePhysicalReservation(
    FGuid ResidentId,
    FLLResidentRuntimeState& Runtime)
{
    if (Runtime.ReservedAnchor.IsValid())
    {
        Runtime.ReservedAnchor->Release(ResidentId);
    }

    Runtime.ReservedAnchor.Reset();
    Runtime.ReservedIntent = ELLActionIntent::Idle;
    Runtime.ActiveAffordanceTier = ELLWorldAffordanceTier::Unavailable;
    Runtime.bUsingEmergencyFallback = false;
    Runtime.bUsingDesignatedSanitationSite = false;
    Runtime.CoreSanitationSiteId = 0;
    Runtime.EmergencyUseTransform = FTransform::Identity;
    Runtime.PhysicalUseElapsedSeconds = 0.0f;
}

FVector ALLWorldDirector::ResolveSocialTargetLocation(
    const ALLResidentCharacter& Character,
    const ALLResidentCharacter& Target,
    ELLCoreSocialIntent SocialIntent) const
{
    FVector AwayDirection = Character.GetActorLocation() - Target.GetActorLocation();
    AwayDirection.Z = 0.0f;
    if (AwayDirection.IsNearlyZero())
    {
        const bool bPositive = (GetTypeHash(Character.GetResidentId()) & 1u) == 0u;
        AwayDirection = bPositive ? FVector(1.0f, 0.0f, 0.0f) : FVector(0.0f, 1.0f, 0.0f);
    }
    AwayDirection.Normalize();

    if (SocialIntent == ELLCoreSocialIntent::Avoid)
    {
        return Character.GetActorLocation() + AwayDirection * 420.0f;
    }

    return Target.GetActorLocation() + AwayDirection * 120.0f;
}
