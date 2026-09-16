# Observer Resident Detail Data v1

## Purpose

This contract defines the authoritative data shown in the Level 2 resident detail panel.

The Observer is a presentation surface only. Core/World remains the simulation authority. The UI may format, sort, summarize, or visualize authoritative values, but it must not manufacture a second resident state.

## Authority by tab

### Overview

Keep the overview concise. It may use the existing compatibility projection for identity and short human-readable summaries while the detailed tabs below use direct Core read contracts.

### Needs

Authoritative source: `FLLCoreResidentObservation::Needs`.

Core needs are deficit/pressure values:
- `0.0` = satisfied
- `1.0` = urgent

The Observer displays user-facing satisfaction as `1 - deficit`, including a numeric percentage and compact bar. The five authoritative physical needs are:
- Hunger
- Thirst
- Sleep, displayed as Energy
- Hygiene
- Bladder

Legacy Social/Fun compatibility placeholders are not authoritative Core needs and must not appear as factual resident needs.

### Personality

Authoritative source: `FLLCoreResidentObservation::Personality`.

Display all 14 Core dimensions rather than the five-axis compatibility projection:
- Introversion
- Conscientiousness
- Openness
- Agreeableness
- Emotional Stability
- Empathy
- Impulsiveness
- Risk Tolerance
- Ambition
- Patience
- Sociability
- Curiosity
- Orderliness
- Adaptability

### Traits & Skills

There is currently no explicit named Core trait taxonomy. The Observer must not interpret personality, genetics, appearance axes, or unrelated state as invented trait labels merely to fill this section.

Until a real Core trait model exists, the Traits section must state that no explicit Core trait taxonomy is available rather than showing `None listed`, which could falsely imply an authoritative empty trait set.

Authoritative current resident skills come from `FLLCoreResidentCivilizationObservation`:
- Gathering
- Crafting
- Learning

Legacy `FLLResidentData::Skills` is not the source for resident-detail display.

There is currently no authoritative Core preference model. Legacy placeholder preferences must therefore remain hidden and the UI should say the authoritative model is not available yet rather than implying a real empty preference set.

### Emotion

Authoritative source: `FLLCoreResidentObservation::Emotion`.

This milestone only preserves truthful read/display behavior. It does not fabricate non-zero emotion values. Causal generation/decay from ordinary life events remains a separate Emotion Runtime Integration milestone.

### Relationships

Authoritative source: directional `FLLCoreRelationshipSnapshot` values on `FLLCoreResidentObservation`.

The Observer may sort relationships by derived SocialBond and show RomancePotential as a summary, but detailed inspection must expose the underlying directional dimensions:
- Affection
- Trust
- Respect
- Comfort
- Familiarity
- Attraction
- Romantic Interest
- Sexual Attraction
- Commitment
- Conflict
- Jealousy
- Fear
- Grudge

The UI must not collapse these into a new symmetric relationship authority.

### Family

Authoritative source: `FLLCoreFamilyObservation`.

Display household, active partner/stage, pregnancy context, parents, children, and siblings from Core only. `None` is valid when the authoritative relation is absent; the UI must not invent family links.

### Knowledge & Gear

Authoritative source: `FLLCoreResidentCivilizationObservation`.

Keep the existing direct-Core model for:
- Gathering/Crafting/Learning skills
- carried inventory
- known/reproducible technique counts
- technique knowledge and provenance

This tab is the reference pattern for future Observer detail work: presentation consumes Core read contracts directly whenever they exist.

## Data availability wording

Use three distinct meanings:
- real empty authoritative set -> `None` / `None listed` is acceptable;
- Core runtime/read contract unavailable -> say Core data is unavailable;
- product model does not exist yet -> explicitly say the authoritative model is not available yet.

Do not conflate these states.

## Ownership / maintenance

The data-completeness implementation is performed under `ASSIST_LOCK-UI-OBSERVER-DATA-1` on `jjun/observer-data-completeness-v1`.

After the milestone PR merges and the assist lock is released, normal Observer UI layout, styling, mobile readability, and presentation maintenance return to the Dagyeom UI lane. Future presentation maintenance must preserve this authority contract unless a deliberate Core/Bridge contract change is made.

## Acceptance

- Needs detail reads Core and shows numeric satisfaction plus compact visual bar.
- Personality detail shows all 14 Core dimensions.
- Traits/Preferences do not use empty legacy placeholders as fake authority.
- Skills use authoritative civilization skill values.
- Relationships expose the directional Core dimensions, not only summary scores.
- Family continues to read authoritative Core family data.
- Knowledge & Gear remains direct-Core.
- Emotion values remain truthful; causal Emotion work is not smuggled into this milestone.
- Level 0/1 Observer behavior, camera input, mobile safe-area/touch behavior, and selection feedback remain intact.
