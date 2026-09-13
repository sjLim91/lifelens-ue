from pathlib import Path
import json

root = Path(__file__).resolve().parents[1]
required = [
    'LifeLens.uproject',
    'Source/LifeLens/LifeLens.Build.cs',
    'Source/LifeLens/Core/LLTypes.h',
    'Source/LifeLens/Simulation/LLSimulationSubsystem.cpp',
    'Source/LifeLens/Save/LLSaveGame.h',
    'Source/LifeLens/AI/LLDecisionComponent.cpp',
    'Source/LifeLens/Characters/LLResidentCharacter.cpp',
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

ui = (root / 'Source/LifeLens/UI/LLObservationSubsystem.h').read_text(encoding='utf-8')
assert 'ObservedResidentId' in ui

print('LifeLens bootstrap structural validation: PASS')
