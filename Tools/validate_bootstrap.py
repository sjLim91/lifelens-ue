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
    'Source/LifeLens/Simulation/LLCoreActionTypes.h',
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h',
    'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.cpp',
    'Source/LifeLens/Simulation/LLCoreActionBridge.cpp',
    'Source/LifeLens/Simulation/LLCoreBridgePersistence.cpp',
    'Source/LifeLens/Simulation/LLCoreCompileUnit.cpp',
    'Source/LifeLens/Save/LLSaveGame.h',
    'Source/LifeLensCore/include/lifelens/SimulationSnapshot.h',
    'Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h',
    'Source/LifeLensCore/include/lifelens/SanitationProblemRecognition.h',
    'Source/LifeLensCore/include/lifelens/PrimitiveSanitation.h',
    'Source/LifeLensCore/include/lifelens/PrimitiveSanitationSnapshotCodec.h',
    'Source/LifeLensCore/src/SimulationSnapshot.cpp',
    'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp',
    'Source/LifeLensCore/tests/test_sanitation_problem_recognition.cpp',
    'Source/LifeLensCore/tests/test_primitive_sanitation_progression.cpp',
    'Source/LifeLensCore/tests/test_designated_sanitation_affordance.cpp',
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
    'SaveObject->SaveVersion = 2;',
    'SaveObject->CoreSnapshotBytes = MoveTemp(SnapshotBytes);',
    'CaptureCoreSnapshotBytes',
    'RestoreCoreSnapshotBytes',
    'SaveObject->SaveVersion == 2',
    'SaveObject->SaveVersion == 1',
    'TargetMinute - StartMinute',
    'ApplyActionOutcome',
    'ApplySocialInteraction',
):
    assert token in sim, f'Missing Core-authoritative runtime/save contract: {token}'
assert 'Residents.Add(GenerateAdult' not in sim
assert 'Residents = SaveObject->Residents' not in sim
assert 'Relationships = SaveObject->Relationships' not in sim
assert 'SaveObject->Residents = Residents' not in sim
assert 'SaveObject->Relationships = Relationships' not in sim
assert 'MakeDeterministicGuid(Random)' not in sim

save_h = (root / 'Source/LifeLens/Save/LLSaveGame.h').read_text(encoding='utf-8')
for token in (
    'SaveVersion = 2',
    'TArray<uint8> CoreSnapshotBytes',
    'UPROPERTY(SaveGame)',
):
    assert token in save_h, f'Missing SaveGame v2 snapshot contract: {token}'

