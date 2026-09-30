# Resident idle recovery — 2026-09-30

Reported preview: `index.html?v=df839012`, day 122, all residents waiting with
high unmet needs. The published `RUNTIME_COMMIT.txt` was
`df839012177bc8f4a0bfd59e30cbf8e5d29d4129`; main `3082729` only adds status docs.
The user's world seed/save was not available, so the exact screenshot world is
not claimed as reproduced.

## Reproduction and cause

Native production `setupNewGame()` with seed 42 reproduces the same failure class:
by day 10 all four residents stop near (-189, 593), with all five needs reaching
1.0, and remain stuck through days 122–123. Logs repeatedly report
`context action route failed`.

Urgent provision selection used distance and quantity without checking the real
Core route. The food access (-190, 587) is dry but disconnected by water from
the resident's bank. Every five-minute planning boundary selected it again.
Urgent acquisition consequently starved other actions indefinitely.

## Correction

Urgent food nodes and local storage must have a Core ground route to their real
interaction point (the existing one-cell interaction radius). Inaccessible
candidates are skipped in favor of another reachable source; existing exploration
remains the fallback when no known usable source exists. Only candidates that
could improve the nearest selection pay for a route check.

No teleport, need reset, free provisions, world regeneration or save schema
change is introduced. The same authoritative path rules still execute movement.

## Verification

- Original seed-42 production simulation: stuck through day 123.
- Corrected seed-42 production simulation: actions and need resolution continue
  through day 123, including completed drinking, eating and toilet use.
- Loading an original-code day-122 stranded snapshot in the corrected Core:
  all four residents move again and eat/drink within one simulated day, without
  resetting the world or its needs.
- New regression: inaccessible dry food access, inaccessible closer storage,
  reachable storage alternative, physical recovery/food consumption, movement
  bounded to one cell per minute, encoded snapshot deterministic continuation.
- Native targeted tests passed: survival reachability, early survival, headless
  locomotion, social utility, civilization spatial targets, Core save/load.
- Regression sensitivity: the new test fails against the original Core at the
  inaccessible provision assertion. The disposition-only test's arbitrary food
  coordinate is replaced with real traversable ground, preserving its assertion
  that emergency provisioning bypasses personality bias.
- Full CMake/CTest, deterministic harness and WASM gates run in GitHub Actions;
  this workspace lacks CMake/Emscripten, so local checks use GCC directly.

Scope: this repairs the demonstrated permanent retry lock. Long-run sleep/hygiene
pressure and other inaccessible physical/exploration targets remain separate
balancing/navigation concerns. Exact user-world confirmation still needs its seed.
