# LifeLens Presentation & Integration Work State — 2026-09-18 23:45 KST

> Truth order: actual GitHub main / open PR / Actions > this snapshot > older handoff/status text.
>
> This document records the late-2026-09-18 screen/presentation sprint so work can resume without reconstructing state from chat history.

## 1. Current product direction

- Unreal-native autonomous life/civilization observer simulation.
- Core / World remains sole simulation authority.
- Presentation consumes authoritative read/action contracts only.
- Android remains first deployment target; long Android Gate B remains paused until explicitly resumed.
- Current execution lane is Presentation/Observer/World readability while Stage C Core roadmap remains the macro simulation path.

## 2. Collaboration reconciliation

### Dagyeom PR #98
Original Dagyeom PR #98 became far behind current main and was not merged wholesale.

Its still-useful intent was ported onto current main in:
- #202 **Port Dagyeom PR #98 onto current main** — MERGED.
  - Observer panel readability tuning.
  - `ll.ViewResidents [index]`.
  - `ll.ViewReset`.
  - QA camera isolation from the production observer camera tick.

After #202 passed Preflight + Unreal Linux Compile and merged, original #98 was closed as superseded.

Standing rule:
- Do not permanently avoid Dagyeom-owned files.
- Correct canonical file wins over artificial file separation.
- If Dagyeom work is stale, selectively port intent onto current main.
- If current Dagyeom work is compatible, integrate it first and continue on top.

## 3. Recent merged Presentation / Observer chain

### Foundation / authority cleanup
- #159 audit closeout docs.
- #160 roadmap regroup.
- #161 C-S1.
- #162 C-S2.

### Facility / character / environment / observer quality
- #163 Facility Presentation.
- #164 Character Daily-life Motion v3.
- #165 Environment Visual Polish v4.
- #166 Observer UX focus camera/social chrome.
- #167 Character Appearance v2.
- #168 Lifecycle Romance/Household Readability v2.
- #169 Observer HUD Visual Hierarchy v2.
- #170 Selected Resident Framing v2.
- #171 Adaptive Resident Camera Framing v3.
- #172 Facility Material Readability v2.
- #173 Resident Label Readability v2.
- #174 Environment Atmosphere Polish v5.
- #175 Family Tab Generation Readability v3.
- #176 Time Weather Chrome v2.
- #177 Lifecycle Notice Visuals v3.
- #178 Selection Ring Readability v3.
- #179 Physical Action Truth Signal v1.
- #180 Character Appearance Polish v3.
- #181 Context Prop Readability v4.
- #182 Social Speech Readability v3.
- #183 Moving Resident Camera Lead v4.
- #184 Offscreen Resident Cue v1.

### Late screen sprint
- #185 Facility condition readability v3.
- #186 Environment surface persistence v6.
- #187 Resident crowd label readability v4.
- #188 Social target facing v5.
- #189 Lifecycle notice priority v4.
- #190 Social world cues v4.
- #191 Time of day progress v3.
- #192 Character proportion polish v4.
- #193 Offscreen resident identity cue v2.
- #194 Settlement expansion visibility v1.
- #195 Resident label distance fade v5.
- #196 Lifecycle notice timestamp v5.
- #197 Environment transition smoothing v7.
- #198 Carry locomotion v5.
- #199 Social event priority v5.
- #200 High speed observer chrome v4.
- #201 Offscreen resident distance cue v3.
- #202 Dagyeom #98 current-main integration.
- #203 Planned facility site readability v1.
- #204 External selection camera sync v5.
- #205 Lifecycle notice focus v6.
- #206 Family tree and world overview v4.
- #207 Storage cargo truth v3.
- #208 Motion transition and sleep polish v6.
- #209 Resident detail state readability v4.
- #210 Recent lifecycle event recovery v7.
- #211 Night fire readability v4.
- #212 Relationship state readability v5.
- #213 Social counterpart link v6.
- #214 Night resident label readability v6.
- #215 Facility event focus v1.

All items above were merged only after their exact tested head satisfied required Preflight + Unreal Linux Compile at merge time.

## 4. In-flight work snapshot

As of 2026-09-18 23:45 KST, the following PRs are open and under CI. GitHub Actions is authoritative if this status becomes stale.

