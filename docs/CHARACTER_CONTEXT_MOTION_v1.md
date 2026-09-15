# LifeLens Character Context Motion v1

## Purpose

LifeLens residents must not only decide actions; their bodies must visibly express the authoritative action in a way an observer can understand.

**Asset presence is not implementation.** An imported animation is only considered implemented when an authoritative Core/World action selects it through a presentation contract and the resident transitions into/out of it correctly.

## Authority boundary

- Core / World owns **what the resident is doing, with whom/what, and whether it succeeded**.
- Character Presentation owns **how that action is shown**: animation, facing, gaze, pose, transition, IK/motion-warp presentation.
- Animation/root motion must never become simulation authority.
- Presentation must not invent an action that Core did not issue.

## Current baseline

Implemented runtime locomotion:
- Idle
- Walk
- Jog
- Sprint

Imported Quaternius assets already available for first context slice include:
- `Idle_Talking_Loop`
- `Sitting_Enter`
- `Sitting_Idle_Loop`
- `Sitting_Talking_Loop`
- `Sitting_Exit`
- `Interact`
- `PickUp_Table`
- `Fixing_Kneeling`
- crouch / jump / swim / death and other assets

These assets are **not automatically considered implemented** merely because they exist in `Content/Characters/**`.

## Phase 1 — Action-to-motion router

Create a presentation-side context-motion state/router that consumes authoritative action data and chooses a visual state.

Minimum visual states:
- Locomotion
- Talking
- SittingEnter
- SittingIdle
- SittingTalking
- SittingExit
- GenericInteract
- PickUp
- KneelingWork
- Sleep/Lie placeholder category until a suitable validated asset is available
- Sanitation/Hygiene placeholder category until suitable validated assets are available

The router must support clean transition back to locomotion.

## Phase 2 — Survival actions

Authoritative physical intents already available through `FLLCoreActionDirective`:
- Eat
- Drink
- Sleep
- Toilet
- Hygiene

Requirements:
- resident stops at resolved use/fallback point before context animation.
- body faces the use point/context direction.
- context animation plays for the authoritative action duration or until completion ACK.
- on completion, return to Idle/locomotion cleanly.
- outdoor emergency fallback must use an appropriate grounded fallback pose; do not display a fake toilet/bed/sink interaction when no such object exists.
- if no dedicated animation exists yet, use an explicit generic fallback state, not unrelated locomotion.

## Phase 3 — Social communication

Authoritative social intent/target already available through `FLLCoreActionDirective`:
- Approach
- Avoid
- Repair
- Comfort
- TargetResidentId

Visual sequence for a direct social interaction:

`approach -> stop at personal-space distance -> face/gaze target -> talking/context gesture -> finish -> resume next action`

Baseline talking asset: `Idle_Talking_Loop`.
If seated social context exists later, use `Sitting_Talking_Loop`.

Speech bubbles/Event Feed/localization follow `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md` and must reflect the same authoritative social event.

## Phase 4 — Civilization/context work

`Gather / Craft / Work / Dig / Build / Carry` need explicit authoritative context-action read data before animation routing is considered complete.

Jjun/Core responsibility:
- expose a stable typed context-action contract for civilization/work execution where current contracts are insufficient.
- include target/object/location identifiers only when they are authoritative and needed for presentation.

Dagyeom/Character responsibility:
- map the typed context action to validated animations such as `PickUp_Table`, `Fixing_Kneeling`, generic `Interact`, or future free/CC0 assets.

Do not infer civilization action solely from nearby scenery or animation choice.

## Missing-animation rule

When no suitable animation exists:
1. preserve the authoritative action.
2. choose a clearly documented neutral/generic presentation fallback if visually reasonable.
3. record the missing animation category.
4. do not substitute a misleading action animation merely to avoid Idle.
5. acquire/create only free/CC0-compatible assets under the project licensing rule.

## Acceptance criteria

A context-motion slice is DONE only when:
- locomotion -> context -> locomotion transition is visually stable.
- no T-pose/sliding/teleport-like visual transition is introduced.
- resident faces the correct target/use direction.
- social actors face one another and respect reasonable personal space.
- the same Core action does not randomly display unrelated motion.
- physical completion ACK still comes from the existing authoritative execution path.
- missing facilities use outdoor/emergency visual fallbacks instead of fabricated infrastructure.
- male/female/outfit variants are checked.
- Android presentation remains within performance budget.

## First implementation order

1. Fix current backwards-walk/facing regression (IR-B).
2. Add context-motion router/state.
3. Wire `Idle_Talking_Loop` to authoritative social interaction.
4. Wire `Sitting_Enter/Idle/Exit` where a real sit affordance exists.
5. Wire `Interact` / `PickUp_Table` / `Fixing_Kneeling` to authoritative matching contexts.
6. Add validated fallback presentation for Eat/Drink/Sleep/Toilet/Hygiene when no dedicated animation exists.
7. Extend Core context-action read contract for Gather/Craft/Work/Dig/Build/Carry.
8. Replace generic fallbacks over time with dedicated validated free/CC0 animations.
