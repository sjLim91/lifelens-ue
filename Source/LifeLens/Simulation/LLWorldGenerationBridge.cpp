#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Civilization.h"
#include "lifelens/Hydrology.h"
#include "lifelens/MacroWorldGenesis.h"
#include "lifelens/NaturalPhysicalObstacle.h"
#include "lifelens/NaturalWorldChunk.h"
#include "lifelens/Simulation.h"
#include "lifelens/WorldHierarchy.h"

namespace
{
const lifelens::ResourceNode* FindResourceNode(
    const lifelens::World& World,
    lifelens::ResourceNodeId Id)
{
    for (const lifelens::ResourceNode& Node : World.resourceNodes)
    {
        if (Node.id == Id)
        {
            return &Node;
        }
    }
    return nullptr;
}

ELLCoreSurfaceWaterKind ToUnrealSurfaceWaterKind(lifelens::SurfaceWaterKind Kind)
{
    switch (Kind)
    {
    case lifelens::SurfaceWaterKind::Spring: return ELLCoreSurfaceWaterKind::Spring;
    case lifelens::SurfaceWaterKind::Stream: return ELLCoreSurfaceWaterKind::Stream;
    case lifelens::SurfaceWaterKind::River: return ELLCoreSurfaceWaterKind::River;
    case lifelens::SurfaceWaterKind::Lake: return ELLCoreSurfaceWaterKind::Lake;
    case lifelens::SurfaceWaterKind::Wetland: return ELLCoreSurfaceWaterKind::Wetland;
    case lifelens::SurfaceWaterKind::Coast: return ELLCoreSurfaceWaterKind::Coast;
    case lifelens::SurfaceWaterKind::Ocean: return ELLCoreSurfaceWaterKind::Ocean;
    case lifelens::SurfaceWaterKind::None:
    default:
        return ELLCoreSurfaceWaterKind::None;
    }
}

ELLCoreWaterSalinity ToUnrealWaterSalinity(lifelens::WaterSalinity Salinity)
{
    switch (Salinity)
    {
    case lifelens::WaterSalinity::Brackish: return ELLCoreWaterSalinity::Brackish;
    case lifelens::WaterSalinity::Salt: return ELLCoreWaterSalinity::Salt;
    case lifelens::WaterSalinity::Fresh:
    default:
        return ELLCoreWaterSalinity::Fresh;
    }
}

void FillHydrologyObservation(
    const lifelens::HydrologyFacts& Facts,
    FLLCoreHydrologyObservation& Out)
{
    Out = FLLCoreHydrologyObservation{};
    Out.bAvailable = true;
    Out.ChunkX = Facts.coord.x;
    Out.ChunkY = Facts.coord.y;
    Out.SurfaceKind = ToUnrealSurfaceWaterKind(Facts.surfaceKind);
    Out.Salinity = ToUnrealWaterSalinity(Facts.salinity);
    Out.SurfaceWaterId = static_cast<int64>(Facts.surfaceWaterId);
    Out.SurfaceAvailability = static_cast<float>(Facts.surfaceAvailability);
    Out.FlowPotential = static_cast<float>(Facts.flowPotential);
    Out.GroundwaterPotential = static_cast<float>(Facts.groundwaterPotential);
    Out.RechargePotential = static_cast<float>(Facts.rechargePotential);
    Out.RunoffPotential = static_cast<float>(Facts.runoffPotential);
    Out.bHasDownstream = Facts.hasDownstream;
    Out.DownstreamChunkX = Facts.downstream.x;
    Out.DownstreamChunkY = Facts.downstream.y;
    Out.bFreshSurfaceWater = lifelens::isFreshSurfaceWater(Facts);
    Out.bHasMarineNeighbour = Facts.hasMarineNeighbour;
    Out.MarineNeighbourChunkX = Facts.marineNeighbour.x;
    Out.MarineNeighbourChunkY = Facts.marineNeighbour.y;
}

bool FillSurfaceWaterPresentationObservation(
    const lifelens::HydrologyFacts& Facts,
    FLLCoreSurfaceWaterPresentationObservation& Out)
{
    Out = FLLCoreSurfaceWaterPresentationObservation{};
    if (Facts.surfaceKind == lifelens::SurfaceWaterKind::None
        || Facts.surfaceWaterId == 0)
    {
        return false;
    }

    const lifelens::GridPos Origin = lifelens::chunkOriginGrid(Facts.coord);
    const int32 HalfChunk = lifelens::WorldChunkSpanGridCells / 2;

    Out.bAvailable = true;
    Out.SurfaceWaterId = static_cast<int64>(Facts.surfaceWaterId);
    Out.SurfaceKind = ToUnrealSurfaceWaterKind(Facts.surfaceKind);
    Out.Salinity = ToUnrealWaterSalinity(Facts.salinity);
    Out.ChunkX = Facts.coord.x;
    Out.ChunkY = Facts.coord.y;
    Out.CenterGridX = Origin.x + HalfChunk;
    Out.CenterGridY = Origin.y + HalfChunk;
    Out.SurfaceAvailability = static_cast<float>(Facts.surfaceAvailability);
    Out.FlowPotential = static_cast<float>(Facts.flowPotential);
    Out.bFreshSurfaceWater = lifelens::isFreshSurfaceWater(Facts);

    if (Facts.hasMarineNeighbour)
    {
        const lifelens::GridPos MarineOrigin =
            lifelens::chunkOriginGrid(Facts.marineNeighbour);
        Out.bHasMarineNeighbour = true;
        Out.MarineCenterGridX = MarineOrigin.x + HalfChunk;
        Out.MarineCenterGridY = MarineOrigin.y + HalfChunk;
    }

    switch (Facts.surfaceKind)
    {
    case lifelens::SurfaceWaterKind::Spring:
        Out.bLinearChannel = true;
        Out.SuggestedChannelWidthCells =
            0.65f + 0.85f * Out.SurfaceAvailability;
        break;
    case lifelens::SurfaceWaterKind::Stream:
        Out.bLinearChannel = true;
        Out.SuggestedChannelWidthCells =
            0.95f
            + 1.35f * Out.SurfaceAvailability
            + 0.45f * Out.FlowPotential;
        break;
    case lifelens::SurfaceWaterKind::River:
        Out.bLinearChannel = true;
        Out.SuggestedChannelWidthCells =
            1.85f
            + 2.75f * Out.SurfaceAvailability
            + 1.10f * Out.FlowPotential;
        break;
    case lifelens::SurfaceWaterKind::Lake:
        Out.SuggestedAreaRadiusCells =
            2.50f + 4.25f * Out.SurfaceAvailability;
        break;
    case lifelens::SurfaceWaterKind::Wetland:
        Out.SuggestedAreaRadiusCells =
            3.00f + 4.00f * Out.SurfaceAvailability;
        break;
    case lifelens::SurfaceWaterKind::Coast:
    case lifelens::SurfaceWaterKind::Ocean:
        Out.SuggestedAreaRadiusCells =
            static_cast<float>(lifelens::WorldChunkSpanGridCells) * 0.55f;
        break;
    case lifelens::SurfaceWaterKind::None:
    default:
        break;
    }

    if (Out.bLinearChannel && Facts.hasDownstream)
    {
        const lifelens::GridPos DownstreamOrigin =
            lifelens::chunkOriginGrid(Facts.downstream);
        Out.bHasDownstreamTarget = true;
        Out.DownstreamCenterGridX = DownstreamOrigin.x + HalfChunk;
        Out.DownstreamCenterGridY = DownstreamOrigin.y + HalfChunk;
    }

    return true;
}


float TerrainCornerElevation(
    const lifelens::WorldGenesisIdentity& Identity,
    lifelens::ChunkCoord A,
    lifelens::ChunkCoord B,
    lifelens::ChunkCoord C,
    lifelens::ChunkCoord D)
{
    return static_cast<float>((
        lifelens::deriveMacroRegionFacts(Identity, A).elevation
        + lifelens::deriveMacroRegionFacts(Identity, B).elevation
        + lifelens::deriveMacroRegionFacts(Identity, C).elevation
        + lifelens::deriveMacroRegionFacts(Identity, D).elevation) * 0.25);
}

void FillTerrainPresentationObservation(
    const lifelens::WorldGenesisIdentity& Identity,
    lifelens::ChunkCoord Center,
    FLLCoreTerrainPresentationObservation& Out)
{
    Out = FLLCoreTerrainPresentationObservation{};

    const lifelens::MacroRegionFacts CenterFacts =
        lifelens::deriveMacroRegionFacts(Identity, Center);
    const lifelens::ChunkCoord West{Center.x - 1, Center.y};
    const lifelens::ChunkCoord East{Center.x + 1, Center.y};
    const lifelens::ChunkCoord North{Center.x, Center.y + 1};
    const lifelens::ChunkCoord South{Center.x, Center.y - 1};
    const lifelens::ChunkCoord NorthWest{Center.x - 1, Center.y + 1};
    const lifelens::ChunkCoord NorthEast{Center.x + 1, Center.y + 1};
    const lifelens::ChunkCoord SouthWest{Center.x - 1, Center.y - 1};
    const lifelens::ChunkCoord SouthEast{Center.x + 1, Center.y - 1};

    const lifelens::GridPos Origin = lifelens::chunkOriginGrid(Center);
    const int32 HalfChunk = lifelens::WorldChunkSpanGridCells / 2;

    Out.bAvailable = true;
    Out.ChunkX = Center.x;
    Out.ChunkY = Center.y;
    Out.CenterGridX = Origin.x + HalfChunk;
    Out.CenterGridY = Origin.y + HalfChunk;
    Out.CenterElevation01 = static_cast<float>(CenterFacts.elevation);
    Out.NorthWestElevation01 = TerrainCornerElevation(
        Identity, Center, West, North, NorthWest);
    Out.NorthEastElevation01 = TerrainCornerElevation(
        Identity, Center, East, North, NorthEast);
    Out.SouthWestElevation01 = TerrainCornerElevation(
        Identity, Center, West, South, SouthWest);
    Out.SouthEastElevation01 = TerrainCornerElevation(
        Identity, Center, East, South, SouthEast);

    const float Minimum = FMath::Min(
        Out.CenterElevation01,
        FMath::Min(
            FMath::Min(Out.NorthWestElevation01, Out.NorthEastElevation01),
            FMath::Min(Out.SouthWestElevation01, Out.SouthEastElevation01)));
    const float Maximum = FMath::Max(
        Out.CenterElevation01,
        FMath::Max(
            FMath::Max(Out.NorthWestElevation01, Out.NorthEastElevation01),
            FMath::Max(Out.SouthWestElevation01, Out.SouthEastElevation01)));
    Out.Relief01 = FMath::Clamp(Maximum - Minimum, 0.0f, 1.0f);
}

void FillNaturalChunkObservation(
    const lifelens::World& World,
    const lifelens::GeneratedNaturalChunk& Chunk,
    FLLCoreNaturalChunkObservation& Out)
{
    Out = FLLCoreNaturalChunkObservation{};
    Out.bMaterialized = true;
    Out.VisualSeed = static_cast<int64>(
        Chunk.chunkSeed & 0x7fffffffffffffffULL);
    Out.ChunkX = Chunk.coord.x;
    Out.ChunkY = Chunk.coord.y;
    Out.Biome = FName(UTF8_TO_TCHAR(lifelens::macroBiomeName(Chunk.biome)));
    Out.Surface = FName(UTF8_TO_TCHAR(lifelens::naturalSurfaceKindName(Chunk.surface)));
    Out.Elevation = static_cast<float>(Chunk.elevation);
    Out.Moisture = static_cast<float>(Chunk.moisture);
    Out.Temperature = static_cast<float>(Chunk.temperature);
    Out.WaterPotential = static_cast<float>(Chunk.waterPotential);
    Out.FertilityPotential = static_cast<float>(Chunk.fertilityPotential);
    Out.TraversalEase = static_cast<float>(Chunk.traversalEase);
    Out.HazardPotential = static_cast<float>(Chunk.hazardPotential);
    Out.MaterializedMinute = Chunk.materializedMinute;
    Out.ResourcePatches.Reserve(static_cast<int32>(Chunk.resourcePatches.size()));

    for (const lifelens::NaturalResourcePatch& Patch : Chunk.resourcePatches)
    {
        FLLCoreNaturalResourcePatchObservation Item;
        Item.ResourceNodeId = static_cast<int64>(Patch.nodeId);
        Item.Material = FName(UTF8_TO_TCHAR(lifelens::materialName(Patch.material)));
        Item.GridX = Patch.pos.x;
        Item.GridY = Patch.pos.y;
        Item.MaxQuantity = Patch.maxQuantity;
        Item.bRenewable = Patch.renewable;
        Item.RegenerationPerDay = Patch.regenerationPerDay;
        Item.VisualDensity = static_cast<float>(Patch.visualDensity);
        Item.VisualSeed = static_cast<int64>(
            Patch.detailSeed & 0x7fffffffffffffffULL);
        if (const lifelens::ResourceNode* Node = FindResourceNode(World, Patch.nodeId))
        {
            Item.CurrentQuantity = Node->quantity;
        }
        Out.ResourcePatches.Add(Item);
    }

    const std::vector<lifelens::NaturalPhysicalObstacle> Obstacles =
        lifelens::deriveNaturalPhysicalObstacles(Chunk);
    Out.PhysicalObstacles.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
        Obstacles.size(), static_cast<std::size_t>(MAX_int32))));
    for (const lifelens::NaturalPhysicalObstacle& Obstacle : Obstacles)
    {
        FLLCoreNaturalObstacleObservation Read;
        Read.ObstacleId = static_cast<int64>(Obstacle.id);
        Read.Kind = Obstacle.kind == lifelens::NaturalPhysicalObstacleKind::Rock
            ? ELLCoreNaturalObstacleKind::Rock
            : ELLCoreNaturalObstacleKind::Tree;
        Read.SourceResourceNodeId = static_cast<int64>(Obstacle.sourceNodeId);
        Read.GridX = Obstacle.grid.x;
        Read.GridY = Obstacle.grid.y;
        Read.OffsetXCells = static_cast<float>(Obstacle.offsetXCells);
        Read.OffsetYCells = static_cast<float>(Obstacle.offsetYCells);
        Read.HalfExtentXCells = static_cast<float>(Obstacle.halfExtentXCells);
        Read.HalfExtentYCells = static_cast<float>(Obstacle.halfExtentYCells);
        Read.HalfHeightCells = static_cast<float>(Obstacle.halfHeightCells);
        Out.PhysicalObstacles.Add(Read);
    }
}
}


