# LifeLens Web Client Architecture v1

> Status: **CANONICAL / IMPLEMENTATION ACTIVE**
>
> Date: 2026-09-21 KST
>
> Decision: LifeLens is no longer defined as "an Unreal project". It is a
> platform-neutral simulation product with multiple presentation clients.

## 1. Product shape

```text
                         LifeLensCore
                World + Human simulation truth
                              |
                 platform-neutral read contracts
                              |
             +----------------+----------------+
             |                                 |
        Unreal Native                     Web / PWA
   Windows / macOS / Android            Browser client
      high visual ceiling           fast access / observer
```

The clients may have different visual quality and performance budgets, but they
must not create different simulation truth for the same save/world identity.

## 2. Authority

### LifeLensCore owns

- WorldSeed / PopulationSeed / WorldGenerationVersion.
- world generation.
- continuous terrain / hydrology / ecology truth.
- resident identity/state.
- needs/emotion/personality/memory/belief.
- relationships/family/lifecycle.
- civilization/history.
- authoritative save state.

### Unreal client owns

- Unreal rendering.
- native terrain/water/vegetation presentation.
- skeletal presentation / animation / IK.
- native collision/navigation execution where delegated by Core contract.
- native UI/camera/VFX/audio.
- high-end desktop visual tier and native Android tier.

### Web client owns

- browser/PWA lifecycle.
- WebGPU/WebGL presentation.
- browser input/camera.
- glTF/GLB character/prop presentation.
- browser-local persistence transport.
- HTML/CSS observer UI.

The Web client never forks or reimplements needs, relationships, world
generation, hydrology or other simulation rules.

## 3. Shared identity

For a given:

```text
WorldSeed
PopulationSeed
WorldGenerationVersion
save snapshot
```

Unreal and Web must observe the same logical world.

64-bit IDs/seeds cross the JavaScript boundary as decimal strings unless a
BigInt-specific contract is explicitly introduced. They must not be silently
rounded through JavaScript Number.

## 4. Web runtime bridge

The first bridge is `lifelens::WebClientBridge`.

It exposes JSON snapshots across the WASM ABI so that browser code does not
depend on C++ memory layout.

Initial contract:

- new game.
- run simulation minutes.
- world overview.
- residents.
- terrain/hydrology observation window.

Future additions:

- ecology/biome window.
- event/history feed.
- selected resident detail.
- genealogy.
- save snapshot import/export.
- observer interest queries.
- persistent world delta.
- action/motion presentation DTO.

## 5. WASM build

`LifeLensCore` stays C++17.

Emscripten is an additional build target, not a separate Core fork.

```bash
python Tools/build_web_client.py
```

Generated `.js/.wasm` files are build artifacts and are excluded from Git.

## 6. Web rendering progression

### WEB-0 — truth shell
- PWA shell.
- WASM load/fail-closed.
- actual Core overview/residents.
- actual terrain/hydrology diagnostic preview.

### WEB-1 — WebGPU world surface
- continuous terrain mesh.
- observer-centered streaming.
- water surface.
- free pan/orbit/zoom.
- same world address as native client.

### WEB-2 — ecology
- biome coverage.
- instanced vegetation.
- forest near/mid/far representation.
- rock/ground-cover clustering.

### WEB-3 — residents
- glTF/GLB resident presentation.
- semantic action -> animation mapping.
- target alignment.
- human realism read contracts.

### WEB-4 — observer product
- resident detail.
- relationship/family/genealogy.
- event tracking.
- speed controls.
- save/load.
- installable PWA/offline cache.

## 7. No fake fallback

If Core WASM fails to load:

- do not spawn fake residents.
- do not generate a JavaScript-only terrain.
- do not synthesize events.
- show explicit Core-unavailable state.

A visual placeholder may exist only as UI chrome and must never look like
authoritative LifeLens world state.

## 8. Performance model

Web is not required to match Unreal desktop rendering feature-for-feature.

Shared:
- simulation truth.
- world identity.
- logical geography.
- resident/history state.

Different:
- visible radius.
- mesh subdivision.
- vegetation density.
- shadow quality.
- materials.
- animation sophistication.
- effects.

The world is not made smaller to fit Web. Only presentation budgets change.

## 9. Persistence

Long-term Web persistence:

- snapshot bytes from the same Core save codec.
- OPFS preferred when supported.
- IndexedDB fallback.
- optional user-controlled file import/export.

Do not invent a separate incompatible Web save format for simulation state.

## 10. Deployment

The Web client must be static-hostable.

No paid API or required cloud backend is allowed for the baseline product.

Generated browser runtime binaries are not committed to source. The canonical
zero-cost publication path builds `LifeLensCore` with Emscripten in GitHub
Actions and replaces the stable `web-runtime-latest` GitHub Release assets:

- `lifelens_core.js`
- `lifelens_core.wasm`
- `SHA256SUMS.txt`

A static browser host may load that stable runtime release. Hosting remains a
replaceable presentation concern and never becomes simulation authority.

GitHub Pages is not a required/canonical deployment dependency. If repository
integration permissions cannot create a Pages site, that must not block Web
runtime publication or the local single-player product.

A backend may later provide optional synchronization/multiplayer/community
features, but local single-player simulation must remain functional without it.

## 11. Relationship to World v2

World v2 work remains valid and becomes more important.

Continuous terrain, hydrology, ecology and observer-interest APIs are
platform-neutral truths that both Unreal and Web consume.

Web work must not delay the current World v2 native critical path when the two
can proceed independently.

## 12. Acceptance

Web foundation is accepted when:

1. native Core tests still pass.
2. Web bridge creates the same deterministic new-game truth for the same seeds.
3. 64-bit seeds survive round-trip without JavaScript precision loss.
4. browser UI fails closed without WASM.
5. after WASM staging, browser can show actual Core residents and world overview.
6. terrain/hydrology preview comes from Core.
7. PWA shell works without a paid backend.

Visual WebGPU acceptance is a later milestone and is not implied by WEB-0.
