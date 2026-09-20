from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")

# Desktop gets a higher visual budget while Android keeps its previous caps.
for token in (
    "MaxFarTreeInstances = 320",
    "MaxFarRockInstances = 160",
    "MaxTreeInstances  = 1480",
    "MaxShrubInstances = 2100",
    "MaxGrassInstances = 6400",
    "MaxRockInstances  = 1650",
):
    assert token in header, f"missing desktop density budget: {token}"

for token in (
    "MaxTreeInstances  = 620",
    "MaxShrubInstances = 760",
    "MaxGrassInstances = 1800",
    "MaxRockInstances  = 720",
):
    assert token in header, f"Android density budget changed unexpectedly: {token}"

# Whole-world dressing remains deterministic and presentation-only.
for token in (
    "AmbientDensityGain",
    "RockDensityGain",
    "ClusterAnchor",
    "ClusterRemaining",
    "ClusterRadiusMax",
    "AmbientDressingKeepFactor",
):
    assert token in cpp, f"missing environment density token: {token}"

assert "Core resource truth" in cpp

# Natural dressing rebuild keys must cover every authoritative value that
# controls count/placement/height. Otherwise unchanged IDs can leave stale art.
for token in (
    "Chunk.bMaterialized",
    "Chunk.FertilityPotential",
    "Chunk.TraversalEase",
    "Patch.VisualDensity",
    "Patch.VisualSeed",
    "TerrainPresentationSignature",
    "Terrain.NorthWestElevation01",
    "Terrain.NorthEastElevation01",
    "Terrain.SouthWestElevation01",
    "Terrain.SouthEastElevation01",
    "BuiltTerrainPresentationSignature",
):
    assert token in cpp or token in header, (
        f"world-presentation refresh signature misses rendered input: {token}"
    )

print("LifeLens environment density v1: PASS")
