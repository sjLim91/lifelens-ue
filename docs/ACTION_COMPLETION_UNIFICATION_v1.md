# Action Completion Unification v1

Status: **ACTIVE**

## Purpose

Social, Civilization, and Parenting decisions must not mutate authoritative simulation state merely because Utility AI selected an intent.

The canonical runtime sequence is:

`Core decision -> runtime-only pending action -> movement / interaction presentation -> token ACK -> Core outcome`

Physical Eat/Drink/Sleep/Toilet/Hygiene already use the same authority principle through the physical completion ACK path. This milestone extends that principle to contextual actions without creating a second simulation authority in Unreal.

## Authority rules

- Core owns the selected action, stable target identifiers, authoritative GridPos where one exists, action duration, validation, and final outcome.
- Unreal owns movement, facing, interaction timing presentation, and animation only.
- Unreal cannot directly add resources, change relationships, relieve child Needs, create sanitation residue, or complete civilization work.
- A context outcome is applied only by `Simulation::completeExternalContextAction` after Core validates the matching pending token and resolved position.
- A stale or wrong token is rejected.
- Target validity is rechecked at ACK time.

## Pending action lifetime

`PendingContextAction` lives only in `Simulation::Runtime`.

It is intentionally omitted from `SimulationStateSnapshot`:
- Save/Load never replays unfinished movement or animation.
- after restore, Utility AI may make a fresh decision from restored authoritative state.
- the monotonic transport token is not simulation randomness and is not serialized.

Timeouts prevent an unreachable presentation target from freezing a resident forever:
- Social: 45 simulation minutes.
- Parenting: 45 simulation minutes.
- Civilization: 120 simulation minutes.

A timeout applies no action outcome.

## Social

The pending record stores the Core `SocialUtilityDecision` and target resident.

Before ACK:
- no relationship event is applied;
- no social memory is created;
- Avoid relief is not applied.

At ACK:
- target must still be alive and valid;
- Approach / Repair / Comfort must resolve near the target's authoritative runtime position;
- Avoid must finish on a different Core grid cell from the target;
- only then does Core run `executeSocialDecision` and start the normal cooldown.

## Civilization

Gather and Store pending actions carry the real Core ResourceNode / StorageSite identifiers and their Core-owned GridPos.

Rules:
- Presentation never guesses a scenery object.
- blocking tree/rock visuals do not require capsule overlap with the exact target centre;
- World presentation may stop at a truthful interaction radius, while Core accepts only the corresponding target grid neighborhood.
- resource/storage inventory mutation happens only after ACK.

Experiment/Craft actions with a real sanitation locus also carry that Core target.

Designated sanitation creation is bound to the exact pending Core GridPos: Core revalidates that location at ACK time and must not recompute a different site after the resident already travelled to the original target.

## Parenting

Dependent care now uses the same pending/ACK model.

Before ACK:
- Feed consumes no provision and changes no child Needs;
- sleep/bathing/comfort/etc. produce no care outcome;
- ToiletAssist produces no residue.

At ACK:
- caregiver and child must still be alive;
- caregiver must still be the child's parent;
- caregiver must resolve near the child's authoritative runtime position;
- Feed rechecks actual PlantFood/Water inventory;
- ToiletAssist records HumanWaste at the physically resolved Core position and aligns the dependent child's runtime position with that care location.

## Headless Core compatibility

Standalone Core tests and deterministic simulation runs normally have external execution disabled. In that mode, the same pending action is created and completed synchronously through `completeContextAction`.

This preserves deterministic headless progression while ensuring there is only one authoritative completion implementation.

## Unreal integration

Bridge:
- `GetResidentPendingContextDirective`
- `CompleteResidentContextAction`
- typed Context kind / token / duration / Parenting action reuse the existing `FLLCoreActionDirective` rather than introducing a parallel DTO.

WorldDirector:
- pending context directives are consumed before the legacy completed-action presentation directive;
- a dedicated `LLWorldDirectorContextActions.cpp` owns movement/use-time/ACK routing;
- the existing physical directive implementation remains isolated.

Character Context Motion may consume the typed directive for animation, but animation never determines success.

## Acceptance

Required before merge:
- Social outcome is unchanged before ACK and changes after valid ACK.
- wrong/stale token is rejected.
- Gather resource/inventory totals are unchanged before ACK and mutate after correct target ACK.
- Parenting care waits for ACK.
- assisted toilet residue appears only after ACK at the resolved location.
- pending context state is absent after Save/Load restore.
- existing headless deterministic behavior remains stable.
- Core test suite PASS.
- Preflight PASS.
- Unreal Linux UHT/UBT PASS.

Android packaging is not required solely for this authority milestone; later device QA validates movement feel and context animation.