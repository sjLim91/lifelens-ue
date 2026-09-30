#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]

header = (root / "Source/LifeLensCore/include/lifelens/WebClientBridge.h").read_text(encoding="utf-8")
bindings = (root / "Source/LifeLensCore/wasm/LifeLensWebBindings.cpp").read_text(encoding="utf-8")
bridge = (root / "Source/LifeLensCore/src/WebClientBridge.cpp").read_text(encoding="utf-8")
presentation = (root / "Source/LifeLensCore/include/lifelens/PresentationDirective.h").read_text(encoding="utf-8")
simulation = (root / "Source/LifeLensCore/src/Simulation.cpp").read_text(encoding="utf-8")
civilization = (root / "Source/LifeLensCore/include/lifelens/CivilizationObserverReadModel.h").read_text(encoding="utf-8")
types = (root / "web/src/runtime/core-types.ts").read_text(encoding="utf-8")
web_bridge = (root / "web/src/runtime/core-bridge.ts").read_text(encoding="utf-8")
session = (root / "web/src/runtime/world-session.ts").read_text(encoding="utf-8")

for method in (
    "residentRuntimeJson",
    "humanTracesWindowJson",
    "recentSocialEventsJson",
    "civilizationWorldJson",
    "civilizationWorldWindowJson",
    "worldObjectsJson",
):
    assert method in header, f"WebClientBridge header missing {method}"
    assert method in bindings, f"WASM binding missing {method}"
    assert method in bridge, f"WebClientBridge implementation missing {method}"
    assert method in types, f"RuntimeClient type missing {method}"

for token in (
    "civilizationItem",
    "civilizationTechnique",
    "civilizationQuantity",
    "civilizationResourceNode",
    "civilizationStorage",
    "facilityId",
    "facilityKind",
    "knowledgeTeachingTechnique",
    "issuedMinute",
):
    assert token in presentation, f"PresentationDirective missing {token}"
    assert token in simulation, f"Simulation presentation projection missing {token}"
    assert token in bridge, f"resident JSON missing {token}"
    assert token in types, f"Web presentation type missing {token}"

assert "GridPos pos{};" in civilization
assert "observed.pos=node.pos;" in civilization
assert "dto.pos=storage.pos;" in civilization
for token in (
    '\\\"gridX\\\"',
    '\\\"gridY\\\"',
    '\\\"resources\\\"',
    '\\\"storages\\\"',
    '\\\"facilities\\\"',
    '\\\"recentDiscoveries\\\"',
):
    assert token in bridge, f"civilization world JSON missing {token}"

for token in (
    '\\\"lifeHistory\\\"',
    '\\\"lifeStages\\\"',
    '\\\"datingCouples\\\"',
    '\\\"engagedCouples\\\"',
    '\\\"marriedCouples\\\"',
    '\\\"separatedCouples\\\"',
    '\\\"baselineTemperature01\\\"',
    '\\\"seasonalTemperatureModifierC\\\"',
    '\\\"dailyTemperatureModifierC\\\"',
    '\\\"calendar\\\"',
):
    assert token in bridge, f"observer JSON missing {token}"

for token in (
    "gradientX?: number",
    "gradientY?: number",
    "salinity?:",
    "radiusChunks?: number",
    "RecentSocialEventsPayload",
    "CivilizationWorldPayload",
    "WorldObjectsPayload",
    "ResidentCivilization",
    "ResidentLifeEvent",
    "SimulationCalendar",
):
    assert token in types, f"core-types missing {token}"

for token in (
    "residentRuntime()",
    "humanTracesWindow(",
    "recentSocialEvents(maxEvents = 32)",
    "civilizationWorld(",
    "civilizationWorldWindow(",
    "worldObjects()",
):
    assert token in web_bridge, f"LifeLensCoreBridge missing {token}"

# The Observer Director may poll the bounded recent-social tail on the normal
# refresh path because it is small and event-oriented. Larger civilization and
# object payloads may be consumed only through an explicit low-rate cache gate.
assert ".recentSocialEvents(32)" in session
assert "worldActivityRefreshCountdown" in session
assert "worldActivityWindowKey" in session
assert "this.worldActivityRefreshCountdown <= 0" in session
assert ".civilizationWorldWindow(" in session
assert ".worldObjects()" in session
gate = session.index("this.worldActivityRefreshCountdown <= 0")
assert gate < session.index(".civilizationWorldWindow(")
assert gate < session.index(".worldObjects()")

print("LifeLens Web authority DTO expansion: PASS")

# High-frequency observer state uses compact runtime DTOs. Full resident detail
# and large world payloads are sampled away from the 500 ms movement cadence.
for token in (
    "residentDetailRefreshCountdown",
    "residentDetailSnapshot",
    "this.core.residentRuntime()",
    "this.core.humanTracesWindow(",
    "terrainStaticChanged",
    "residentDetailRefreshEverySnapshots",
):
    assert token in session or token in types, (
        f"observer hot-path decoupling missing: {token}"
    )
