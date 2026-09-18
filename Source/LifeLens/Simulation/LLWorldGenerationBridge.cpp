#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Civilization.h"
#include "lifelens/NaturalPhysicalObstacle.h"
#include "lifelens/NaturalWorldChunk.h"
#include "lifelens/Simulation.h"

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

void FillNaturalChunkObservation(
    const lifelens::World& World,
    const lifelens::GeneratedNaturalChunk& Chunk,
    FLLCoreNaturalChunkObservation& Out)
{
    Out = FLLCoreNaturalChunkObservation{};
    Out.bMaterialized = true;
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
