# LifeLens Canonical Work State

> 현재 진행 상태의 단일 기준판. 실제 GitHub 상태가 항상 우선한다.
>
> 제품 기준: `docs/LIFELENS_SPEC_v1.1.md` + `docs/CIVILIZATION_PROGRESSION_v1.md`
> 협업 기준: `docs/STATE_MANAGEMENT.md` + `docs/INTEGRATION_SPRINT.md`

Last reconciled: 2026-09-14 KST — `main` HEAD includes PR #53 merge `ec30d80b2986247f0f16572efb2c082a933d796d` and prior macOS clang hotfix PR #54 merge `3b649b900c44a4e48bb89171b38f5e685e757b14`.

## Mandatory sync gate

1. 작업 시작/재개 전에 actual `main` HEAD, 대상 branch/PR/Actions를 확인한다.
2. 이 문서 → 역할별 READY queue → `TEAM_BOARD.md` → `HANDOFF_LOG.md`와 대조한다.
3. 다르면 코드보다 문서를 먼저 갱신한다.
4. 의미 있는 checkpoint마다 상태를 갱신한다.
5. 쭌이 다겸 소유 작업을 도울 때는 `docs/INTEGRATION_SPRINT.md`를 적용한다.

---

## Canonical product direction — Autonomous Civilization

**초기 4명 → 자연 자원 채집 → 저장/소유 → 실험/실패/발견 → 개인 지식 → 제작/도구 → 목격·모방·교육 → 전문화/교환 → 세대 누적 → emergent civilization.**

- 전역 recipe/tech 자동 unlock 금지
- 강제 시대 gate 금지
- 기술은 needs/resources/experiment/personal knowledge/skill에서 발생
- 지식은 개인에서 시작하며 전파·소실 가능
- 현재 현대형 bootstrap anchor는 개발용 affordance일 뿐 canonical 시작세계가 아님

---

## Active work

### 1. Integration Sprint — ACTIVE

- Owner: 쭌 + 다겸, 각자 기존 소유권 유지
- Status: `IN_PROGRESS / REVIEW_FIRST`
- Protocol: `docs/INTEGRATION_SPRINT.md`
- Goal: 빠르게 전진한 Core/Bridge와 Dagyeom UI/Presentation을 하나의 최신 main 기반으로 합류시킨다.
- Jjun default mode: `REVIEW_ONLY`
- Jjun이 Dagyeom-owned 코드를 직접 수정해야 하면:
  1. exact target PR/HEAD 재확인
  2. `TEAM_BOARD.md`에 `ASSIST_LOCK`
  3. `integration/dagyeom-<scope>-assist` 생성
  4. locked paths만 수정
  5. handoff/merge 후 lock 해제
- `dagyeom/*` direct push 금지
- 큰 새 Jjun Core slice는 integration checkpoint 동안 보류

### 2. Dagyeom PR #17 — Observer HUD v2 reconciliation

- Owner: 다겸 + 다겸 AI
- Branch/PR: `dagyeom/observer-ui-v2`, PR #17
- Status: `READY TO RESUME R1`
- macOS clang shadow blocker IR-MAC-SHADOW-01: **RESOLVED** by PR #54
- Civilization read blocker: **RESOLVED** by PR #53
- Next: latest main reconcile → existing Bridge binding → civilization detail binding where appropriate → review fixes → validation

### 3. Dagyeom stacked chain

- #26 UI foundation: latest-main reconcile 가능
- #29 Character Presentation: #17 이후
- #30 Observer UX Polish: #17 이후
- #36 Mobile Touch: #30 이후
- #38 Visual Feedback: #36 이후
- parent-first, mass force-rebase 금지

### 4. Old Android validation

- `task/03-fast-test`, PR #2: `FROZEN`
- Run `34739283266`: failure, Cook/Package/APK 미도달
- 수정/재실행/부활/병합 금지

---

## Latest completed milestones

### PR #54 — macOS clang shadow hotfix
- Merge: `3b649b900c44a4e48bb89171b38f5e685e757b14`
- `ObserverReadModelV2.h` loop variable `pregnancy` → `entry` only
- behavior change 없음
- Preflight PASS
- Core Build/Test/deterministic harness PASS
- Dagyeom PR #17 R1 blocker 해제

### PR #53 — Civilization Observer Read DTOs v1
- Status: **DONE / MERGED**
- Feature head: `e3a9f6611118866a6c79eb300a7b6ab2d5ffaf31`
- Merge: `ec30d80b2986247f0f16572efb2c082a933d796d`
- Core Run `34821150702`: PASS including full tests + deterministic harness
- Preflight `34821150693`: PASS
- Unreal Linux Compile Run #16 `34821150704`: PASS including UE 5.6 image verify + UHT + UBT + final link
- Superseded Run #15 link failure was fixed by adding `CivilizationKnowledgeTransmission.cpp` to `LLCoreCompileUnit.cpp` and adding validator coverage
- Published read-only APIs:
  - `GetResidentCivilizationObservation(...)`
  - `GetCivilizationWorldObservation(...)`
- Readable data:
  - resident inventory / carrying
  - technique level/confidence/practice
  - gathering/crafting/learning skills
  - SelfDiscovery / DirectWitness / Teaching provenance
  - ResourceNode / StorageSite summaries
  - discovery/knowledge aggregates + recent discoveries
  - stable Resident FGuid projection
- No UI/Character Presentation authority added; no second civilization authority/cache

### PR #52 — Civilization Knowledge Transmission v1
- Merge: `b90da9242003fbc0cbc553605b9abc46a17aa044`
- 37/37 + deterministic harness PASS; Preflight PASS

### PR #51 — Autonomous Civilization Action Loop v1
- Merge: `55d5211160c8edad32b01177e2b9326a9faa2b78`
- Gather/Store/Experiment/Craft autonomous; Core+Preflight PASS

### PR #50 / #49 / #48 / #47 / #46
- #50 Runtime/Persistence: `c31c422c305a3a79a9553d37ac86247aa31d1853`
- #49 Civilization Foundation: `36bd1ac81192bc689c1e811553f068f255642508`
- #48 World Affordance: `a90ff6a5d870858a9e555ddf1e6e526bb7aa34e1`, UE Run #14 PASS
- #47 Witness/Rumor: `f1aab37f6f8abad1217783cc1d168cbdd2e0c20c`
- #46 Physical Action Bridge: `753df19657ea634ea2fa7c2ac935f6273ce14c10`, UE Run #10 PASS

---

## Next sequencing

1. Dagyeom PR #17 latest-main reconciliation first.
2. Jjun supports under Integration Sprint rules; REVIEW_ONLY by default.
3. PR #26 where independent, then #29/#30 → #36 → #38 parent-first.
4. Integrated runtime verification: Core civilization + Observer UI + Character Presentation.
5. Android smoke APK after the integrated slice is observable.
6. Deeper civilization chains (fire, stone tools, containers, construction, agriculture, metallurgy) resume after integration checkpoint.

## Recovery rule

If interrupted: fetch actual main/branch/PR/Actions → compare with this file/queue/board → update stale docs first → resume only from last verified checkpoint.
