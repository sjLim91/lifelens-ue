#!/usr/bin/env python3
from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]

def text(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

workflow = text(".github/workflows/android-apk.yml")
engine = text("Config/DefaultEngine.ini")
android_game = text("Config/Android/AndroidGame.ini")
project = json.loads(text("LifeLens.uproject"))

for token in (
    "workflow_dispatch:",
    "- container",
    "tasks/ANDROID_BUILD_REQUEST.md",
    "ghcr.io/epicgames/unreal-engine:dev-slim-5.6.0",
    "ANDROID_API: '34'",
    "ANDROID_BUILD_TOOLS: '34.0.0'",
    "ANDROID_NDK_VERSION: '25.1.8937393'",
    "python3 Tools/validate_platform_cook_boundaries.py",
    "python3 Tools/validate_zero_cost_asset_provenance.py",
    "LifeLens Android Development",
    "BuildCookRun",
    "-targetplatform=Android",
    "-cookflavor=ASTC",
    "-clientconfig=Development",
    "-build -cook -stage -pak -package -archive",
    "Verify APK exists",
    "Validate packaged APK integrity",
    "dump badging",
    "package: name='com.lifelens.sim'",
    "sdkVersion:'30'",
    "targetSdkVersion:'34'",
    "launchable-activity:",
    "lib/arm64-v8a/",
    "lib/x86_64/",
    "apksigner",
    "verify --verbose --print-certs",
    "LIFELENS_APK_FILE_PRESENT",
    "LIFELENS_APK_INTEGRITY_PASS",
    "LIFELENS_APK_PIPELINE_PASS",
):
    assert token in workflow, f"Android build workflow missing required gate: {token}"

assert "Seed Linux cook tools" not in workflow, (
    "Android pipeline regressed to rebuilding the full Linux UnrealEditor from source"
)
assert "git clone --depth 1 --single-branch --branch" not in workflow, (
    "container Android path must reuse the prebuilt UE host instead of cloning/rebuilding Engine"
)

compile_pos = workflow.index("Compile LifeLens Android Development")
package_pos = workflow.index("Cook and package LifeLens APK")
exists_pos = workflow.index("Verify APK exists")
integrity_pos = workflow.index("Validate packaged APK integrity")
upload_pos = workflow.index("Upload LifeLens APK artifact")
release_pos = workflow.index("Publish LifeLens APK release")
assert compile_pos < package_pos < exists_pos < integrity_pos < upload_pos < release_pos, (
    "Android build gates must run compile -> package -> exists -> integrity -> upload -> release"
)
assert workflow.index("LIFELENS_APK_FILE_PRESENT") < integrity_pos
assert workflow.index("LIFELENS_APK_PIPELINE_PASS") > integrity_pos, (
    "Android pipeline must not report PASS before APK integrity verification"
)

for token in (
    "PackageName=com.lifelens.sim",
    "MinSDKVersion=30",
    "TargetSDKVersion=34",
    "bBuildForArm64=True",
    "bBuildForX8664=False",
    "bPackageDataInsideApk=True",
    "Orientation=SensorLandscape",
):
    assert token in engine, f"Android runtime setting missing: {token}"

world_map = root / "Content/Maps/LifeLensWorld.umap"
assert world_map.is_file(), "Default Android launch map is missing"
assert world_map.stat().st_size > 1024, "Default Android launch map looks empty"
assert "GameDefaultMap=/Game/Maps/LifeLensWorld" in engine

plugins = {entry["Name"]: entry for entry in project.get("Plugins", [])}
assert plugins.get("Water", {}).get("Enabled") is True, "Water plugin must stay enabled"
for desktop_only in ("PCG", "ProceduralMeshComponent"):
    plugin = plugins.get(desktop_only, {})
    assert plugin.get("Enabled") is True, f"{desktop_only} plugin metadata missing"
    allow = set(plugin.get("PlatformAllowList", []))
    assert "Android" not in allow, f"{desktop_only} must remain outside the Android plugin load set"

assert 'DirectoriesToNeverCook=(Path="/Game/Desktop")' in android_game
assert 'DirectoriesToNeverCook=(Path="/Game/Environment/PCG")' in android_game
assert 'DirectoriesToNeverCook=(Path="/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01")' in android_game
assert 'DirectoriesToNeverCook=(Path="/Game/Environment/Photoreal/PolyHaven/island_tree_02")' in android_game

print("LifeLens Android container build/APK integrity contract: PASS")
