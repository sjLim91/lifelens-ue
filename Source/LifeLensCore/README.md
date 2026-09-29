# LifeLensCore

LifeLensCore is the renderer-independent C++17 simulation authority for LifeLens.

It owns deterministic world/resident state, needs and behavior, relationships/family/lifecycle, civilization/resources/facilities, environment, persistence, and the contracts exposed to presentation clients.

The active product path is:

```text
LifeLensCore -> Emscripten/WASM -> Web Observer
```

## Local verification

```bash
cmake -S Source/LifeLensCore -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/ll_harness --days 1 --seed 42
```

Useful harness options:

- `--days N`: simulated days
- `--seed S`: deterministic WorldSeed
- `--tick-log`: per-minute Needs logging

The same seed and build must produce deterministic event output for deterministic test scenarios.

## Web runtime

The browser runtime is built from this same Core with Emscripten:

```bash
python Tools/build_web_client.py
```

Do not move simulation authority into React/TypeScript/Three.js. Browser code consumes explicit Core/WASM contracts.
