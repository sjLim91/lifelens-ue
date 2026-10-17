# Native tiger candidate — actual Observer acceptance pending

This candidate replaces the rejected Sloyd body in Draft #728 only. Main has
not been merged. Neither Blender success nor a subsystem screenshot is final
acceptance. Human replacement and physical Android performance remain open.

The new body is **Running Tiger by Amil (francescolima74), CC BY 4.0**.
Walking motion is retargeted from **tiger walk v02 by Unity (shehab house),
CC BY 4.0**. Both publicly distributed originals, individual license metadata,
retrieval dates, immutable redistribution URLs and SHA-256 hashes are retained
in [../distributed-tigers](../distributed-tigers/). Full attribution and
adaptations are in [CREDITS.md](CREDITS.md). The requested white Sketchfab
tiger required authenticated download and was not acquired or bypassed.

## Original body and editable derivative

Near retains the source topology, UV, native 47-joint skeleton and textured
face, including source cheek fur cards and whiskers. It is not a remeshed
primitive assembly. Sole-pad weights on 446 original vertices are corrected
to stop the pads stretching during stance. The original gallop and 38 mapped
walk-donor joints supply upper-body motion; stance trajectories use the native
limb chains. Seven clips cover idle, walk, run, hunt, forage, drink and rest.
They illustrate Core observations without fabricating prey capture or feeding
outcomes. Existing Core IDs, positions, direction, behavior and observation
data remain authoritative.

The packed, editable [tiger-native-authoring.blend](tiger-native-authoring.blend)
and all three generated GLBs are retained. Every runtime asset is generated
from hash-checked originals; the texture-reduced local inspection copies are
explicitly rejected by the production loader.

| LOD | Triangles per animal | Runtime work | Asset bytes |
| --- | ---: | --- | ---: |
| Near | 4,650 | At most 2 native skins | 2,780,544 |
| Mid | 2,600 | At most 4 native skins | 2,811,892 |
| Far | 998 | At most 128 instances, 3 shared pose batches | 574,848 |

Mid retains the original vertex streams and improves triangle/GPU cost; its
file is slightly larger because the extra reduced index stream is appended.
Far bakes the same source into idle, lying and lowered-head poses, with a
small GPU gait for moving animals. It does not allocate a skeleton per animal.
Far motion is an approximation and must be reviewed at its actual viewing
distance. Animals beyond these budgets retain the existing fallback.

## Checks performed

- Actual native skin contacts and seven clip loop seams pass. Source walk
  maximum measured stance drift is 2.85 mm, hunt 1.53 mm, run 46.43 mm.
  All four paws participate; the test samples the deformed mesh, not markers.
- A turning, uneven sloped-terrain test passes across idle, walk, hunt, run,
  rest and drink. Maximum measured stance contact error is 14.14 mm in walk;
  the other five states remain below 10.05 mm. The correction changes the
  visual model frame and original limb bones, never the actor's Core root.
- The real production WildlifeLayer passes ID/position/behavior binding to
  unmodified native Core fixture `25:32:1704075738371588112`, Hunt, origin
  rebasing, LOD refresh, removal and observation immutability. Native pose
  crossfades and bounded 128-actor far instancing pass.
- Source evidence run [39001708627](https://github.com/sjLim91/lifelens-ue/actions/runs/39001708627)
  passed source, runtime binding, TypeScript, wildlife regressions and Chromium
  production-layer screenshots with no browser errors. Its evidence commit
  encountered a concurrent branch update; the workflow now retries publication.
  This run did **not** run the full React/WASM Observer job.

Validation JSON files explicitly retain `mergeAllowed: false`. Physical
Android FPS has not been measured. The mobile viewport screenshot is not a
physical-device performance result.

## Visual review so far

The [front](idle-front.png), [side](idle-side.png),
[quarter](idle-quarter.png), [back](idle-back.png),
[face](idle-face.png), [paws](idle-feet.png), [walk](walk.png),
[run](run.png), [hunt](hunt.png), [feeding](forage.png),
[drinking](drink.png) and [lying](rest.png) images were inspected or preserved
for review. The face and connected shoulder/torso silhouette improve visibly
over the rejected Sloyd candidate: the muzzle no longer has petal-like white
geometry, and the eyes, nose, ears and textured cheek fur belong to the source
surface. The lying pose folds the original limbs rather than flattening the
entire body. Close paw silhouettes remain somewhat rounded, and fast-running
stance drift is larger than walking drift. Fur uses alpha cards and textured
surface detail rather than dense strand rendering.

These neutral Blender views do not establish quality in the live environment.
The full Observer must show both old fallback and new candidate under the
same paused WASM observation, camera and lighting, then show actual Core-driven
movement, far instancing and a mobile viewport without runtime/shader errors.
Actual-device 30 FPS measurement and the separate male/female resident work
are still required before the requested project can be called complete.

## 2026-10-17 interrupted deployment checkpoint recovered

The screenshot checkpoint was commit `f0c345bc`: publish generated character
GLBs despite the copied source `.gitignore`, and frame the full tiger. That
commit was preserved. Its local React/WASM capture passed, but public checking
still recorded character 404 responses and timed out in the full-page screenshot.
A subsequent normal main Pages publication removed the isolated Draft review.

Recovery code commit `f18b6209` verifies the staged character receipt before
publishing, waits for the review publication, checks all three public character
GLBs against pinned local receipts, rejects browser HTTP errors, and uses the
existing actual Observer canvas export instead of the hanging full-page capture.

[Recovery run 39018352801](https://github.com/sjLim91/lifelens-ue/actions/runs/39018352801)
passed source/skin/grounding/runtime checks, actual React/WASM comparison and
movement capture, and public-page verification. The retained
[`published-validation.json`](../../../web/tests/tiger-observer/review/published-validation.json)
records all four model/animation responses as HTTP 200, matching SHA-256,
ready WASM runtime, no browser/HTTP errors, and an actual `threeWorldCanvas`
image. This fixes the interrupted deployment checkpoint; it is not final
visual approval or measured physical Android performance.

[Draft review](https://sjlim91.github.io/lifelens-ue/review/tiger-728/index.html?review=1).
Normal main deployments can still replace this review directory until the
separate preview lifecycle is resolved. Main remains unmerged. The independent
MakeHuman resident-retarget Draft #731 remains source-input preparation; the
restored existing resident GLBs do not complete that replacement work.
