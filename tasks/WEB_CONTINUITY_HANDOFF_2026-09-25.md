# Web resident continuity — recoverable Work Mode checkpoint

Status: DONE — #447 merged on explicit user request as `4ddf85bb9104f51d68a2d4b925eed66dbd49cecc`. All four exact-head CI checks succeeded.
Branch: `work/web-resident-continuity-20260925`.
Base: `9b895a2d61535100d32e33f5fb44438ac6f7de44`.

## Scope
- Product: `web/src/runtime/resident-continuity.ts` only.
- Test: `web/tests/runtime-resilience/continuity.mjs`.
- CI: existing `.github/workflows/web-runtime-resilience.yml` adds the new test command.
- No Core simulation, actor animation, rain, trees, camera, speed, shared types or release workflow edits.
- Open PRs checked: #423, #425, #426, #427, #428, #393, #392. None modified these code/test/CI files at scope selection.
- Remote checks cannot observe unpushed edits in another conversation. Recheck before integration.

## Fixes
- Complete authoritative resident lists remove superseded cached identities and positions.
- Resident expiry removes both resident and coordinate records.
- Position records expire independently when newer resident payloads lack positions.
- Unavailable payload contents cannot insert identities or refresh cache timestamps.
- Explicit Core death removes an identity even in a partial observation.
- Rendering reads no longer refresh cache timestamps or expose mutable cached coordinates.
- Invalid population count is rejected before cache mutation; nonfinite coordinates are not cached.
- Existing display grace duration retained; injectable monotonic clock supports deterministic tests.

## Validation
- 13 new continuity regression cases PASS, including 1,000 identity replacements.
- 17 existing runtime resilience checks PASS.
- `npm run typecheck --prefix web` PASS.
- `npm run build --prefix web` PASS; existing bundle-size warning remains.
- `git diff --check` PASS.
- No claim of visual QA or whole-game completion. Cache behavior verified in executable tests.

## Resume after interruption
1. Fetch this branch and latest main; inspect the PR's actual head/status.
2. Read AGENTS.md and compare current open PR filenames with the scope above.
3. Run `node web/tests/runtime-resilience/continuity.mjs` and existing `run.mjs` if product inputs changed.
4. Check PR CI for the exact head. Investigate failures; do not repeat successful gates without a changed input.
5. Coordinate integration with Chat. Do not merge/deploy automatically: main triggers preview publication.

Previous Work PR #422 is already merged, verified through GitHub on this turn.
No background automation or indefinite execution has been scheduled. This checkpoint
is the safe stopping boundary; future work begins with fresh conflict inspection.

## Closeout
The earlier no-merge instruction above was superseded by the user's explicit merge request. Code integrated; scope lock released. Existing main-triggered preview publication may run; no independent hosting deployment invoked.
