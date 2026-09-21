#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parenting = (root / "Source/LifeLensCore/include/lifelens/Parenting.h").read_text(encoding="utf-8")
simulation = (root / "Source/LifeLensCore/src/Simulation.cpp").read_text(encoding="utf-8")
context_runtime = (root / "Source/LifeLensCore/src/ContextActionRuntime.cpp").read_text(encoding="utf-8")

for token in (
    "bool authorizedFallbackCaregiver=false",
    "!isParentOf(caregiver,child) && !authorizedFallbackCaregiver",
):
    assert token in parenting, f"household caregiver primitive contract missing: {token}"

care_start = simulation.index("void Simulation::advanceDependentCare()")
care_end = simulation.index("void Simulation::updatePregnanciesAndBirths()", care_start)
care = simulation[care_start:care_end]
for token in (
    "for(CharacterId parentId:child.parentIds)",
    "bool hasLivingBiologicalParent=false",
    "if(chosenCaregiver==nullptr)",
    "households_.householdOf(child.id)",
    "for(const HouseholdMember& member:childHome->members)",
    "genealogy_.relationBetween(candidate.id,child.id)",
    "KinshipType::Grandparent",
    "KinshipType::Sibling",
    "KinshipType::HalfSibling",
    "if(chosenCaregiver==nullptr && !hasLivingBiologicalParent)",
    "for(auto& candidate:world_.characters)",
    "lifeStageProfile(caregiver->lifeStage).canParent",
):
    assert token in care, f"dependent-care fallback missing: {token}"

assert care.index("for(CharacterId parentId:child.parentIds)") < care.index(
    "if(chosenCaregiver==nullptr)"
), "biological parents must remain first-choice caregivers"
assert care.index("households_.householdOf(child.id)") < care.index(
    "genealogy_.relationBetween(candidate.id,child.id)"
), "co-resident caregivers must precede out-of-household kin"
assert care.index("genealogy_.relationBetween(candidate.id,child.id)") < care.index(
    "if(chosenCaregiver==nullptr && !hasLivingBiologicalParent)"
), "close kin must precede unrelated orphan community care"
assert "child.parentIds.empty()" not in care, (
    "dependent care must not disappear merely because biological parent ids are absent"
)

for token in (
    "sameDependentCareHousehold",
    "closeDependentCareKin",
    "hasLivingBiologicalParent",
    "lifeStageProfile(actor.lifeStage).canParent",
    "const bool householdCaregiver",
    "const bool kinCaregiver",
    "const bool orphanCommunityCaregiver",
    "const bool fallbackCaregiver",
    "pending.parentingAction,context,fallbackCaregiver",
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
