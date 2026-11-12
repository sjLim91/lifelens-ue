# Mobile observer UI and camera interaction

Base: `main@7c52c555f5dbaf37e78875ff32495529dbc1927d`.
Branch: `feat/mobile-observer-ui-camera-20261112`.
User requested UI improvements from their mobile screenshots, then explicitly requested checking all mobile movement, zoom and rotation. The user authorized merge and publication after verification.

## Changes

- Group date/time and weather into a legible HUD, with secondary biome/population/era chips. The current Core values and existing status/clock DOM contracts remain authoritative.
- Add direct mobile observation/residents/journal/menu navigation, pause/1x/4x controls and return-to-residents control. Existing observer actions remain the only mutation path.
- Replace the centered mobile overlay with a native dialog bottom sheet: focus containment, Escape/backdrop dismissal, return focus and release on a desktop breakpoint transition.
- Put all five factual Need values into compact resident summaries; full meters and individual detail remain available after selection. No new urgency thresholds or inferred resident state.
- Separate advanced restart/seed and rendering-recovery controls into disclosures while retaining their existing confirmation and corruption-safety behavior. Keep asset credits accessible in the menu.
- Do not mount the heavy desktop inspector on mobile; retain its desktop behavior after resizing.
- Support narrow portrait, coarse-pointer landscape and desktop layouts, larger touch targets, reduced motion, and plain surfaces instead of additional backdrop blur. Preserve the previous mobile 30FPS and Qwen work limits.
- Replace the fixed screen-pan multiplier with actual camera ray/ground-plane intersections, accounting for heading, pitch, FOV and viewport dimensions. Apply direct user camera input immediately while retaining ordinary observation interpolation.
- Preserve the focus-plane world point under the pinch midpoint, and the mouse-wheel pointer. The 2D rescue view also receives sub-chunk pan and a consistent screen projection.
- Coalesce both touch pointers per RAF so translating at a zoom limit does not introduce false scale changes. Preserve the multi-touch gesture until every finger lifts; cancel pending input on disposal and foreground loss. Consume the tap threshold smoothly and recover near-coincident finger spans.

## Verification

- `npm --prefix web run typecheck` and `npm --prefix web run build`: PASS.
- `node web/tests/input/run.mjs`: existing tap/orbit/pan/cancel/desktop checks, coalesced translation at max zoom, midpoint anchor, finger release, disposal, near-coincident spans and foreground loss PASS.
- `web/tests/input/projection.mjs`, imported by the input runner: 108 real Three.js camera combinations across 3 viewport shapes, 4 headings, 3 pitches and 3 zoom levels. Ground displacement follows the requested screen drag and the pinch midpoint stays anchored on the focus plane.
- Runtime/persistence/pause/GPU-failover and cognitive regressions: PASS locally. Full source and behavior regressions are run by the implementation PR's exact-head CI.
- `web/tests/input/browser-review.cjs`: actual production build with the published, checksum-verified Core runtime. Portrait 393x852, small portrait 320x568, coarse landscape 915x412 and desktop 1440x1000. Checks navigation, all 5 Need meters per resident, detail selection, dialog focus/dismissal, pause/speed, protected new-world cancellation, no horizontal overflow, breakpoint transition, no page exceptions. Chrome touch events exercise orbit, pan, pinch in/out and multi-touch release against the running world.

Optional browser review needs Playwright and Chromium; these are development tools, not app dependencies. Build, serve `web/dist` with `npm --prefix web run preview -- --host 127.0.0.1 --port 4173`, then run `node web/tests/input/browser-review.cjs`. Set `LIFELENS_PLAYWRIGHT_MODULE`, `LIFELENS_CHROMIUM_PATH` or `LIFELENS_REVIEW_URL` when needed. Review creates fresh browser contexts and dismisses restart confirmation; it does not use an existing user's save.

The local browser story uses the product's 2D rescue mode because the cloud browser has no physical mobile GPU. Camera projection tests use real Three.js perspective cameras. Physical-phone feel, sustained 3D FPS, thermals and Qwen throughput are not claimed by these checks. Core, save format, model and assets are unchanged.
