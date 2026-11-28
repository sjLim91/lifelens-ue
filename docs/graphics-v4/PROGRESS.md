# Graphics V4 progress — 2026-11-25
Canonical plan: [MASTER_PLAN.md](MASTER_PLAN.md).
Baseline main: `4b2039780aa39c6bc0580f6519875751bcc74093`; GitHub compare base...main identical (0 commits, 0 files) at start.
Working branch: `graphics-v4/g00-baseline-audit`. No default mode change, simulation source changes or production deployment authorized by this checkpoint.

| Milestone | State | Evidence / blocker |
|---|---|---|
| G00 | IN_PROGRESS | Recursive Git tree returned 1,860 entries, not truncated; initial broad inventory 684 tracked candidate blobs, 23 STATIC_PARTIAL and 661 TREE_ONLY_PENDING. 100 render source files, 45 field assets. Complete reachability, source review, binary measurements, actual fixed-seed browser baseline and paint-pass reproduction outstanding. |
| G01 | NOT_STARTED | Required predecessor gates not accepted. |
| G02 | NOT_STARTED | Required predecessor gates not accepted. |
| G03 | NOT_STARTED | Required predecessor gates not accepted. |
| G04 | NOT_STARTED | Required predecessor gates not accepted. |
| G05 | NOT_STARTED | Required predecessor gates not accepted. |
| G06 | NOT_STARTED | Required predecessor gates not accepted. |
| G07 | NOT_STARTED | Required predecessor gates not accepted. |
| G08 | NOT_STARTED | Required predecessor gates not accepted. |
| G09 | NOT_STARTED | Required predecessor gates not accepted. |
| G10 | NOT_STARTED | Required predecessor gates not accepted. |
| G11 | NOT_STARTED | Required predecessor gates not accepted. |
| G12 | NOT_STARTED | Required predecessor gates not accepted. |

## Actual findings
- Field compositor `web/src/render/field-terrain-compositor.ts:255–260` uses constant elevation .5 to calculate an axis-aligned destination rectangle. Meadow overlay is land-only at 261–277; shore stroke uses sampled elevation at 330–337. This is a static candidate cause of rectangular water / geometry mismatch, **not reproduced in browser**.
- Field renderer at 338–343 handles only Deer/Hare; other species are not shown through that path. Actual Core species census pending.
- Observer engine at 917–920 schedules a field-motion draw every 100ms when running; end-to-end frame pacing pending.
- Existing 390×844 field QA validates real Core connection and smoke thresholds, not explicit seed or geometry acceptance. Do not replace or weaken it.
- Canonical architecture and prior visual reference contract preserve one simulation authority. Older roadmap priority note is superseded for this graphics-only lane by current user instruction; do not alter AL milestone status.
- GitHub source inspection available; local checkout blocked: `git ls-remote https://github.com/sjLim91/lifelens-ue.git refs/heads/main` returned `Could not resolve host: github.com`. Chromium exists but no local app checkout/runtime; no browser capture or test run claimed.
- Inventory is a broad candidate ledger, not a complete graphics reachability classification. Unclassified rows remain open; expand relevant paths if discovered.

## Continuation
1. Finish file-by-file import/reachability review and actual asset metadata/license inspection.
2. Execute QA_MATRIX against exact baseline source, record release runtime hashes and real Core seed/minute/camera; collect actual captures and frame intervals, trace rectangular draw pass.
3. Require G00 gate before G01 prototype comparison/ADR. No new render pipeline or asset swap before G01.
