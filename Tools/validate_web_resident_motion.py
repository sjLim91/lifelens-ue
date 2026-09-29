#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
resident = (
    root / "web/src/render/resident-world-layer.ts"
).read_text(encoding="utf-8")
contract = (
    root / "web/src/runtime/lifelens-contract.ts"
).read_text(encoding="utf-8")
appearance = (
    root / "web/src/render/resident-appearance.ts"
).read_text(encoding="utf-8")

# The imported web character must share the same forward convention as the
# presentation root. A second PI rotation makes an otherwise forward-moving
# actor visibly walk backward.
assert "source.rotation.y = Math.PI" not in resident
assert "modelForwardYawOffsetRadians" in contract
assert "Math.atan2(travelDelta.x, travelDelta.z)" in resident
assert (
    "+ RESIDENT_PRESENTATION_CONTRACT.modelForwardYawOffsetRadians"
    in resident
)

# Core positions arrive more slowly than render frames. Presentation keeps a
# small buffer so the actor does not arrive early, idle, then restart walking
# on the next Core sample.
for token in (
    "movementSampleSeconds",
    "targetArrivalPaddingSeconds",
    "targetTravelSpeedWorldUnitsPerSecond",
    "smoothedTravelSpeedWorldUnitsPerSecond",
    "speedResponsivenessPerSecond",
    "walkStopGraceSeconds",
    "presentationSeconds",
    "locomotionBudget",
):
    assert token in resident or token in contract, (
        f"missing web resident smoothing contract: {token}"
    )

# Simulation-time rebasing must affect visible motion too. The former 4x
# pace is the new 1x, while higher visual animation playback is capped so
# fast-forward remains observable instead of becoming an unreadable strobe.
for token in (
    "SIMULATION_BASELINE_SPEED_MULTIPLIER = 4 as const",
    "/ SIMULATION_BASELINE_SPEED_MULTIPLIER",
    "motionTimeScaleAt1x: SIMULATION_BASELINE_SPEED_MULTIPLIER",
    "maxMotionTimeScale: SIMULATION_BASELINE_SPEED_MULTIPLIER * 2",
    "residentPresentationMotionTimeScale",
):
    assert token in contract, f"missing rebased resident motion contract: {token}"

for token in (
    "residentPresentationMotionTimeScale(this.simulationSpeed)",
    "const motionDt = dt * motionTimeScale",
    "actor.mixer.update(motionDt)",
    "speedResponsivenessPerSecond",
    "turnResponsivenessPerSecond",
):
    assert token in resident, f"resident motion did not adopt simulation pace: {token}"

assert "actor.mixer.update(dt);" not in resident, (
    "resident animation mixer must not remain pinned to wall-clock time"
)

# Heading and animation transitions preserve continuity and gait rate follows
# the presentation velocity instead of playing a fixed-speed walk on every hop.
for token in (
    "turnResponsivenessPerSecond",
    "Math.exp(",
    "walkGraceRemainingSeconds",
    "animationCrossFadeSeconds",
    "syncWalkPlaybackRate",
    "walkReferenceSpeedWorldUnitsPerSecond",
    "walkMinTimeScale",
    "walkMaxTimeScale",
    "setEffectiveTimeScale",
    "next.play().fadeIn(blendSeconds)",
):
    assert token in resident or token in contract, (
        f"missing web resident motion continuity token: {token}"
    )

assert "next.reset().fadeIn" not in resident, (
    "walk loop must not restart from frame zero on every movement sample"
)

# Residents are deterministic individuals, not one cloned visual with only a
# tiny hue jitter. Identity drives overlapping height/body-width, clothing,
# scalp coverage and gait phase/rate variation.
for token in (
    "createResidentAppearanceProfile",
    "heightWorldUnits",
    "widthScale",
    "depthScale",
    "GARMENT_PALETTE",
    "LOWER_GARMENT_PALETTE",
    "SHOE_PALETTE",
    "lowerGarmentColor",
    "shoeColor",
    "waistHeight01",
    "bodyHeight01",
    "upperGarment",
    "lowerGarment",
    "hairStyle",
    "gaitRateBias",
    "applyResidentMaterialVariant",
    "headWeights",
    "headBottom",
    "headTop",
    "hairline",
    "hairColor,",
):
    assert token in appearance or token in resident, (
        f"missing resident appearance diversity token: {token}"
    )

