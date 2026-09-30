#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

session = (root / "web/src/runtime/world-session.ts").read_text(encoding="utf-8")
store = (root / "web/src/state/observer-store.ts").read_text(encoding="utf-8")
feed = (root / "web/src/state/observation-feed.ts").read_text(encoding="utf-8")
ui = (root / "web/src/ui/observation-feed.tsx").read_text(encoding="utf-8")
engine = (root / "web/src/observer-engine.ts").read_text(encoding="utf-8")
actions = (root / "web/src/state/observer-actions.ts").read_text(encoding="utf-8")
app = (root / "web/src/App.tsx").read_text(encoding="utf-8")
styles = (root / "web/src/styles.css").read_text(encoding="utf-8")

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
    "socialInteractionMilestoneEvent",
    "socialPresentationSignature",
    "directionalRelationshipResult",
    "SOCIAL_RELATIONSHIP_RESULT_DIMENSIONS",
    "directionalPairCounts",
    "signedPercentagePoint",
    "KnowledgeTeaching",
    "Parenting",
    "presentation.phase !== 'Moving'",
    "presentation.phase !== 'Interacting'",
    "directNaturalWaterSource",
    "designatedSanitationSite",
    "event.presentationLevel === 'Important'",
    "resident.lifeHistory",
    "socialParticipants",
    "if (!historySupported)",
    "focusGridX",
    "focusGridY",
    "residentFocus",
    "presentationFocus",
):
    assert token in feed, f"observer director missing factual event logic: {token}"

# Exact Core social events suppress same-refresh relationship/activity guesses
# for their participants instead of duplicating one event three ways.
assert "!socialParticipants.has(resident.id)" in feed
assert "resident.activityKind !== 'Social'" in feed

# Initial/unavailable exact payloads never replay an old event tail as new.
assert "previous?.available !== true" in feed
assert "next?.available !== true" in feed

# UI remains opt-in: event arrival never moves the camera automatically.
# Only a user click invokes the explicit world-focus action.
assert "case 'social': return '사회'" in ui
assert "onClick={() => onFocus(event)}" in ui
assert "사건 현장 보기" in ui
assert "ObservationFocusBanner" in ui
assert "focusObservation: (event: ObservationEvent) => void" in actions
assert "focusObservation(event: ObservationEvent)" in actions
assert "focusedObservationId" in store
assert "focusObservation(event: ObservationEvent)" in engine
assert "humanTraceFocus({ gridX, gridY })" in engine
assert "observerStore.focusObservation(event.id, preferredResidentId)" in engine
assert "ObservationFocusBanner" in app
assert "observerActions.focusObservation(event)" in app
assert "observation-focus-banner" in styles
assert "socialEvents: snapshot.socialEvents" in engine
assert "focusObservation(snapshot.socialEvents" not in engine
assert "moveObserver(snapshot.socialEvents" not in engine

# Heavy authority payloads are not part of the event-director hot path.
# A later observation layer may sample them only behind the low-rate cache gate.
assert "worldActivityRefreshCountdown" in session
gate = session.index("this.worldActivityRefreshCountdown <= 0")
assert gate < session.index(".civilizationWorldWindow(")
assert gate < session.index(".worldObjects()")

print("LifeLens Web Observer Director v1: PASS")

# Exact social outcomes may expose relationship changes only from the actual
# recipient -> actor Core relationship snapshot. Multiple events for the same
# directional pair in one heavy refresh window must not receive invented
# per-event attribution.
assert "event.targetId" in feed
assert "event.actorId" in feed
assert "previousResidents" in feed
assert "nextResidents" in feed
assert "directionalPairCounts.get(pair) === 1" in feed
