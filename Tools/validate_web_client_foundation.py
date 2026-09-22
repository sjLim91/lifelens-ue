#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
cmake = (root / "Source/LifeLensCore/CMakeLists.txt").read_text(encoding="utf-8")
bridge_h = (root / "Source/LifeLensCore/include/lifelens/WebClientBridge.h").read_text(encoding="utf-8")
bridge_cpp = (root / "Source/LifeLensCore/src/WebClientBridge.cpp").read_text(encoding="utf-8")
bindings = (root / "Source/LifeLensCore/wasm/LifeLensWebBindings.cpp").read_text(encoding="utf-8")
app = (root / "Clients/Web/app.js").read_text(encoding="utf-8")
adapter = (root / "Clients/Web/core-adapter.js").read_text(encoding="utf-8")
html = (root / "Clients/Web/index.html").read_text(encoding="utf-8")
sw = (root / "Clients/Web/sw.js").read_text(encoding="utf-8")
manifest = (root / "Clients/Web/manifest.webmanifest").read_text(encoding="utf-8")
architecture = (root / "docs/WEB_CLIENT_ARCHITECTURE_v1.md").read_text(encoding="utf-8")
web_package = (root / "web/package.json").read_text(encoding="utf-8")
web_contract_generator = (root / "web/scripts/generate-core-contract.mjs").read_text(encoding="utf-8")
web_generated_contract = (root / "web/src/runtime/generated-core-contract.ts").read_text(encoding="utf-8")
web_contract = (root / "web/src/runtime/lifelens-contract.ts").read_text(encoding="utf-8")

for token in (
    "class WebClientBridge",
    "worldOverviewJson",
    "residentsJson",
    "terrainWindowJson",
    "std::unique_ptr<Simulation>",
):
    assert token in bridge_h, f"missing web bridge contract: {token}"

for token in (
    "simulation_->observeWorldOverview()",
    "simulation_->observeAllResidents()",
    "simulation_->runtimePosition",
    "deriveContinuousTerrainSample",
    "deriveHydrologyFacts",
):
    assert token in bridge_cpp, f"web bridge must consume Core truth: {token}"


for token in (
    "WorldGenesis.h",
    "SimulationCalendar.h",
    "WorldChunkSpanGridCells",
    "SimulationMinutesPerHour",
    "SimulationHoursPerDay",
    "generated-core-contract.ts",
):
    assert token in web_contract_generator, f"missing Core -> Web contract generation: {token}"

for token in (
    "CORE_WORLD_CHUNK_SPAN_GRID_CELLS",
    "CORE_SIMULATION_MINUTES_PER_DAY",
):
    assert token in web_generated_contract, f"missing generated Core Web contract: {token}"
    assert token in web_contract, f"Web contract must consume generated Core authority: {token}"

for token in (
    '"generate:core-contract"',
    '"predev"',
    '"pretypecheck"',
    '"prebuild"',
):
    assert token in web_package, f"missing automatic Core contract generation hook: {token}"

# JavaScript must not reimplement simulation truth.
for forbidden in (
    "Math.random(",
    "fakeResident",
    "fakeTerrain",
    "generateResident(",
    "generateTerrain(",
):
    assert forbidden not in app
    assert forbidden not in adapter

assert 'String(worldSeed)' in adapter
assert '"worldSeed":\""' in bridge_cpp or '\"worldSeed\\":\\\"' in bridge_cpp
assert "LifeLensCore WASM unavailable" in html
assert "Core WASM not built" in app

for token in (
    "if(EMSCRIPTEN)",
    "lifelens_web_core",
    "--bind",
    "-sMODULARIZE=1",
    "-sEXPORT_ES6=1",
    "-sALLOW_MEMORY_GROWTH=1",
):
    assert token in cmake, f"missing Emscripten build contract: {token}"

for token in (
    'class_<lifelens::WebClientBridge>',
    '"LifeLensWebClient"',
    '.function("newGame"',
):
    assert token in bindings, f"missing Embind contract: {token}"

assert '"display": "standalone"' in manifest
assert 'self.addEventListener("fetch"' in sw

for token in (
    "LifeLensCore",
    "Unreal Native",
    "Web / PWA",
    "No fake fallback",
):
    assert token in architecture

print("LifeLens Web client foundation structural validation: PASS")
