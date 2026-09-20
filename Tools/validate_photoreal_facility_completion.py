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

# Completed timber hero art is shared with Android so the mobile-first target
# does not regress to stretched Engine cubes. Furnace masonry reuses the
# already-cooked mobile rock while desktop keeps the photoreal boulder.
structure_ref = 'SM_LL_dead_tree_trunk.SM_LL_dead_tree_trunk'
structure_component = 'TEXT("PhotorealStructureLogs"), PhotoStructureLog.Object'
boulder_component = 'TEXT("PhotorealFurnaceStones"), PhotoBoulder.Object'
assert structure_ref in cpp
assert structure_component in cpp
assert boulder_component in cpp

structure_ref_pos = cpp.index(structure_ref)
nearest_android_guard = cpp.rfind("#if !PLATFORM_ANDROID", 0, structure_ref_pos)
nearest_guard_end = cpp.rfind("#endif", 0, structure_ref_pos)
assert nearest_android_guard <= nearest_guard_end, (
    "completed structure log hard reference regressed behind desktop-only guard"
)

structure_component_pos = cpp.index(structure_component)
nearest_component_guard = cpp.rfind("#if !PLATFORM_ANDROID", 0, structure_component_pos)
nearest_component_guard_end = cpp.rfind("#endif", 0, structure_component_pos)
assert nearest_component_guard <= nearest_component_guard_end, (
    "PhotorealStructureLogs component regressed behind desktop-only guard"
)

android_furnace = cpp.index("#if PLATFORM_ANDROID", cpp.index("PhotorealStructureLogs"))
android_furnace_end = cpp.index("#else", android_furnace)
android_furnace_block = cpp[android_furnace:android_furnace_end]
assert "MobileRockA.Succeeded()" in android_furnace_block
assert boulder_component not in android_furnace_block

desktop_furnace_end = cpp.index("#endif", android_furnace_end)
desktop_furnace_block = cpp[android_furnace_end:desktop_furnace_end]
assert "PhotoBoulder.Succeeded()" in desktop_furnace_block
assert boulder_component in desktop_furnace_block

# Completed facilities suppress the obvious Cube structure when approved hero
# art exists, and incomplete facilities use the same approved staged materials
# on Android/desktop before falling back to compact primitives.
assert "!bUsePhotorealSleepFrame && FacilityFoundationInstances" in cpp
assert "!bUsePhotorealFurnace && FacilityFoundationInstances" in cpp
assert "bStructurallyComplete && PhotorealStructureLogInstances" in cpp
for token in (
    "bUsePhotorealConstructionStaging",
    "Facility.Kind == ELLCoreFacilityKind::FirePit",
    "PhotorealFurnaceStoneInstances != nullptr",
    "PhotorealStructureLogInstances != nullptr",
    "The staged hero material itself communicates Planned",
):
    assert token in cpp, f"mobile construction staging regression: {token}"

staging_start = cpp.index("const bool bStoneConstruction")
staging_end = cpp.index("// Ruins stay visible", staging_start)
staging_block = cpp[staging_start:staging_end]
assert "#if !PLATFORM_ANDROID" not in staging_block, (
    "approved construction hero staging regressed to desktop-only"
)
assert "continue;" in staging_block, (
    "hero-staged construction still falls through to Engine-cube structure"
)

# PrimitiveStorage contents are selected through the facility's authoritative
# LinkedStorageId. Relinking must invalidate the facility presentation even when
# the storage collection and its totals happen to remain otherwise unchanged.
assert "const uint64 LinkedStorageId = static_cast<uint64>(Facility.LinkedStorageId);" in cpp
assert "LinkedStorageId & 0xFFFFFFFFu" in cpp
assert "(LinkedStorageId >> 32)" in cpp

print("LifeLens photoreal facility completion: PASS")
