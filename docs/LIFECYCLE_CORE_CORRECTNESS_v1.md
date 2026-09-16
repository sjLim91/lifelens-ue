# LifeLens Lifecycle Core Correctness v1

Status: ACTIVE
Owner: Jjun / Core-AI-Simulation-World
Branch: `jjun/lifecycle-core-correctness-v1`

## Purpose

Close the correctness gaps found during the 2026-09-16 full-source audit before deeper society/civilization expansion.

This milestone covers authoritative lifecycle semantics only. Presentation may display these facts later, but it must not create death, parenting, genealogy, pregnancy, household, or birth outcomes independently.

## Runtime invariants

1. **Dead means inactive**
   - Dead residents do not decay Needs, choose actions, execute actions, teach, gather, craft, socialize, or retain active runtime plans.
   - SmartObject reservations held by a deceased resident are released.

2. **Dependent stages are not tiny adults**
   - Baby/Toddler residents never run the ordinary adult autonomous action loop.
   - Civilization work is gated by `LifeStageProfile::canWork`.
   - Low-autonomy stages do not initiate ordinary social decisions.
   - Parent care is the authoritative way to resolve direct-care Needs for Baby/Toddler.

3. **Parent care cannot synthesize food/water**
   - `Feed` only relieves food/water Needs when the caregiver actually has the matching provision.
   - Production care consumes the matching caregiver inventory units.
   - `ToiletAssist` is a real dependent-care action; babies/toddlers do not need adult toilet AI to resolve bladder pressure.

4. **Close genetic kin cannot enter autonomous romance**
   - Self / Parent / Child / Sibling / HalfSibling / Grandparent / Grandchild are prohibited.
   - The prohibition applies before chemistry growth, dating candidate creation, later autonomous romance progression, and pregnancy attempts.
   - Historical spouse/in-law relationships are not treated as genetic incest by this rule.

5. **Birth has a real runtime position**
   - A newborn inherits the gestational parent's authoritative Core runtime position.
   - Fallback is partner runtime position, then the selected start-region center.
   - Birth must not default to absolute Core grid `{0,0}`.

6. **Child growth rates do not compound accidentally**
   - Newborn base metabolism/sleep tendency remain base values.
   - Life-stage multipliers are applied exactly once through `applyLifeStageProfile`.

7. **Death is deterministic Core simulation**
   - Daily natural mortality is deterministic from WorldSeed + CharacterId + day.
   - Extreme old age is guaranteed to terminate rather than producing immortal residents.
   - Disease/accident-specific mortality remains a later health-system extension; current cause classification only uses available age/physical-health state.

8. **Death closes dependent state**
   - Active gestational pregnancy ends if the gestational parent dies.
   - The deceased leaves active household membership; empty households are pruned.
   - Surviving household members receive a HouseholdChanged life event.
   - Surviving romantic partner receives PartnerWidowed history where applicable.
   - Genealogy remains historical; death does not erase lineage.

## Current implementation

- `Genealogy.h`: `isRomanceProhibitedKinship`.
- `Pregnancy.h`: lifecycle termination API.
- `Death.h`: deterministic daily mortality policy and corrected death-memory source provenance.
- `Parenting.h`: direct-care stage helper, resource-aware Feed, ToiletAssist, living-parent validation.
- `Simulation.cpp`: production lifecycle wiring.
- `test_lifecycle_runtime_correctness.cpp`: dead-runtime, dependent-care, incest, newborn-position, death/pregnancy/household integration regression.

## Explicitly not solved here

- Physical/presentation completion for Parenting/Social/Civilization actions. That is the next authority milestone: **Action Completion Unification**.
- Full health/pathogen/starvation/accident model.
- Child-specific Character mesh/capsule scaling; that is presentation/runtime integration after Core lifecycle truth is stable.
- Death-body/corpse presentation.

## Validation gate

Before merge:
- all Core tests PASS, including `test_lifecycle_runtime_correctness`;
- deterministic harness PASS;
- Preflight PASS;
- Unreal Linux UHT/UBT PASS because `Simulation.h/.cpp` is compiled into the Unreal module;
- no Android build is required solely for this Core correctness milestone.
