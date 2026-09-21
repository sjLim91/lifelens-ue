#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
android_game = (root / "Config/Android/AndroidGame.ini").read_text(encoding="utf-8")

for token in (
    "MaxHeroTreeInstances = 8",
    "MaxHeroTreesPerChunk = 1",
    "HeroTreeCullStartUU = 5500.0f",
    "HeroTreeCullEndUU = 14000.0f",
    "HeroTreeMeshes",
    "HeroTreeInstances",
    "PlacedHeroTrees",
):
    assert token in header, f"photoreal canopy tier missing: {token}"

for token in (
    "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01/SM_LL_pachira_aquatica_01",
    "/Game/Environment/Photoreal/PolyHaven/island_tree_02/SM_LL_island_tree_02",
    "TreeMeshes.Add(PhotoTreePachira.Object)",
    "HeroTreeMeshes.Add(PhotoHeroIslandTree.Object)",
    "Place(HeroTreeInstances, HeroTreeCount, PlacedHeroTrees, MaxHeroTreeInstances",
):
    assert token in cpp, f"photoreal canopy runtime path missing: {token}"

# The heavyweight hero source must never enter the normal/far catalogue.
assert "TreeMeshes.Add(PhotoHeroIslandTree.Object)" not in cpp
assert "FarHeroTree" not in cpp

# Android must retain the lightweight mobile catalogue and explicitly exclude
# both newly imported desktop-only directories.
for asset_id in ("pachira_aquatica_01", "island_tree_02"):
    token = f'DirectoriesToNeverCook=(Path="/Game/Environment/Photoreal/PolyHaven/{asset_id}")'
    assert token in android_game, f"Android cook boundary missing: {asset_id}"

print("LifeLens photoreal canopy tiering: PASS")
