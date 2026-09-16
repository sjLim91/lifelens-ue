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

Authoritative trait source: `lifelens::TraitProfile` from `TraitsPreferences.h`, projected through `FLLCoreTraitPreferenceObservation`.

The named trait model is explicit Core semantics, not a UI inference. The profile is deterministically derived from the resident's already-authoritative persistent `Personality` and `GeneticsProfile`, which avoids storing a second mutable copy that could desynchronize during Save/Load.

The eight authoritative trait dimensions are:
- Resilience
- Creativity
- Discipline
- Compassion
- Adaptability
- Boldness
- Perseverance
- Resourcefulness

Authoritative current resident skills come from `FLLCoreResidentCivilizationObservation`:
- Gathering
- Crafting
- Learning

Legacy `FLLResidentData::Skills` is not the source for resident-detail display.

### Preferences

Authoritative preference source: `lifelens::PreferenceProfile` from `TraitsPreferences.h`, projected through `FLLCoreTraitPreferenceObservation`.

The preference profile follows the same authority rule as Traits: it is a named Core read model deterministically derived from persistent Personality/Genetics, not presentation-owned placeholder data. The eight authoritative preference dimensions are:
- Socializing
- Solitude
- Exploration
- Crafting
- Gathering
- Comfort
- Novelty
- Order

The Observer displays these values directly and must not fall back to legacy `FLLResidentData::Preferences` placeholders.

### Persistence / determinism rule for Traits & Preferences

Traits and Preferences intentionally do not introduce duplicate mutable serialization fields. Their inputs (`Personality` and `GeneticsProfile`) are part of authoritative resident state and already survive Core snapshot encode/decode. Therefore the named profiles reproduce exactly after Save/Load while remaining consistent with the resident's source state.

Regression coverage must prove:
- same WorldSeed + PopulationSeed produces identical profiles;
- changing PopulationSeed can produce different founder profiles;
- all profile values remain normalized to `[0,1]`;
- snapshot encode/decode reproduces exactly the same profiles.

### Behavioral influence rule for Traits & Preferences

Traits and Preferences are not display-only metadata. Core utility scoring consumes the same authoritative profiles when evaluating ordinary Social and Civilization choices.

Behavioral influence must remain bounded and must not replace hard simulation constraints:
- Socializing / Solitude / Compassion / Boldness and related traits may shift ordinary social intent utility.
- Gathering / Crafting / Exploration / Novelty / Order and related traits may shift ordinary civilization utility.
- Profile effects are bounded modifiers, not permission to fabricate unavailable actions or targets.
- Critical survival provision gathering for Hunger/Thirst bypasses ordinary preference modulation so personality cannot suppress required survival acquisition.
- UI never computes these behavior effects; it only displays the same Core-owned profile that Core decision logic consumes.

Regression coverage must prove that changing relevant profile inputs can change ordinary utility ordering while urgent survival behavior remains dominant and available.

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

## Ownership / maintenance

The data-completeness implementation is performed under `ASSIST_LOCK-UI-OBSERVER-DATA-1` on `jjun/observer-data-completeness-v1`.

After the milestone PR merges and the assist lock is released, normal Observer UI layout, styling, mobile readability, and presentation maintenance return to the Dagyeom UI lane. Future presentation maintenance must preserve this authority contract unless a deliberate Core/Bridge contract change is made.

## Acceptance

- Needs detail reads Core and shows numeric satisfaction plus compact visual bar.
- Personality detail shows all 14 Core dimensions.
- Traits show the eight authoritative Core `TraitProfile` dimensions.
- Preferences show the eight authoritative Core `PreferenceProfile` dimensions.
- Traits/Preferences are deterministic for the same population seed and reproduce exactly after snapshot restore.
- Traits/Preferences influence ordinary Core Social/Civilization utility without weakening urgent survival acquisition.
- Skills use authoritative civilization skill values.
- Relationships expose the directional Core dimensions, not only summary scores.
- Family continues to read authoritative Core family data.
- Knowledge & Gear remains direct-Core.
- Emotion values remain truthful; causal Emotion work is not smuggled into this milestone.
- Level 0/1 Observer behavior, camera input, mobile safe-area/touch behavior, and selection feedback remain intact.
