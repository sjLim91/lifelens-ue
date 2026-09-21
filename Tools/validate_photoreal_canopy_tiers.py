#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
android_game = (root / "Config/Android/AndroidGame.ini").read_text(encoding="utf-8")

required_header = (
    "MaxPhotorealBroadleafInstances = 260",
    "MaxPhotorealBroadleafTreesPerChunk = 32",
    "MaxHeroTreeInstances = 8",
    "MaxHeroTreesPerChunk = 1",
    "PhotorealBroadleafInstances",
    "PhotorealHeroTreeInstances",
    "PlacedPhotorealBroadleafTrees",
    "PlacedHeroTrees",
)
for token in required_header:
    assert token in header, f"photoreal canopy header contract missing: {token}"

for token in (
    "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01/SM_LL_pachira_aquatica_01",
    "/Game/Environment/Photoreal/PolyHaven/island_tree_02/SM_LL_island_tree_02",
    'TEXT("PhotorealBroadleaf_Pachira")',
    'TEXT("PhotorealHero_IslandTree")',
    "FMath::RoundToInt(static_cast<float>(TreeCount) * 0.28f)",
    "bHeroBiomeSuitable",
    "TreeCount >= 72",
    "Place(\n        PhotorealBroadleafInstances",
    "Place(\n        PhotorealHeroTreeInstances",
):
    assert token in cpp, f"photoreal canopy runtime contract missing: {token}"

# The high-detail island tree must never enter the generic ambient/far catalogue.
assert "TreeMeshes.Add(PhotoHeroIslandTree.Object)" not in cpp
assert "FarTreeInstances.Add(PhotoHeroIslandTree" not in cpp

# Both new photoreal tree packages are desktop-only; Android retains the compact
# mobile catalogue and cannot accidentally cook the heavy desktop source.
for virtual_dir in (
    "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01",
    "/Game/Environment/Photoreal/PolyHaven/island_tree_02",
):
    exclusion = f'DirectoriesToNeverCook=(Path="{virtual_dir}")'
    assert exclusion in android_game, f"Android canopy cook exclusion missing: {virtual_dir}"

# Authoritative Core trees still use TreeInstances; photoreal tiers remain
# presentation-only and participate in the same reversible observer sightline
# registration through the shared Place() path.
assert "RegisterDynamicCanopyInstance(Component, InstanceIndex, InstanceTransform)" in cpp
assert "Chunk.ResourcePatches" in cpp

print("LifeLens photoreal canopy tier contract: PASS")