bridge_h = (root / 'Source/LifeLens/Simulation/LLCoreBridgeSubsystem.h').read_text(encoding='utf-8')
for token in (
    'StartCoreNewGame',
    'StartCoreObserverDemo',
    'AdvanceCoreMinutes',
    'GetWorldObservation',
    'GetResidentObservations',
    'GetResidentObservation',
    'GetResidentActionDirective',
    'GetFamilyObservation',
    'CaptureCoreSnapshotBytes',
    'RestoreCoreSnapshotBytes',
    'MakeStableResidentGuid',
    'OnCoreRuntimeStateChanged',
    'GetRecommendedOutdoorReliefGridPosition',
    'GetSanitationUseTarget',
    'bDesignatedSanitationSite',
    'SanitationSiteId',
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

bridge_action = (root / 'Source/LifeLens/Simulation/LLCoreActionBridge.cpp').read_text(encoding='utf-8')
for token in (
    'GetResidentActionDirective',
    'Observation.physicalGoal',
    'Observation.socialIntent',
    'ELLCorePhysicalIntent::Drink',
    'ELLCoreSocialIntent::Avoid',
    'MakeStableResidentGuid',
    'CoreSimulation->sanitationUseTarget',
    'SanitationUseTargetKind::DesignatedArea',
    'designatedSanitationUseDurationTicks',
    'static_cast<lifelens::SanitationSiteId>(SanitationSiteId)',
):
    assert token in bridge_action, f'Missing authoritative action/sanitation bridge contract: {token}'

action_types = (root / 'Source/LifeLens/Simulation/LLCoreActionTypes.h').read_text(encoding='utf-8')
for token in (
    'FLLCoreActionDirective',
    'ELLCorePhysicalIntent',
    'ELLCoreSocialIntent',
    'TargetResidentId',
):
    assert token in action_types, f'Missing Core action DTO: {token}'

bridge_persistence = (root / 'Source/LifeLens/Simulation/LLCoreBridgePersistence.cpp').read_text(encoding='utf-8')
for token in (
    'captureSnapshot()',
    'encodeSimulationSnapshot',
    'decodeSimulationSnapshot',
    'restoreSnapshot',
    'std::make_unique<lifelens::Simulation>',
    'RebuildGuidIndex()',
):
    assert token in bridge_persistence, f'Missing Core snapshot persistence bridge: {token}'

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

core_read_model = (root / 'Source/LifeLensCore/include/lifelens/ObserverReadModel.h').read_text(encoding='utf-8')
for token in (
    'Goal physicalGoal = Goal::Idle',
    'SocialIntent socialIntent = SocialIntent::None',
    'dto.physicalGoal = physicalGoal',
    'dto.socialIntent = socialIntent',
):
    assert token in core_read_model, f'Missing typed Core action observation: {token}'

ll_types = (root / 'Source/LifeLens/Core/LLTypes.h').read_text(encoding='utf-8')
assert 'Drink,' in ll_types, 'Missing Drink presentation intent'

compile_unit = (root / 'Source/LifeLens/Simulation/LLCoreCompileUnit.cpp').read_text(encoding='utf-8')
for token in (
    '#include "../../LifeLensCore/src/Simulation.cpp"',
    '#include "../../LifeLensCore/src/SimulationSnapshot.cpp"',
    '#include "../../LifeLensCore/src/SimulationSnapshotCodec.cpp"',
):
    assert token in compile_unit, f'Missing Core compile unit source: {token}'

snapshot_h = (root / 'Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h').read_text(encoding='utf-8')
assert 'SimulationSnapshotBinaryFormatVersion=5' in snapshot_h, 'Designated sanitation site persistence requires snapshot binary format v5'

snapshot_codec = (root / 'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp').read_text(encoding='utf-8')
for token in (
    "'L','L','S','N','A','P','0','1'",
    'SimulationSnapshotBinaryFormatVersion',
    'writeWorld',
    'writeRuntime',
    'decodeSimulationSnapshot',
    'snapshot contains trailing bytes',
    'writePrimitiveSanitationSnapshotExtension',
    'readPrimitiveSanitationSnapshotExtension',
    'validatePrimitiveSanitationSitesForCodec',
    'PrimitiveSanitationSnapshotExtensionMagic',
):
    assert token in snapshot_codec, f'Missing persistent Core snapshot/sanitation contract: {token}'

sanitation_snapshot = (root / 'Source/LifeLensCore/include/lifelens/PrimitiveSanitationSnapshotCodec.h').read_text(encoding='utf-8')
for token in (
    'PrimitiveSanitationSnapshotExtensionMagic',
    'writePrimitiveSanitationSite',
    'readPrimitiveSanitationSite',
    'restorePrimitiveSanitationSites',
    'site.useCount',
):
    assert token in sanitation_snapshot, f'Missing primitive sanitation persistence contract: {token}'

sanitation_recognition = (root / 'Source/LifeLensCore/include/lifelens/SanitationProblemRecognition.h').read_text(encoding='utf-8')
for token in (
    'sanitationProblemBeliefProposition',
    'isDirectSanitationProblemEvidence',
    'recognizeSanitationProblem',
    'hasRecognizedSanitationProblem',
    'result.qualifyingMemories>=2',
    'belief.supportWeight=std::max',
):
    assert token in sanitation_recognition, f'Missing sanitation problem recognition contract: {token}'
assert 'belief.supportWeight+=' not in sanitation_recognition, 'Sanitation recognition must not inflate evidence on repeated evaluation'

environmental_exposure = (root / 'Source/LifeLensCore/include/lifelens/EnvironmentalExposure.h').read_text(encoding='utf-8')
for token in (
    'recognizeSanitationProblem(character,currentMinute)',
    'sanitationProblemRecognized',
    'sanitationProblemNewlyRecognized',
):
    assert token in environmental_exposure, f'Missing sanitation recognition perception wiring: {token}'

primitive_sanitation = (root / 'Source/LifeLensCore/include/lifelens/PrimitiveSanitation.h').read_text(encoding='utf-8')
for token in (
    'PrimitiveSanitationOpportunity',
    'PrimitiveSanitationSite',
    'SanitationUseTarget',
    'establishDesignatedSanitationArea',
    'resolveSanitationUseTarget',
    'recordDesignatedSanitationSiteUse',
    'TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible',
    'PrimitiveSanitationCleanSiteExposureLimit',
):
    assert token in primitive_sanitation, f'Missing primitive sanitation affordance contract: {token}'
assert 'LatrineUnlocked' not in primitive_sanitation, 'Primitive sanitation must not introduce a global latrine unlock'

world_core = (root / 'Source/LifeLensCore/include/lifelens/World.h').read_text(encoding='utf-8')
for token in (
    'std::vector<PrimitiveSanitationSite> primitiveSanitationSites',
    'primitiveSanitationSites.clear()',
):
    assert token in world_core, f'Missing Core-owned primitive sanitation world state: {token}'

civilization = (root / 'Source/LifeLensCore/include/lifelens/Civilization.h').read_text(encoding='utf-8')
for token in (
    'DesignatedSanitationArea',
    'DesignateSanitationArea',
    'context.sanitationProblemRecognized && context.sanitationSiteAvailable',
    'KnowledgeLevel::Hypothesized',
    'KnowledgeLevel::Reproducible',
):
    assert token in civilization, f'Missing sanitation experiment/knowledge contract: {token}'
assert 'LatrineUnlocked' not in civilization, 'Civilization model must not add a global sanitation tech flag'

civilization_decision = (root / 'Source/LifeLensCore/include/lifelens/CivilizationDecision.h').read_text(encoding='utf-8')
for token in (
    'evaluatePrimitiveSanitationOpportunity',
    'ExperimentKind::DesignateSanitationArea',
    'sanitationOpportunity.problemRecognized',
    'sanitationOpportunity.siteAvailable',
    'sanitationBoost',
    'canEstablishDesignatedSanitationArea',
    'establishDesignatedSanitationArea',
    'result.sanitationSiteId=site.siteId',
):
    assert token in civilization_decision, f'Missing sanitation civilization progression/establishment wiring: {token}'
assert 'LatrineUnlocked' not in civilization_decision, 'Sanitation utility must remain evidence-driven, not globally unlocked'

simulation_core = (root / 'Source/LifeLensCore/include/lifelens/Simulation.h').read_text(encoding='utf-8')
for token in (
    'sanitationUseTarget',
    'resolveSanitationUseTarget',
    'SanitationSiteId sanitationSiteId=0',
    'recordPrimitiveSanitationSiteUse',
    'primitiveSanitationUseDurationTicks',
):
    assert token in simulation_core, f'Missing Core sanitation target/completion authority: {token}'

civilization_snapshot = (root / 'Source/LifeLensCore/include/lifelens/CivilizationSnapshotCodec.h').read_text(encoding='utf-8')
for token in (
    'TechniqueId::DesignatedSanitationArea',
    'TechniqueId::DugSanitationPit',
):
    assert token in civilization_snapshot, f'Sanitation personal knowledge must survive snapshot validation: {token}'

civilization_transmission = (root / 'Source/LifeLensCore/include/lifelens/CivilizationKnowledgeTransmission.h').read_text(encoding='utf-8')
assert 'raw<=static_cast<int>(TechniqueId::DugSanitationPit)' in civilization_transmission, 'Sanitation knowledge transmission must cover dug-pit progression'
assert 'TechniqueId::DesignatedSanitationArea' in civilization_transmission, 'Dug-pit transmission must preserve designated-area prerequisite context'

civilization_observer = (root / 'Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h').read_text(encoding='utf-8')
for token in (
    'raw<=static_cast<int>(TechniqueId::DugSanitationPit)',
    'TechniqueId::DugSanitationPit)+1',
):
    assert token in civilization_observer, f'Missing sanitation technique observer/provenance coverage: {token}'

core_cmake = (root / 'Source/LifeLensCore/CMakeLists.txt').read_text(encoding='utf-8')
assert 'lifelens_add_test(test_sanitation_problem_recognition)' in core_cmake, 'Missing sanitation recognition Core test registration'
assert 'lifelens_add_test(test_primitive_sanitation_progression)' in core_cmake, 'Missing primitive sanitation progression Core test registration'
assert 'lifelens_add_test(test_designated_sanitation_affordance)' in core_cmake, 'Missing designated sanitation affordance Core test registration'
assert 'lifelens_add_test(test_primitive_latrine_progression)' in core_cmake, 'Missing primitive latrine progression Core test registration'

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

world_h = (root / 'Source/LifeLens/World/LLWorldDirector.h').read_text(encoding='utf-8')
for token in (
    'bUsingDesignatedSanitationSite',
    'CoreSanitationSiteId',
):
    assert token in world_h, f'Missing designated sanitation World runtime state: {token}'

world = (root / 'Source/LifeLens/World/LLWorldDirector.cpp').read_text(encoding='utf-8')
for token in (
    'SpawnResidents()',
    'GetResidentActionDirective',
    'ApplyCoreDirective',
    'SetMovementTarget',
    'ELLCoreSocialIntent::Avoid',
    'SaveGame()',
    'CoreBridge->GetSanitationUseTarget',
    'ELLWorldAffordanceTier::Primitive',
    'Runtime.bUsingDesignatedSanitationSite',
    'Runtime.CoreSanitationSiteId',
    'CoreGridCellSizeUU * 0.45f',
    'CompleteResidentPhysicalAction',
):
    assert token in world, f'Missing Core-driven WorldDirector sanitation contract: {token}'
assert 'Direction * 650.0f' not in world, 'WorldDirector must not invent an independent emergency sanitation target'
assert 'GetRecommendedOutdoorReliefGridPosition(' not in world, 'WorldDirector must consume the unified Core sanitation target, not bypass designated sites'
assert 'ChooseAction(' not in world, 'WorldDirector must not independently choose covered life actions'
assert 'ApplyActionOutcome(' not in world, 'WorldDirector must not mutate projected Needs as action authority'
assert 'ApplySocialInteraction(' not in world, 'WorldDirector must not mutate projected relationships as social authority'

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
