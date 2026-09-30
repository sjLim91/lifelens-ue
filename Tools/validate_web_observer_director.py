#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

session = (root / "web/src/runtime/world-session.ts").read_text(encoding="utf-8")
store = (root / "web/src/state/observer-store.ts").read_text(encoding="utf-8")
feed = (root / "web/src/state/observation-feed.ts").read_text(encoding="utf-8")
ui = (root / "web/src/ui/observation-feed.tsx").read_text(encoding="utf-8")
engine = (root / "web/src/observer-engine.ts").read_text(encoding="utf-8")

for token in (
    "RecentSocialEventsPayload",
    "this.core.recentSocialEvents(32)",
    "socialEvents,",
):
    assert token in session, f"world session missing exact social-event path: {token}"

for token in (
    "socialEvents: RecentSocialEventsPayload",
    "this.snapshot.socialEvents",
    "nextSocialEvents",
    "deriveObservationEvents(",
):
    assert token in store, f"observer store missing director state: {token}"

for token in (
    "exactSocialEvents",
    "exactLifeEvents",
    "physicalNeedMilestoneEvent",
    "physicalPresentationSignature",
    "presentation.phase !== 'Moving'",
    "presentation.phase !== 'Interacting'",
    "directNaturalWaterSource",
    "designatedSanitationSite",
    "event.presentationLevel === 'Important'",
    "resident.lifeHistory",
    "socialParticipants",
    "if (!historySupported)",
):
    assert token in feed, f"observer director missing factual event logic: {token}"

# Exact Core social events suppress same-refresh relationship/activity guesses
# for their participants instead of duplicating one event three ways.
assert "!socialParticipants.has(resident.id)" in feed
assert "resident.activityKind !== 'Social'" in feed

# Initial/unavailable exact payloads never replay an old event tail as new.
assert "previous?.available !== true" in feed
assert "next?.available !== true" in feed

# UI remains opt-in: events are rendered as buttons; the engine only forwards
# event snapshots to state and does not force-select/focus from event arrival.
assert "case 'social': return '사회'" in ui
assert "onClick={() => onSelect(event.residentId!)}" in ui
assert "socialEvents: snapshot.socialEvents" in engine
assert "selectResident(snapshot.socialEvents" not in engine
assert "moveObserver(snapshot.socialEvents" not in engine

# Heavy authority payloads are not part of the event-director hot path.
# A later observation layer may sample them only behind the low-rate cache gate.
assert "worldActivityRefreshCountdown" in session
gate = session.index("this.worldActivityRefreshCountdown <= 0")
assert gate < session.index(".civilizationWorldWindow(")
assert gate < session.index(".worldObjects()")

print("LifeLens Web Observer Director v1: PASS")
