from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]


def read(path):
    return (root / path).read_text(encoding="utf-8")


actor = read("Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.cpp")
for token in (
    "PCG_LL_GroundCover",
    "GetNaturalChunkObservation",
    "InitialChunk.VisualSeed",
    "PCG->Seed",
    "GenerateLocal(true)",
    "if (!bEnableGroundCoverPCG)",
    "#if PLATFORM_ANDROID",
):
    assert token in actor, f"missing desktop PCG runtime token: {token}"

game_mode = read("Source/LifeLens/Core/LLLifeLensGameMode.cpp")
assert "#if !PLATFORM_ANDROID" in game_mode
assert "ALLPCGGroundCoverPresentationActor" in game_mode

build = read("Source/LifeLens/LifeLens.Build.cs")
for token in (
    "UnrealTargetPlatform.Win64",
    "UnrealTargetPlatform.Mac",
    "UnrealTargetPlatform.Linux",
    'PrivateDependencyModuleNames.Add("PCG")',
):
    assert token in build, f"desktop-only PCG module contract missing: {token}"

project = json.loads(read("LifeLens.uproject"))
pcg = next((p for p in project["Plugins"] if p["Name"] == "PCG"), None)
assert pcg is not None and pcg.get("Enabled") is True
allow = set(pcg.get("PlatformAllowList", []))
assert {"Win64", "Mac", "Linux"} <= allow
assert "Android" not in allow

header = read("Source/LifeLens/WorldPresentation/LLPCGGroundCoverPresentationActor.h")
assert "bEnableGroundCoverPCG = false" in header, (
    "known-bad weed graph must fail closed until regenerated"
)

pcg_import = read("Content/Environment/PCG/Import/create_lifelens_groundcover_pcg.py")
assert "Grass_Common_Tall" in pcg_import
assert "weed_plant_02" not in pcg_import

print("LifeLens desktop PCG runtime dressing: PASS")
