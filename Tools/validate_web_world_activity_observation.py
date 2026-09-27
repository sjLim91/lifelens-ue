#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

session = (root / "web/src/runtime/world-session.ts").read_text(encoding="utf-8")
store = (root / "web/src/state/observer-store.ts").read_text(encoding="utf-8")
feed = (root / "web/src/state/observation-feed.ts").read_text(encoding="utf-8")
feed_ui = (root / "web/src/ui/observation-feed.tsx").read_text(encoding="utf-8")
activity_ui = (root / "web/src/ui/world-activity.tsx").read_text(encoding="utf-8")
app = (root / "web/src/App.tsx").read_text(encoding="utf-8")
world_scene = (root / "web/src/render/world-scene.ts").read_text(encoding="utf-8")

# Large authority payloads are cached and sampled away from the 500 ms hot path.
for token in (
    "worldActivityRefreshCountdown",
    "this.worldActivityRefreshCountdown <= 0",
    "this.core.civilizationWorld(16)",
    "this.core.worldObjects()",
    "this.civilizationSnapshot",
    "this.worldObjectsSnapshot",
):
    assert token in session, f"low-rate world activity sampling missing: {token}"

assert session.index("this.worldActivityRefreshCountdown <= 0") < session.index(
    "this.core.civilizationWorld(16)"
)
assert session.index("this.worldActivityRefreshCountdown <= 0") < session.index(
    "this.core.worldObjects()"
)

# Observer state carries exact snapshots and derives only deltas between them.
for token in (
    "civilization: CivilizationWorldPayload",
    "worldObjects: WorldObjectsPayload",
    "this.snapshot.civilization",
    "nextCivilization",
    "this.snapshot.worldObjects",
    "nextWorldObjects",
):
    assert token in store, f"observer world activity state missing: {token}"

for token in (
    "exactCivilizationEvents",
    "exactWorldObjectEvents",
    "previous?.available !== true",
    "previousCivilization",
    "nextCivilization",
    "previousWorldObjects",
    "nextWorldObjects",
    "livingKnowerCount",
    "facility.state",
    "resource.quantity",
    "site.improvedMinute",
):
    assert token in feed, f"exact world activity event logic missing: {token}"

# Observation UI exposes current authoritative world state without altering it.
for token in (
    "WorldActivityPanel",
    "civilization.resourceNodeCount",
    "civilization.totalStoredUnits",
    "civilization.facilities",
    "civilization.recentDiscoveries",
    "worldObjects.sanitationSites",
    "worldObjects.smartObjects",
):
    assert token in activity_ui, f"world activity readout missing: {token}"

assert "WorldActivityPanel" in app
for label in ("문명", "시설", "위생"):
    assert label in feed_ui, f"world activity feed label missing: {label}"

# This tranche is observer-only. World presentation must not infer geometry/state
# from these aggregate inspection payloads.
for forbidden in ("civilizationWorld", "worldObjects"):
    assert forbidden not in world_scene, (
        f"world activity inspection leaked into presentation authority: {forbidden}"
    )

assert "Math.random(" not in activity_ui
assert "Math.random(" not in feed

print("LifeLens Web world activity observation: PASS")
