# Native resident integration — active checkpoint, 2026-10-17

## User scope and authorization
- Complete the MakeHuman male/female integration and merge into main after verification.
- Use the connected GitHub tools for all remote repository operations. Do not retry terminal network access.
- Main merge is explicitly authorized in this conversation; no additional permission is pending.
- Do not spawn subagents unless the user explicitly asks.

## Canonical branch / PR
- PR #731: feat/native-resident-retarget-20261017 -> main.
- Main baseline: 6b4fd15f8b690df0e195f8f446c000cf108d4ebf.
- Included latest tiger #728 head bc17a47c5196748e66decc147ce6b334aa47b9bf.
- Main's later AI/Core changes and tiger/human changed paths had no overlap.
- Integrated tree was built on latest main, preserving its AI/Core files.
- Current implementation code checkpoint: 21eaf5d80f1f76f086a961ca78c7b78cee6d5751.
- Re-query actual PR head: workflows append generated assets/reports using [skip ci] commits.

## Implemented
- Tools/blender/retarget_native_residents.py: original CC0 body/sex morphs/UV/official weights, source eyes and hair, retarget to existing UAL skeleton, embedded resized textures, actual decimated mid/far skins.
- resident-native-body.ts: six local native GLBs, existing per-actor skeleton sharing, bone-name remap, phenotype/skin/hair/eye pigments, UV seam normals, near/mid/far presentation.
- Existing resident anatomy/appearance/world layer now use native assets, with established fallback if unavailable.
- Existing actions, sleep calibration, props, pregnancy/growth and Core identities remain on the existing runtime.
- Native foot contact samples deduplicate UV vertices, retain per-skin-group extrema, and perform bounded positive-only IK correction; Core root is not moved.
- Late phenotype refresh is applied in the attached mesh's correct frame, not the actor's current world transform.
- Source verification has --verify-local to validate committed source receipts without unauthenticated GitHub API rate limits.
- Native source workflow includes WebGL contracts plus actual production React/WASM Observer capture. WASM cache resident-wasm-v1 is populated for this unchanged Core.

## Evidence already observed
- Run 39020612119, implementation 25097bf1: both native-web and actual-observer SUCCESS.
- Actual React/WASM had all four founders on new skins, 2 male / 2 female, Core time progressed, idle/carry/walk observed, browser errors 0 and HTTP errors 0.
- Old source geometry left about 6 cm of foot penetration; bounded sole contact reduced this but strict new test still found 14–19 mm.
- Late phenotype refresh invariant PASS: maximum rest-position difference ~2.2e-14 at world offset (10000,40,-10000), Core input unchanged.
- Existing typecheck, 15 resident-motion regressions and resident lifecycle tests passed in native workflow.
- These are earlier checkpoints, not acceptance of the latest unverified correction.

## Latest corrections needing verification
- Contact coverage now includes calf-blended heel/toe vertices, directional extrema, 3 bounded passes and 4 mm clearance; strict minimum ground y remains >= -0.01 (not weakened).
- Corrected palm frame endpoint to middle-finger origin instead of short wrist-bone tail.
- Corrected foot/ball frame shared point to average metatarsal origins, with toe-tip average for ball endpoint.
- New normalized segment ratio checks: female hand/foot ~1.217/1.209; male ~0.998/1.055. This avoids previous over-stretch.
- Current workflow triggered from 21eaf5d8 must regenerate assets and re-verify these changes.

## Finish
1. Fetch current runs for 21eaf5d8 and PR #731. Fix any actual failure; do not infer success from artifacts alone.
2. Inspect latest native WebGL and actual Observer validation JSON and front/side/lifecycle images. Ensure strict contact and phenotype invariant pass.
3. Require current typecheck/build/shared Web regressions and native actual-Observer checks. Avoid rerunning unchanged Core unnecessarily.
4. Update this checkpoint and PR description with final evidence and limitations.
5. Mark #731 ready and merge using expected current head SHA. Preserve main's latest changes.
6. Verify main Web Preview / Pages and external probe, and confirm gh-pages includes all six resident GLBs and tiger assets.
7. Update tasks/WORK_STATE.md after actual completion and return main preview link.

Physical Android FPS has NOT been measured. Browser viewport success is not a physical-device result. Do not claim final photorealism or measured Android 30 FPS.
