from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
required = [
    'LifeLens.uproject',
    'Source/LifeLens/LifeLens.Build.cs',
    'Source/LifeLens/Core/LLTypes.h',
    'Source/LifeLens/Core/LLLifeLensGameMode.cpp',
    'Source/LifeLens/Simulation/LLSimulationSubsystem.cpp',
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

sim = (root / 'Source/LifeLens/Simulation/LLSimulationSubsystem.cpp').read_text(encoding='utf-8')
assert sim.count('Residents.Add(GenerateAdult(Random, ELLSex::Male') == 2
assert sim.count('Residents.Add(GenerateAdult(Random, ELLSex::Female') == 2
assert 'SaveObject->Residents = Residents;' in sim
assert 'Residents = SaveObject->Residents;' in sim
assert 'MakeDeterministicGuid(Random)' in sim
assert 'ApplyActionOutcome' in sim
assert 'ApplySocialInteraction' in sim

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

config = (root / 'Config/DefaultGame.ini').read_text(encoding='utf-8')
assert 'GlobalDefaultGameMode=/Script/LifeLens.LLLifeLensGameMode' in config

print('LifeLens autonomous observer structural validation: PASS')
