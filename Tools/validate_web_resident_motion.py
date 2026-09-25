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
# hair silhouette and gait phase/rate variation.
for token in (
    "createResidentAppearanceProfile",
    "heightWorldUnits",
    "widthScale",
    "depthScale",
    "GARMENT_PALETTE",
    "HAIR_PALETTE",
    "hairStyle",
    "gaitRateBias",
    "applyResidentMaterialVariant",
    "addResidentHairVariant",
):
    assert token in appearance or token in resident, (
        f"missing resident appearance diversity token: {token}"
    )

assert "const hueShift = ((variantSeed % 17) - 8) * 0.006;" not in resident, (
    "resident identity must not collapse back to tiny whole-model hue jitter"
)

assert "defaultMobileZoom: 2.3" in contract
assert "maxZoom: 3.6" in contract
assert "garmentMix: 0.64 + hash01(seed, 43) * 0.18" in appearance

print("LifeLens web resident gait, readability and appearance diversity: PASS")
