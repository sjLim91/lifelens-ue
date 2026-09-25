# Web touch / observation interaction repair — 2026-09-25

- Base: a75584f308b55fec8b191ce834384fe68f000cdc.
- Branch: work/web-touch-20260925.
- User requested touch direction and overall interaction corrections.

## Changes

- Ground pan now follows the fingers, using the inverse camera translation in all orbit quadrants. Vertical orbit sign corrected to match direct manipulation; horizontal direction retained.
- Tap movement below the existing threshold no longer rotates the scene under the selection ray. Release displacement also checked when no move event arrives.
- Multi-touch remains latched until all fingers lift, preventing pinch-to-single-finger accidental orbit. Lost pointer capture clears stale state; cancellation does not select.
- Selected-life section precedes world setup controls; opening the sheet or changing selection resets its scroll position. Selected-resident sheet capped at 46svh on mobile, retaining the world view. Accessible toggle state, scroll containment and reduced-motion handling included.
- Canonical one-finger orbit / two-finger pan and pinch / tap select / desktop controls preserved. No Core or world-state changes.

## Validation and limits

- Seven input regressions pass: tap jitter, orbit direction, pinch release, cancel/capture loss, release without move, mouse controls and all-quadrant pan projection.
- Typecheck/build pass; pre-existing bundle size warning remains.
- Cloud browser previously reports WebGL disabled; no real-device visual acceptance claimed. Device checks still needed for subjective sensitivity and panel layout.
- Existing #423 action-context lane untouched; no actor/asset/weather edits.

## Integration

- PR #450 merged as `ae6d90fc4abd7c907d1aa9b41d07cbe4d4e126a8` after all four exact-head GitHub checks passed.
- CI executes 50 regressions including seven new input cases; typecheck/build pass.
- Scope released. Device interaction/visual acceptance remains pending.