FLLCoreWorldHierarchyObservation
ULLCoreBridgeSubsystem::GetWorldHierarchyObservation() const
{
    FLLCoreWorldHierarchyObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::WorldGenesisIdentity Identity = World.genesisIdentity();
    const lifelens::PlanetIdentity Planet =
        lifelens::derivePrimaryPlanetIdentity(Identity);

    Result.bAvailable = lifelens::validPlanetIdentity(Planet);
    Result.PlanetId = static_cast<int64>(Planet.id);
    Result.PlanetSeed = static_cast<int64>(Planet.seed & 0x7fffffffffffffffULL);
    Result.SurfaceRegionSpanChunks = lifelens::SurfaceRegionSpanChunks;

    if (World.hasInitialStartRegionSelection)
    {
        const lifelens::SurfaceRegionIdentity Region =
            lifelens::deriveSurfaceRegionIdentityForChunk(
                Identity,
                World.initialStartRegionCoord);
        Result.bHasInitialSurfaceRegion =
            lifelens::validSurfaceRegionIdentity(Region);
        Result.InitialSurfaceRegionId = static_cast<int64>(Region.id);
        Result.InitialSurfaceRegionSeed =
            static_cast<int64>(Region.seed & 0x7fffffffffffffffULL);
        Result.InitialSurfaceRegionX = Region.coord.x;
        Result.InitialSurfaceRegionY = Region.coord.y;
    }
    return Result;
}

