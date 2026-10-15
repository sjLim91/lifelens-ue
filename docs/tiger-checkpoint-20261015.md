# Tiger integration checkpoint — 2026-10-15

Status: incomplete Draft, main merge prohibited.

## GitHub retry and actual-source results

GitHub writes recovered on retry. Code was published in `af3a860`, followed by
capture-path/actual-Observer QA changes in `7fbe1fb`. Draft PR #728 remains open.
Preflight, Blender toolchain and the complete Web Checks run for `7fbe1fb` passed.
The local-write failure notes below describe the earlier checkpoint.

Run 38685825943 regenerated from the exact pinned original and passed source
skin/animation contracts, flat-ground walk, production layer/Core binding,
typecheck and the browser model-loading comparison. The recorded near/mid GLB
hashes are `c797317862d664b7ad421e283e7d82ed7eb7acf2c428b85e16158571d57c49ce`
and `f0e1230f450a8a7b841f548b6936c5490338b1e1e4b5731c7c17e8c8bfa7e0e7`.
Those files, credits/receipts and Blender sources are now on the Draft branch.

Manual inspection found that the subsystem comparison harness positioned its
chunk geometries at the wrong origin, leaving visible animals without terrain.
That old comparison is not valid ground-contact evidence. The harness now uses
the production chunk offsets and asserts a terrain-ray/ground-sampler match at
the actual observed tiger. New screenshots must be inspected again.

The real React/WASM app also booted successfully with seed 77 and observed the
same actual Core tiger. Its first full-screen capture timed out while drawing;
`web/tests/tiger-observer/review/observer-validation.json` records failure, not
success. The test now pauses the presentation render loop for individual stills,
using the real renderer, scene, materials and Core. Live motion sampling resumes
the original loop and advances only through existing Observer actions.

A separate CPU skin/terrain diagnostic found about 21 cm front-paw clearance
on the native Core slope. A presentation-only ground frame now follows sampled
terrain with bounded pitch/roll and correct support height. On the local
inspection derivative, minimum sole contact errors became approximately
-1.08 / -0.04 / -0.16 / +0.38 cm for the four paws. A regression test checks the
actual deformed skin against Core terrain, including a visible-window boundary.
Exact-source CI and new Observer images are still required for this change.

Android device FPS, turning foot locks, full clip deformation and final face/fur
quality remain unaccepted. No overall completion or main merge is authorized
by these intermediate checks.

## New verified progress

- Recovered Three.js r180 (matching Web dependency 0.180.0), GLTFLoader,
  SkeletonUtils and required utility directly from the public upstream GitHub
  tag, preserving MIT LICENSE. These are local test dependencies, not vendored
  application changes or a new production dependency path.
- Restored 271 unchanged Core source/header/build files from Draft head
  `09cd15252b8da56ed78a934801a740f3e938d9bb`. Built the existing native
  `harness/ecology_fixture.cpp` directly with g++ C++17 because local CMake is
  unavailable. No Core files were modified. Seed 77 / generation 4 / two minutes
  produced 26 observed animals, including two actual Core tigers.
- Parsed near/mid inspection GLBs in the real Three.js GLTFLoader. Verified
  24 skin joints, normalized weights and seven animation clips. The actual skin
  deformed by up to 0.257283 m in the sampled test. Node substitutes empty
  textures only for this structural test: texture appearance remains untested.
- Tested pooled skin identity, bounded near/mid counts (2/4), removal, loading
  fallback and disposal after an outstanding load resolves.
- Tested the production WildlifeLayer against actual native Core observations:
  skin selection, ID, position, behavior, unchanged observation, origin rebasing
  retaining the same skeleton and removal all passed.
- Existing `grounding-wildlife.mjs` passed after the renderer fix: 1,144 actual
  terrain intersection cases, five-species geometry/LOD, Core-only observation,
  duplicate/removal/disposal handling and juvenile/adult fish water bounds.

## Renderer defect found and fixed

The existing camera-distance calculation measured camera Y against a constant
0.5 m instead of the animal's ground height. On the actual Core fixture, ground
was about 23.1 m high. A camera only a few metres from a tiger therefore chose
far LOD and never displayed the imported skin. Camera distance now uses the
animal's sampled ground height plus 0.5 m. The native Core binding test failed
before the fix and passed afterward. Simulation truth was not changed.

The native observation used ID `25:32:1704075738371588112`, behavior `Hunt`,
grid position `(801,1038)`. Static presentation does not invent an attack:
locomotion clips require observed presentation travel.

## Browser failure is not a pass

Prepared before/after capture tests using the production layer and the native
Core fixture. Chromium failed before opening a page: Crashpad setsockopt and
then sandbox-host shutdown returned Operation not permitted. Normal startup,
disabling Crashpad and using a single process did not overcome the environment
restriction. A request for additional network permissions did not return and
was cancelled. It is not retried. No browser screenshot or Android performance
result is claimed. A mobile viewport would not establish Android performance.

The capture test writes explicit failure evidence even if startup fails. Future
Actions steps are prepared for exact-source regeneration, native Core fixture,
typecheck, layer contracts/regressions and subsystem browser comparisons. They
have not run; they are not full React/WASM Observer acceptance.

## Source and quality limitations remain

Local GLBs remain inspection derivatives reconstructed from rounded source
coordinates and a reduced JPEG. The pinned original is preserved in PR #728;
the strict staging script rejects the local reconstruction as a source.
Final exact-source rig/LOD generation in Actions has not run. Face/whisker
artifacts, limited fur detail, static rest/feeding anatomy, transitions, slopes,
turns and foot locks need further visual work. Do not mark these natural or
accepted. Human final quality/retargeting is still pending.

## Earlier publishing failure and local recovery record

GitHub reads work, but the previous write calls did not complete. Branch head
remains the hash above; no current CI or PR update is claimed. Local patch and
evidence archives are preserved in `/workspace/lifelens-delivery`.

The patch also adds a visible Korean asset-credit link and prepares CC BY
credits/receipt generation. No paid services, authentication bypass or changes
to Core authority, RNG, genetics, AI or persistence are introduced.
