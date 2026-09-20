from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORLD_H = (ROOT / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
WORLD_CPP = (ROOT / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
WATER_CPP = (ROOT / "Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp").read_text(encoding="utf-8")
TERRAIN_CPP = (ROOT / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp").read_text(encoding="utf-8")

for token in (
    "ClearProjectedWorld",
    "bHasProjectedWorld",
):
    assert token in WORLD_H, f"world stale-projection contract missing: {token}"

for token in (
    "!Bridge || !Bridge->IsCoreRunning()",
    "ClearProjectedWorld();",
    "Ground->SetVisibility(false, true);",
    "Ground->SetHiddenInGame(true, true);",
    "FarGround->SetVisibility(false, true);",
    "bProjectionNeedsBuild",
    "bHasProjectedWorld = true;",
):
    assert token in WORLD_CPP, f"world stale-projection cleanup missing: {token}"

for token in (
    "ClearStaleWater",
    "ClearProjectedWater();",
    "bBuiltOnce = false;",
    "BuiltSignature = 0;",
):
    assert token in WATER_CPP, f"water stale-projection cleanup missing: {token}"

for token in (
    "ClearStaleTerrain",
    "TerrainMesh->ClearAllMeshSections();",
    "LastSignature = 0;",
    "bBuiltOnce = false;",
):
    assert token in TERRAIN_CPP, f"terrain stale-projection cleanup missing: {token}"

print("LifeLens stale world projection cleanup: PASS")