### First active wave
- #216 **Detail resident navigation v1**
  - Relationships/Family rows become direct resident navigation.
  - Keeps active detail tab and relies on external-selection camera sync.
  - Head: `0f9be32bc07a93686b257f2a5844bf615f9e87e9`.

- #217 **Sleep site posture v7**
  - Distinguish degraded ground sleep from real SleepingPlace sleep.
  - Uses authoritative settlement sleep target.
  - Head: `b12b989ae2cef484ab63b5a195952b4b5af5ac49`.

- #218 **Resource depletion visual v2**
  - Natural resource patch instance count/scale follows authoritative quantity.
  - Zero quantity produces zero visual resource instances.
  - Head: `56f1648205e75aa51d8db2a3dad09b45122a1dd4`.

- #219 **Selected social counterpart v7**
  - Selected resident social target receives a compact counterpart label.
  - Head: `f90ddcdfed1e8473f32e53d1686b7f99e4f6dd78`.

At snapshot time:
- #216~#219 Preflight: PASS.
- #216~#219 Unreal Linux Compile: in progress.

### Expanded parallel wave
Parallel-count restriction was removed by the user. Independent files may now run in larger batches.

- #220 **Snow cover accumulation v6**
  - Snow presentation accumulates through storms and thaws by temperature.
  - Head: `f476461294f62124db041b11f38f6419d3c18c9d`.

- #221 **Detailed daypart chrome v5**
  - Dawn / morning / day / sunset / evening / night.
  - Head: `9d2d961c27959680a30eff6ce2a585f7215cfbf9`.

- #222 **Resident identity badge v7**
  - Korean life-stage badge and selected resident age.
  - Head: `910de7f511400c608681f70f68471dec228d7f0d`.

- #223 **Offscreen action cue v4**
  - Offscreen selected-resident cue includes current action.
  - Head: `d6ae030ac315fde7fb1ab71eda04f34d98ec211b`.

- #224 **World event focus return v2**
  - First empty tap after facility/world-event focus returns camera to selected resident without closing inspector.
  - Head: `bcbbbeee0d2ab9912bf0a7215fbecddfc6154d77`.

At snapshot time:
- #220~#221 Preflight: PASS.
- #222~#224 Preflight: running.
- #220~#224 Unreal Linux Compile: running.

## 5. Merge / CI standing rule

For every PR:
1. Refresh exact current PR head.
2. Required Preflight + Unreal Linux Compile must both be successful for that exact head.
3. If compile fails, fetch failed run/job/log and patch the same branch/PR.
4. Require new exact-head green CI after the patch.
5. Check current open PR/file overlap.
6. Merge with expected head SHA.
7. Never merge based only on an older successful run.

## 6. Parallel execution rule

The previous practical 4-item batch cap is removed.

From now on:
- 6, 10 or more PRs may run in parallel when source ownership/files are independent.
- Avoid opening concurrent PRs that modify the same canonical file unless intentionally coordinated.
- Do not distort architecture merely to create file independence.
- If the correct responsibility belongs in the same file, sequence or combine the work instead.

## 7. Immediate Presentation backlog after current wave

High-value next work:
- resident/family/relationship navigation polish after #216 runtime validation.
- more truthful load/carry/unload distinction.
- sleep/lie/wake authored animation path when suitable free assets exist.
- settlement path/zone readability derived from actual activity, not invented zoning truth.
- stronger facility construction/ruin/material evolution.
- world event/history browsing beyond the compact recent event trail.
- long-generation genealogy readability.
- mobile safe-area and dense-population runtime QA.
- Android visual LOD/performance tuning when device lane resumes.

## 8. Macro roadmap remains unchanged

Presentation work is a parallel quality lane. Simulation macro order remains:

1. Stage C — settlement, survival, early civilization.
2. C-S3 — durable water/food/storage/spoilage/cultivation.
3. C-S4 — emergent settlement geometry.
4. C-S5 — tin/bronze after real prerequisites.
5. Stage D — long-run history/fast-forward + open-ended knowledge/capability/technology engine.
6. Stage E — health, education, economy, migration, multiple settlements.
7. Stage F — historical civilization through modern/digital/AI/space into open-ended future.

No time-only era unlocks.
