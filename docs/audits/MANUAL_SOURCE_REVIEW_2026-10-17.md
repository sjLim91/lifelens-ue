# LifeLens 실코드 수동 감사 — 2026-10-17 (1차 원문 분석)

> 기준: main 42191487d0412806ef26333478a13b29ffae915e.
> 이 문서는 단순 파일 인벤토리나 CI 결과가 아니다. GitHub 원문에서 직접 읽은 함수 흐름, 호출 우선순위, 자료구조와 테스트 가정을 정리했다.
> 모든 719개 파일을 줄 단위로 수동 검토했다는 의미는 아니다. Core 주요 cpp/h, 자율 목표, 사회/가족, 건강, 저장, Web bridge, 렌더·클라이언트 원문을 선택적으로 깊게 검토한 단계다.
> P1은 감사를 우선해야 한다는 의미이며, 증상이 실제 모든 시드에서 발생했다고 단정하지 않는다.

## SR-01 / P1 / 확정: 사회 회귀 테스트의 생산 경로 미검증
- Source/LifeLensCore/tests/test_simulation_social_loop.cpp:9 uses setupSocialDemo(), not setupNewGame().
- Source/LifeLensCore/src/Simulation.cpp:265-318 initializes SocialA/B with high sociability and pre-made relationship familiarity 0.82~0.88.
- Production setupNewGame():322-390 initializes four founders with familiarity random [0.0,0.04].
- Production NEW GAME first contact -> earned familiarity -> repeat social -> dating is NOT proved by the demo test.
- Needed: production seed + first contact attempt/decision/pending/completion/familiarity assertion, without preseeded bond.

## SR-02 / P1 / 확정: 사망 주민이 사회 후보 랭킹에 남음
- Source/LifeLensCore/include/lifelens/SocialUtility.h:259-284
- chooseSocialUtilityDecision() loops all world.characters and only skips candidate.id == self.id; candidate.alive is not filtered.
- Source/LifeLensCore/src/Simulation.cpp:1993-2011 rejects a dead target AFTER winner selection.
- An eligible living candidate can be displaced by a high-utility deceased target, leading to no social initiation in that planning slot.
- Must add a regression for 1 living actor, 1 dead high-bond target, 1 living lower-bond valid target.

