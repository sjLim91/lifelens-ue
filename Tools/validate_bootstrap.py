from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
required = [
    'LifeLens.uproject',
    'Config/Windows/WindowsEngine.ini',
    'Config/Android/AndroidEngine.ini',
    'docs/CINEMATIC_RENDERING_STRATEGY_v1.md',
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
    'Source/LifeLensCore/include/lifelens/SimulationRuleset.h',
    'Source/LifeLensCore/include/lifelens/SimulationRulesetSnapshotCodec.h',
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
enabled_plugins = {p['Name'] for p in project.get('Plugins', []) if p.get('Enabled') is True}
for plugin in ('Niagara', 'PCG', 'Water'):
    assert plugin in enabled_plugins, f'Missing required Unreal-native presentation plugin: {plugin}'

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
    'SaveObject->SaveVersion = ULLSaveGame::CurrentSaveVersion;',
    'SaveObject->CoreSnapshotBytes = MoveTemp(SnapshotBytes);',
    'CaptureCoreSnapshotBytes',
    'RestoreCoreSnapshotBytes',
    'SaveObject->SaveVersion != ULLSaveGame::CurrentSaveVersion',
):
    assert token in sim, f'Missing Core-authoritative runtime/save contract: {token}'
for obsolete in (
    'SaveObject->SaveVersion == 1',
    'SaveObject->SaveVersion == 2',
    'TargetMinute - StartMinute',
    'StartCoreNewGame(SaveObject->WorldSeed)',
):
    assert obsolete not in sim, f'Pre-release SaveGame migration path must stay removed: {obsolete}'
assert 'Residents.Add(GenerateAdult' not in sim
assert 'Residents = SaveObject->Residents' not in sim
assert 'Relationships = SaveObject->Relationships' not in sim
assert 'SaveObject->Residents = Residents' not in sim
assert 'SaveObject->Relationships = Relationships' not in sim
assert 'MakeDeterministicGuid(Random)' not in sim

save_h = (root / 'Source/LifeLens/Save/LLSaveGame.h').read_text(encoding='utf-8')
for token in (
    'CurrentSaveVersion = 3',
    'int32 SaveVersion = CurrentSaveVersion',
    'TArray<uint8> CoreSnapshotBytes',
    'UPROPERTY(SaveGame)',
):
    assert token in save_h, f'Missing current SaveGame snapshot contract: {token}'
for obsolete in (
    'int32 WorldSeed',
    'int64 SimulationMinute',
    'TArray<FLLResidentData> Residents',
    'TArray<FLLRelationshipData> Relationships',
):
    assert obsolete not in save_h, f'Pre-release SaveGame compatibility payload must stay removed: {obsolete}'

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
    'Snapshot.world.populationSeed',
    'Snapshot.world.generationVersion',
    'Snapshot.ruleset',
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

ruleset_h = (root / 'Source/LifeLensCore/include/lifelens/SimulationRuleset.h').read_text(encoding='utf-8')
for token in (
    'struct NeedsRuleset',
    'struct UtilityAIRuleset',
    'struct SimulationRuleset',
    'DefaultSimulationRuleset',
    'validSimulationRuleset',
    'sameSimulationRuleset',
):
    assert token in ruleset_h, f'Missing SimulationRuleset contract: {token}'

needs_h = (root / 'Source/LifeLensCore/include/lifelens/Needs.h').read_text(encoding='utf-8')
for token in (
    'const NeedsRuleset& rules',
    'rules.hungerPerMinute*metabolism',
    'rules.thirstPerMinute*metabolism',
    'rules.sleepPerMinute*sleepTendency',
    'rules.bladderPerMinute*metabolism',
    'rules.hygienePerMinute',
):
    assert token in needs_h, f'Missing ruleset-driven Needs decay: {token}'
assert '0.0010*metabolism' not in needs_h
assert '0.0013*metabolism' not in needs_h

utility_h = (root / 'Source/LifeLensCore/include/lifelens/UtilityAI.h').read_text(encoding='utf-8')
for token in (
    'const UtilityAIRuleset& rules',
    'rules.needExponent',
    'rules.urgentThreshold',
    'rules.urgentSlope',
    'rules.idleScore',
    'rules.sleepNightMultiplier',
    'rules.secondChoiceProbability',
):
    assert token in utility_h, f'Missing ruleset-driven UtilityAI tuning: {token}'
