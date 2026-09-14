from pathlib import Path

root = Path(__file__).resolve().parents[1]

required = [
    'Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h',
    'Source/LifeLensCore/include/lifelens/Simulation.h',
    'Source/LifeLens/Simulation/LLCivilizationReadTypes.h',
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h',
    'Source/LifeLens/Simulation/LLCoreBridgeCivilization.cpp',
    'Source/LifeLensCore/tests/test_civilization_observer_read_model.cpp',
]
missing = [path for path in required if not (root / path).exists()]
assert not missing, f'Missing civilization observer files: {missing}'

core_read = (root / required[0]).read_text(encoding='utf-8')
for token in (
    'ResidentCivilizationObservation',
    'CivilizationWorldObservation',
    'CivilizationDiscoveryObservation',
    'CivilizationKnowledgeSource',
    'SelfDiscovery',
    'DirectWitness',
    'Teaching',
    'buildResidentCivilizationObservation',
    'buildCivilizationWorldObservation',
    'recentDiscoveries',
    'uniqueReproducibleTechniqueTypes',
):
    assert token in core_read, f'Missing Core civilization observer contract: {token}'

simulation_h = (root / 'Source/LifeLensCore/include/lifelens/Simulation.h').read_text(encoding='utf-8')
for token in (
    'observeResidentCivilization',
    'observeCivilizationWorld',
    'buildResidentCivilizationObservation(world_,socialKnowledge_',
    'buildCivilizationWorldObservation(world_,socialKnowledge_',
):
    assert token in simulation_h, f'Missing Simulation civilization read API: {token}'

unreal_types = (root / 'Source/LifeLens/Simulation/LLCivilizationReadTypes.h').read_text(encoding='utf-8')
for token in (
    'FLLCoreResidentCivilizationObservation',
    'FLLCoreCivilizationWorldObservation',
    'FLLCoreTechniqueKnowledgeObservation',
    'FLLCoreCivilizationDiscoveryObservation',
    'ELLCoreKnowledgeSource',
    'ProvenanceFactId',
    'OriginResidentId',
    'ImmediateSourceResidentId',
    'RecentDiscoveries',
):
    assert token in unreal_types, f'Missing Unreal civilization DTO: {token}'

bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
for token in (
    'GetResidentCivilizationObservation',
    'GetCivilizationWorldObservation',
    'LLCivilizationReadTypes.h',
):
    assert token in bridge_h, f'Missing civilization Bridge API: {token}'

bridge_cpp = (root / 'Source/LifeLens/Simulation/LLCoreBridgeCivilization.cpp').read_text(encoding='utf-8')
for token in (
    'CoreSimulation->observeResidentCivilization',
    'CoreSimulation->observeCivilizationWorld',
    'GuidToCore.Find',
    'MakeStableResidentGuid',
    'ToUnrealKnowledgeSource',
    'StableFactId',
):
    assert token in bridge_cpp, f'Missing civilization Bridge projection: {token}'

# Observer APIs are intentionally read-only. Do not introduce a second Unreal
# authority for inventory, knowledge, resources or technology unlocks.
for forbidden in (
    'SetResidentCivilization',
    'SetCivilizationWorld',
    'UnlockTechnique',
    'AddInventoryItem',
    'MutateKnowledge',
):
    assert forbidden not in bridge_h and forbidden not in bridge_cpp, \
        f'Civilization observer bridge must remain read-only: {forbidden}'

workflow = (root / '.github/workflows/unreal-linux-compile.yml').read_text(encoding='utf-8')
assert "'Source/LifeLens/Simulation/**'" in workflow, \
    'Simulation changes must trigger actual UE 5.6 UHT/UBT compile'

print('LifeLens civilization observer read validation: PASS')