FLLCoreWorldGenerationObservation ULLCoreBridgeSubsystem::GetWorldGenerationObservation() const
{
    FLLCoreWorldGenerationObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    Result.bAvailable = true;
    Result.WorldSeed = static_cast<int64>(World.seed);
    Result.GenerationVersion = static_cast<int32>(World.generationVersion);
    Result.MaterializedChunkCount = static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(), static_cast<std::size_t>(MAX_int32)));
    Result.bHasInitialStartRegion = World.hasInitialStartRegionSelection;

    if (!World.hasInitialStartRegionSelection)
    {
        return Result;
    }

    Result.InitialChunkX = World.initialStartRegionCoord.x;
    Result.InitialChunkY = World.initialStartRegionCoord.y;
    Result.InitialViability = static_cast<float>(World.initialStartRegionViability);
    const lifelens::GridPos Center = World.initialStartRegionCenterGrid();
    Result.InitialCenterGridX = Center.x;
    Result.InitialCenterGridY = Center.y;

    if (const lifelens::GeneratedNaturalChunk* Chunk = World.findGeneratedNaturalChunk(World.initialStartRegionCoord))
    {
        FillNaturalChunkObservation(World, *Chunk, Result.InitialChunk);
    }
    return Result;
}

TArray<FLLCoreNaturalChunkObservation> ULLCoreBridgeSubsystem::GetMaterializedNaturalChunkObservations() const
{
    TArray<FLLCoreNaturalChunkObservation> Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(),
        static_cast<std::size_t>(MAX_int32))));

    // World::materializeNaturalChunk keeps this registry sorted by ChunkCoord,
    // so the projected list is deterministic and independent of exploration
    // request order.
    for (const lifelens::GeneratedNaturalChunk& Chunk : World.generatedNaturalChunks)
    {
        FLLCoreNaturalChunkObservation Observation;
        FillNaturalChunkObservation(World, Chunk, Observation);
        Result.Add(MoveTemp(Observation));
    }
    return Result;
}

