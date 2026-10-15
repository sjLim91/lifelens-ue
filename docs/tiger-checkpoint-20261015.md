# Tiger integration checkpoint — 2026-10-15

Status: incomplete, unpublished, main merge prohibited.

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

## Publishing and recovery

GitHub reads work, but the previous write calls did not complete. Branch head
remains the hash above; no current CI or PR update is claimed. Local patch and
evidence archives are preserved in `/workspace/lifelens-delivery`.

The patch also adds a visible Korean asset-credit link and prepares CC BY
credits/receipt generation. No paid services, authentication bypass or changes
to Core authority, RNG, genetics, AI or persistence are introduced.
