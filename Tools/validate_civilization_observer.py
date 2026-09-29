#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

required = [
    "Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h",
    "Source/LifeLensCore/include/lifelens/Simulation.h",
    "Source/LifeLensCore/src/CivilizationKnowledgeTransmission.cpp",
    "Source/LifeLensCore/tests/test_civilization_observer_read_model.cpp",
]
missing = [path for path in required if not (root / path).exists()]
assert not missing, f"Missing civilization observer files: {missing}"

core_read = (root / required[0]).read_text(encoding="utf-8")
for token in (
    "ResidentCivilizationObservation",
    "CivilizationWorldObservation",
    "CivilizationDiscoveryObservation",
    "CivilizationKnowledgeSource",
    "SelfDiscovery",
    "DirectWitness",
    "Teaching",
    "buildResidentCivilizationObservation",
    "buildCivilizationWorldObservation",
    "recentDiscoveries",
    "uniqueReproducibleTechniqueTypes",
):
    assert token in core_read, f"Missing Core civilization observer contract: {token}"

simulation_h = (root / "Source/LifeLensCore/include/lifelens/Simulation.h").read_text(encoding="utf-8")
for token in (
    "observeResidentCivilization",
    "observeCivilizationWorld",
    "buildResidentCivilizationObservation(world_,socialKnowledge_",
    "buildCivilizationWorldObservation(world_,socialKnowledge_",
):
    assert token in simulation_h, f"Missing Simulation civilization read API: {token}"

# Observer APIs remain read-only. Presentation clients consume this state
# through explicit adapters; they do not own inventory/knowledge mutation.
for forbidden in (
    "SetResidentCivilization",
    "SetCivilizationWorld",
    "UnlockTechnique",
    "AddInventoryItem",
    "MutateKnowledge",
):
    assert forbidden not in core_read, f"Civilization observer model must remain read-only: {forbidden}"

test = (root / "Source/LifeLensCore/tests/test_civilization_observer_read_model.cpp").read_text(encoding="utf-8")
for token in (
    "observeResidentCivilization",
    "observeCivilizationWorld",
):
    assert token in test, f"Missing civilization observer regression coverage: {token}"

print("LifeLens Core civilization observer read validation: PASS")