bool ULLCoreBridgeSubsystem::GetNaturalChunkObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreNaturalChunkObservation& OutObservation) const
{
    OutObservation = FLLCoreNaturalChunkObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::ChunkCoord Coord{ChunkX, ChunkY};
    const lifelens::GeneratedNaturalChunk* Chunk = World.findGeneratedNaturalChunk(Coord);
    if (!Chunk)
    {
        return false;
    }

    FillNaturalChunkObservation(World, *Chunk, OutObservation);
    return true;
}


TArray<FLLCoreTerrainPresentationObservation>
ULLCoreBridgeSubsystem::GetMaterializedTerrainPresentationObservations() const
{
    TArray<FLLCoreTerrainPresentationObservation> Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::WorldGenesisIdentity Identity = World.genesisIdentity();
    Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(),
        static_cast<std::size_t>(MAX_int32))));

    for (const lifelens::GeneratedNaturalChunk& Chunk : World.generatedNaturalChunks)
    {
        FLLCoreTerrainPresentationObservation Observation;
        FillTerrainPresentationObservation(Identity, Chunk.coord, Observation);
        Result.Add(MoveTemp(Observation));
    }
    return Result;
}

TArray<FLLCoreTerrainPresentationObservation>
ULLCoreBridgeSubsystem::GetRegionalTerrainPreviewObservations(
    int32 RadiusChunks) const
{
    TArray<FLLCoreTerrainPresentationObservation> Empty;
    if (!CoreSimulation)
    {
        return Empty;
    }

    const lifelens::World& World = CoreSimulation->world();
    if (!World.hasInitialStartRegionSelection)
    {
        return Empty;
    }

    // Bound presentation cost independently from simulation authority.
    const int32 Radius = FMath::Clamp(RadiusChunks, 1, 16);
    const lifelens::WorldGenesisIdentity Identity = World.genesisIdentity();
    const lifelens::ChunkCoord Origin = World.initialStartRegionCoord;

    const bool bCacheHit =
        CachedRegionalTerrainRadiusChunks == Radius
        && CachedRegionalTerrainWorldSeed == Identity.worldSeed
        && CachedRegionalTerrainGenerationVersion
            == static_cast<int32>(Identity.generationVersion)
        && CachedRegionalTerrainStartChunkX == Origin.x
        && CachedRegionalTerrainStartChunkY == Origin.y
        && CachedRegionalTerrainPreview.Num() == (Radius * 2 + 1) * (Radius * 2 + 1);
    if (bCacheHit)
    {
        return CachedRegionalTerrainPreview;
    }

    TArray<FLLCoreTerrainPresentationObservation> Result;
    const int32 Diameter = Radius * 2 + 1;
    Result.Reserve(Diameter * Diameter);

    for (int32 Y = -Radius; Y <= Radius; ++Y)
    {
        for (int32 X = -Radius; X <= Radius; ++X)
        {
            FLLCoreTerrainPresentationObservation Observation;
            FillTerrainPresentationObservation(
                Identity,
                {Origin.x + X, Origin.y + Y},
                Observation);
            Result.Add(MoveTemp(Observation));
        }
    }

    CachedRegionalTerrainPreview = Result;
    CachedRegionalTerrainWorldSeed = Identity.worldSeed;
    CachedRegionalTerrainGenerationVersion =
        static_cast<int32>(Identity.generationVersion);
    CachedRegionalTerrainStartChunkX = Origin.x;
    CachedRegionalTerrainStartChunkY = Origin.y;
    CachedRegionalTerrainRadiusChunks = Radius;
    return Result;
}