assert 'return 0.035;' not in utility_h
assert '(n-0.70)*1.8' not in utility_h

simulation_core = (root / 'Source/LifeLensCore/include/lifelens/Simulation.h').read_text(encoding='utf-8')
for token in (
    'SimulationRuleset ruleset=DefaultSimulationRuleset',
    'const SimulationRuleset& ruleset() const',
    'const SimulationRuleset ruleset_',
    'sanitationUseTarget',
    'resolveSanitationUseTarget',
    'SanitationSiteId sanitationSiteId=0',
    'recordPrimitiveSanitationSiteUse',
    'primitiveSanitationUseDurationTicks',
):
    assert token in simulation_core, f'Missing Core Simulation contract: {token}'

simulation_cpp = (root / 'Source/LifeLensCore/src/Simulation.cpp').read_text(encoding='utf-8')
for token in (
    ':ruleset_(ruleset),world_',
    'chooseGoal(world_,c,ruleset_.utilityAI)',
    'c.needs.decay(ruleset_.needs,c.metabolism,c.sleepTendency)',
):
    assert token in simulation_cpp, f'Missing runtime SimulationRuleset wiring: {token}'

snapshot_state_h = (root / 'Source/LifeLensCore/include/lifelens/SimulationSnapshot.h').read_text(encoding='utf-8')
for token in (
    'SimulationSnapshotVersion=2',
    'SimulationRuleset ruleset=DefaultSimulationRuleset',
):
    assert token in snapshot_state_h, f'Missing ruleset snapshot state contract: {token}'

snapshot_h = (root / 'Source/LifeLensCore/include/lifelens/SimulationSnapshotCodec.h').read_text(encoding='utf-8')
snapshot_binary_version = int(snapshot_h.split('SimulationSnapshotBinaryFormatVersion=', 1)[1].split(';', 1)[0])
assert snapshot_binary_version == 7, 'Cleanup B current-only ruleset persistence requires snapshot binary format v7'
assert 'MinimumSupportedSimulationSnapshotBinaryFormatVersion' not in snapshot_h

snapshot_codec = (root / 'Source/LifeLensCore/src/SimulationSnapshotCodec.cpp').read_text(encoding='utf-8')
for token in (
    'SimulationSnapshotBinaryFormatVersion',
    'readBinaryFormatVersion',
    'decodeSimulationSnapshotBaseBody',
    'writeCivilizationSnapshotExtension',
    'readCivilizationSnapshotExtension',
    'writePrimitiveSanitationSnapshotExtension',
    'readPrimitiveSanitationSnapshotExtension',
    'validatePrimitiveSanitationSitesForCodec',
    'PrimitiveSanitationSnapshotExtensionMagic',
    'writeWorldGenerationSnapshotExtension',
    'readWorldGenerationSnapshotExtension',
    'writeSimulationRulesetSnapshotExtension',
    'readSimulationRulesetSnapshotExtension',
    'SimulationRulesetSnapshotExtensionMagic',
):
    assert token in snapshot_codec, f'Missing current persistent Core snapshot contract: {token}'
for obsolete in (
    'binaryVersion==1',
    'initializeLegacyCivilizationState',
    'patchBinaryFormatVersion',
    'decodeLegacyBodyForVersion',
    'MinimumSupportedSimulationSnapshotBinaryFormatVersion',
):
    assert obsolete not in snapshot_codec, f'Pre-release snapshot migration path must stay removed: {obsolete}'

ruleset_snapshot = (root / 'Source/LifeLensCore/include/lifelens/SimulationRulesetSnapshotCodec.h').read_text(encoding='utf-8')
for token in (
    'SimulationRulesetSnapshotExtensionMagic',
    'writeSimulationRulesetSnapshotExtension',
    'readSimulationRulesetSnapshotExtension',
    'validSimulationRuleset',
):
    assert token in ruleset_snapshot, f'Missing ruleset snapshot codec contract: {token}'

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

world_generation_types = (root / 'Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h').read_text(encoding='utf-8')
for token in (
    'FLLCoreSurfaceWaterPresentationObservation',
    'SuggestedChannelWidthCells',
    'SuggestedAreaRadiusCells',
    'DownstreamCenterGridX',
    'bHasDownstreamTarget',
):
    assert token in world_generation_types, f'Missing water presentation read contract: {token}'

