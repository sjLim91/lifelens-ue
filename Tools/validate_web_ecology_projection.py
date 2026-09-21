#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
bridge = (root / "Source/LifeLensCore/src/WebClientBridge.cpp").read_text(encoding="utf-8")
bridge_test = (root / "Source/LifeLensCore/tests/test_web_client_bridge.cpp").read_text(encoding="utf-8")
renderer = (root / "Clients/Web/world-surface-renderer.js").read_text(encoding="utf-8")

for token in (
    '#include "lifelens/ContinuousEcology.h"',
    "deriveContinuousEcologySample(identity, center)",
    "continuousEcologyBiomeName(ecology.biome)",
    "forestCoverage01",
    "grassCoverage01",
    "shrubCoverage01",
    "rockCoverage01",
    "wetlandCoverage01",
    "worldSeed",
):
    assert token in bridge, f"WEB-2 bridge ecology contract missing: {token}"

for token in (
    "biome",
    "forestCoverage01",
    "grassCoverage01",
    "rockCoverage01",
    "wetlandCoverage01",
):
    assert token in bridge_test, f"WEB-2 bridge test missing ecology field: {token}"

for token in (
    "appendEcologyPresentation",
    "presentationHash01",
    "forestCoverage01",
    "grassCoverage01",
    "shrubCoverage01",
    "rockCoverage01",
    "wetlandCoverage01",
    'data.worldSeed',
    "appendCrossedBillboard",
    "appendRockMarker",
):
    assert token in renderer, f"WEB-2 renderer ecology projection missing: {token}"

assert "Math.random(" not in renderer, (
    "WEB-2 ecology visualization must be deterministic and must not invent "
    "browser-random world structure"
)

# Coverage is presentation density only; renderer must not imply or fabricate
# authoritative Core resource inventory.
for forbidden in (
    "resourceNodes",
    "ResourceNode",
    "createResource",
    "spawnResource",
):
    assert forbidden not in renderer, f"WEB-2 renderer crossed resource authority boundary: {forbidden}"

forest_count = renderer.index("const treeCount =")
forest_use = renderer.index("appendCrossedBillboard(", forest_count)
assert renderer.index("forestCoverage01") < forest_count < forest_use

rock_count = renderer.index("const rockCount =")
rock_use = renderer.index("appendRockMarker(", rock_count)
assert renderer.index("rockCoverage01") < rock_count < rock_use

print("LifeLens WEB-2 ecology projection validation: PASS")
