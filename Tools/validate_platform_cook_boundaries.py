from pathlib import Path

root = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")


def require(text: str, tokens, label: str) -> None:
    for token in tokens:
        assert token in text, f"{label}: missing {token}"


android_game = read("Config/Android/AndroidGame.ini")
windows_game = read("Config/Windows/WindowsGame.ini")
mac_game = read("Config/Mac/MacGame.ini")

desktop_nature_dirs = (
    "/Game/Environment/Photoreal/PolyHaven/boulder_01",
    "/Game/Environment/Photoreal/PolyHaven/fir_sapling",
    "/Game/Environment/Photoreal/PolyHaven/pine_sapling_small",
    "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01",
    "/Game/Environment/Photoreal/PolyHaven/island_tree_02",
    "/Game/Environment/Photoreal/PolyHaven/shrub_02",
    "/Game/Environment/Photoreal/PolyHaven/shrub_03",
    "/Game/Environment/Photoreal/PolyHaven/tree_stump_01",
    "/Game/Environment/Photoreal/PolyHaven/weed_plant_02",
)

require(
    android_game,
    (
        'DirectoriesToNeverCook=(Path="/Game/Desktop")',
        'DirectoriesToNeverCook=(Path="/Game/Environment/PCG")',
        *[f'DirectoriesToNeverCook=(Path="{path}")' for path in desktop_nature_dirs],
    ),
    "Android cook exclusions",
)

assert 'DirectoriesToNeverCook=(Path="/Game/Environment/Photoreal")' not in android_game, (
    "Android must not exclude the complete photoreal root yet: the small "
    "facility hero set is intentionally shared until dedicated mobile props land"
)

require(
    windows_game,
    ('DirectoriesToNeverCook=(Path="/Game/Mobile")',),
    "Windows cook exclusions",
)
require(
    mac_game,
    ('DirectoriesToNeverCook=(Path="/Game/Mobile")',),
    "macOS cook exclusions",
)

world = read("Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp")
android_begin = world.index("#if PLATFORM_ANDROID")
desktop_begin = world.index("#else", android_begin)
platform_end = world.index("#endif", desktop_begin)

mobile_block = world[android_begin:desktop_begin]
desktop_block = world[desktop_begin:platform_end]
shared_after = world[platform_end:]

require(
    mobile_block,
    (
        "/Game/Environment/Quaternius/StylizedNature/Pine_1/",
        "/Game/Environment/Quaternius/StylizedNature/CommonTree_1/",
        "/Game/Environment/Quaternius/StylizedNature/Bush_Common/",
        "/Game/Environment/Quaternius/StylizedNature/Grass_Common_Short/",
        "/Game/Environment/Quaternius/StylizedNature/Grass_Wispy_Short/",
        "/Game/Environment/Quaternius/StylizedNature/Rock_Medium_1/",
        "/Game/Environment/Quaternius/StylizedNature/Rock_Medium_2/",
    ),
    "Android mobile nature refs",
)

assert "/Game/Environment/Photoreal/PolyHaven/fir_sapling/" not in mobile_block
assert "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01/" not in mobile_block
assert "/Game/Environment/Photoreal/PolyHaven/island_tree_02/" not in mobile_block
assert "/Game/Environment/Photoreal/PolyHaven/weed_plant_02/" not in mobile_block

mobile_asset_files = (
    "Content/Environment/Quaternius/StylizedNature/Pine_1/StaticMeshes/Pine_1.uasset",
    "Content/Environment/Quaternius/StylizedNature/CommonTree_1/StaticMeshes/CommonTree_1.uasset",
    "Content/Environment/Quaternius/StylizedNature/Bush_Common/StaticMeshes/Bush_Common.uasset",
    "Content/Environment/Quaternius/StylizedNature/Grass_Common_Short/StaticMeshes/Grass_Common_Short.uasset",
    "Content/Environment/Quaternius/StylizedNature/Grass_Wispy_Short/StaticMeshes/Grass_Wispy_Short.uasset",
    "Content/Environment/Quaternius/StylizedNature/Rock_Medium_1/StaticMeshes/Rock_Medium_1.uasset",
    "Content/Environment/Quaternius/StylizedNature/Rock_Medium_2/StaticMeshes/Rock_Medium_2.uasset",
)
for relative in mobile_asset_files:
    path = root / relative
    assert path.is_file(), f"Android runtime nature asset is missing from source: {relative}"
    assert path.stat().st_size > 1024, f"Android runtime nature asset is unexpectedly empty/tiny: {relative}"

# The Android exclusion list must stay selective. A broad Quaternius or
# StylizedNature exclusion would make ConstructorHelpers references compile while
# producing a packaged runtime with no trees/ground cover.
for forbidden in (
    '/Game/Environment/Quaternius',
    '/Game/Environment/Quaternius/StylizedNature',
):
    assert f'DirectoriesToNeverCook=(Path="{forbidden}")' not in android_game, (
        f"Android cook must retain runtime nature assets; broad exclusion found: {forbidden}"
    )

require(
    desktop_block,
    (
        "/Game/Environment/Photoreal/PolyHaven/fir_sapling/",
        "/Game/Environment/Photoreal/PolyHaven/pine_sapling_small/",
        "/Game/Environment/Photoreal/PolyHaven/pachira_aquatica_01/",
        "/Game/Environment/Photoreal/PolyHaven/island_tree_02/",
        "/Game/Environment/Photoreal/PolyHaven/boulder_01/",
        "/Game/Environment/Photoreal/PolyHaven/shrub_02/",
        "/Game/Environment/Photoreal/PolyHaven/shrub_03/",
        "/Game/Environment/Photoreal/PolyHaven/weed_plant_02/",
    ),
    "Desktop photoreal nature refs",
)

require(
    shared_after,
    (
        "/Game/Environment/Photoreal/PolyHaven/stone_fire_pit/",
        "/Game/Environment/Photoreal/PolyHaven/wicker_basket_01/",
        "/Game/Environment/Photoreal/PolyHaven/wooden_axe/",
    ),
    "Shared compact facility hero refs",
)

# Quantify that the boundary is meaningful in the current source tree.
excluded_source_bytes = 0
for virtual_dir in desktop_nature_dirs:
    disk_dir = root / "Content" / virtual_dir.removeprefix("/Game/")
    if disk_dir.exists():
        excluded_source_bytes += sum(
            p.stat().st_size for p in disk_dir.rglob("*") if p.is_file()
        )

assert excluded_source_bytes > 25 * 1024 * 1024, (
    f"Expected >25 MiB of desktop nature source behind the Android cook boundary, "
    f"found {excluded_source_bytes} bytes"
)

default_game = read("Config/DefaultGame.ini")
assert "bCookAll=True" not in default_game, (
    "CookAll would weaken platform content-boundary guarantees"
)

android_workflow = read(".github/workflows/android-apk.yml")
require(
    android_workflow,
    ("python3 Tools/validate_platform_cook_boundaries.py",),
    "Android packaging workflow cook-boundary gate",
)

print(
    "LifeLens platform cook boundary: PASS "
    f"(desktop nature excluded from Android source set: "
    f"{excluded_source_bytes / (1024 * 1024):.1f} MiB)"
)