## SR-03 / P1 / 원인 후보: 사회가 생존·문명·신체 효용 경쟁에서 소외
- Source/LifeLensCore/src/Simulation.cpp:2048-2199: urgent physical/provision, tryCivilizationDecision(), trySocialDecision() in ordered planner; social only when !hasUrgentPhysicalNeed.
- Source/LifeLensCore/include/lifelens/SocialUtility.h:805-868: social >= 0.18 and > physical*1.05; civilization can supersede according to minimum and margin.
- This is a concrete scheduling condition, not yet proof of a single root cause.
- Runtime NEW GAME 7-day observed socialEvents=0 in both baseline seeds while civilizationEvents=399 and 335 (PR #730 causal smoke).
- Needed: per-resident eligibleMinutes, socialScore, physicalScore, civilizationScore, winner, suppressedReason. Don't blindly tune thresholds.

## SR-04 / P1 / 확정: 밀집 관계 갱신·선형 조회 조합의 극단적 확장성
- Source/LifeLensCore/src/Simulation.cpp:3364-3373 creates or gets directional relationships for EVERY pair every day, including deceased residents in the first loop.
- Source/LifeLensCore/src/Simulation.cpp:3384-3418 scans every pair again for dating.
- Source/LifeLensCore/include/lifelens/Relationship.h:129-144 uses linear search through std::deque for both getOrCreate() and find().
- Dense relation state can use O(N^2) entries. O(N^2) pair evaluation coupled with O(N^2) search can reach O(N^4) scan comparisons in that code path in worst case.
- At scale, 1000 living population produces 499500 unordered pairs, 999000 directional rows before other filters (not a measured benchmark).
- Needed: explicit locality/encounter authority and indexed lookup preserving deterministic iteration/order; benchmark with population sizes.

## SR-05 / P1 / 인과 설계 경계: 태어난 아이가 모든 주민과 근거 없는 친밀도 데이터 생성
- Source/LifeLensCore/src/Simulation.cpp:3137-3158 creates child->all residents and all residents->child relationships, including non-parent.
- For non-parents, both directional familiarity fields are set to 0.08 without spatial or encounter gate here.
- This is demonstrated creation of the relationship rows, NOT proof that private memories or knowledge receipts are created.
- If intended, document its observed-world evidence; otherwise restrict to caregivers, witnessed birth or real encounter.

## SR-06 / P1 / 확정 기능 경계: 외부 모델 판단은 실제 행동 입력이 아님
- Source/LifeLensCore/include/lifelens/Simulation.h:121-127 explicitly documents acceptance only records input and does not execute/bias goals.
- Source/LifeLensCore/src/Simulation.cpp:230-256 validates fingerprint and journals the event.
- Source/LifeLensCore/include/lifelens/AutonomousGoalContract.h:13-17 and AutonomousAffordancePlanner.h:16-18 describe read-only candidates.
- Source/LifeLensCore/wasm/LifeLensWebBindings.cpp:7-31 exports request and goal preview, not acceptCognitiveProposal.
- web/src/cognition/shadow-observer.ts:80-89 read-only Shadow mode.
- Nuance: AutonomousSocialGoalRouting.h and ContextActionRuntime.cpp contain bounded PERSONAL goal target-selection/outcome learning AFTER Core selected an ordinary social slot. That is not a general AI proposal-to-action bridge.
- Needed: validated acceptance -> native Core planner candidate -> legal action/pending token -> observed outcome -> grounded learning.

## SR-07 / P1 / 검토 필요: 기본 사회 타깃의 거리 검사 불균일
- SocialUtility.h:259-284 ranks targets globally without distance.
- Simulation.cpp:1974-1985 applies SettlementServiceRadiusGrid only to optional AI-preferred alternative target; original baseline social target is not radius-filtered there.
- Simulation.cpp:2011-2037 routes baseline target by runtime position and ContextActionRuntime.cpp:176-215 checks proximity at completion.
- This may waste planning/execution time on unreachable or nonlocal targets; actual route-failure frequency and context lifetime remain to be measured.

## SR-08 / P1 / 확정 기능 경계: 지구 관측과 지리 조건화는 다른 단계
- Source/LifeLensCore/src/WebClientBridge.cpp:981-1052 newEarthGame binds geographic origin to standard procedural founder region.
- earthScenarioJson() explicitly reports singleOrigin=true, terrainGeographicallyConditioned=false, metricScaleProvisional=true, snapshotPersistence=false.
- This does NOT deliver two independent regions of 4 residents or Earth-conditioned water/climate/terrain. Save/load preservation is not yet promised by Core.
- web/src/earth/core-earth-scenario.ts:23-51 deliberately rejects DTO that claims otherwise.

## SR-09 / P2 / 실제 구현 + 품질 미승인: 성별 형태·동물
- web/src/render/resident-appearance.ts:89-161 DOES derive sex-related morphology, dimensions, genetics, age, skin/hair. Therefore saying 'sex difference absent in code' is incorrect.
- web/src/render/resident-anatomical-body.ts builds generated parametric body with rig-derived motion.
- web/src/render/resident-world-layer.ts:54-68 still references pinned external GLB avatar/animations and vendored fallbacks.
- WildlifeEcology.h:230-355 and 384-448 computes prey/forage, hunger, movement, birth and death, but clamps within a chunk without terrain-aware pathfinding.
- web/src/render/wildlife-layer.ts uses procedural geometry. Real-world visual quality, animation and mobile frame rate still require visual acceptance; #725/#727/#728 are not main.
- Read-only source review CANNOT certify how human/tiger looks on device.

## SR-10 / P2 / deterministic staging vs genuinely emergent family choices
- FamilyProgression.h:13-23 configures hard progression gates 30, 90, 60, 30-day intervals.
- Simulation.cpp:3439-3534 applies staged dating/cohabitation/engagement/marriage conditions.
- Simulation.cpp:3579-3622 attempts conception only for married opposite-sex biological pair after 30 days, every 7 days (same-sex partnership is not precluded by this reproduction rule).
- This is deterministic rules + choices; do not call it free-form self-invented culture and romance. A product-direction decision is needed to reduce fixed progression, not a blind bug fix.

## SR-11 / positive control: infant toileting is NOT simply missing
- Parenting.h:219-284 records care effects; ContextActionRuntime.cpp:730-790 deposits actual HumanWaste residue for child at resolved sanitation site / outdoor target and logs it.
- Avoid claiming 'child waste disappears' without auditing exact spatial return, carry/escort and pending token behavior.
- ContextActionRuntime.cpp:680-732 also handles resource consumption and nursing needs.
- Open #651/#652/#656 should be compared against existing main logic and tested by exact regression cases, not assumed wholly absent.

## SR-12 / positive control: short binary snapshot guard already exists
- SimulationSnapshotCodec.cpp:273-291 takes bytes.begin()+12 after readBinaryFormatVersion(bytes), which rejects byte arrays shorter than 12 and returns before that point.
- Do NOT label it a short-input buffer error merely from the isolated line.
- Snapshot restore validates character and context IDs, social facts, runtime state and simulation ruleset via SimulationSnapshot.cpp:17-235 and :297-361. This doesn't mean full corruption/security review is complete.

## Next deep-inspection gates

1. NEW GAME all founders from seed, chooseSocialUtilityDecision decision scores, pending token, actual relationship delta. Add regression with no privileged demo setup.
2. Restrict spatial Social candidate set to encountered/locally reachable characters while preserving actor-owned memories and no fabricated knowledge.
3. Remove all-pairs unconditional relationship creation or index lookups without changing deterministic iteration/snapshot compatibility.
4. Trace all save/load authority: world -> SnapshotCodec -> WASM bridge -> Web session, including earth origin and all actions.
5. Continue per-file manual review on unvisited source and tests. Report remains IN PROGRESS until completed.

## Honest coverage statement

Automated inventory read every Git-tracked path and hashed source content in PR #730; separate **human semantic review has NOT read all 719 files**, and these issue codes are based on the exact specified files/lines. CI and long-run are supplementary evidence only. No simulation rule changes were made here.
