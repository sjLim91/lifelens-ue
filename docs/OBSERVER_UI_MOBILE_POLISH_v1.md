# LifeLens Observer UI Mobile Polish v1

Status: ACTIVE consolidation contract

This document preserves the still-useful product behavior from legacy stacked PRs #30, #36, and #38 while the implementation is rebased onto current `main`.

The old branches are not merge sources. They are hundreds of commits behind current `main`; only the behavior below is carried forward. Current Core-backed Observer tabs/data authority and later product requirements must not be overwritten by old file snapshots.

## 1. Navigation / hierarchy

- LEVEL 2 has an explicit Back affordance that returns to LEVEL 1.
- Empty resident lists render an honest empty state rather than a blank/fabricated resident row.
- While LEVEL 2 is open, the always-visible resident strip may be visually de-emphasized so detail is the focus.
- Existing empty-world tap semantics and simulation authority remain unchanged.

## 2. Android / mobile usability

- HUD chrome respects platform title-safe / cutout / rounded-corner safe insets plus the normal LifeLens margin.
- Interactive detail tabs and Back affordance use at least the existing 48 logical-pixel touch target baseline.
- The resident strip fits whole resident entries inside the safe width and summarizes hidden entries as `+N` rather than clipping arbitrary text.
- Detail content that cannot fit vertically must visibly indicate overflow instead of silently looking complete.
- Do not add a second simulation/data authority to achieve responsive layout.

## 3. Lightweight visual feedback

- Observation level transitions may use a short, non-blocking fade-in.
- A newly selected resident may show a brief selection flash.
- The selected resident may have a small persistent focus marker while LEVEL 1/2 is active.
- Feedback is presentation-only. It must not change resident selection, movement, action, relationship, or simulation state.

## 4. Localization direction

Canonical language direction remains `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`:

- normal user-facing UI defaults to Korean;
- Core/action/event identifiers remain language-neutral;
- user-visible strings belong in the localization/display layer, not scattered through gameplay code.

Therefore the old English literals from PRs #30/#36/#38 are behavior references, not final wording requirements.

## 5. Modern-main integration rules

- Port behavior onto current `main`; do not merge the legacy stacked branches wholesale.
- Preserve the current eight-tab Core-backed resident detail surface and all newer read contracts.
- Preserve current touch hit-testing / Core observation authority.
- Changes stay in Observer presentation scope (`Source/LifeLens/UI/**`) unless a real cross-owner contract gap is discovered.
- One consolidated validation cycle replaces separate CI for the three legacy PRs.

## 6. Acceptance

The consolidated implementation is DONE only when:

1. Back navigation works from LEVEL 2 to LEVEL 1.
2. Empty resident state is explicit.
3. safe-area placement is used on mobile-facing HUD chrome.
4. detail tabs / Back meet the 48 logical-pixel touch baseline.
5. resident strip overflow is `+N` and detail vertical overflow is visibly marked.
6. selection feedback is visible but unobtrusive.
7. current Core-backed tabs/read models remain intact.
8. Structural Preflight passes.
9. one Unreal Linux Compile passes because UI C++ changes.
10. PIE/APK visual QA confirms landscape readability and touch behavior.

Supersedes tracking in legacy stacked PRs: #30, #36, #38.
