from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/World/LLWorldDirector.h").read_text(encoding="utf-8")
config = (root / "Config/DefaultGame.ini").read_text(encoding="utf-8")

for token in (
    "TSet<FIntPoint> Walkable;",
    'Chunk.Surface != FName(TEXT("Ocean"))',
    "Walkable.Add(FIntPoint(X, Y));",
    "const bool bGoalEnvironmentBlocked = Blocked.Contains(Goal);",
    "|| !Walkable.Contains(Next)",
    "!Walkable.Contains(SideX)",
    "!Walkable.Contains(SideY)",
    "Character.ClearMovementTarget();",
    "Do not bypass the authoritative route mask with a direct sweep.",
    "bHasFailedRouteTarget",
    "LastFailedRouteTarget",
    "NextRouteRetryWorldSeconds",
    "FailedRouteRetrySeconds",
    "SameFailedTargetToleranceUU",
    "GetMaterializedFreshSurfaceWaterTraversalObservations",
    "bBlocksGroundTraversal",
    "GroundHalfWidthCells",
    "GroundRadiusCells",
    "GroundAccessGridX",
    "GroundAccessGridY",
):
    assert token in cpp or token in (root / "Source/LifeLens/World/LLWorldDirector.h").read_text(encoding="utf-8"), \
        f"missing materialized-surface pathfinding guard: {token}"

assert "Character.SetMovementTarget(DesiredLocation);" not in cpp[
    cpp.index("void ALLWorldDirector::MoveResidentToward"):
    cpp.index("void ALLWorldDirector::CollectActivityAnchors")
], "A* failure must not fall back to an unsafe direct sweep"

endpoint_block = cpp[
    cpp.index("const bool bGoalEnvironmentBlocked"):
    cpp.index("const int32 Margin")
]
assert "Blocked.Remove(Goal);" in endpoint_block
assert endpoint_block.index("bGoalEnvironmentBlocked") < endpoint_block.index("Blocked.Remove(Goal);")
assert "!Walkable.Contains(Start)" in endpoint_block
assert "!Walkable.Contains(Goal)" in endpoint_block

astar_start = cpp.index("TArray<FVector> ALLWorldDirector::BuildLocalAStarPath")
astar_end = cpp.index("void ALLWorldDirector::MoveResidentToward", astar_start)
astar = cpp[astar_start:astar_end]
assert "SuggestedChannelWidthCells" not in astar
assert "SuggestedAreaRadiusCells" not in astar
assert "GetMaterializedSurfaceWaterPresentationObservations" in astar, (
    "marine routing still consumes marine orientation from presentation DTO"
)
assert "GetMaterializedFreshSurfaceWaterTraversalObservations" in astar, (
    "freshwater routing must consume the gameplay traversal DTO"
)

# Core macro terrain must influence route preference without allowing visual
# terrain presentation to become gameplay authority.
for token in (
    "TerrainTraversalEaseMaxStepCost",
    "TerrainElevationTransitionCostScale",
):
    assert token in header, f"terrain-aware path config missing: {token}"

for token in (
    "TMap<FIntPoint, float> ChunkTraversalEase",
    "TMap<FIntPoint, float> ChunkElevation",
    "Chunk.TraversalEase",
    "Chunk.Elevation",
    "TerrainMovePenalty",
    "AverageEase",
    "ElevationPenalty",
    "BaseMoveCost + TerrainPenalty",
):
    assert token in astar, f"terrain-aware A* contract missing: {token}"

for forbidden in (
    "FLLCoreTerrainPresentationObservation",
    "TerrainSurfaceZUU",
    "LocalSurfaceZUU",
    "RegionalSurfaceZUU",
):
    assert forbidden not in astar, (
        f"A* must not use presentation terrain as gameplay authority: {forbidden}"
    )

for token in (
    "TerrainTraversalEaseMaxStepCost=6",
    "TerrainElevationTransitionCostScale=40.000000",
):
    assert token in config, f"terrain-aware production path config missing: {token}"

print("LifeLens materialized surface pathfinding: PASS")