bool ULLCoreBridgeSubsystem::GetTerrainPreviewObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreTerrainPresentationObservation& OutObservation) const
{
    OutObservation = FLLCoreTerrainPresentationObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    if (!World.hasInitialStartRegionSelection)
    {
        return false;
    }

    // Pure deterministic projection only. Do not materialize a simulation
    // chunk just because Presentation needs a continuation height.
    FillTerrainPresentationObservation(
        World.genesisIdentity(),
        {ChunkX, ChunkY},
        OutObservation);
    return OutObservation.bAvailable;
}

bool ULLCoreBridgeSubsystem::GetTerrainPresentationObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreTerrainPresentationObservation& OutObservation) const
{
    OutObservation = FLLCoreTerrainPresentationObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::ChunkCoord Coord{ChunkX, ChunkY};
    const lifelens::GeneratedNaturalChunk* Chunk =
        World.findGeneratedNaturalChunk(Coord);
    if (!Chunk)
    {
        return false;
    }

    FillTerrainPresentationObservation(
        World.genesisIdentity(), Chunk->coord, OutObservation);
    return true;
}

TArray<FLLCoreHydrologyObservation> ULLCoreBridgeSubsystem::GetMaterializedHydrologyObservations() const
{
    TArray<FLLCoreHydrologyObservation> Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(),
        static_cast<std::size_t>(MAX_int32))));

    for (const lifelens::GeneratedNaturalChunk& Chunk : World.generatedNaturalChunks)
    {
        const lifelens::HydrologyFacts Facts =
            lifelens::deriveHydrologyFacts(World.genesisIdentity(), Chunk.coord);
        FLLCoreHydrologyObservation Observation;
        FillHydrologyObservation(Facts, Observation);
        Result.Add(MoveTemp(Observation));
    }
    return Result;
}

