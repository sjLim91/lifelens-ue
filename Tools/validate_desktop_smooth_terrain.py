from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]

actor = (root / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp").read_text(encoding="utf-8")
for token in (
    "UProceduralMeshComponent",
    "GetMaterializedTerrainPresentationObservations",
    "GetMaterializedNaturalChunkObservations",
    "GetCivilizationWorldObservation",
    "CenterWeight",
    "FacilityCentersUU",
    "CreateMeshSection",
    "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
    "SetCanEverAffectNavigation(false)",
):
    assert token in actor, f"missing smooth-terrain token: {token}"

game_mode = (root / "Source/LifeLens/Core/LLLifeLensGameMode.cpp").read_text(encoding="utf-8")
assert "ALLDesktopTerrainPresentationActor" in game_mode
assert "#if !PLATFORM_ANDROID" in game_mode

build = (root / "Source/LifeLens/LifeLens.Build.cs").read_text(encoding="utf-8")
assert 'PrivateDependencyModuleNames.Add("ProceduralMeshComponent")' in build

header = (root / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.h").read_text(encoding="utf-8")
assert "UProceduralMeshComponent* TerrainMesh = nullptr;" in header
assert "TObjectPtr<UProceduralMeshComponent>" not in header

project = json.loads((root / "LifeLens.uproject").read_text(encoding="utf-8"))
plugin = next((p for p in project["Plugins"] if p["Name"] == "ProceduralMeshComponent"), None)
assert plugin is not None and plugin.get("Enabled") is True
allow = set(plugin.get("PlatformAllowList", []))
assert {"Win64", "Mac", "Linux"} <= allow
assert "Android" not in allow

print("LifeLens desktop smooth terrain: PASS")
