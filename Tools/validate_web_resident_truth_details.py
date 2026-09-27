#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

bridge = (
    root / "Source/LifeLensCore/src/WebClientBridge.cpp"
).read_text(encoding="utf-8")
types = (
    root / "web/src/runtime/core-types.ts"
).read_text(encoding="utf-8")
readout = (
    root / "web/src/ui/observer-readout.tsx"
).read_text(encoding="utf-8")
world_scene = (
    root / "web/src/render/world-scene.ts"
).read_text(encoding="utf-8")

for token in (
    'character->genetics.faceShape',
    'character->genetics.heightPotential',
    'character->lifeCondition.physicalHealth',
    'character->lifeCondition.movementCapacity',
    'character->development.attachment',
    'character->development.emotionalSecurity',
    'family.hasRomanceHistory',
    'family.isGestationalParent',
    'simulation_->households().householdOf(resident.id)',
    'member.responsibilities.caregiving',
    'simulation_->pregnancies().activeFor(resident.id)',
    'pregnancyStageName(pregnancy->stage)',
    'pregnancy->dueMinute',
    'pregnancy->health',
    'pregnancy->nutrition',
    'kinshipName(member.kinship)',
    'memory.recallScore(world.minute)',
    'memory.decayPerDay',
    'memory.tags',
    'belief.supportWeight',
    'belief.contradictionWeight',
    'belief.supportCount',
    'belief.contradictionCount',
):
    assert token in bridge, f"resident truth JSON missing Core value: {token}"

for token in (
    "ResidentGenetics",
    "ResidentLifeCondition",
    "ResidentDevelopment",
    "ResidentKinship",
    "recallScore?: number",
    "decayPerDay?: number",
    "tags?: string[]",
    "supportWeight?: number",
    "contradictionWeight?: number",
    "supportCount?: number",
    "contradictionCount?: number",
    "isGestationalParent?: boolean",
    "hasRomanceHistory?: boolean",
    "ResidentHouseholdResponsibilities",
    "ResidentHouseholdMember",
    "ResidentHousehold",
    "ResidentPregnancyStage",
    "ResidentPregnancy",
    "household?: ResidentHousehold | null",
    "pregnancy?: ResidentPregnancy | null",
):
    assert token in types, f"resident truth TypeScript contract missing: {token}"

for token in (
    "resident.lifeCondition",
    "resident.genetics",
    "resident.development",
    "family.isGestationalParent",
    "resident.household",
    "resident.pregnancy",
    "ownHouseholdMember",
    "pregnancyStageLabels",
    "member.kinship",
    "memory.recallScore",
    "memory.tags",
    "belief.supportCount",
    "belief.contradictionCount",
):
    assert token in readout, f"selected-resident readout missing factual detail: {token}"

# P1 truth detail stays in the inspection panel. It must not change world
# simulation/presentation geometry or infer traits from browser-side hashes.
for forbidden in (
    "resident.genetics",
    "resident.lifeCondition",
    "resident.development",
):
    assert forbidden not in world_scene, (
        f"resident truth detail leaked into world-scene authority/presentation: {forbidden}"
    )

assert "Math.random(" not in readout
assert "hash" not in readout.lower()

print("LifeLens Web resident truth details: PASS")
