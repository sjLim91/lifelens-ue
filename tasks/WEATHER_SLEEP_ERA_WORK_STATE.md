# Weather sleep, primitive bedding and current civilization era

Base: 66f623db9413ff626ec03dee433cec482c4e8efc (#611).
Branch: work/weather-sleep-primitive-bedding-era-20261003.
User authorized GitHub connector workflow instead of clone/pull after terminal access failed.
No merge authorized.

Implemented: central SleepEnvironment evaluation, fatigue-bounded reachable weather-aware target selection, exact occupied sleep facility recovery, exposed emergency factual DTO, bounded shelter replan, local sleeping Health protection; primitive mat support 0.14; derived observer era registry and Korean badge/dialog.

Checkpoint: direct g++ Core library compilation passed; 14 focused/new tests passed. CMake is unavailable locally. GitHub Actions full Preflight/CTest/WASM/Web checks pending. Two-seed 100-day baseline/candidate comparison in progress; initial candidate seed 874213954 has long sleep saturation, do not claim long-run acceptance.

Next: capacity/replan regressions, full remote gates, resolve failures, compare long-run metrics to base and report limitations. Review raw mobile/visual screenshots when available. Do not merge.

PR: https://github.com/sjLim91/lifelens-ue/pull/612 (Draft, never merge).
Remote checkpoint before this update: 4d95b3d179fe94f7955f32ab8e59579dc5d7f193.
Latest local policy: weather recovery multiplier 1 - exposure*0.15 (protected residual 0.15), emergency bedding ground baseline; ordinary calm/moderate bedding service radius preserved, emergency/protected adverse-weather routes bounded 16..64. Original urgent planning priority preserved.
16 focused tests rerunning after policy restoration; three-seed 365-day comparison in progress. Previous local numerical experiments are not accepted results. Repaired capacity/weather-transition snapshot fixture retains original resident runtime IDs. Centralized Korean sleep labels feed resident panel and observation feed. Added slope-clearance support for mat alongside existing LayToIdle calibration.

Latest 365-day acceptance FAILED for seed874213954: max continuous fatigue saturation83270 min, total427650 resident-minutes, living3. Seeds874213955 and2 passed365 days (501/278 max). All remote CI at47505d3e passed104Core tests andWeb, but only100d longrun atthathead; do notmarkready based onthat. Newlocalverify_sleep_longrun guard detects failure. Trace late failure before deciding causal fix; avoid numerical overfitting or unrelated urgent AI rebalance.

Causaltrace capturedminute202617/day141: fatigue/bladder/hygieneall1, repeatedUseToiletfailuresthenthirty-minuteIdlebackoff. LatestimplementationallowsgroundemergencySleepONLY duringplanningbackoffandSleepNeed>=.98; nochangeordinaryEat/Drink/Toiletorder. Freezeactualpositionandretainexistingurgentwake; central thresholdSleepEnvironmentContract. Addedexplicitbackoffregressiontest. V12three-seed365/O2and16focusedtestsnowrunning; doNOTclaimpasseduntilcomplete. Localuser-facingmobilebadge44pxtouch. LatestCIguardrequires365daysandfailscontinuousfatiguesaturation>=10000minutes.