bool ULLCoreBridgeSubsystem::GetHydrologyObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreHydrologyObservation& OutObservation) const
{
    OutObservation = FLLCoreHydrologyObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::ChunkCoord Coord{ChunkX, ChunkY};

    // Hydrology is deterministic for any coordinate, but the Unreal Local
    // Surface bridge intentionally exposes it only once Core has materialized
    // that chunk. Regional/planetary LOD will get a separate contract later.
    if (!World.findGeneratedNaturalChunk(Coord))
    {
        return false;
    }

    const lifelens::HydrologyFacts Facts =
        lifelens::deriveHydrologyFacts(World.genesisIdentity(), Coord);
    FillHydrologyObservation(Facts, OutObservation);
    return true;
}


TArray<FLLCoreSurfaceWaterPresentationObservation>
ULLCoreBridgeSubsystem::GetMaterializedSurfaceWaterPresentationObservations() const
{
    TArray<FLLCoreSurfaceWaterPresentationObservation> Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& World = CoreSimulation->world();
    Result.Reserve(static_cast<int32>(FMath::Min<std::size_t>(
        World.generatedNaturalChunks.size(),
        static_cast<std::size_t>(MAX_int32))));

    for (const lifelens::GeneratedNaturalChunk& Chunk : World.generatedNaturalChunks)
    {
        const lifelens::HydrologyFacts Facts =
            lifelens::deriveHydrologyFacts(World.genesisIdentity(), Chunk.coord);
        FLLCoreSurfaceWaterPresentationObservation Observation;
        if (FillSurfaceWaterPresentationObservation(Facts, Observation))
        {
            Result.Add(MoveTemp(Observation));
        }
    }
    return Result;
}

bool ULLCoreBridgeSubsystem::GetSurfaceWaterPresentationObservation(
    int32 ChunkX,
    int32 ChunkY,
    FLLCoreSurfaceWaterPresentationObservation& OutObservation) const
{
    OutObservation = FLLCoreSurfaceWaterPresentationObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::World& World = CoreSimulation->world();
    const lifelens::ChunkCoord Coord{ChunkX, ChunkY};
    if (!World.findGeneratedNaturalChunk(Coord))
    {
        return false;
    }

    const lifelens::HydrologyFacts Facts =
        lifelens::deriveHydrologyFacts(World.genesisIdentity(), Coord);
    return FillSurfaceWaterPresentationObservation(Facts, OutObservation);
}
