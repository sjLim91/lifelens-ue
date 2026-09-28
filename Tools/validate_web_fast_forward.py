#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


engine = read("web/src/observer-engine.ts")
actions = read("web/src/state/observer-actions.ts")
store = read("web/src/state/observer-store.ts")
session = read("web/src/runtime/world-session.ts")
model = read("web/src/state/fast-forward.ts")
ui = read("web/src/ui/fast-forward-control.tsx")
app = read("web/src/App.tsx")

for token in (
    "fastForwardDays: (days: number) => Promise<void>",
    "clearFastForwardResult",
):
    assert token in actions, f"fast-forward action contract missing: {token}"

for token in (
    "beginFastForward(",
    "updateFastForwardProgress(",
    "completeFastForward(",
    "failFastForward(",
    "fastForward: FastForwardState",
):
    assert token in store, f"fast-forward store contract missing: {token}"

for token in (
    "MAX_FAST_FORWARD_DAYS = 3650",
    "FAST_FORWARD_CHUNK_MINUTES = 360",
    "MINUTES_PER_DAY = 1440",
    "buildFastForwardSummary(",
    "facilityChanges",
    "newDiscoveries",
    "lifeEvents",
    "resourceChanges",
):
    assert token in model, f"fast-forward summary contract missing: {token}"

for token in (
    "simulationClock?.stop();",
    "worldSession.runMinutes(chunkMinutes);",
    "FAST_FORWARD_CHUNK_MINUTES",
    "await yieldToBrowser();",
    "updateFastForwardProgress(completedMinutes)",
    "worldSession.forceWorldActivityRefresh();",
    "worldSession.recenterToResidents();",
    "buildFastForwardSummary(requestedDays, before, after)",
    "simulationClock?.start();",
):
    assert token in engine, f"authoritative fast-forward orchestration missing: {token}"

# Intermediate render/refresh must not occur inside the chunk loop.
loop_start = engine.index("while (completedMinutes < totalMinutes)")
loop_end = engine.index("worldSession.forceWorldActivityRefresh();", loop_start)
loop_body = engine[loop_start:loop_end]
assert "refresh();" not in loop_body, (
    "fast-forward chunk loop must not render/refresh intermediate states"
)
assert "runMinutes(totalMinutes)" not in engine, (
    "fast-forward must remain chunked so the browser can update progress"
)

assert "forceWorldActivityRefresh" in session

for token in (
    "일수 건너뛰기",
    "이만큼 진행",
    "계산 중…",
    "<progress",
    "시설 변화",
    "새로운 발견",
    "주요 삶의 변화",
    "자연 자원 변화",
):
    assert token in ui, f"fast-forward Korean UI missing: {token}"

for token in (
    "<FastForwardControl",
    "snapshot.fastForward.status === 'running'",
    "세계 변화 계산 중",
):
    assert token in app, f"fast-forward app integration missing: {token}"

assert "world.minute +" not in engine
assert "minute +=" not in engine

print("LifeLens authoritative day fast-forward: PASS")
