# Work Mode — Web runtime resilience integration handoff

Status: implemented and locally validated; separate review branch, NOT merged/deployed.
Base: `6abf47f7baa3d8023bc7e7b69145d6a0680bd764`.
Branch: `work/web-runtime-resilience-20260922`.

## Scope and coexistence

Checked open PRs #421 (Core locomotion, camera, clock, actor presentation), #417
(runtime publication provenance), #393 and #392 (native ecology/presentation).
No product/workflow file modified by those PRs is modified here. Shared canonical
coordination files receive append-only scope/status entries.

Existing Sites copy is not changed. GitHub `web/` is the integration target.
Do not merge automatically: pushing main would trigger the existing preview deploy.
Before integration, recheck changed filenames on all active PRs and current main.
Unpushed edits in another conversation cannot be observed from GitHub.

## Implemented

- Network deadlines cover response headers AND text/JSON/binary bodies; stalled
  fetch implementations are also bounded and the AbortSignal is triggered.
- Keep existing source priority and JS/WASM pairs together. A source succeeds only
  after module import, WASM initialization and required ABI checks complete.
- Reject empty/HTML scripts and malformed WASM headers before initialization.
  This is file-format validation, NOT checksum/provenance verification (#417).
- All sources failing surfaces aggregated error reasons; no fake world fallback.
- Invalid world overview raises an error instead of becoming `{}`/zero residents.
  Existing refreshSafely retains the last published observer state.
- Residents reject malformed/duplicate IDs and non-finite positioned coordinates.
  IDs and seeds remain strings; valid zero-population snapshots are still accepted.
- Existing 6s transport / 12s initialization budgets moved to an injectable policy.
  No simulation constants, movement, seeds, population or time rules added/changed.
- Standalone executable tests plus a narrowly scoped PR workflow. No dependency,
  package manifest, shared tests.json or release workflow modifications.

## Validation

- `node web/tests/runtime-resilience/run.mjs`: 17 checks passed.
- `npm run typecheck --prefix web`: passed.
- `npm run build --prefix web`: passed (existing large-bundle warning remains).
- `git diff --check`: passed.
- No browser visual claim: this change concerns runtime I/O and DTO validation.

## Limitations

Browser JS evaluation / synchronous WASM work cannot be interrupted by a timer
while the event loop is blocked. Deadlines bound asynchronous waits only.
Version provenance and exact JS/WASM checksums remain the release lane's concern.
Transport policy injection is optional; existing `LifeLensCoreBridge.connect()`
callers need no changes. Malformed mandatory snapshots now intentionally throw;
callers should keep using the existing refresh error boundary.
