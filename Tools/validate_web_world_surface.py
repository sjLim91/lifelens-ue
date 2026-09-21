#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
renderer = (root / "Clients/Web/world-surface-renderer.js").read_text(encoding="utf-8")
app = (root / "Clients/Web/app.js").read_text(encoding="utf-8")
html = (root / "Clients/Web/index.html").read_text(encoding="utf-8")
css = (root / "Clients/Web/styles.css").read_text(encoding="utf-8")

for token in (
    'getContext("webgl2"',
    'getContext("webgl2"',
    'gl.drawArrays(gl.TRIANGLES',
    'terrainColor',
    'elevation01',
    'waterKind',
    'pointerdown',
    'pointermove',
    'wheel',
    'pinchDistance',
):
    assert token in renderer, f"missing WEB-1 surface renderer token: {token}"

for forbidden in (
    "Math.random(",
    "fakeTerrain",
    "fakeWater",
    "generateTerrain(",
):
    assert forbidden not in renderer, f"WEB-1 renderer must not invent world truth: {forbidden}"

assert 'WorldSurfaceRenderer' in app
assert 'core.terrainWindow(centerChunkX, centerChunkY, 8)' in app
assert 'renderer.render(terrain)' in app
assert '3D truth surface' in html
assert 'touch-action: none' in css

print("LifeLens WEB-1 3D truth-surface structural validation: PASS")
