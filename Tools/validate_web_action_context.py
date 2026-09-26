#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

header = (
    root / "Source/LifeLensCore/include/lifelens/PresentationDirective.h"
).read_text(encoding="utf-8")
simulation_h = (
    root / "Source/LifeLensCore/include/lifelens/Simulation.h"
).read_text(encoding="utf-8")
simulation_cpp = (
    root / "Source/LifeLensCore/src/Simulation.cpp"
).read_text(encoding="utf-8")
bridge = (
    root / "Source/LifeLensCore/src/WebClientBridge.cpp"
).read_text(encoding="utf-8")
types = (
    root / "web/src/runtime/core-types.ts"
).read_text(encoding="utf-8")
formatter = (
    root / "web/src/render/resident-action-context.ts"
).read_text(encoding="utf-8")
resident_layer = (
    root / "web/src/render/resident-world-layer.ts"
).read_text(encoding="utf-8")

for token in (
    "PresentationActionKind",
    "PresentationActionPhase",
    "ResidentPresentationObservation",
    "hasTargetGrid",
    "targetResidentId",
    "hasObjectTarget",
    "contextActionToken",
):
    assert token in header, f"missing Core presentation DTO token: {token}"

assert '#include "PresentationDirective.h"' in simulation_h
assert "observeResidentPresentation(CharacterId id) const" in simulation_h
assert "Simulation::observeResidentPresentation" in simulation_cpp
assert "PresentationActionPhase::Moving" in simulation_cpp
assert "PresentationActionPhase::Interacting" in simulation_cpp

for token in (
    '"presentation"',
    "presentationActionKindName",
    "presentationActionPhaseName",
    '"targetResidentId"',
    '"targetGridX"',
    '"targetGridY"',
):
    assert token in bridge, f"missing web presentation JSON token: {token}"

assert "ResidentPresentationDirective" in types
assert "presentation?: ResidentPresentationDirective" in types

# Web action wording may translate a Core enum or resolve an authoritative
# resident/object target. It may not derive intent from needs, relationships,
# memories, or browser-side distance heuristics.
for token in (
    "resident.presentation",
    "directive.kind",
    "directive.phase",
    "directive.targetResidentId",
    "directive.objectKind",
):
    assert token in formatter, f"missing factual action cue token: {token}"

for forbidden in (
    "resident.needs",
    "resident.relationships",
    "resident.memories",
    "Math.random(",
):
    assert forbidden not in formatter, (
        f"action cue must not invent intent from browser state: {forbidden}"
    )

# Motion stays conservative: authoritative movement can preserve walk state,
# and nearby resident conversation can talk. Object-bound physical motions are
# deliberately not invented in this tranche.
assert "actor.presentation = resident.presentation ?? null" in resident_layer
assert "actor.presentation.phase === 'Moving'" in resident_layer
assert "presentation.phase !== 'Interacting'" in resident_layer
assert "presentation.kind === 'KnowledgeTeaching'" in resident_layer
assert "presentation.kind === 'Social'" in resident_layer
assert "Object-bound physical/civilization/parenting motions remain neutral" in resident_layer
assert "actor.activityLabel === 'Talk'" not in resident_layer

print("LifeLens web authoritative action context: PASS")
