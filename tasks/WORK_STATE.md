## 2026-10-20 ACTIVE — 실제 식물 소비와 주민 야생동물 사냥·현장 식사

- 기준 main `3162b2debd9e4d54148a9710662808b496c3d994`; 브랜치 `codex/wildlife-subsistence-20261020`.
- 사용자 요청: 클론 없이 GitHub 연결로 초식 자원 고갈·기아/번식 영향·주민 사냥/식사를 연결.
- 기존 고정 forage 확률과 주민 사냥 경로 부재를 확인. production은 실제 ResourceNode를 소비하며 사냥은 실제 개체/이동/시간/토큰을 검증하고 현장 식사만 완료한다.
- 신규 Core 회귀와 저장 재개 검증 추가. 전체 CMake/CTest/결정론/장기/WASM/Web 결과는 PR Actions 확인 전 대기.
- 고기 운반/보관/조리·수생 먹이망·동물의 청크 간 이주는 미구현. main 병합/배포하지 않음.
- 상세: `docs/WILDLIFE_SUBSISTENCE_2026-10-20.md`.

## 2026-10-19 DRAFT — 주민 인지 의도와 실제 Core 기회 통합 (#798)

- 기준 main `aaddbd6ba71a6b66664be0e71fa06c56138f5a62`; 브랜치 `codex/790-resident-core-opportunities-20261019`.
- 사용자 요청: 클론 없이 GitHub 연결로 미완성 구현. GitHub 연결 도구로 파일/브랜치/커밋/PR을 작성했다.
- #796/#797의 같은 main 기반 교육·돌봄 변경만 선택적으로 재사용하고 교역 상대/여행자 및 실제 공동 압력을 충족한 가구/지도자 선호를 연결했다. 원래 PR을 수정하거나 병합하지 않았다.
- 공통 최신 입력·1일·유효 우선순위·생존 대상 guard, 최대 0.12 순위 선호. 실제 성공 점수·재고·지식·경로·생존·가족·합의·완료 토큰은 기존 Core authority 유지.
- 연결로 읽은 필요한 소스 파일만 사용한 로컬 C++17 구문 및 실제 Core 대상 회귀 5개 PASS: 교육, 돌봄, 상호 교역/guard, 정착지 교역/귀환/중단 재개, 가구 이주/binary replay.
- 돌봄 회귀의 bool 실패 반환·실행 시점·대상 인덱스 오류를 수정했고, 실제 음식 소비/배고픔 감소와 중복 ACK 거부를 확인했다.
- 첫 구현 커밋의 Preflight/WASM/지구 좌표/생태/야생 binary/자율성 감사 PASS. 최종 HEAD의 전체 Core/결정론/365일 및 Web 검증은 PR Actions로 확인한다.
- main 병합/배포는 아직 수행하지 않았다. #790의 장기 인지 영향 및 나머지 지구/생태/시각/건강 요구 전체의 완료를 뜻하지 않는다.
- 상세 범위: `docs/RESIDENT_COGNITIVE_OPPORTUNITIES_2026-10-19.md`.

## 2026-10-17 DONE — 남녀 원본 메시·기존 동작/생애/LOD와 호랑이 main 통합 (#731)

