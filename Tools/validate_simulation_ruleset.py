#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

def read(path):
    return (root / path).read_text(encoding='utf-8')

rules = read('Source/LifeLensCore/include/lifelens/SimulationRuleset.h')
needs = read('Source/LifeLensCore/include/lifelens/Needs.h')
utility = read('Source/LifeLensCore/include/lifelens/UtilityAI.h')
sim_h = read('Source/LifeLensCore/include/lifelens/Simulation.h')
sim_cpp = read('Source/LifeLensCore/src/Simulation.cpp')
snapshot_h = read('Source/LifeLensCore/include/lifelens/SimulationSnapshot.h')
codec_h = read('Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h')
codec = read('Source/LifeLensCore/src/SimulationSnapshotCodec.cpp')
rules_codec = read('Source/LifeLensCore/include/lifelens/SimulationRulesetSnapshotCodec.h')
bridge_h = read('Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h')
bridge_cpp = read('Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp')
persistence = read('Source/LifeLens/Simulation/LLCoreBridgePersistence.cpp')
save_h = read('Source/LifeLens/Save/LLSaveGame.h')
sim_ue = read('Source/LifeLens/Simulation/LLSimulationSubsystem.cpp')
game_ini = read('Config/DefaultGame.ini')

for token in (
    'struct NeedsRuleset', 'struct UtilityAIRuleset', 'struct SimulationRuleset',
    'CurrentSimulationRulesetVersion', 'DefaultSimulationRuleset',
    'validSimulationRuleset', 'sameSimulationRuleset'):
    assert token in rules, f'missing Core ruleset contract: {token}'

for token in (
    'rules.hungerPerMinute*metabolism', 'rules.thirstPerMinute*metabolism',
    'rules.sleepPerMinute*sleepTendency', 'rules.bladderPerMinute*metabolism',
    'rules.hygienePerMinute'):
    assert token in needs, f'missing ruleset-driven Needs contract: {token}'
assert '0.0010*metabolism' not in needs
assert '0.0013*metabolism' not in needs

for token in (
    'rules.needExponent', 'rules.urgentThreshold', 'rules.urgentSlope',
    'rules.idleScore', 'rules.sleepNightMultiplier',
    'rules.secondChoiceProbability'):
    assert token in utility, f'missing ruleset-driven UtilityAI contract: {token}'
assert 'return 0.035;' not in utility
assert '(n-0.70)*1.8' not in utility

for token in (
    'SimulationRuleset ruleset=DefaultSimulationRuleset',
    'const SimulationRuleset& ruleset() const',
    'const SimulationRuleset ruleset_'):
    assert token in sim_h, f'missing immutable Simulation ruleset ownership: {token}'
for token in (
    ':ruleset_(ruleset),world_',
    'chooseGoal(world_,c,ruleset_.utilityAI)',
    'c.needs.decay(ruleset_.needs,c.metabolism,c.sleepTendency)'):
    assert token in sim_cpp, f'missing Simulation ruleset runtime wiring: {token}'

assert 'SimulationSnapshotVersion=2' in snapshot_h
assert 'SimulationRuleset ruleset=DefaultSimulationRuleset' in snapshot_h
assert 'SimulationSnapshotBinaryFormatVersion=7' in codec_h
assert 'MinimumSupportedSimulationSnapshotBinaryFormatVersion' not in codec_h
for token in (
    'writeSimulationRulesetSnapshotExtension',
    'readSimulationRulesetSnapshotExtension',
    'SimulationRulesetSnapshotExtensionMagic'):
    assert token in codec, f'missing ruleset snapshot integration: {token}'
for obsolete in (
    'binaryVersion==1', 'initializeLegacyCivilizationState',
    'patchBinaryFormatVersion', 'decodeLegacyBodyForVersion'):
    assert obsolete not in codec, f'pre-release snapshot migration must stay removed: {obsolete}'
for token in (
    'SimulationRulesetSnapshotExtensionMagic',
    'writeSimulationRulesetSnapshotExtension',
    'readSimulationRulesetSnapshotExtension',
    'validSimulationRuleset'):
    assert token in rules_codec, f'missing ruleset snapshot extension contract: {token}'

assert 'UCLASS(Config=Game, DefaultConfig)' in bridge_h
for token in (
    'NeedsHungerPerMinute', 'NeedsThirstPerMinute', 'NeedsSleepPerMinute',
    'NeedsBladderPerMinute', 'NeedsHygienePerMinute',
    'UtilityNeedExponent', 'UtilityUrgentThreshold', 'UtilityUrgentSlope',
    'UtilityIdleScore', 'UtilitySleepNightStartHour', 'UtilitySleepNightEndHour',
    'UtilitySleepNightMultiplier', 'UtilityWashBaseMultiplier',
    'UtilityWashConscientiousnessMultiplier', 'UtilitySleepBaseMultiplier',
    'UtilitySleepIntroversionMultiplier', 'UtilitySecondChoiceProbability'):
    assert token in bridge_h, f'missing Config-backed ruleset field: {token}'
for token in (
    'CreateConfiguredSimulation',
    'Rules.needs.hungerPerMinute',
    'Rules.utilityAI.needExponent',
    'Rules.utilityAI.secondChoiceProbability',
    'validSimulationRuleset(Rules)',
    'lifelens::CurrentWorldGenerationVersion'):
    assert token in bridge_cpp, f'missing Config -> Core ruleset translation: {token}'

assert '[/Script/LifeLens.LLCoreBridgeSubsystem]' in game_ini
for token in (
    'NeedsHungerPerMinute=0.001000', 'NeedsThirstPerMinute=0.001300',
    'NeedsSleepPerMinute=0.000800', 'NeedsBladderPerMinute=0.001100',
    'NeedsHygienePerMinute=0.000700', 'UtilityNeedExponent=4.000000',
    'UtilityUrgentThreshold=0.700000', 'UtilityUrgentSlope=1.800000',
    'UtilityIdleScore=0.035000', 'UtilitySecondChoiceProbability=0.080000'):
    assert token in game_ini, f'missing production ruleset tuning source: {token}'

for token in ('Snapshot.world.populationSeed', 'Snapshot.world.generationVersion', 'Snapshot.ruleset'):
    assert token in persistence, f'missing saved ruleset/world identity restore: {token}'

assert 'CurrentSaveVersion = 3' in save_h
assert 'TArray<uint8> CoreSnapshotBytes' in save_h
for obsolete in (
    'int32 WorldSeed', 'int64 SimulationMinute',
    'TArray<FLLResidentData> Residents', 'TArray<FLLRelationshipData> Relationships'):
    assert obsolete not in save_h, f'pre-release SaveGame compatibility payload must stay removed: {obsolete}'
assert 'SaveObject->SaveVersion != ULLSaveGame::CurrentSaveVersion' in sim_ue
assert 'SaveObject->SaveVersion == 1' not in sim_ue
assert 'TargetMinute - StartMinute' not in sim_ue

print('SimulationRuleset + current-only persistence structural validation: PASS')