assert "const hueShift = ((variantSeed % 17) - 8) * 0.006;" not in resident, (
    "resident identity must not collapse back to tiny whole-model hue jitter"
)

assert "defaultMobileZoom: 2.3" in contract
assert "maxZoom: 6.4" in contract
assert "garmentMix: 0.7 + hash01(seed, 43) * 0.18" in appearance
for token in (
    "canvas.width = 768",
    "canvas.height = 144",
    "sprite.scale.set(4.8, 0.9, 1)",
    "let fontSize = 42",
):
    assert token in resident, f"resident world label readability regressed: {token}"

# The scalp belongs to the skinned body. Fixed-height spherical proxies caused
# the exposed-scalp/collar regression reported on a real device.
assert "addResidentHairVariant" not in resident + appearance
assert "new THREE.SphereGeometry" not in appearance
assert "mesh.geometry = mesh.geometry.clone()" in appearance
assert "mesh.geometry.setAttribute(\'color\'" in appearance



# Semantic motion mapping stays conservative: use already-imported CC0 clips
# only for actions whose authoritative context supports the body pose. Daily
# life actions without a dedicated clip fail closed to Idle instead of being
# visually misrepresented.
semantic = (
    root / "web/src/render/resident-semantic-motion.ts"
).read_text(encoding="utf-8")
for token in (
    "Fixing_Kneeling",
    "Crouch_Idle_Loop",
    "resolveResidentSemanticMotion",
    "designatedSanitationSite === true",
    "objectKind === 'Toilet'",
    "objectKind === 'Sink'",
    "civilizationIntent === 'Craft'",
    "civilizationIntent === 'Experiment'",
    "presentation.hasTargetGrid",
):
    assert token in resident + semantic, (
        f"missing conservative semantic motion token: {token}"
    )

assert "return 'idle';" in semantic
assert "PickUp_Table" not in resident, (
    "web must not fake ground gathering with the table-height pickup clip"
)
assert "Sword_Attack" not in resident, (
    "web must not fake chopping/hammering with a sword clip"
)

# UAL2 is additive. UAL1 remains the locomotion baseline if the secondary
# library cannot be fetched, and only verified Standard clip names are bound.
for token in (
    "ANIMATION2_COMMIT",
    "84fd636910bf713099010efbab7f3c84550f4bcb",
    "UAL2_Standard.glb",
    "Consume",
    "Farm_Harvest",
    "Walk_Carry_Loop",
    "UAL2 animation asset unavailable",
    "...animation2Asset.animations",
    "actor.carry?.setEffectiveTimeScale(timeScale)",
):
    assert token in resident, f"missing pinned UAL2 motion token: {token}"

# TreeChopping_Loop exists in the free UAL2 pack, but the current Web
# presentation DTO does not yet carry authoritative tool/source semantics.
# Never infer chopping merely because the gathered material is Wood.
assert "TreeChopping_Loop" not in resident + semantic
assert "civilizationMaterial === 'Wood'" not in semantic
assert "civilizationMaterial === 'PlantFood'" in semantic
assert "facilityAction === 'DeliverMaterial'" in semantic
assert "physicalGoal === 'Eat'" in semantic
assert "presentation.physicalGoal === 'Drink'" in semantic

# Sleep has no verified lie-down/sleep/wake sequence in the loaded libraries.
# Behavioral regression tests keep it neutral until one is actually reviewed.
assert "SleepRest" not in resident

print("LifeLens web resident gait, readability, appearance and UAL2 semantic motion: PASS")
