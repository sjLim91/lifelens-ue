# Web mobile rendering and resident inference pacing

- User request: reduce the mobile slowdown and heat that worsen during use.
- Base: `main@1f72d27484a0bf2e32fb0c0de64491257d632d29`.
- Branch: `perf/mobile-render-inference-budget-20261112`.
- Scope: the first recommended changes, mobile rendering and Qwen inference pacing.

## Behavior

- Coarse-pointer devices render at a target of 30 FPS. Scene updates, reflections and the main draw are skipped together. Animation delta uses the time since the last rendered frame, retains the existing 50 ms cap, and never replays a backlog of frames.
- Desktop rendering continues on every RAF, including the first queued RAF whose timestamp can precede `start()` slightly.
- A shared pointer-capability check covers landscape phones and large touch tablets. Those devices also receive the existing low-power renderer, disabled antialiasing and DPR 1.35 cap. Fine-pointer desktop windows retain their existing graphics budget.
- Mobile resident AI attempts start at least 15 seconds apart. After inference, the GPU gets idle time equal to inference duration, bounded to 5–30 seconds. For example, a 20-second inference is followed by 20 seconds of rest. This deliberately increases strategic reasoning latency in exchange for less continuous GPU use.
- Observer-triggered and direct sampling use the same limit. Successful, failed and cancelled inference all respect the work budget. Cancellation reserves rest synchronously, so reset/resume cannot bypass it before Promise cleanup runs. Stale completions cannot extend a new world's rest budget.
- The pinned Qwen3-1.7B model, full individual Core context, allowed intents, one-inference concurrency, validation, timeout, stale window and resident round-robin are preserved. Desktop AI retains its 5-second cadence. Core survival and physical execution continue independently of Qwen pacing.
- Save frequency, simulation clock/speed, Core authority, rendering assets and Core/WASM ABI are unchanged. Moving simulation to a Worker and changing automatic saves are separate work.

## Validation

- `npm run typecheck` — PASS.
- `npm run build` — PASS. Existing CSS-minifier and large-bundle warnings remain outside this change.
- `node tests/cognitive-agent/run.mjs` — PASS: 38 adapter checks, 16 Shadow checks, live resident AI checks, and one-engine production isolation. Added coverage for mobile minimum interval, slow inference/rest, full individual context, round-robin fairness, reset/resume and same-event cancellation.
- `node tests/runtime-resilience/run.mjs` — PASS: runtime, lifecycle, storage, pause, writer lease and rendering failover checks. Added production-renderer coverage for 60/90/120/144 Hz input: exactly 300 scene updates/draws over 10 seconds, with 10 seconds of accumulated animation delta. Desktop refresh behavior, render stop, long gaps and pre-start RAF timestamps are covered.
- Chromium with software WebGL — PASS: the actual `WorldRenderer` initializes in 393×852 touch, 915×412 touch, and 1280×720 desktop contexts; the mobile profile/DPR apply in both orientations; rendering stops cleanly; no page exceptions or renderer failure callbacks. This is an isolated renderer smoke test, not a complete Core/Qwen session or a device FPS benchmark.
- `git diff --check` — PASS.
- Exact-head remote checks are tracked in the implementation PR.

## Evidence limits

No physical phone temperature, battery, sustained FPS, model throughput or memory-leak improvement was measured. The deterministic frame counts prove a work limit, not a claimed percentage improvement in heat or battery life. Qwen correctness remains covered by existing contract tests and controlled providers; this work does not claim a new real-model device acceptance result.

Implementation is prepared on the feature branch. Main integration and production publication remain separate actions.
