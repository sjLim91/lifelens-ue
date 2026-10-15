# Tiger integration checkpoint — 2026-10-15

**Not accepted for merge.** Draft PR #728 remains experimental. Core simulation,
RNG, genetics, decisions and persistence are unchanged. Human final quality and
web retargeting remain pending until the tiger unit passes visual review.

## Implemented and measured

The licensed TopoForge/Sloyd CC BY 4.0 source is retained with its original URL,
creator credit and SHA-256. Source geometry is separated from its fused log,
retains UV/PBR data, and receives a 24-bone rig and seven presentation clips.
Exact-source Blender generation, staging hashes, actual Three.js loader/skin
contracts and existing wildlife/grounding regressions have run in Actions.

The production WildlifeLayer uses the actual Core animal ID, position, heading
and behavior. Near/mid imported skins have explicit budgets (2 near, 4 mid),
16,913/5,919 triangles, and 2048/1024 textures. Failed loads and distant animals
use the existing instanced fallback. Terrain orientation and camera-distance
selection account for the real ground elevation and origin rebasing.

Flat-ground authored walk validation measures 0.01335 m vertical contact error,
0.01945 m stance slip and zero horizontal root translation. These are bounded
clip measurements, not proof of natural motion on all terrain or turns.

## Actual React / WASM Observer evidence

The real React application, production Vite build, compiled unchanged WASM Core,
WorldScene and WorldRenderer are exercised. Images are exports of the actual
`threeWorldCanvas` after its real renderer draws. They are not standalone Blender
renders or a separate model viewer. They are canvas images, not full UI screenshots.

Run https://github.com/sjLim91/lifelens-ue/actions/runs/38690890134 saved actual
production Observer front/side/quarter/back/face images, a mobile viewport image,
and motion samples after Core time progressed. It failed honestly because two
Korean UI tokens were missing and the separate baseline/candidate worlds paused
one minute apart. The current follow-up translates the observed institution and
organization tokens and compares fallback/candidate in **one paused WASM world**.
Only the existing renderer presentation is switched; the Core observation must
stay byte-identical throughout the comparison. Actual Core movement is tested
after those still captures. Workflow runs and evidence remain authoritative;
no pending run is described as passing.

Images and structured observations: `web/tests/tiger-observer/review/`.
Subsystem grounding/skin evidence: `web/tests/procedural-environment/review/`.
Near GLB SHA-256: `c797317862d664b7ad421e283e7d82ed7eb7acf2c428b85e16158571d57c49ce`.
Mid GLB SHA-256: `f0e1230f450a8a7b841f548b6936c5490338b1e1e4b5731c7c17e8c8bfa7e0e7`.

## Visual review: FAIL / unresolved

The new tiger is distinguishable from the previous primitive model in the actual
Observer. That improvement is insufficient for acceptance: the muzzle/whisker
region has jagged source defects, fur is limited, the resting/feeding poses need
anatomical work, and turns, transitions and foot locks need review. Nearby trees
and other animals obscure parts of several captured views. Existing primitive
fauna also remain visible. They are not hidden or moved to improve the evidence.

A mobile-size Chromium viewport is **not** an Android device measurement.
Real Android >=30 FPS, scene/population/resolution benchmarks, source-derived far
LOD, final multi-angle visual acceptance and humans are still outstanding.

## Better source research, without admission

A more realistic reference tiger was found in the official public
`google-ar/jetpack-xr-natives` repository, pinned at
`4eb56548351f7f8277df7eaab5354c83db6e1e16`, path
`impress/core/window/Tiger.glb`. The ephemeral metadata-only workflow retrieved
9,598,664 bytes, SHA-256
`617104e0890bdbcd2c62e36663343fdc6db105d10e1815233201ea4bc1ae7989`.
It has 120 joints, one animation and embedded JPEG textures. The GLB has no author
or copyright metadata; the repository has an Apache 2.0 code license and its
third-party notices do not identify the tiger. Asset-specific provenance and
redistribution rights are therefore **unverified**. No copy of that model is
admitted or committed, and it is not used in LifeLens. Research receipt:
`docs/asset-reviews/google-tiger/metadata.json`.

## Environment and approval status

GitHub connector reads/writes now work; source and evidence are on the Draft
branch. Local proxy/Chromium failures are avoided through authorized GitHub
Actions runners. There is no current local Chromium permission request waiting
for the user. No paid provider or authentication bypass is used.
