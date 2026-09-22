# LifeLens Core → Observer Visualization Matrix

This document is the presentation contract for existing Core read models.
Presentation may translate, animate, group, or visually emphasize Core-owned
facts, but it must not invent simulation state, positions, dialogue content, or
outcomes.

## Current coverage

| Core authority | Web observer presentation |
| --- | --- |
| Resident runtime position | 3D resident movement at authoritative grid position |
| PresentationDirective Physical | Korean current-action bubble + matching validated animation |
| PresentationDirective Social | target-facing interaction, Korean social-action bubble, talk animation where supported |
| Pending Social actions | Approach / Avoid / Repair / Comfort shown with actor and target |
| Recent SocialCommunication events | “사람들 사이” feed with actor, target, type, time, intensity |
| Parenting context actions | Korean care-action bubble and target-facing presentation |
| KnowledgeTeaching context actions | teaching bubble + technique provenance in focused resident detail |
| Resident needs / emotion / personality / traits | focused-life detail |
| Directional relationships | focused-life relationship panel |
| Family / romance / pregnancy / genealogy | partner, pregnancy, parents, siblings, children |
| Memories | Korean text, confidence, location and provenance (direct / told / inferred) |
| Beliefs | Korean proposition presentation while raw Core semantics remain unchanged |
| World major life events | recent event history, participant and related residents |
| Civilization resident inventory | focused-life inventory |
| Civilization skills | gathering / crafting / learning values |
| Civilization technique knowledge | level, confidence, uses and provenance |
| Civilization discoveries | “기술 발견과 전파” feed |
| ResourceNode positions | visible world resource markers at Core grid positions |
| StorageSite positions / contents | linked storage readability and stored-unit presentation |
| ConstructedFacility | planned / under construction / operational / ruined world objects |
| Facility progress | visible construction state and percentage |
| FirePit / Furnace runtime | lit/heat visual presentation |
| PrimitiveSanitationSite | designated area / dug pit world object |
| EnvironmentalResidue HumanWaste | ground contamination using Core intensity and radius |
| DynamicEnvironment | weather badge, atmosphere, precipitation and visibility |
| Terrain / hydrology / ecology | Core terrain surface, water and vegetation |
| Simulation clock | canonical continuous observer speed controls |

## Important boundary: dialogue

The current Core social loop owns semantic social actions and social events
(positive interaction, comfort, apology, conflict, betrayal, rejection,
intimacy, commitment) plus knowledge teaching and rumor/belief propagation.

It does **not** currently own a multi-turn utterance transcript. The observer
therefore shows truthful semantic speech/status bubbles such as “대화 중”,
“사과 중”, or “기술을 가르치는 중” and never fabricates quoted dialogue.

If multi-turn spoken dialogue is implemented later, it must first become a Core
state/read-model contract; only then may Web/Unreal render actual utterance
turns.

## Presentation rules

- Initial spawn is a coordinate entry point, not a permanent living-zone authority.
- World consequences use Core grid positions only.
- Presentation never teleports or extrapolates resident simulation position.
- English/internal keys may remain in Core serialization, but user-facing text is localized.
- Repeated high-frequency snapshots should reuse presentation objects when Core state is unchanged.
- Important actions are readable without turning the world into a debug overlay: short contextual bubbles, world consequences in-place, history in observer feeds.
