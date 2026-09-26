# LifeLens Web Motion Handoff — 2026-09-27

Canonical source baseline: `523dccd8d1f72d0607e70a94926d676a649fafba` (#465)

## Purpose

This document is the current handoff for Web resident animation work after Action Context, Human Trace and semantic-motion integration.

The objective is not to maximize animation count. The objective is to make authoritative Core actions visually readable without inventing body language, tools, targets or outcomes that Core did not establish.

## Current animation sources

### Quaternius Universal Animation Library 1 [Standard]
- License: CC0 1.0 Universal.
- Role: proven locomotion/basic-action baseline.
- Current useful clips include:
  - `Idle_Loop`
  - `Walk_Loop`
  - `Idle_Talking_Loop`
  - `Interact`
  - `Crouch_Idle_Loop`
  - `Fixing_Kneeling`
  - sitting/crouch/swim/jump and other baseline clips.
- Provenance: `Content/Characters/Quaternius/PROVENANCE.md`.

### Quaternius Universal Animation Library 2 [Standard]
- License: CC0 1.0 Universal.
- Web role: fail-soft secondary action library.
- Pinned mirror commit: `84fd636910bf713099010efbab7f3c84550f4bcb`.
- Consumed file only: `anims/UAL2_Standard.glb`.
- Git blob: `dc684c2a664927964307e8eb7b27b0000ebf6a18`.
- Size: 8,061,600 bytes.
- If UAL2 cannot load, UAL1 locomotion/presentation must continue.

## Current authoritative mappings

### Locomotion
- normal movement -> `Walk_Loop`.
- facility material delivery while moving -> `Walk_Carry_Loop`.
- gait rate follows presentation velocity.

### Social / teaching
- Social interaction -> `Idle_Talking_Loop` only when the authoritative target resident exists and is nearby.
- KnowledgeTeaching -> Talk under the same real-target condition.
- Parenting Educate/Discipline -> Talk only with a real nearby resident.

### Physical daily life
- Eat -> UAL2 `Consume`.
- Drink -> UAL2 `Consume`.
- UseToilet -> `Crouch_Idle_Loop` only when Core reports a designated sanitation site or real Toilet object.
- Wash -> `Interact` only when Core reports a real Sink target.

### Civilization
- PlantFood Gather -> UAL2 `Farm_Harvest`.
- Craft / Experiment -> reviewed kneeling work motion.
- facility Work / Repair -> reviewed kneeling work motion.
- facility DeliverMaterial -> carry locomotion while moving.
- other target-backed generic facility interactions may use `Interact` only where semantics are not contradicted by the clip.

## Deliberately unbound actions

### Sleep
Status: **blocked on correct animation sequence**.

Required before binding:
1. lie-down / enter-bed transition,
2. sleep/rest loop,
3. wake / stand transition,
4. correct target/ground/bed alignment.

Do not substitute Sitting Idle or standing Interact.

### Tree chopping
UAL2 contains `TreeChopping_Loop`, but it is **not currently bound**.

Reason:
- `civilizationMaterial === 'Wood'` does not prove the actor is chopping a tree.
- Wood may come from collection, storage, retrieval or another future source.

Required Core fact before binding:
- authoritative source/resource type and/or explicit chopping action/tool semantics.

### Ground gathering
Do not use `PickUp_Table` as ground collection.

Required:
- reviewed ground-height gather/pickup clip,
- authoritative target/source position,
- enough alignment information to avoid grabbing empty air.

### Digging
Do not substitute kneeling repair if the visible action is digging.

Required:
- reviewed Dig clip,
- authoritative Dig/Excavate action semantics,
- target ground position,
- optional tool identity when tools become authoritative.

### Heavy carry / place / drop
Current carry covers moving material delivery.

Still needed:
- pick-up/shoulder/lift transition,
- heavy carry variant,
- place/drop transition,
- target alignment.

## Next candidate priority

### P0 — highest visual value
1. Sleep enter / loop / wake.
2. Ground Gather / PickUp.
3. Dig.
4. Tree Chop.
5. Place / Drop material.
6. Heavy Carry / two-hand carry.

### P1 — social readability
1. Listen.
2. Agree / nod.
3. Argue.
4. Comfort.
5. Hug / affection.
6. Reject / turn away.
7. Give / receive.
8. Teach / observe.

### P2 — daily life depth
1. Wash body / bathe.
2. water collect/fill.
3. food preparation.
4. fire ignition/tending/fuel.
5. repair variants.
6. ground sit/rest.

### P3 — family/lifecycle
1. hold infant.
2. feed infant.
3. put child to sleep.
4. bathe child.
5. play with child.
6. illness/rest.
7. aging gait.
8. collapse/death lifecycle sequence.

## Motion selection invariant

A motion is eligible only when both are true:

1. the clip meaning matches the visible action closely enough;
2. Core exposes sufficient authoritative facts to prove that action/target/context.

If either condition is false:

> **Fail closed to neutral Idle/Walk and keep the factual action cue visible.**

This is preferable to a visually active but false animation.

## Prohibited shortcuts

- no `Sword_Attack` as chopping/hammering.
- no `PickUp_Table` as ground gather.
- no Sit as Sleep.
- no browser inference from Needs to choose an action.
- no browser inference from material alone to choose a tool action.
- no random social gesture disconnected from Core social state.
- no external asset without verified zero-cost compatible license and provenance.

## Validation requirements for each motion tranche

Before merge:
1. exact clip name verified.
2. license/source/version recorded.
3. Core authority condition documented.
4. semantic resolver test added.
5. unsupported/missing-context fallback test added.
6. Typecheck PASS.
7. Runtime Resilience PASS.
8. Preflight PASS.
9. build/preview PASS where applicable.
10. actual mobile/browser visual review remains required after merge.

## Current next step

Review the available UAL2/free-license motion inventory for:
- Sleep,
- Dig,
- Tree Chop,
- Ground Gather,
- Carry/Place variants.

For each candidate, first determine whether current Core presentation DTO contains enough authority. Add only the safe subset in the next isolated motion PR.

After this motion tranche, return to the observation roadmap:
- Social Cue,
- Observer Director,
- deeper authoritative Human Trace / lived-space accumulation.
