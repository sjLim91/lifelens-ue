# PR #612 cumulative sleep saturation follow-up

Repository: sjLim91/lifelens-ue; branch work/weather-sleep-primitive-bedding-era-20261003.
Original head f24f4f695b98825e0c2520deb5989e35b141df03; base 66f623db9413ff626ec03dee433cec482c4e8efc.
Continue this PR only; never merge. GitHub connector only, no clone/pull.

## Causal evidence (confirmed)

Opt-in diagnostics reproduce original 365d results exactly for all three seeds. For seed954, residents made 57,397 failed toilet plans and 24,088 backoff rest attempts. In day200 snapshot, residents at1097,498 select outdoor dry points1097,492 /1097,491 with no Core route. Contamination-only ordering ignores connected ground. Repeated route failures→retry cooldown→short ground rests→bladder1 urgent wake→same unreachable target. Weather itself is not the root cause: seed2 has more severe exposure but very little saturation. Roof-capacity rejections/replans are0; distance rejections don't explain the failure loop.

Same exact day200 snapshot replay, 2 days:
- original: toilet failures430, total fatigue saturation983; mean end need .9269/.9074/.9488.
- reachability only: failures3, saturation98; mean end need .6761/.6512/.6278.

Minimal fix: pass an optional reachability predicate through the existing sanitation target pure evaluators. Prefer reachable local active sites; rank reachable outdoor candidates with unchanged contamination score and deterministic tie order. Fallback remains actor's actual position if all local alternatives are inaccessible. Navigation, interaction, residue, hygiene, materials, needs balance, AI priority, weather sleep selection/recovery/duration/wake/replan all unchanged. Read model reuses frozen actual toilet target instead of repeating selection/pathfinding on each observation. Ground-invalid endpoint rejection is logically identical to route failure, avoids pointless searches.

Identified initial-duration underestimation (start→travel/weather change→budget end before.12) as secondary evidence, not modified: isolated reachability fix already improves seeds955/2. Do not broaden sleep session balance without new failing evidence.

## Current execution status

No remote follow-up commit yet. Local source /workspace/lifelens-review is reconstructed from GitHub, not a checkout.
Original diagnostic build /tmp/ll_balance_audit_diag_before and /tmp/liblifelens-diag-before.a; original outputs /tmp/sleep-diag-before-954.log, -874213955.log, -2.log. Snapshots /tmp/sleep-before-954-snapshots/day-{100,140,200}.llsave. Original base metrics retained tasks/WEATHER_SLEEP_ERA_LONGRUN.json and /tmp/lifelens-baseline-365-*.log.

First reachability-only 365 outputs:
-955 longest204,total2739,living4
-2 longest173,total1298,living4
-954 still running (session39738,pid6399); uses expensive old observer repeated pathfinding. Stop only this experimental run when final executable is ready, rerun final observer optimization with identical selection logic.

Final executable building /tmp/ll_balance_audit_followup (session55253); library /tmp/liblifelens-followup.a. Latest only Simulation.cpp and harness additionally changed since reachability build; others .o current. Final 365 runs and full remote CI still REQUIRED.

Local checks PASS: weather sleep, daily physiology budget, action commitment, era, designated sanitation affordance, world generation sanitation reference; new sleep_saturation_recovery PASS. Same new test linked against original head diagnostics FAILS reachable-destination assertion (not setup assertion), proving regression. Python guard4tests PASS; pinned main all3 logs PASS; original head fails954/955 cumulative limit (45402/14905), improved955/2 PASS. No decay/recovery/threshold/weather/travel/capacity/priority/session changes.

## Next steps

1 Finish final build, final same-snapshot2day equality check, final3seed365 comparison. Keep progress updates; no long blocking sleeps.
2 Local focused tests on final library; remaining evaluator predicate tests and diagnostic non-interference checks.
3 Capture original/final perresident/world diagnostic JSON + condensed timeline/replay evidence under review/sleep-saturation-followup (avoid4MB CSV in git), update baseline report with before and after, summary docs.
4 Connector create_tree/create_commit/update_ref fast-forward same branch. Upload only actual follow-up files (not original backups); final parent verify actual#612 head.
5 All7CI gates incl105Core tests/WASM/Web/longrun must PASS on final code head. Update PR body removes obsolete unresolved regression statements and includes cause,trace,same-snapshotproof,3seedtable,context ratios/replans/sessions,unchangedfeatures,CI links. Do not merge.

Final optimized runs now active: session26129 (seed954), session96564 (955 then2); final955=204/2739/4 and2=173/1298/4. Old954 experiment terminated intentionally, final954 pending. Final same day200 snapshot replay exactly matches earlier reachability-only state/metrics (excluding elapsed/instrumentation fields). Final8focusedCoretests andPython4guardtests PASS; added diag-vs-no-diag encoded-save equality PASS. Remote follow-up code commit next, full CI pending.