world_generation_bridge = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
for token in (
    'FillSurfaceWaterPresentationObservation',
    'GetMaterializedSurfaceWaterPresentationObservations',
    'GetSurfaceWaterPresentationObservation',
    'chunkOriginGrid',
    'WorldChunkSpanGridCells',
):
    assert token in world_generation_bridge, f'Missing deterministic hydrology presentation projection: {token}'

world = (root / 'Source/LifeLens/World/LLWorldDirector.cpp').read_text(encoding='utf-8')
for token in (
    'SpawnResidents()',
    'GetResidentActionDirective',
    'ApplyCoreDirective',
    'MoveResidentToward',
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
assert 'LLObserverText::TapHint' in hud
assert 'GetObservedResidentId' in hud

engine_config = (root / 'Config/DefaultEngine.ini').read_text(encoding='utf-8')
game_config = (root / 'Config/DefaultGame.ini').read_text(encoding='utf-8')
assert '[/Script/EngineSettings.GameMapsSettings]' in engine_config
assert 'GlobalDefaultGameMode=/Script/LifeLens.LLLifeLensGameMode' in engine_config
assert 'GameDefaultMap=/Game/Maps/LifeLensWorld' in engine_config
assert 'TargetSDKVersion=34' in engine_config
assert 'bBuildForArm64=True' in engine_config
assert 'bPackageDataInsideApk=True' in engine_config
assert 'Orientation=SensorLandscape' in engine_config
assert '[/Script/Engine.GameMapsSettings]' not in game_config

windows_engine = (root / 'Config/Windows/WindowsEngine.ini').read_text(encoding='utf-8')
for token in (
    'DefaultGraphicsRHI=DefaultGraphicsRHI_DX12',
    '+D3D12TargetedShaderFormats=PCD3D_SM6',
    'r.GenerateMeshDistanceFields=True',
    'r.DynamicGlobalIlluminationMethod=1',
    'r.ReflectionMethod=1',
    'r.Shadow.Virtual.Enable=1',
    'r.AntiAliasingMethod=4',
    'r.Nanite.ProjectEnabled=True',
    'r.Lumen.HardwareRayTracing=False',
):
    assert token in windows_engine, f'Missing Windows cinematic renderer contract: {token}'

android_engine = (root / 'Config/Android/AndroidEngine.ini').read_text(encoding='utf-8')
for token in (
    'r.GenerateMeshDistanceFields=False',
    'r.DynamicGlobalIlluminationMethod=0',
    'r.ReflectionMethod=0',
    'r.Shadow.Virtual.Enable=0',
    'r.Nanite.ProjectEnabled=False',
    'r.Lumen.HardwareRayTracing=False',
    'r.AntiAliasingMethod=2',
):
    assert token in android_engine, f'Missing Android renderer contract: {token}'

rendering_strategy = (root / 'docs/CINEMATIC_RENDERING_STRATEGY_v1.md').read_text(encoding='utf-8')
for token in (
    'Lumen Global Illumination',
    'Virtual Shadow Maps',
    'Temporal Super Resolution (TSR)',
    'Nanite project support',
    'fallback mesh/LOD',
):
    assert token in rendering_strategy, f'Missing cinematic rendering strategy invariant: {token}'

for target in ('Source/LifeLens.Target.cs', 'Source/LifeLensEditor.Target.cs'):
    target_text = (root / target).read_text(encoding='utf-8')
    assert 'EngineIncludeOrderVersion.Unreal5_6' in target_text
    assert 'ExtraModuleNames.Add("LifeLens")' in target_text

world_generation_types = (root / 'Source/LifeLens/Simulation/LLWorldGenerationReadTypes.h').read_text(encoding='utf-8')
assert world_generation_types.count('VisualSeed = 0') >= 2, 'Missing deterministic PCG visual seed contract'

world_generation_bridge = (root / 'Source/LifeLens/Simulation/LLWorldGenerationBridge.cpp').read_text(encoding='utf-8')
for token in (
    'Chunk.chunkSeed & 0x7fffffffffffffffULL',
    'Patch.detailSeed & 0x7fffffffffffffffULL',
):
    assert token in world_generation_bridge, f'Missing deterministic PCG seed projection: {token}'


environment_types = (root / 'Source/LifeLens/Simulation/LLEnvironmentReadTypes.h').read_text(encoding='utf-8')
for token in (
    'FLLCoreSkyPresentationObservation',
    'SunElevationDegrees',
    'SunAzimuthDegrees',
    'SunIntensity01',
    'FogAmount01',
    'SurfaceWetness01',
):
    assert token in environment_types, f'Missing sky/atmosphere presentation DTO: {token}'

environment_bridge = (root / 'Source/LifeLens/Simulation/LLEnvironmentBridge.cpp').read_text(encoding='utf-8')
for token in (
    'GetInitialRegionSkyPresentationObservation',
    'deriveSimulationCalendar',
    'SimulationSunriseMinute',
    'SimulationSunsetMinute',
    'CoreEnvironment.cloudCover01',
    'CoreEnvironment.visibility01',
):
    assert token in environment_bridge, f'Missing sky/atmosphere provider projection: {token}'


dynamic_environment_presentation = (root / 'Source/LifeLens/WorldPresentation/LLDynamicEnvironmentPresentationActor.cpp').read_text(encoding='utf-8')
for token in (
    'GetInitialRegionSkyPresentationObservation',
    'Sky.SunElevationDegrees',
    'Sky.SunAzimuthDegrees',
    'Sky.SunIntensity01',
    'Sky.SkyBrightness01',
    'Sky.FogAmount01',
):
    assert token in dynamic_environment_presentation, f'Missing authoritative sky consumer: {token}'
assert 'SolarElevationDegrees =' not in dynamic_environment_presentation, 'Presentation must not reconstruct a second solar elevation path'

default_engine = (root / 'Config/DefaultEngine.ini').read_text(encoding='utf-8')
assert 'Name="WaterBodyCollision"' in default_engine, 'Unreal Water plugin requires WaterBodyCollision profile during editor/runtime load'


world_presentation = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.cpp').read_text(encoding='utf-8')
world_presentation_header = (root / 'Source/LifeLens/WorldPresentation/LLWorldPresentationActor.h').read_text(encoding='utf-8')
for token in (
    'PresentationSeed(Chunk.VisualSeed)',
    'PresentationSeed(Patch.VisualSeed)',
    'SM_LL_stone_fire_pit',
    'SM_LL_wicker_basket_01',
    'SM_LL_wooden_axe',
    'PhotorealFirePitInstances',
    'PhotorealStorageBasketInstances',
    'PhotorealWorkToolInstances',
    'TerrainSurfaceZUU',
    'TerrainTileRotation',
    'GetTerrainPresentationObservation',
    'fir_sapling_b',
    'pine_sapling_small_b',
    'shrub_02_b',
    'weed_plant_02_b_LOD0',
):
    assert token in world_presentation, f'Missing visible-world uplift consumer: {token}'
for token in (
    '#if PLATFORM_ANDROID',
    'MaxTreeInstances  = 620',
    'MaxGrassInstances = 1800',
    'MaxTreeInstances  = 1480',
    'MaxGrassInstances = 6400',
    'TerrainReliefAmplitudeUU = 180.0f',
    'FLLCoreTerrainPresentationObservation',
):
    assert token in world_presentation_header, f'Missing platform/terrain presentation policy: {token}'
assert 'const int32 StoneCount = bStructurallyComplete ? 8' not in world_presentation, 'Completed firepit must not regress to the Engine-cube ring'


resident_motion = (root / 'Source/LifeLens/Characters/LLResidentMotionComponent.cpp').read_text(encoding='utf-8')
for token in (
    'SM_LL_wicker_basket_01',
    'SM_LL_wooden_bowl_01',
    'Pebble_Round_1.Pebble_Round_1',
    'SM_LL_dead_tree_trunk.SM_LL_dead_tree_trunk',
    'SharpFlakeMesh = PrimitiveStoneFinder.Succeeded()',
    'StoneCuttingToolMesh = PrimitiveStoneFinder.Succeeded()',
    'DiggingStickMesh = PrimitiveStickFinder.Succeeded()',
):
    assert token in resident_motion, f'Missing production held-prop policy: {token}'
assert 'SharpFlakeMesh = nullptr' not in resident_motion
assert 'StoneCuttingToolMesh = nullptr' not in resident_motion
for forbidden in (
    '/Engine/BasicShapes/Cone.Cone',
    '/Engine/BasicShapes/Cylinder.Cylinder',
    '/Engine/BasicShapes/Sphere.Sphere',
):
    assert forbidden not in resident_motion, f'Production held props must not use Engine primitive: {forbidden}'

print('LifeLens autonomous observer structural validation: PASS')