- 사용자 요청대로 #731을 main에 merge. 제품 코드: `42a9053ff3f88fe78612ad4463911f7da949c0c6`. #728도 동일 통합 이력으로 병합 상태다.
- 원본 CC0 남녀 표면·UV·눈·헤어를 기존 UAL 골격/모션에 연결. 실제 3단계 스킨 LOD, 성장/임신/유전 표현, 실제 스킨 발 접지와 늦은 유전 갱신 좌표 불변 회귀를 완료했다.
- Native WebGL + 실제 React/WASM 앱 run [39021921627](https://github.com/sjLim91/lifelens-ue/actions/runs/39021921627) PASS. 초기 4명 남2/여2 새 스킨, Core 시간 진행·carry/walk, 브라우저/HTTP 오류 0.
- 전체 Web Checks [39021982708](https://github.com/sjLim91/lifelens-ue/actions/runs/39021982708) PASS: typecheck/build/전체 회귀/8개 화면 영역.
- 병합 직전 main `34ea6295`의 Core/Web 브리지/로컬 추론 294개 파일은 병합 후 해시 동일. 기존 AI 수정 보존.
- Web Preview [39023026143](https://github.com/sjLim91/lifelens-ue/actions/runs/39023026143), Pages [39023106736](https://github.com/sjLim91/lifelens-ue/actions/runs/39023106736), 외부 점검 [39023110559](https://github.com/sjLim91/lifelens-ue/actions/runs/39023110559) 모두 PASS.
- gh-pages `57e2183f`는 제품 source `42a9053f`를 게시했다. 새 남녀 GLB 6개가 main과 Git blob 해시 동일하게 게시됐음을 확인.
- 실행: https://sjlim91.github.io/lifelens-ue/index.html?v=42a9053f
- 실제 Android FPS는 미측정이다. 브라우저 검증을 실기기 성능이나 최종 사진 수준 품질로 표시하지 않는다.
- 복구/증거: [NATIVE_RESIDENT_INTEGRATION_HANDOFF_2026-10-17.md](NATIVE_RESIDENT_INTEGRATION_HANDOFF_2026-10-17.md). 원격 작업은 GitHub 연결 도구를 사용하며 터미널 네트워크 재시도는 하지 않는다.

## 2026-10-13 DONE — 발 접지 보정·직접 제작 동식물 시각화 (#710/#711)

- Core #710 main `db4d93163948edc0c69f4c50f749a154a4769663`; 화면 #711 main `6135ea4afb5e9a4dfa163b1ba0c2e4526a130de8` 병합 완료. 최신 주민 의복/외형/동작 변경을 보존.
- 실제 16×16/기존 10×10 렌더링 삼각형 접지, raycast 1,144점/음수 좌표/rebasing PASS.
- 직접 제작 토끼·사슴·늑대·담수어와 기존 나무·풀·관목·꽃/열매. Core 관측만 표시하며 기존 저장에 동물을 생성하지 않음. 어린/성체 물고기 수면/바닥 검증 PASS.
- Core 최종 PR 9개 workflow PASS(CMake/CTest·결정론·365일·WASM 포함). 화면 최종 PR Preflight + Web Checks 모두 PASS(typecheck/build/전체 회귀/실제 WebGL 화면 캡처).
- Web Preview 38385310238 / Pages deployment 38385364177 / External Preview Probe 38385372273 모두 PASS. gh-pages `5e3135a919128c41c77415b19646aac06639f31a`, 화면 source `6135ea4`, Core runtime source `db4d931` 및 checksum 확인.
- 실제 실행: https://sjlim91.github.io/lifelens-ue/index.html?v=6135ea4 . 동물 확인은 새 게임(v4). 연속 동물 보행/사냥은 후속 범위.
- 병합 후 추가 main 검사: Core CTest·결정론 PASS, 365일 run 38384160386과 Web Checks run 38385310317은 이 기록 시점에 진행 중. 병합 전 검증·배포 결과와 구분함.
- 상세: docs/TERRAIN_GROUNDING_WILDLIFE_VISUALS_2026-10-13.md / docs/WILDLIFE_WORLD_OBSERVATION_2026-10-13.md.

## 2026-10-13 DONE — CI 공통화와 변경 범위별 검수 (#714)

- 사용자 승인 범위: 반복 Web install/typecheck/build/전체 회귀를 통합하고 화면·365일 검사를 관련 변경으로 제한.
- 별도 브랜치: `codex/ci-deduplication-20261013`, 기반 main `5a54041f5f7a54ce7c12e7d9d5c08a78e5ad2abc`.
- 자동 Web workflow 16개를 공통 `web-checks.yml`로 통합하고 기존 진단은 수동 실행으로 유지. 실제 WASM Shadow invariant와 배포 pipeline은 보존.
- Core configure/build/CTest/결정론을 1회 실행하고 관련 Core 변경·02:17 UTC nightly·수동 실행의 365일 검사가 같은 실행 파일을 사용.
- 로컬 14개 selector/runner 회귀, 전체 workflow YAML/jobs/steps, Python 구문, diff 검사 PASS. 최종 PR head `e645ee0e9b8c388a27f92d1e66fed7884641d6d3`의 Preflight/Core Tests/Web Checks/WASM Shadow invariant 모두 PASS. 전체 Web 회귀·타입·빌드와 8개 화면 capture PASS; 이번 CI 정책 변경의 365일 단계 skipped 확인.
- 현재 main의 required status contexts/rulesets는 비어 있음. branch protection/권한 변경 없음.
- 제품 PR #710/#711을 포함하거나 병합하지 않음. 추가 grounding/wildlife 검사·capture는 공통 runner가 파일 존재 시 발견.
- main 반영: PR #714 squash 병합, `75ba930e1c378eb2507f5d3db3d23febd04852b1`. 병합 후 Preflight PASS; Core Tests run 38356379638 / Web Checks run 38356379634 진행 중. 제품 변경이 없어 배포 workflow는 이 커밋에서 시작되지 않음.
- 상세 정책: `docs/CI_SCOPE_AND_DEDUPLICATION_2026-10-13.md`.

## 2026-10-12 DONE — #700 병합·실행 미리보기 반영

- 모든 exact-head CI PASS. Core CTest 119개/실패0, 3 seed × 365일 검증 PASS.
- main merge SHA: `5203449a52960923882a5f16379a18e834ea0e62`; #700 squash 병합 완료.
- 제품 preview source SHA: 같은 `5203449`; gh-pages publication SHA: `a74cedd544b61443dd25b3c79e894b1ac875a0ee`.
- Web Preview https://github.com/sjLim91/lifelens-ue/actions/runs/38254855263 PASS.
- External Preview Probe https://github.com/sjLim91/lifelens-ue/actions/runs/38254936779 PASS: launcher/index/hashed JS/WASM 200.
- 실제 최신 제품 실행: https://sjlim91.github.io/lifelens-ue/index.html?v=5203449 .
- 이 항목은 위의 DRAFT/검증 대기/병합 보류 기록을 대체한다. 구현 한계는 기존 보고서를 유지한다.
- 문서 상태 동기화는 제품 코드 변경이 아니며 새 WASM 빌드나 중복 preview 배포를 요구하지 않는다.

## 2026-10-12 DRAFT — 주민 자체 키프레임·지면/손발 IK 기반

- PR [#700](https://github.com/sjLim91/lifelens-ue/pull/700), `work/resident-procedural-motion-20261012`, main `bcff5d9` 기준. main 병합하지 않음.
- 제품 구현 HEAD `eaca7280fa3cace3933a0bdfc9db2a5ef5a50390`: 공통 관절 18개·자체 모션 33개·지면/제한적 IK·실제 물리 대상 guard·시각 LOD. Core/저장/AI 성공 판정 변경 없음.
- 로컬 신규 12/기존 실제 모션 15/외형 10/행동 문맥 27/runtime resilience 18/continuity 13 및 Core determinism/save-load/codec/context 완료 PASS.
- 실제 typecheck/Preflight/runtime resilience CI PASS. 첫 시각 회귀 실패는 음식 없는 기존 Eat fixture 기대값이며 실제 Food/carry fixture로 수정. 브라우저 WebGL CI 및 전체 최종-head checks 확인 전 DRAFT 유지.
- 분석/오류/성능/미완료 범위: `docs/RESIDENT_PROCEDURAL_MOTION_2026-10-12.md`, 복구: `tasks/RESIDENT_PROCEDURAL_MOTION_HANDOFF.md`.
- 다음: 최종-head CI 실패 수정·브라우저 artifact 검토. 임의 단일 메시 자동 스키닝/좌석·후진·측면 Core 계약/Android GPU·1000명 장기 검증은 완료가 아님.

## 2026-10-04 WAITING_CI — Web 고속 진행 캡처 검증 / 사망 기록 보완

- 기준 main: `7c2f4ecaaad6d64d71256f78f5b9512ffd4992f3` (#637). 이전 #637의 exact-head 10개 PR workflow 모두 PASS 및 실제 merge 확인.
- 별도 Web lane: `work/web-fast-forward-capture-integrity-20261004`; Core 교역/탐색/건강 순서 유지.
- 추가 분석: 상세 조회 unavailable/부분 누락 시 stale cache로 완료 요약 생성 가능. 화면용 living 주민 목록으로 요약을 만들어 deceased 이름/Death 기록 누락.
- 수정: 고속 진행 전후 strict refresh로 현재 인구와 일치하는 fresh full resident details + available 문명/시설 응답을 요구. 실패 시 기존 error 경로로 종료; 계산된 Core 시간을 되돌리거나 다시 계산하지 않음.
- 요약은 사망자를 포함한 Core 상세 기록을 사용하고 화면은 기존 ResidentContinuity 필터 유지. 평상시 detail cadence/지형 캐시 유지.
- 로컬: 기존 실패 재현 후 strict unavailable/partial/zero-living missing-record 검증, 실제 Death 요약/재알림 억제, cadence/terrain 회귀 PASS. authoritative fast-forward validator PASS.
- 다음: 후속 PR exact-head Preflight + Web Typecheck/build + Runtime Resilience 검증. 크론 추가 없음.

## 2026-10-04 CURRENT — R10 current-main reconciliation

- main: `49aaf6c2be943aeeb7b4552d7e2a787af00005a0` (#634).
- #633 sanitation snapshot ✅, #634 COG-2 ✅.
- #626/#635 premature founder split fix는 **merge 없이 close**. current main same-seed에서 원래 day100 active second-settlement blocker가 재현되지 않음.
- current main seed874: day100 settlements1/active1; day1000 living3, birth1, married1, pregnancy1.
- current main seed424 365-day control: day100 settlements1/active1/chunks28; day365 settlements1/active1/chunks99.
- #635 candidate는 seed424 day365 chunks141, seed874 day1000 living1로 current main보다 악화되어 폐기.
- 현재 P0: **inter-settlement trade mission completion/persistence**. current main seed874 day1000 departures8 / exchanges0 / returns0.
- 그 다음: exploration/chunk growth diagnosis -> current-main health/family closeout -> C6 close -> C7.
- canonical: `docs/CURRENT_EXECUTION_ORDER_2026-10-04.md`.

## 2026-10-04 CANONICAL CURRENT — C6 CLOSEOUT + COGNITIVE AGENT FOUNDATION

> 실제 GitHub `main` / PR / exact-head Actions가 최우선 truth다.
> 다음 실행 순서의 canonical short-form은 `docs/CURRENT_EXECUTION_ORDER_2026-10-04.md`.

- 기준 main: `74c372e6cc5506e57df46cbcf502cab2e630c623` (#633).
- 활성 제품 경로: **LifeLensCore -> WASM -> React/TypeScript/Three.js Web Observer**.
- C3/C4/C5 foundation은 닫혀 있고 C6는 기능 추가보다 **closeout 검증 단계**다.
- C6의 group migration / settlement decline / local knowledge divergence / production specialization / settlement relations는 각각 #592/#594/#595/#596으로 이미 main에 통합됐다.
- #623 이후 facility-chain runaway는 크게 완화됐다. seed 4242001의 1000일은 facilities 22 / chunks 4 / 약 1분35초로 개선됐다.
- #633 ✅ 복수 정착지 sanitation snapshot 계약 정합성 closeout 완료.
- 남은 C6 구조 blocker:
  1. #626 — migration pressure 없는 초기 2인 정착지 분할 억제.
  2. 실제 trade exchange 후 return mission이 survival preemption으로 유실될 수 있는 문제.
  3. 동일 두 seed 1000일에서 최종 population collapse / illness 우세 원인 재검증.
- Web 관찰성은 #611/#615/#618/#619/#621/#624/#631까지 통합되어 주민 행동, 환경 결과, 시설 변화, 생애주기, 생산물류, 정착지/이주압력/교역 관계를 Core truth에서 관찰할 수 있다.
- Cognitive Agent:
  - #628 COG-0 ✅ typed request/proposal + fail-closed Core validation.
  - #629 COG-1 ✅ free-local provider + JSON schema + bounded scheduler.
  - #634 COG-2 🟡 Core-owned read-only Personality/Emotion/Memory/Belief/Relationship context bridge. latest main@#633 재적용 완료, exact-head 재검증 중.
- C7 Population / Settlement Maturation은 아직 시작하지 않는다.
- 고정 실행 순서:
  `#634 -> #626 + split R10 -> trade return persistence -> full R10 -> health causal fix(if needed) -> C6 close -> C7`.
- Cognitive 행동 연결은 accepted cognition event persistence/replay가 먼저 준비된 뒤에만 허용한다.

## 2026-10-03 READY_FOR_REVIEW — Web environmental consequences

- PR: [#615](https://github.com/sjLim91/lifelens-ue/pull/615); branch `work/web-environment-consequences-20261003`. User requested PR only; **main merge/auto-merge forbidden**.
- Start/recovery/final remote main verification: `4f3715e8b0983e8109397ab9eeb2d18c55514470`; no later main changes at verification.
- Verified product HEAD: `3cac0a9f5f29011efff21ec88cc4ae585edeb0c3`; [environment CI](https://github.com/sjLim91/lifelens-ue/actions/runs/37123316493) PASS: typecheck/build, environmental 9, presentation 64, human-trace 11, weather 7, motion 15, primitive bedding/era; mobile/desktop WebGL 12 screenshots + combined rain/fire budget.
- Local runtime resilience 18 and continuity 13 PASS. No Core/save/AI/Needs/progression changes; #605/#611/#612 contracts retained.
- Added overlays: puddle ≤128 patches, scorch ≤64 instances, smoke ≤192 points; measured simultaneous maximum +3 draw calls. Repeated refresh keeps scene children/GPU geometry stable; shader/browser errors 0.
- Docs-only closeout follows verified product HEAD; use product validation evidence rather than requiring repeated heavy compilation for text changes. Existing PR-triggered checks may rerun automatically.
- Recovery: `tasks/ENVIRONMENT_CONSEQUENCE_HANDOFF.md`; PR body checkpoint: `tasks/ENVIRONMENT_CONSEQUENCE_PR_BODY.md`. Re-query actual main/PR/Actions before any follow-up. Device GPU FPS and optional shelter precipitation clipping remain outside this implementation.

## 2026-10-02 — Web 자연 자원 / 환경 통합 (PR #605 / HOLD)

- PR: https://github.com/sjLim91/lifelens-ue/pull/605. 사용자 요청대로 PR만 생성하며 main merge 금지.
- 최종 동기화 main: `705d917ba4ebce9585d9748cfe4e102a17ee81b2` (#606). Core-only 진전과 Web 수정 overlap 없음.
- Web 코드 검증 HEAD `d1dbdb0b7cf426b5eaf825ed8b252237d7ac6631`: Preflight / Web Typecheck / Runtime Resilience 모두 PASS, presentation 64/64, production build PASS.
- 이후 closeout/main 동기화에서 Web 제품 코드는 동일. 최신 branch CI는 PR 본문과 Actions에서 확인.
- 재개/원인/설계/파일/성능/실패 근거: `tasks/RESOURCE_ENVIRONMENT_HANDOFF.md`.

## 2026-10-02 CANONICAL CURRENT — CURRENT-MAIN LONG-RUN INTEGRATION GATE

> Actual GitHub `main` / PR / exact-head Actions remain the highest-priority truth.

- Baseline entering the gate: `b277e091bc39d071ea30ba0d57281251da811dba`.
- C3 technology framework, C4 health, C5 society/economy/institutions and the C6-A/B/C foundation are integrated.
- C6-A: local scarcity and travel burden can create migration pressure and bounded long-range frontier exploration.
- C6-B: distant settlements can own independent storage/fire/furnace infrastructure and are observed as separate spatial clusters.
- C6-C: inter-settlement barter requires a resident to physically travel, exchange only after arrival, then physically return; repeated evidence forms an observed trade route.
- #587 preserves sex-independent dating/marriage while natural conception uses female gestation + male genetic contribution.
- C6 remains foundation-integrated rather than fully closed; canonical remaining scope is group migration, abandonment/decline, local knowledge divergence, specialization and cooperation/conflict foundations.
- Immediate work is **not C7 feature expansion**. It is current-main 100/365/1000-day integration measurement.
- Active branch: `audit/current-main-longrun-20261002`.
- Do not merge stale #539/#540 runtime changes. Salvage measurement concepts only.
- Audit contract: `docs/SIMULATION_BALANCE_CONTRACT_V2.md`.
- Audit checklist: `tasks/CURRENT_MAIN_LONGRUN_AUDIT.md`.
- Next feature after a green/classified gate: C7 Population / Settlement Maturation.

## 2026-10-02 CANONICAL CURRENT — C5 SOCIETY / ECONOMY / INSTITUTIONS CLOSEOUT

> Actual GitHub `main` / PR / Actions remain the highest-priority truth.

- Completion chain: #578 C5-A + #579 C5-B + #580 C5-C/D closeout.
- C3 open-ended technology framework and C4 health foundation remain closed; this unit closes C5 and does not start C6.
- Roles are derived from lived skill/knowledge/use/care evidence. They are not fixed occupations.
- Runtime Civilization selection now consumes role + settlement demand + institution membership + household resource policy through the actual UnifiedUtility path.
- Resident coordination covers provision/material supply, tool production, cultivation, metallurgy, shared storage, education, care and inquiry with generalist fallback.
- Personal reserve and household reserve are protected before tradable/shared surplus; only actual Store completion can create shared-contribution history.
- Reciprocal barter, trade partnerships, apprenticeships and institution memberships remain persistent SocialKnowledge evidence.
- Proto-recordkeeping can consume real Clay/Wood to create RecordTablet media and durable-record facts; no free Writing Technology or era unlock is introduced.
- Web observes coordination/resource-disposition/record-media facts from Core; it does not assign jobs or invent economic outcomes.
- Closeout gate: targeted C5 regressions + exact-head Core Tests/determinism + Preflight + Web WASM + Web Typecheck + Runtime Resilience.
- The 100-day long-run audit remains intentionally excluded.
- Next roadmap milestone after merge: C6 — Migration / Multiple Settlements / Trade Networks. Do not begin without a new user start signal.

## 2026-10-01 CANONICAL CURRENT — C4 HEALTH / DISEASE / POPULATION RESILIENCE

> Actual GitHub `main` / PR / Actions remain the highest-priority truth.

- Completion unit: #577.
- C3 open-ended civilization framework is already closed; this unit closes C4 without starting C5.
- Resident health authority now persists pathogen load, illness, immunity, injury, environmental stress, care knowledge, and infection/recovery/accident history.
- Real environmental residue and direct natural-water contamination drive exposure; sanitation knowledge reduces that pressure.
- Daily Core health progression handles illness, recovery, resilience, climate stress, regional accidents and cause-specific mortality deterministically.
- Health consequences feed Needs and movement; death still uses the existing authoritative lifecycle/relationship/household cleanup.
- Web receives only Core-authored health facts through runtime/full resident DTOs and world-health summary.
- Snapshot health is an optional appended extension so pre-C4 snapshots remain readable with default health state.
- Targeted C4 regressions cover clean baseline, contamination, sanitation mitigation, recovery/resilience, environmental stress, deterministic accidents and snapshot roundtrip.
- Closeout gate: exact-head Core Tests + deterministic harness + Preflight + Web WASM + Web Typecheck + Runtime Resilience, followed by merged-main runtime/pages verification.
- 100-day long-run audit is intentionally excluded from this C4 closeout.
- Next development milestone after C4 closeout: C5 — Education / Recording / Specialization / Economy / Institutions.

## 2026-09-30 LATE CANONICAL CURRENT — RESIDENT AUTONOMY #536 INTEGRATED

> This section supersedes the earlier 2026-09-30 DU-01 checkpoint below. Actual GitHub main/PR/Actions remain the highest-priority truth.

- Current validated main: `df839012177bc8f4a0bfd59e30cbf8e5d29d4129` (#536).
- D-026 keeps development choices resident-specific through disposition and lived technique experience.
- D-027 / #534 makes non-critical Eat timing resident-specific and freshness-aware while urgent/critical Hunger retains survival priority.
- D-028 / #536 connects the same real PlantFood to shared allocation opportunity cost: a fresh final local unit can be retained for eating or committed as cultivation seed depending on resident state/disposition/experience; a nearby real Storage reserve removes false last-unit scarcity.
- Store remains the reversible third path and only moves real surplus. No Eat/Plant/Store path fabricates inventory or relies on presentation-side authority.
- Regression now proves identical Hunger + PlantFood + cultivation knowledge/tool/plot can split into grower Plant versus impulsive resident Eat-oriented retention, and that real stored reserve changes the same resident's feasible choice.
- #536 exact-head and merged-main gates all passed: Core Tests + deterministic harness, Preflight, Web WASM, Runtime Release, Pages Preview and External Preview Probe.
- Next development unit: authoritative decision rationale observation. Persist/expose factual selected-decision inputs so Web can show why resident choices differ without browser-side motive inference.
- GitHub commit/PR text for this project remains Korean.

## 2026-09-30 CANONICAL CURRENT — DU-01 Living Settlement ACTIVE

> This section supersedes older execution snapshots below. Actual GitHub main/PR/Actions remain the highest-priority truth.

- Baseline main: `3135e7bb8a45dc4d3ecc785d89273e74b68f4015` (#521).
- Active branch: `feature/du01-living-settlement-20260930`.
- Active development unit: `docs/DU01_LIVING_SETTLEMENT.md`.
- #521 already connects shared local storage stock to real construction/restoration logistics.
- Current objective: make the entire lived settlement loop observable and non-wasteful — reuse usable facilities, restore rational candidates, build only for unmet local demand, and expose pressure -> logistics -> work -> result in the Web Observer.
- Work lanes: A Core/Simulation, B Web/Observation, C QA/contracts. Non-overlapping files may progress in parallel; one integrated PR owns the unit.
- Validation policy: targeted regressions during development; one full Core + deterministic harness + WASM + Web typecheck/build + Preflight gate before merge; then Runtime Release + Pages + External Preview Probe.
- Link handoff occurs only after merged main is proven on GitHub Pages and `RUNTIME_COMMIT.txt` matches the merge SHA.

# LifeLens Canonical Work State

## 2026-09-29 CANONICAL CURRENT — Unreal retired from active main

> This section supersedes older platform/current snapshots below. Older sections are historical evidence only.

- Active product path: **LifeLensCore -> WASM -> React/TypeScript/Three.js Web Observer**.
- Final pre-removal Unreal state: `archive/unreal-final-20260929` at `0ee1e161be22ede0a9f7ef854a1ca34597b3c6a6`.
- PR #497 closed without merge because it only repaired the retired Unreal external-sleep execution path.
- Active `main` no longer carries `.uproject`, Unreal client source, Config/Content, UE compile/APK pipelines, or UE-only validation tooling.
- Current P0 living-causality baseline through #496 remains intact.
- Required development gates are Core Tests + deterministic harness + Web WASM + Web typecheck/build + Web/Core Preflight.
- GitHub Pages is the canonical checkable preview.
- Future Unreal/native reintroduction is allowed only as an optional client around then-current LifeLensCore contracts.


## 2026-09-28 CANONICAL CURRENT — local-demand settlement growth + cultivation foundation (#482/#483)

> This section supersedes older "current" Web/Stage-C snapshots below. Actual GitHub main/Actions remain the highest-priority truth.

- Current main: `77b4dceb39eade6111e153682f235025ad2cfb35` (#483).
- Published GitHub Pages source: `preview: 77b4dceb...`.
- #482 — settlement foundation capacity now scales from **local resident demand** instead of one facility satisfying the whole world forever. SleepingPlace/Shelter/WorkSurface projects remain physical, material-backed and activity-anchored.
- #483 — authoritative cultivation foundation:
  - discoverable `Cultivation` knowledge; no era timer or free unlock.
  - `CultivatedPlot` is an authoritative Core facility with physical construction requirements.
  - planting consumes PlantFood seed stock; watering consumes carried Water; tending requires resident labor/tool context.
  - growth depends on authoritative local ecology/climate: fertility proxy, moisture/water influence, temperature and season.
  - harvest creates PlantFood only after real growth reaches maturity.
  - crop runtime persists in civilization snapshot v6 and is exposed through Core/Web/Unreal read models.
  - Web 3D facility presentation shows prepared plot -> crop growth -> harvest-ready state from Core crop facts.
  - Korean Web/Unreal labels are registered for cultivation and plot actions.
- #483 exact-head gates: Preflight, Web Typecheck, Web Runtime Resilience, Web WASM, Core Tests and Unreal Linux Compile — PASS.
- Main Web Runtime Release / Web Preview / Pages publish — PASS. External Preview Probe remains an independent known failure and is not the Pages publish authority.
- **C-S3 remains ACTIVE, not complete.** Water handling/storage and food spoilage existed already; #483 completes the cultivation/season-moisture-fertility foundation. Remaining C-S3 implementation is local scarcity -> explicit exploration/search/movement pressure and its long-run validation.
- Immediate implementation target: **C-S3 scarcity-driven resource search**, then strengthen fast-forward development observability from authoritative food/water/crop/reserve deltas.


## 2026-09-27 CANONICAL CURRENT — Web observation chain + social cues integrated (#459–#466)

> This section overrides older dated Web-current sections below. Actual GitHub `main`, PRs and Actions remain the highest-priority truth.

- Current Web product baseline: `996c7bc059ab839f15dc69d3ee803c3974deed4a` (#466).
- Published GitHub Pages preview: https://sjlim91.github.io/lifelens-ue/?v=996c7bc0
- AppDeploy is not part of the LifeLens Web workflow. GitHub source + GitHub Pages are the canonical Web path.
- Recent authoritative Web chain:
  - #459 — Core downstream hydrology projected to Web; browser-side flow guessing removed.
  - #460 — factual observation feed from consecutive authoritative Core snapshots.
  - #462 — authoritative resident action context/phase/targets exposed in-world.
  - #463 — authoritative Human Trace read model for resource use, residue and started facility work.
  - #464 — conservative semantic motion mapping using already-pinned Quaternius UAL1 clips.
  - #465 — pinned Quaternius UAL2 added as fail-soft secondary Web motion library; authoritative Eat/Drink, PlantFood harvest and facility material-carry mappings added.
  - #466 — bounded in-world Social / KnowledgeTeaching / Parenting connectors from authoritative active interaction targets.
- #466 baseline validation:
  - LifeLens Preflight — PASS.
  - Web Typecheck — PASS.
  - LifeLens Web Runtime Release — PASS.
  - LifeLens Web WASM — PASS.
  - LifeLens Core Tests — PASS.
  - LifeLens Unreal Linux Compile — PASS.
  - LifeLens Web Preview — PASS.
  - External Preview Probe still fails independently; GitHub Pages publish itself succeeds.
- Visual acceptance rule remains strict: green CI is not mobile/device visual acceptance.
- Current observer milestone sequence:
  1. Event Visibility — DONE (#460).
  2. Action Context — DONE (#462).
  3. Human Trace v1 — DONE (#463).
  4. Semantic Motion v1/v2 — DONE (#464/#465).
  5. Motion tranche 2 — NEXT: only reviewed Sleep/Dig/Chop/Ground-Gather/Carry variants for which Core exposes enough authority.
  6. Social Cue v1 — DONE (#466).
  7. Observer Director — NEXT.
  8. Motion tranche 2 — continue in parallel only for reviewed/authoritative clips; do not block Observer Director on speculative asset work.
  9. Deeper Human Trace / lived paths / activity centers — requires authoritative accumulated Core facts; never browser-fabricated.
- Motion safety:
  - exact behavior/target facts come from Core.
  - unsupported actions fail closed to Idle/Walk.
  - no Sword_Attack-as-chopping, no PickUp_Table-as-ground-gathering.
  - Sleep stays neutral until an authored/reviewed lie-down -> sleep loop -> wake sequence exists.
  - TreeChopping_Loop exists in UAL2 but stays unbound until Core exposes authoritative source/tool semantics; `Wood` alone is not enough.
- Open PR hygiene:
  - #423 and #461 are superseded in substance by the current #462/#464/#465 path; do not merge them wholesale onto current main.
  - older World-v2 / spawn-lane PRs must be re-evaluated against current main before any reuse.
- Motion handoff: `tasks/WEB_MOTION_HANDOFF_2026-09-27.md`.

Last reconciled: **2026-09-27 KST / main 996c7bc0.**


## 2026-09-26 CURRENT — Web camera / water repair integrated (#458)

- Current Web product baseline: `369f813047a333289a4425d103b92f76304ab9aa`; preserves #452–457 environment/outfit/water/snow changes.
- Terrain-grounded camera focus/eye clearance, streamed-origin pan continuity, water front faces, river-mouth level and WorldSeed cache invalidation repaired.
- 60 web regression checks, typecheck/build and all four exact-head GitHub checks passed.
- Web Preview run 36247613583 and Pages deployment 36247646656 succeeded; published branch `eeac7553c45019a1b1b6545d09164576616a1753` references the product merge.
- Published URL: https://sjlim91.github.io/lifelens-ue/?v=369f813
- Real-device visual acceptance remains OPEN. The cloud browser loads the published app but cannot create a WebGL context; CPU geometry/projection tests do not replace device review.
- Open #423 resident context and #425 runtime provenance lanes remain separate. Scope released; recovery: `tasks/WEB_CAMERA_WATER_REPAIR_2026-09-26.md`.
- Older dated lane snapshots below are historical; actual GitHub remains authoritative.

> Actual GitHub `main` / PR / Actions is the highest-priority truth.
>
> Whole-source audit / current P0 fixes: `docs/SOURCE_AUDIT_2026-09-18.md`.
>
> Canonical roadmap: `docs/DEVELOPMENT_MILESTONES.md`.
>
> Earth / human expansion architecture: `docs/EARTH_AND_HUMAN_FOUNDATION.md`.
>
> Long-range civilization direction: `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.
>
> Ownership / locks / IR: `tasks/TEAM_BOARD.md`.
>
> Dagyeom presentation assist handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-18.md`.
>
> **Current presentation freeze / Dagyeom handoff:** `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.
>
> Current late-day Presentation/Observer work state: `docs/PRESENTATION_WORK_STATE_2026-09-18.md`.
>
> Historical audit: `docs/INTEGRATED_AUDIT_2026-09-17.md` (point-in-time only; do not treat as live status).

Last reconciled: **2026-09-26 UTC for the Web repair above; other lane snapshots retain their original dates.**


## 2026-09-21 CURRENT — World Architecture v2 IMPLEMENTATION ACTIVE / W2-2 + W2-3

Canonical design: `docs/WORLD_ARCHITECTURE_v2.md`.

User-approved direction:
- World-first: characters appear on an already-existing Earth surface; the spawn point is not a settlement/living-zone authority.
- continuous Earth: camera scrolling must keep revealing deterministic terrain/water/biome instead of a decorated island on a flat plate.
- settlement emerges from repeated human activity and facilities; natural terrain/vegetation is not cleared around the initial spawn just because it is the initial spawn.
- terrain, hydrology, biome, resources and persistent human deltas share one world-coordinate truth.
- SimulationInterest and ObserverInterest are separate; observer preview must not invent simulation truth.
- Local -> Regional -> Planetary -> Orbital -> Interplanetary remain one world identity at different representation levels.

**Execution state:** user start signal received on 2026-09-21.
- W2-0 provider-contract foundation merged as PR #374 / main `f311e90e2fd63f6e5e78b996c8d30fca1cc0cc3d`.
- W2-1 deterministic continuous terrain Core field merged as PR #375 / main `1336c23453b2707b40823d223aeb76d198981893`.
- W2-2 continuous-terrain drainage graph hydrology merged as PR #378 / main `5c5f18286efa89036a6d622d11a96e9c61524ac4`.
- W2-3 observer-centered streaming interest is active as PR #379.
- current main already has authoritative `GetMaterializedNaturalChunkObservations()` over `generatedNaturalChunks`; the historical count/ring concern is not being redone.
- current new work: continuous-terrain hydrology graph + observer-centered read-only streaming/presentation.
- Character appearance/motion/context implementation remains deferred until World v2 visual/physical foundation.
- implementation ownership is now integrated: after provider contracts stabilize, Jjun may continue directly into WorldPresentation/Environment/Character/UI as the milestone requires; Dagyeom remains visual QA/targeted-polish collaboration.

The previous environment source closeout is reclassified as **structural/source closeout only, not runtime visual acceptance** because the latest runtime screenshots still show flat local terrain, missing forest canopy/water continuity and visible gray/white proxy artifacts.


## 2026-09-21 CURRENT — Whole-source audit 후반부 / 지구·인간 완성도 검증

Canonical checkpoint:
- `docs/PROJECT_STATUS_2026-09-21.md`.
- **whole-source audit resume/dedup ledger: `tasks/AUDIT_LEDGER_2026-09-21.md`.**
- zero-cost external asset policy: `docs/ZERO_COST_ASSET_POLICY_v1.md`.

Current main checkpoint before this docs sync:
- terrain relief v2 and opening settlement ecology clearing v2 are landed on main.
- #364 coast nature/resource visibility recovery — MERGED.
- #360 high-speed route budget — MERGED.
- #361 dependent care after parent loss — MERGED after Core Tests / Preflight / Unreal Linux Compile PASS.
- #362 visible-water fallback — MERGED after Preflight / Unreal Linux Compile PASS and post-#361 mergeability refresh.
- #370 CC0 canopy import — MERGED.
- #372 desktop canopy runtime tier — MERGED after Preflight #1360 / Unreal Linux Compile #620 PASS.
- #373 water fallback terrain clearance — MERGED after Preflight #1355 / Unreal Linux Compile #619 PASS.
- mature-canopy provenance is recorded; Android cook exclusion and desktop-only runtime tier are guarded.

Current audit lane:
- Earth / terrain / hydrology / vegetation / ecology runtime visibility.
- source presence alone is not completion; actual runtime visibility and context correctness are required.
- zero-cost external production assets may be integrated under `docs/ZERO_COST_ASSET_POLICY_v1.md`.

Standing execution rule:
- compile/CI failure -> inspect root cause, fix, rerun.
- exact-head required CI green + latest-main mergeability/overlap check -> merge using expected head SHA.
- do not stop the whole-source audit after a successful merge.

Next audit order:
1. **notify the user before entering character audit — mandatory boundary reached.**
2. character appearance / motion / context correctness.
3. lifecycle / family / society continuity.
4. Observer UI / camera / scale-transition **consumer wiring** audit — do not re-check already confirmed scale enums/contracts.
5. Android-first packaging, Windows/macOS native QA, Save/Load, long-run/performance/crash gates.

Environment/graphics runtime-only carryovers:
- authored Rain/Snow Niagara is not yet present; packaged primitive precipitation remains a safety fallback and requires later visual QA/authored-VFX work.
- sparse high-poly hero canopy requires Windows/macOS native performance profiling before final visual acceptance.

Visual completion rule:
> source presence or green compile alone is not visual acceptance. The runtime must visibly show the intended terrain, water, vegetation, characters and interaction context.

External asset rule:
> zero-cost assets may be downloaded and integrated when they improve production quality, but every new external asset must have verified commercial/package-compatible licensing and recorded provenance. Ambiguous, NC, personal-use-only, editorial-only and ripped assets are prohibited.


## 2026-09-19 CURRENT — Presentation frozen, Core lane resumed

- user froze further Jjun-side visual polishing and handed Presentation back to Dagyeom.
- current visual baseline is main after #282: `c58c63739c13cda24cd9994e26ff6ac5b1d43e76`.
- historical note: #281 / #283 / #284 were later merged on 2026-09-19; this section is no longer live execution state.
- visual CI success is not runtime visual acceptance.
- Jjun active implementation lane is **C1-D Durable Subsistence**:
  - authoritative water carrying/storage.
  - food storage/spoilage.
  - cultivation/agriculture.
  - renewable food production under real time/land/input constraints.
  - season/moisture/fertility effects.
  - scarcity pressure hooks.
- Android Gate B remains paused until explicitly resumed.

Canonical handoff: `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.


---

## 1. Current main checkpoint

Current main before this docs-only closeout sync:

- main SHA: `baa986b08d504765d8baa05ee8886b415b1eacd4`.
- #145 Character Context Motion v2 — MERGED.
- #149 Visual Catch-up v3 packaged rain/snow fallback — MERGED.
- #150 Lifecycle Presentation v2 — MERGED.
- #151 Observer Adaptive Information Density v1 — MERGED.
- #152 C1 Settlement Facility Authority Foundation v1 — MERGED.
- #153 docs reconciliation — MERGED.
- #154 explicit Dagyeom presentation assist handoff — MERGED.
- #156 AUDIT-0A Observer chrome single authority — MERGED.
- #157 AUDIT-0B resident-local environmental pressure — MERGED.
- #158 AUDIT-0C whole regression guards — MERGED.

Recent validated functional baseline:
- Core Tests #721 — PASS.
- deterministic harness — PASS.
- Preflight #789 — PASS.
- Unreal Linux Compile #241 — PASS.
- whole-source audit invariants — PASS.

Authority rule remains:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation consumes authoritative contracts and must not invent outcomes.

---

## 2. Whole-source audit P0 — COMPLETE

### AUDIT-0A — DONE (#156)
- duplicate Canvas runtime chrome removed.
- production HUD hierarchy no longer owns a second time/weather/speed surface.
- one UI speed mutation path remains.

### AUDIT-0B — DONE (#157)
- environmental Need pressure uses resident authoritative runtime GridPos.
- GridPos -> chunk -> local DynamicEnvironment.
- regression covers residents in different climate chunks.

### AUDIT-0C — DONE (#158, automated source gate)
- Preflight #789 — PASS.
- Core Tests #721 — PASS.
- deterministic harness — PASS.
- Unreal Linux Compile #241 — PASS.
- regression guard prevents the two audit bugs from silently returning.
- snapshot restore + local-climate deterministic continuation regression added.

Runtime-only PIE/device smoke remains a QA item because CI does not execute an interactive Unreal viewport/device session.

**Stage C / C-S1 is no longer blocked by the source-audit gate.**

---

## 3. P1 audit follow-up

### AUDIT-1A — Explicit materialized chunk enumeration

Current WorldPresentation and obstacle collision proxy infer materialized chunk coordinates from:
- `MaterializedChunkCount`.
- a ring around the initial chunk.

This can miss distant/non-contiguous materialized chunks later.

Required before migration/multi-settlement:
- Core bridge exposes authoritative materialized chunk coordinate list.
- WorldPresentation consumes the list.
- obstacle collision proxy consumes the list.
- no count-based coordinate guessing.

This is not ahead of P0 and does not replace C1-B after stabilization.

---

## 4. Recent completed chain

### Time / environment
- #135 Simulation Time Authority & Variable Speed.
- #136 Calendar + Day/Night.
- #137 Seasons + Dynamic Weather Core.
- #138 Environmental Consequences.
- #139 Dynamic Environment Presentation Foundation.
- #140 Visual Catch-up v2.
- #142 Runtime Chrome.
- #147 Observer time/weather/speed controls.
- #148 Dynamic Observer Canopy Visibility.
- #149 packaged rain/snow fallback.

### Character / Observer / lifecycle
- #143 Character Context Motion v2a.
- #144 Detail Scrolling v1.
- #145 Character Context Motion v2.
- #146 lifecycle live event feed.
- #150 selected-resident persistent family/lifecycle card + observed timeline.
- #151 adaptive information density.

### C1 settlement
- #152 C1-A authority:
  - WorkSurface constructible.
  - SleepingPlace constructible.
  - Shelter constructible.
  - real materials + real work.
  - deterministic sites.
  - weather friction on construction work.
  - snapshot persistence.
  - no free NEW GAME settlement facility.

---

## 5. Stage C — Settlement, Survival & Early Civilization — ACTIVE

Legacy mapping is preserved so no planned work is lost:

### C-S0 — Facility authority — DONE (#152)
- WorkSurface / SleepingPlace / Shelter.
- deterministic sites.
- real materials + real work.
- environment-sensitive construction.
- persistence.
- no free New Game facilities.

### C-S1 — Autonomous settlement need recognition — DONE (#161)
1. SleepingPlace utility from sleep/outdoor-rest pressure.
2. Shelter utility from resident-local environmental exposure.
3. WorkSurface utility from repeated craft/build demand.
4. missing project materials -> Gather demand.
5. Plan / DeliverMaterial / Work -> authoritative spatial ContextAction.
6. Presentation only executes/visualizes Core directive.
7. same-seed / snapshot continuation deterministic.

### C-S2 — Facility effects / maintenance — DONE (#162)
- sleep benefit.
- shelter environmental protection.
- work/craft benefit.
- durability / maintenance.
- ruined/inactive facilities stop providing benefit.
- repairs require actual material/labor.

### C-S3 — Durable subsistence — ACTIVE
- water handling/storage — DONE foundation.
- food storage/spoilage — DONE foundation.
- cultivation/renewable production — DONE foundation (#483).
- season/moisture/fertility dependency — DONE cultivation foundation (#483).
- local scarcity -> exploration/search/movement pressure — **NEXT / remaining**.

### C-S4 — Emergent settlement geometry
- activity centers emerge from real use.
- household space differentiation.
- sanitation away from dense living.
- storage/fire/work/sleep clusters emerge.
- no hard-coded town-center authority.

### C-S5 — Early material expansion
- TinOre.
- Bronze.
- bronze tools/processes only after actual physical/knowledge/facility prerequisites.
- no automatic Bronze Age switch.

---

## 6. Macro roadmap after Stage C

### Stage D — Long-Run Simulation & Civilization Engine
- long-run deterministic multi-generation simulation.
- History/Fast-forward and event-aware slowdown.
- CPU/memory/snapshot/per-frame work budgets.
- inactive/distant simulation strategy.
- Knowledge -> Capability -> Technology -> CivilizationTransformation.
- discovery -> experiment -> reproducibility -> diffusion -> adoption.
- loss, collapse and rediscovery.

### Stage E — Human Society, Health, Education, Economy & Migration
- health/disease/contamination/immunity/recovery.
- education/apprenticeship/recording/writing.
- roles/jobs/specialization.
- production, exchange, ownership/shared-resource policy.
- institutions/research/education organizations.
- exploration/migration.
- multiple settlements.
- trade routes/resource specialization.
- cooperation/conflict foundations.

### Stage F — Historical Civilization -> Open Future
- advanced metallurgy / urban / science.
- mechanical / industrial.
- electrical / chemical / modern infrastructure.
- digital / network.
- AI / robotics / automation.
- advanced energy / materials / biotechnology.
- planetary / space / interplanetary civilization.
- open future / unknown civilization.

Stage labels organize execution only. They are **not Core era gates**.

---

## 7. Runtime QA not covered by green compile/tests

Still requires runtime/device validation:
- actual HUD z-order/layout/safe area.
- touch gesture conflicts.
- camera feel.
- animation transitions / hand-tool alignment.
- Niagara/material visual quality.
- Android GPU/performance.
- long Unreal session memory/performance.
- APK packaging/device launch.

Android Gate B remains PAUSED BY USER.

---

## 8. Collaboration / merge rule

- Review source as one LifeLens product; ownership is for coordination, not quality silos.
- Correct responsibility beats artificial file separation.
- Same responsibility -> same canonical file.
- coordinate/rebase actual overlaps.
- Dagyeom #98 was selectively ported by #202 onto current main and the original #98 is closed as superseded.

Standing Jjun merge rule remains:
- refresh exact head.
- required CI green.
- refresh active PRs/files.
- no unresolved overlap/conflict.
- expected_head_sha on merge.

---

## 9. Immediate next implementation target

> **C-S3 local scarcity -> exploration/search/movement pressure.**

The older Earth/hydrology foundation sequence below this file is historical: those provider/hydrology prerequisites are already integrated on current main.

Immediate sequence:
1. measure provision scarcity from authoritative resident-local inventory/storage/resource availability.
2. distinguish nearby reachable supply from absent/depleted local supply.
3. turn scarcity into explicit resource-search/exploration utility rather than global first-node selection.
4. keep movement/pathfinding authoritative and physical; no teleport and no browser-generated search target.
5. validate deterministic same-seed / snapshot continuation and long fast-forward behavior.
6. expose only authoritative search/provision outcomes in Web fast-forward/world-activity summaries.


---

## 10. Late 2026-09-18 Presentation / Observer sprint — HISTORICAL

The earlier sections preserve Stage C / audit context. This section is historical; current Presentation execution is frozen on the Jjun side and handed to Dagyeom per the 2026-09-19 handoff.

Canonical detailed snapshot:
- `docs/PRESENTATION_WORK_STATE_2026-09-18.md`.

Merged since the earlier work-state snapshot:
- #159~#215, including environment, character appearance/motion, facility readability, observer camera/HUD, lifecycle/social/family navigation and Dagyeom #98 integration via #202.

Latest merged wave:
- #216 Detail resident navigation v1.
- #217 Sleep site posture v7.
- #218 Resource depletion visual v2.
- #219 Selected social counterpart v7.
- #220 Snow cover accumulation v6.
- #221 Detailed daypart chrome v5.
- #222 Resident identity badge v7.
- #223 Offscreen action cue v4.
- #224 World event focus return v2.

Each merged only after its exact-head Preflight + Unreal Linux Compile succeeded.
#225 docs reconciliation is merged. #226/#227 Local Surface environment fixes are merged.

The previous practical four-PR batch cap is removed. Independent canonical files may now be developed/compiled in larger parallel batches while same-file responsibilities are sequenced or combined.


---

## 11. Earth & Human Foundation — ACTIVE

Canonical design:
- `docs/EARTH_AND_HUMAN_FOUNDATION.md`.

Preserved:
- current Core authority.
- deterministic WorldSeed/chunks.
- residents, Needs, emotions, relationships, family/lifecycle.
- facilities, civilization, weather/time.
- Save/Load and Observer contracts.

Structural expansion:
- `Planet -> Surface Region -> Chunk -> Local Surface`.
- Observer scale: `Local -> Regional -> Planetary -> Orbital -> Interplanetary`.
- deterministic Simulation LOD for distant regions/populations.
- terrain/hydrology before deeper settlement expansion.
- ecology and richer human body/culture/society layers.

Latest local-surface presentation:
- #226 Far world visual envelope — MERGED.
- #227 Horizon atmosphere blend — MERGED.

These fixes hide the bootstrap square/void at Local scale only; they are not the final planetary geometry.


---

## 12. Cinematic / photoreal graphics foundation — FROZEN / DAGYEOM OWNED

Merged foundation through the handoff baseline includes:
- #238 zero-cost Poly Haven photoreal nature pipeline v2.
- #239 Windows cinematic renderer tier v1.
- #240 Unreal-native environment stack v1.
- #241 photoreal primitive-facility asset wave v2.
- #242 Hydrology -> Unreal Water presentation contract v1.
- #243 deterministic PCG presentation seeds v1.
- #269 platform content/cook boundary.
- #271 desktop PCG runtime dressing.
- #276 Water terrain alignment.
- #277 completed primitive structure photoreal path.
- #278 desktop smooth terrain overlay.
- #279 remaining completed facility visual upgrade.
- #280 whole-world environment density.
- #282 screenshot-driven runtime visual sanity hotfix.

Current visual baseline:
- main `c58c63739c13cda24cd9994e26ff6ac5b1d43e76`.

Open handoff PRs:
- #281 Character animation polish.
- #283 Lighting / atmosphere polish.
- #284 Observer UI / camera polish.

These are Dagyeom review/decision items. Jjun does not auto-merge them.
Actual latest-main runtime screenshots, not compile success alone, decide visual acceptance.

Canonical handoff:
- `docs/DAGYEOM_PRESENTATION_HANDOFF_2026-09-19.md`.

Android Gate B remains paused until the user explicitly resumes device/APK validation.


---

## Platform content / cook boundary — FOUNDATION MERGED

- one repository / one simulation truth.
- Android and Desktop packages carry only required presentation payloads.
- #269 merged the Android/mobile vs desktop photoreal hard-reference and cook boundary. Future presentation payload changes are Dagyeom-owned unless a Core/provider contract is required.
- Android Gate B remains paused; no APK artifact-size claim until actual packaging resumes.

## 2026-09-21 DESIGN ADDENDUM — Earth + Human realism hooks

Canonical human companion: `docs/HUMAN_REALISM_FOUNDATION_v1.md`.

Added future-safe hooks:
- Perception / Knowledge / Belief confidence.
- Place Memory / Attachment.
- Habit / Routine / Preference.
- embodied health/need effects on motion and decisions.
- personal space / social distance / destination reservation.
- ownership / familiarity.
- sensory sound/smell affordances.
- ecological succession / regrowth / decay.
- fauna population LOD.
- regional weather cells.
- persistent historical traces.

Motion asset decision:
- actual behavior gap이 있으면 commercial/package-compatible 비용 0원 animation asset을 선별/다운로드/retarget해 사용 가능.
- provenance / license / skeleton / root-motion / retarget / semantic-action mapping 기록 필수.

Execution lock remains unchanged: **World v2 start signal 전 구현 없음.**

## 2026-09-21 OWNERSHIP UPDATE — integrated implementation

User direction:
- do not block World/Character/Presentation work behind a separate Dagyeom ownership lane.
- Jjun handles end-to-end implementation across Core, World, WorldPresentation, Character, UI and required Content as milestones demand.
- Dagyeom remains available for actual-screen review, visual QA and explicitly delegated polish.
- architectural authority boundaries remain unchanged: Core/World truth stays authoritative; Presentation/Character do not invent simulation outcomes.
- same-file active branch conflicts are still coordinated before edits.

Effect on World v2:
- provider contract -> Presentation handoff is no longer a human ownership gate.
- runtime screenshot acceptance remains mandatory.

## 2026-09-21 ACTIVE — Web/PWA first-class client foundation

User decision:
- retain Unreal Native as high-quality client.
- add Web/PWA as a first-class client sharing the same `LifeLensCore`.

Active branch:
- `jjun/web-client-foundation-v1`

WEB-0 scope:
- platform-neutral `WebClientBridge`.
- native contract test.
- Emscripten/Embind build target.
- PWA shell.
- actual Core world overview/resident/terrain/hydrology truth preview.
- explicit fail-closed state when WASM is unavailable.
- canonical architecture/docs/preflight.

This lane is parallel to active World v2 work and must not fork simulation truth.

## 2026-09-25 Work Mode — Resident continuity / DONE

- Previous Work PR #422 is merged (actual GitHub confirmed); its old integration-pending state is superseded.
- Branch: `work/web-resident-continuity-20260925`, base `9b895a2`.
- Cache identity/coordinate expiry and authoritative removal implemented.
- Local checks: 13 new + 17 existing regression cases, typecheck, production build PASS.
- User requested merge: #447 merged as `4ddf85b`; all four exact-head CI checks passed. Existing main-triggered preview publication is separate from merge completion.
- Recovery instructions: `tasks/WEB_CONTINUITY_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web rain correction integrated (#448)

- Rain distribution, projected streak direction, DPR sizing and explicit dry-state transitions merged as `e11119280ae122879c38a0e13d496124679b9064`.
- Seven weather + 30 existing regression checks, typecheck/build and all four GitHub checks pass.
- Follow-up: real-device visual QA (cloud browser cannot create WebGL). Preview publication tracked through existing GitHub Pages workflow.
- Checkpoint: `tasks/WEB_RAIN_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web character appearance repair integrated (#449)

- Appearance range/proportion, skin/garment coloring and head-attached hair repair merged as `9f5fa4653e27f304fe5f8851dfffed9bbde78e53`.
- All four GitHub checks pass, including 43 regression checks and typecheck/build. Actual GLB rig binding checked without GPU.
- Remaining: separate female base asset and real-device appearance/gait validation; #423 remains the action-context lane.
- Checkpoint: `tasks/WEB_CHARACTER_HANDOFF_2026-09-25.md`.

## 2026-09-25 — Web touch interaction correction integrated (#450)

- Merged `ae6d90fc4abd7c907d1aa9b41d07cbe4d4e126a8`: corrected pan/vertical orbit direction, tap slop and multi-touch release transitions; selected-life-first mobile sheet.
- Four exact-head GitHub checks and 50 regression cases pass. Subjective device touch/visual acceptance remains open.
- Checkpoint: `tasks/WEB_TOUCH_HANDOFF_2026-09-25.md`.

## 2026-09-26 — Scalp regression correction integrated (#451)

- Merged `d8fcd616a344833a0da13707c80be18d8c8638c4`: removed collar-like spherical hair proxies; actual skinned scalp coverage and darker eyes.
- All three final-head checks pass, including 49 regression cases. Device visual acceptance remains pending; CPU model close-up is diagnostic only.
- Checkpoint: `tasks/WEB_SCALP_HANDOFF_2026-09-25.md`.


