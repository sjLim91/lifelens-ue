#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parenting = (root / "Source/LifeLensCore/include/lifelens/Parenting.h").read_text(encoding="utf-8")
simulation = (root / "Source/LifeLensCore/src/Simulation.cpp").read_text(encoding="utf-8")
context_runtime = (root / "Source/LifeLensCore/src/ContextActionRuntime.cpp").read_text(encoding="utf-8")

for token in (
    "bool authorizedHouseholdCaregiver=false",
    "!isParentOf(caregiver,child) && !authorizedHouseholdCaregiver",
):
    assert token in parenting, f"household caregiver primitive contract missing: {token}"

care_start = simulation.index("void Simulation::advanceDependentCare()")
care_end = simulation.index("void Simulation::updatePregnanciesAndBirths()", care_start)
care = simulation[care_start:care_end]
for token in (
    "for(CharacterId parentId:child.parentIds)",
    "if(chosenCaregiver==nullptr)",
    "households_.householdOf(child.id)",
    "for(const HouseholdMember& member:childHome->members)",
    "lifeStageProfile(caregiver->lifeStage).canParent",
):
    assert token in care, f"dependent-care fallback missing: {token}"

assert care.index("for(CharacterId parentId:child.parentIds)") < care.index(
    "if(chosenCaregiver==nullptr)"
), "biological parents must remain first-choice caregivers"
assert "child.parentIds.empty()" not in care, (
    "dependent care must not disappear merely because biological parent ids are absent"
)

for token in (
    "sameDependentCareHousehold",
    "lifeStageProfile(actor.lifeStage).canParent",
    "const bool householdCaregiver",
    "pending.parentingAction,context,householdCaregiver",
):
    assert token in context_runtime, f"parenting ACK caregiver validation missing: {token}"

# Temporary household care must not rewrite biological genealogy.
for forbidden in (
    "child.parentIds.push_back",
    "child.parentIds=",
    "genealogy_.registerBirth",
):
    assert forbidden not in care, f"caregiver fallback must not rewrite genealogy: {forbidden}"

print("LifeLens household dependent-care fallback: PASS")
