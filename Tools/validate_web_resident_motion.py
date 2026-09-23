#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
resident = (
    root / "web/src/render/resident-world-layer.ts"
).read_text(encoding="utf-8")
contract = (
    root / "web/src/runtime/lifelens-contract.ts"
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

# Core positions arrive at a lower cadence than render frames. Small target
# deltas should be spread across that sample interval instead of arriving
# early, idling, and restarting the walk loop on every refresh.
for token in (
    "movementSampleSeconds",
    "targetArrivalPaddingSeconds",
    "travelSpeedWorldUnitsPerSecond",
    "walkStopGraceSeconds",
    "presentationSeconds",
    "Math.min(",
    "locomotionBudget",
):
    assert token in resident or token in contract, (
        f"missing web resident smoothing contract: {token}"
    )

# Heading and animation transitions must preserve visual continuity.
for token in (
    "turnResponsivenessPerSecond",
    "Math.exp(",
    "walkGraceRemainingSeconds",
    "animationCrossFadeSeconds",
    "next.play().fadeIn(blendSeconds)",
):
    assert token in resident or token in contract, (
        f"missing web resident motion continuity token: {token}"
    )

assert "next.reset().fadeIn" not in resident, (
    "walk loop must not restart from frame zero on every movement sample"
)

print("LifeLens web resident forward/smooth locomotion: PASS")
