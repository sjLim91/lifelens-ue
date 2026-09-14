from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
required = [
    'LifeLens.uproject',
    'Source/LifeLens.Target.cs',
    'Source/LifeLensEditor.Target.cs',
    'Source/LifeLens/LifeLens.Build.cs',
    'Source/LifeLens/Core/LLTypes.h',
    'Source/LifeLens/Core/LLLifeLensGameMode.cpp',
    'Source/LifeLens/Simulation/LLSimulationSubsystem.h',
    'Source/LifeLens/Simulation/LLSimulationSubsystem.cpp',
    'Source/LifeLens/Simulation/LLCoreReadTypes.h',
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h',
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp',
    'Source/LifeLens/Simulation/LLCoreCompileUnit.cpp',
    'Source/LifeLens/Save/LLSaveGame.h',
    'Source/LifeLens/AI/LLDecisionComponent.cpp',
    'Source/LifeLens/Characters/LLResidentCharacter.cpp',
    'Source/LifeLens/World/LLActivityAnchor.cpp',
    'Source/LifeLens/World/LLWorldDirector.cpp',
    'Source/LifeLens/UI/LLObservationSubsystem.h',
    'Source/LifeLens/UI/LLObserverPlayerController.cpp',
    'Source/LifeLens/UI/LLObserverHUD.cpp',
]
missing = [p for p in required if not (root / p).exists()]
if missing:
    raise SystemExit(f'Missing files: {missing}')

project = json.loads((root / 'LifeLens.uproject').read_text(encoding='utf-8'))
assert project['Modules'][0]['Name'] == 'LifeLens'

build_rules = (root / 'Source/LifeLens/LifeLens.Build.cs').read_text(encoding='utf-8')
assert 'PublicIncludePaths.Add(ModuleDirectory);' in build_rules
assert 'PrivateIncludePaths.Add(ModuleDirectory);' in build_rules
assert 'CppStandardVersion.Cpp20' in build_rules
assert 'LifeLensCore' in build_rules and 'include' in build_rules

sim_h = (root / 'Source/LifeLens/Simulation/LLSimulationSubsystem.h').read_text(encoding='utf-8')
assert 'TArray<FLLResidentData> GetResidents() const' in sim_h
assert 'TArray<FLLRelationshipData> GetRelationships() const' in sim_h
assert 'IsCoreAuthoritativeRuntime' in sim_h
assert 'RefreshProjectionFromCore' in sim_h
assert 'GenerateInitialPopulation' not in sim_h
assert 'GenerateAdult' not in sim_h

sim = (root / 'Source/LifeLens/Simulation/LLSimulationSubsystem.cpp').read_text(encoding='utf-8')
for token in (
    'StartCoreNewGame(WorldSeed)',
    'GetResidentObservations()',
    'GetFamilyObservation',
    'AdvanceCoreMinutes',
    'RefreshProjectionFromCore',
    'SaveObject->WorldSeed = WorldSeed;',
    'SaveObject->SimulationMinute = SimulationMinute;',
    'TargetMinute - StartMinute',
    'ApplyActionOutcome',
    'ApplySocialInteraction',
):
    assert token in sim, f'Missing Core-authoritative runtime contract: {token}'
assert 'Residents.Add(GenerateAdult' not in sim
assert 'Residents = SaveObject->Residents' not in sim
assert 'MakeDeterministicGuid(Random)' not in sim

bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
for token in (
    'StartCoreNewGame',
    'StartCoreObserverDemo',
    'AdvanceCoreMinutes',
    'GetWorldObservation',
    'GetResidentObservations',
    'GetResidentObservation',
    'GetFamilyObservation',
    'MakeStableResidentGuid',
    'OnCoreRuntimeStateChanged',
):
    assert token in bridge_h, f'Missing Core bridge contract: {token}'

bridge_cpp = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp').read_text(encoding='utf-8')
for token in (
    'CoreSimulation->setupNewGame()',
    'observeResident',
    'makeEmotionObservation',
    'observeFamily',
    'observeWorldOverview',
    'OutObservation.Sex',
    'OutObservation.AgeYears',
    'OutObservation.LifeStage',
    'OutObservation.Personality',
    'RomanticInterest',
    'SexualAttraction',
    'Commitment',
    'Conflict',
    'Grudge',
    'MemoryCount',
    'BeliefCount',
    'Households',
    'MarriedCouples',
    'ActivePregnancies',
    'MajorLifeEventRecords',
):
    assert token in bridge_cpp, f'Missing Core observer projection: {token}'
