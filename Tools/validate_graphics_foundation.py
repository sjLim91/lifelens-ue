from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]


def read(path):
    return (root / path).read_text(encoding="utf-8")


def require(text, tokens, label):
    for token in tokens:
        assert token in text, f"{label}: missing {token}"


windows = read("Config/Windows/WindowsEngine.ini")
require(windows, (
    "DefaultGraphicsRHI=DefaultGraphicsRHI_DX12",
    "+D3D12TargetedShaderFormats=PCD3D_SM6",
    "r.GenerateMeshDistanceFields=True",
    "r.DynamicGlobalIlluminationMethod=1",
    "r.ReflectionMethod=1",
    "r.Shadow.Virtual.Enable=1",
    "r.AntiAliasingMethod=4",
    "r.Nanite.ProjectEnabled=True",
    "r.Lumen.HardwareRayTracing=False",
), "Windows cinematic renderer")

mac = read("Config/Mac/MacEngine.ini")
require(mac, (
    "r.GenerateMeshDistanceFields=True",
    "r.DynamicGlobalIlluminationMethod=1",
    "r.ReflectionMethod=1",
    "r.Shadow.Virtual.Enable=0",
    "r.AntiAliasingMethod=4",
    "r.Nanite.ProjectEnabled=False",
    "r.Lumen.HardwareRayTracing=False",
    "r.RayTracing=False",
), "macOS desktop cinematic renderer")

profiles = read("Config/DefaultDeviceProfiles.ini")
require(profiles, (
    "[Windows DeviceProfile]",
    "[Mac DeviceProfile]",
    "[Android DeviceProfile]",
), "cross-platform device profiles")

android = read("Config/Android/AndroidEngine.ini")
require(android, (
    "r.GenerateMeshDistanceFields=False",
    "r.DynamicGlobalIlluminationMethod=0",
    "r.ReflectionMethod=0",
    "r.Shadow.Virtual.Enable=0",
    "r.Nanite.ProjectEnabled=False",
    "r.Lumen.HardwareRayTracing=False",
    "r.RayTracing=False",
), "Android mobile renderer")

project = json.loads(read("LifeLens.uproject"))
plugins = {p["Name"] for p in project.get("Plugins", []) if p.get("Enabled")}
for plugin in ("Niagara", "PCG", "Water"):
    assert plugin in plugins, f"Unreal-native graphics plugin disabled: {plugin}"

diagnostics = read("Source/LifeLens/World/LLRendererDiagnosticsSubsystem.cpp")
require(diagnostics, (
    'ReadRendererCVar(TEXT("r.DynamicGlobalIlluminationMethod"))',
    'ReadRendererCVar(TEXT("r.ReflectionMethod"))',
    'ReadRendererCVar(TEXT("r.Shadow.Virtual.Enable"))',
    'ReadRendererCVar(TEXT("r.AntiAliasingMethod"))',
    'ReadRendererCVar(TEXT("r.Nanite.ProjectEnabled"))',
    'ReadRendererCVar(TEXT("r.GenerateMeshDistanceFields"))',
    'LifeLens renderer effective tier:',
    'IsCinematicMacTier',
    'LifeLens macOS desktop renderer contract is not fully active at runtime.',
), "runtime renderer diagnostics")

game_mode = read("Source/LifeLens/Core/LLLifeLensGameMode.cpp")
require(game_mode, (
    "FloorComponent->SetHiddenInGame(true, true);",
    "FloorComponent->SetCastShadow(false);",
    "ALLWaterPresentationActor",
), "runtime world bootstrap")
assert game_mode.index("FloorComponent->SetStaticMesh(RuntimeFloorMesh);") < game_mode.index(
    "FloorComponent->SetHiddenInGame(true, true);"
), "bootstrap collision floor must be hidden after its mesh is assigned"

water = read("Source/LifeLens/WorldPresentation/LLWaterPresentationActor.cpp")
require(water, (
    "GetMaterializedSurfaceWaterPresentationObservations()",
    "AWaterBodyRiver",
    "AWaterBodyLake",
    "WaterBody->SetActorEnableCollision(false);",
    "Component->SetCanEverAffectNavigation(false);",
    "ELLCoreSurfaceWaterKind::Ocean",
    "marineDeferred",
), "authoritative Unreal Water projection")

world_presentation = read(
    "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp")
require(world_presentation, (
    "SM_LL_fir_sapling",
    "SM_LL_pine_sapling_small",
    "SM_LL_boulder_01",
    "SM_LL_shrub_02",
    "SM_LL_shrub_03",
    "SM_LL_weed_plant_02",
), "photoreal natural presentation")
android_split = world_presentation.index("#if PLATFORM_ANDROID")
desktop_split = world_presentation.index("#else", android_split)
split_end = world_presentation.index("#endif", desktop_split)
android_nature = world_presentation[android_split:desktop_split]
desktop_nature = world_presentation[desktop_split:split_end]
assert "/Game/Environment/Quaternius/" in android_nature, (
    "Android presentation must keep an explicit lightweight nature set"
)
assert "/Game/Environment/Quaternius/" not in desktop_nature, (
    "desktop production WorldPresentation must not silently restore Quaternius local-view fallback"
)
assert "/Game/Environment/Photoreal/PolyHaven/" not in android_nature, (
    "Android nature block must not hard-reference desktop photoreal dressing"
)

print("LifeLens graphics foundation regression gate: PASS")
