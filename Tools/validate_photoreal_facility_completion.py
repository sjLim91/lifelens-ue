from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp").read_text(encoding="utf-8")
header = (root / "Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h").read_text(encoding="utf-8")

for token in (
    "PhotorealStructureLogInstances",
    "PhotorealFurnaceStoneInstances",
    "AddPhotorealStructureLog",
    "AddPhotorealFurnaceStone",
    "bUsePhotorealSleepFrame",
    "bUsePhotorealFurnace",
):
    assert token in cpp or token in header, f"missing facility completion token: {token}"

# Desktop hero assets remain behind the explicit Android compile boundary.
structure_ref = 'SM_LL_dead_tree_trunk.SM_LL_dead_tree_trunk'
boulder_component = 'TEXT("PhotorealFurnaceStones"), PhotoBoulder.Object'
assert structure_ref in cpp
assert boulder_component in cpp
desktop_begin = cpp.index("#if !PLATFORM_ANDROID", cpp.index("PhotoWoodenAxe"))
desktop_end = cpp.index("#endif", desktop_begin)
desktop_block = cpp[desktop_begin:desktop_end]
assert structure_ref in desktop_block
assert "PhotoBoulder.Succeeded()" in cpp
android_furnace = cpp.index("#if PLATFORM_ANDROID", cpp.index("PhotorealStructureLogs"))
android_furnace_end = cpp.index("#else", android_furnace)
android_furnace_block = cpp[android_furnace:android_furnace_end]
assert "MobileRockA.Succeeded()" in android_furnace_block
assert boulder_component not in android_furnace_block

# Completed desktop facilities suppress the obvious Cube structure only when
# approved art exists, so Android/missing-art fallback remains readable.
assert "!bUsePhotorealSleepFrame && FacilityFoundationInstances" in cpp
assert "!bUsePhotorealFurnace && FacilityFoundationInstances" in cpp
assert "bStructurallyComplete && PhotorealStructureLogInstances" in cpp

# PrimitiveStorage contents are selected through the facility's authoritative
# LinkedStorageId. Relinking must invalidate the facility presentation even when
# the storage collection and its totals happen to remain otherwise unchanged.
assert "const uint64 LinkedStorageId = static_cast<uint64>(Facility.LinkedStorageId);" in cpp
assert "LinkedStorageId & 0xFFFFFFFFu" in cpp
assert "(LinkedStorageId >> 32)" in cpp

print("LifeLens photoreal facility completion: PASS")