assert 'MakeStableResidentGuid(CoreCharacterId)' in bridge_cpp

read_types = (root / 'Source/LifeLens/Simulation/LLCoreReadTypes.h').read_text(encoding='utf-8')
for token in (
    'ELLCoreSex',
    'ELLCoreLifeStage',
    'FLLCorePersonalitySnapshot',
    'FLLCoreEmotionSnapshot',
    'FLLCoreRelationshipSnapshot',
    'FLLCoreFamilyMemberSnapshot',
    'FLLCoreFamilyObservation',
    'ELLCoreRomanceStage',
    'FLLCoreResidentObservation',
    'FLLCoreWorldObservation',
    'AgeYears',
    'LifeStage',
    'Personality',
    'ActivityTargetResidentId',
    'PartnerResidentId',
    'PregnancyPartnerResidentId',
    'Households',
    'ActiveCouples',
    'MarriedCouples',
    'ActivePregnancies',
    'MajorLifeEventRecords',
):
    assert token in read_types, f'Missing Unreal read DTO: {token}'

compile_unit = (root / 'Source/LifeLens/Simulation/LLCoreCompileUnit.cpp').read_text(encoding='utf-8')
assert '#include "../../LifeLensCore/src/Simulation.cpp"' in compile_unit

# The simulation core remains standard-library C++ with a C++17 baseline.
forbidden_core_tokens = (
    '#include "CoreMinimal.h"',
    'UCLASS(',
    'USTRUCT(',
    'UPROPERTY(',
    'UFUNCTION(',
    'GENERATED_BODY(',
)
for path in (root / 'Source/LifeLensCore').rglob('*'):
    if path.suffix not in {'.h', '.hpp', '.cpp', '.cc'}:
        continue
    text = path.read_text(encoding='utf-8')
    leaked = [token for token in forbidden_core_tokens if token in text]
    assert not leaked, f'Unreal dependency leaked into pure Core: {path.relative_to(root)} {leaked}'

world = (root / 'Source/LifeLens/World/LLWorldDirector.cpp').read_text(encoding='utf-8')
assert 'SpawnResidents()' in world
assert 'ChooseAction(Resident)' in world
assert 'SetMovementTarget' in world
assert 'ApplySocialInteraction' in world
assert 'SaveGame()' in world

character = (root / 'Source/LifeLens/Characters/LLResidentCharacter.cpp').read_text(encoding='utf-8')
assert 'VInterpConstantTo' in character
assert 'NameLabel->SetText' in character

controller = (root / 'Source/LifeLens/UI/LLObserverPlayerController.cpp').read_text(encoding='utf-8')
assert 'GetHitResultUnderCursor' in controller
assert 'GetHitResultUnderFinger' in controller
assert 'ObserveResident' in controller

hud = (root / 'Source/LifeLens/UI/LLObserverHUD.cpp').read_text(encoding='utf-8')
assert 'Tap/click a resident for details' in hud
assert 'GetObservedResidentId' in hud

engine_config = (root / 'Config/DefaultEngine.ini').read_text(encoding='utf-8')
game_config = (root / 'Config/DefaultGame.ini').read_text(encoding='utf-8')
assert '[/Script/EngineSettings.GameMapsSettings]' in engine_config
assert 'GlobalDefaultGameMode=/Script/LifeLens.LLLifeLensGameMode' in engine_config
assert 'GameDefaultMap=/Engine/Maps/Entry' in engine_config
assert 'TargetSDKVersion=34' in engine_config
assert 'bBuildForArm64=True' in engine_config
assert 'bPackageDataInsideApk=True' in engine_config
assert 'Orientation=SensorLandscape' in engine_config
assert '[/Script/Engine.GameMapsSettings]' not in game_config

for target in ('Source/LifeLens.Target.cs', 'Source/LifeLensEditor.Target.cs'):
    target_text = (root / target).read_text(encoding='utf-8')
    assert 'EngineIncludeOrderVersion.Unreal5_6' in target_text
    assert 'ExtraModuleNames.Add("LifeLens")' in target_text

print('LifeLens autonomous observer structural validation: PASS')
