# Native resident Web integration — merged, 2026-10-17

## Completed product checkpoint
- PR #731 merged to main: 42a9053ff3f88fe78612ad4463911f7da949c0c6.
- Tiger #728 is included and marked merged. Older rejected Q3/Q4 prototype PRs were not separately merged.
- Final implementation code 21eaf5d8 plus generated assets/reports at PR head 8346c98e.
- Current user authorization (complete people integration and merge) has been fulfilled.
- Remote repository work uses connected GitHub tools. Do not retry terminal network access.

## Implementation
- Original MakeHuman CC0 anatomy, sex morphs, UV skin/eyes/hair and official weights are retained with pinned source receipts.
- Tools/blender/retarget_native_residents.py uses real palm/metatarsal landmarks and the established UAL bind skeleton. It produces near, mid and far skinned GLBs.
- resident-native-body.ts shares each existing actor skeleton, retargets source joint names, applies phenotype/PBR pigments, preserves UV seam normals and selects actual LODs.
- Existing Core IDs, coordinates, motion meanings, save format and life/relationship authority remain unchanged.
- Existing growth, pregnancy, sleep and prop/action paths use the new surface. Native soles use bounded per-skin-group extrema, excluding UV duplicates, for positive-only contact correction without moving Core roots.
- Late phenotype refresh at large world coordinates is independent of the actor's current world transform.

## Verified evidence
- Native WebGL and actual production React/WASM: https://github.com/sjLim91/lifelens-ue/actions/runs/39021921627 — SUCCESS.
- Four actual founders (2 male, 2 female) use new skins while Core time and carry/walk actions progress. Browser and HTTP errors: 0.
- Source skin/UV/PBR/common skeleton, decreasing LOD triangle counts, growth/pregnancy/sleep, strict contact and phenotype-refresh invariant: PASS.
- Maximum rest-position difference for the same phenotype at world offset (10000,40,-10000): about 2.2e-14; input Core facts unchanged.
- Full shared Web regression/typecheck/build/eight capture domains: https://github.com/sjLim91/lifelens-ue/actions/runs/39021982708 — SUCCESS.
- Authoritative originals/generated receipts: docs/asset-reviews/resident-makehuman/.
- Browser evidence: web/tests/native-resident/review/ and web/tests/native-resident/observer-review/.

## Main and deployment
- Main parent at merge was 34ea6295932d7a403d8f967e9f1ac1aa8381c0b0. Its 294 Core/Web bridge/cognition files were unchanged by the integration.
- Web Preview 39023026143, Pages 39023106736 and External Preview Probe 39023110559: SUCCESS.
- gh-pages 57e2183f0af5a83017233cf06c2f3727cee75ad7 publishes product commit 42a9053f.
- All six resident GLBs were confirmed on gh-pages with Git blob hashes identical to main.
- Product: https://sjlim91.github.io/lifelens-ue/index.html?v=42a9053f
- This closeout documentation does not require a new runtime build or preview publication.

Physical Android FPS has NOT been measured. The evidence establishes source integration and browser/runtime correctness, not measured device 30 FPS or final photorealism.
