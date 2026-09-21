from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]

actor = (root / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.cpp").read_text(encoding="utf-8")
terrain_contract = (root / "Source/LifeLens/WorldPresentation/LLTerrainPresentationContract.h").read_text(encoding="utf-8")
for token in (
    "UProceduralMeshComponent",
    "GetMaterializedTerrainPresentationObservations",
    "GetMaterializedNaturalChunkObservations",
    "GetCivilizationWorldObservation",
    "LLTerrainPresentationContract::LocalSurfaceZUU",
    "FacilityCentersUU",
    "CreateMeshSection",
    "SetCollisionEnabled(ECollisionEnabled::QueryOnly)",
    "SetCollisionResponseToAllChannels(ECR_Ignore)",
    "SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block)",
    "SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore)",
    "SetGenerateOverlapEvents(false)",
    "SetCanEverAffectNavigation(false)",
):
    assert token in actor, f"missing smooth-terrain token: {token}"

for token in (
    "Terrain.bAvailable",
    "Terrain.NorthWestElevation01",
    "Terrain.NorthEastElevation01",
    "Terrain.SouthWestElevation01",
    "Terrain.SouthEastElevation01",
    "World.InitialChunkX",
    "World.InitialChunkY",
    "World.InitialCenterGridX",
    "World.InitialCenterGridY",
    "World.InitialChunk.Elevation",
):
    assert token in actor, f"smooth-terrain refresh signature misses rendered input: {token}"

game_mode = (root / "Source/LifeLens/Core/LLLifeLensGameMode.cpp").read_text(encoding="utf-8")
assert "ALLDesktopTerrainPresentationActor" in game_mode
assert "#if !PLATFORM_ANDROID" in game_mode

build = (root / "Source/LifeLens/LifeLens.Build.cs").read_text(encoding="utf-8")
assert 'PrivateDependencyModuleNames.Add("ProceduralMeshComponent")' in build

header = (root / "Source/LifeLens/WorldPresentation/LLDesktopTerrainPresentationActor.h").read_text(encoding="utf-8")
assert "UProceduralMeshComponent* TerrainMesh = nullptr;" in header
assert "TObjectPtr<UProceduralMeshComponent>" not in header
assert "SettlementFlattenRadiusUU" not in header
assert "SettlementBlendBandUU" not in header
assert "LocalReliefAmplitudeUU = 220.0f" in terrain_contract
assert "SettlementFlattenRadiusUU" not in terrain_contract
assert "SettlementBlendBandUU" not in terrain_contract
assert "FacilityReliefBlend(" in terrain_contract
assert "FMath::Lerp(CenterSurface, CornerSurface, 0.72f)" in terrain_contract

world_presentation = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
world_presentation_header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
assert "TerrainReliefFlattenRadiusUU" not in world_presentation_header
assert "TerrainReliefBlendBandUU" not in world_presentation_header
assert "LLTerrainPresentationContract::LocalSurfaceZUU" in world_presentation
assert "MobileTerrainTilesPerAxis = 4" in world_presentation, (
    "Android terrain must not regress to one flat 3200-UU chunk tile"
)
assert "TerrainTileRotation(World, Terrain, TileCenterUU, TileSpanUU)" in world_presentation
assert "for (int32 TileY = 0; TileY < MobileTerrainTilesPerAxis; ++TileY)" in world_presentation
assert "for (int32 TileX = 0; TileX < MobileTerrainTilesPerAxis; ++TileX)" in world_presentation
chunk_start = world_presentation.index("void ALLWorldPresentationActor::BuildChunkGround")
chunk_end = world_presentation.index("void ALLWorldPresentationActor::BuildFarEnvironment", chunk_start)
chunk_block = world_presentation[chunk_start:chunk_end]
assert "#if !PLATFORM_ANDROID" in chunk_block
assert "Keep them strictly mobile-only." in chunk_block
assert "LocalGroundUnderlayDropUU = 3.0f" in world_presentation

dynamic_environment = (root / "Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp").read_text(encoding="utf-8")
assert "ALLDesktopTerrainPresentationActor" in dynamic_environment
assert "ApplyWeatherParameters" in dynamic_environment
assert "SetScalarParameterValueOnMaterials(WetnessMaterialParameter" in dynamic_environment
assert "SetScalarParameterValueOnMaterials(SnowMaterialParameter" in dynamic_environment

project = json.loads((root / "LifeLens.uproject").read_text(encoding="utf-8"))
plugin = next((p for p in project["Plugins"] if p["Name"] == "ProceduralMeshComponent"), None)
assert plugin is not None and plugin.get("Enabled") is True
allow = set(plugin.get("PlatformAllowList", []))
assert {"Win64", "Mac", "Linux"} <= allow
assert "Android" not in allow

section_pos = actor.index("TerrainMesh->CreateMeshSection")
section_block = actor[section_pos:section_pos + 500]
assert "Tangents,\n            true);" in section_block, (
    "desktop smooth terrain must create query geometry for visual surface projection"
)

print("LifeLens desktop smooth terrain: PASS")
