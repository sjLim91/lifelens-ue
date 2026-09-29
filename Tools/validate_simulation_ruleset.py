#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    return (root / path).read_text(encoding="utf-8")

rules = read("Source/LifeLensCore/include/lifelens/SimulationRuleset.h")
needs = read("Source/LifeLensCore/include/lifelens/Needs.h")
utility = read("Source/LifeLensCore/include/lifelens/UtilityAI.h")
sim_h = read("Source/LifeLensCore/include/lifelens/Simulation.h")
sim_cpp = read("Source/LifeLensCore/src/Simulation.cpp")
snapshot_versions_h = read("Source/LifeLensCore/include/lifelens/SimulationSnapshotVersions.h")
snapshot_h = read("Source/LifeLensCore/include/lifelens/SimulationSnapshot.h")
codec_h = read("Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h")
codec = read("Source/LifeLensCore/src/SimulationSnapshotCodec.cpp")
rules_codec = read("Source/LifeLensCore/include/lifelens/SimulationRulesetSnapshotCodec.h")
cmake = read("Source/LifeLensCore/CMakeLists.txt")

for token in (
    "struct NeedsRuleset", "struct UtilityAIRuleset", "struct SimulationRuleset",
    "CurrentSimulationRulesetVersion", "DefaultSimulationRuleset",
    "validSimulationRuleset", "sameSimulationRuleset",
):
    assert token in rules, f"missing Core ruleset contract: {token}"

for token in (
    "rules.hungerPerMinute*metabolism",
    "rules.thirstPerMinute*metabolism",
    "rules.sleepPerMinute*sleepTendency",
    "rules.bladderPerMinute*metabolism",
    "rules.hygienePerMinute",
):
    assert token in needs, f"missing ruleset-driven Needs contract: {token}"

assert "0.0010*metabolism" not in needs
assert "0.0013*metabolism" not in needs

for token in (
    "rules.needExponent",
    "rules.urgentThreshold",
    "rules.urgentSlope",
    "rules.idleScore",
    "rules.sleepNightMultiplier",
    "rules.secondChoiceProbability",
):
    assert token in utility, f"missing ruleset-driven UtilityAI contract: {token}"

assert "return 0.035;" not in utility
assert "(n-0.70)*1.8" not in utility

for token in (
    "SimulationRuleset ruleset=DefaultSimulationRuleset",
    "const SimulationRuleset& ruleset() const",
    "const SimulationRuleset ruleset_",
):
    assert token in sim_h, f"missing immutable Simulation ruleset ownership: {token}"

for token in (
    ":ruleset_(ruleset),world_",
    "chooseGoal(world_,c,ruleset_.utilityAI)",
    "c.needs.decay(ruleset_.needs,c.metabolism,c.sleepTendency)",
):
    assert token in sim_cpp, f"missing Simulation ruleset runtime wiring: {token}"

for token in (
    "SimulationSnapshotVersion",
    "SimulationSnapshotBinaryFormatVersion",
):
    assert token in snapshot_versions_h, f"missing centralized snapshot version contract: {token}"

assert "SimulationSnapshotVersions.h" in snapshot_h
assert "SimulationRuleset ruleset=DefaultSimulationRuleset" in snapshot_h
assert "SimulationSnapshotVersions.h" in codec_h
assert "MinimumSupportedSimulationSnapshotBinaryFormatVersion" not in codec_h

for token in (
    "writeSimulationRulesetSnapshotExtension",
    "readSimulationRulesetSnapshotExtension",
    "SimulationRulesetSnapshotExtensionMagic",
):
    assert token in codec, f"missing ruleset snapshot integration: {token}"

for obsolete in (
    "binaryVersion==1",
    "initializeLegacyCivilizationState",
    "patchBinaryFormatVersion",
    "decodeLegacyBodyForVersion",
):
    assert obsolete not in codec, f"pre-release snapshot migration must stay removed: {obsolete}"

for token in (
    "SimulationRulesetSnapshotExtensionMagic",
    "writeSimulationRulesetSnapshotExtension",
    "readSimulationRulesetSnapshotExtension",
    "validSimulationRuleset",
):
    assert token in rules_codec, f"missing ruleset snapshot extension contract: {token}"

# Runtime tuning/persistence authority is now entirely Core-owned. The retired
# Unreal Config/SaveGame bridge must never be required to validate the ruleset.
for retired in (
    "Source/LifeLens/",
    "Config/DefaultGame.ini",
    "ULLCoreBridgeSubsystem",
    "ULLSaveGame",
):
    assert retired not in __file__, "validator must remain renderer-independent"

assert "lifelens_add_test(test_snapshot_codec)" in cmake
assert "lifelens_add_test(test_needs)" in cmake
assert "lifelens_add_test(test_utility)" in cmake

print("SimulationRuleset + Core persistence structural validation: PASS")
