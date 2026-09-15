# LifeLens Project Progress Snapshot — 2026-09-15

> 목적: 실제 GitHub 상태를 기준으로 현재 구현 완료 범위, 병렬 작업, 다음 실행 순서, 남은 구조적 과제를 한 문서에서 확인한다.
>
> 이 문서는 제품 설계를 대체하지 않는다. 제품 방향은 각 canonical 문서를 따르고, 실제 최신 상태는 GitHub `main` / PR / Actions가 최우선 진실이다.

## 1. 기준점

- Product-code baseline: `157ce9937e53d5868d7e558b4149a4fa56c4c454`
- Latest merged product slice: PR #75 `[CORE] Add environmental exposure perception and avoidance v1`
- Documentation-only commits may advance `main` beyond this product-code SHA without changing runtime behavior.
- PR #75 validation at head `c36a31d0f0e1caa069f5389735b3828289d252e9`:
  - Structural Preflight: PASS
  - Core Tests: PASS, 41/41
  - deterministic harness: PASS through Core workflow
  - Unreal Linux Compile: PASS
  - actual UE 5.6 UHT / UBT / link step: PASS

## 2. Canonical design map

- Master product spec: `docs/LIFELENS_SPEC_v1.1.md`
- Civilization progression: `docs/CIVILIZATION_PROGRESSION_v1.md`
- Open-ended invention / emergent artifact: `docs/OPEN_ENDED_INVENTION_v1.md`
- World affordance / environmental consequence: `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`
- World visual environment: `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- Environmental consequence visual feedback: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`
- Character appearance direction: `docs/CHARACTER_APPEARANCE_ROADMAP.md`
- Character appearance data contract: `docs/CHARACTER_APPEARANCE_DATA_CONTRACT.md`
- Whole-project verification baseline: `docs/WHOLE_PROJECT_VERIFICATION_2026-09-14.md`
- Collaboration / state rules: `docs/STATE_MANAGEMENT.md`, `docs/INTEGRATION_SPRINT.md`, `tasks/WORK_STATE.md`, `tasks/TEAM_BOARD.md`

## 3. 구현 완료 축

### Core human / society baseline

구축된 기반:
- Needs
- Personality
- Emotion
- Memory
- Belief
- Relationship
- social cognition / witness / rumor
- autonomous social decisions
- Observer read models

### Family / generation baseline

구축된 기반:
- Romance
- Engagement / Marriage
- Household
- Pregnancy
- Birth
- Genetics baseline
- Child growth / life stage
- Aging
- Death
- Life history
- Genealogy / generation continuity

### Civilization baseline

구축된 기반:
- natural resource nodes
- finite resource quantity / regeneration hooks
- personal inventory
- gather / store actions
- experiment / discovery
- crafting baseline
- personal knowledge
- knowledge teaching / transmission
- civilization observer DTOs
- Save/Load persistence

중요 경계:
- 현재 runtime은 아직 완전한 open-ended artifact physics engine이 아니다.
- `OPEN_ENDED_INVENTION_v1.md`의 arbitrary component/material/connection 조합과 기능 검증은 후속 구현이다.
- 고정 Tech Tree로 회귀하지 않는다.

### Core ↔ Unreal physical execution

PR #70~#74로 완료한 주요 항목:
- dynamic resident reconciliation after Core population changes
- dynamic ActivityAnchor reconciliation baseline
- all LifeLens C++ paths trigger UE compile gate
- external physical execution ACK contract
- Core physical outcome waits for World arrival/use completion
- Core duration ticks projected to World
- actual acknowledged World position is written back to Core runtime position
- environmental consequence uses acknowledged position
- Core Grid ↔ Unreal World baseline contract: 100 uu per grid tile
- resident position restored from Core runtime state after load/spawn
- external physical execution mode reasserted after Core replacement

### World affordance / missing infrastructure

구축된 규칙:
- no implicit modern infrastructure on production NEW GAME
- `Preferred → Primitive → Natural → Emergency → Unavailable`
- missing bed/toilet/sink/etc. remains actually missing
- Eat / Drink emergency path never synthesizes provisions
- World presentation does not invent simulation truth

### Environmental residue

PR #68 완료:
- Core-owned `EnvironmentalResidueField`
- `HumanWaste`
- position / source / amount / intensity / radius / timestamps
- accumulation
- deterministic decay
- exposure query
- snapshot binary format v4 environmental extension
- legacy v1-v3 environment migration to empty state
- Unreal read-only environment observation

### Environmental exposure / perception / avoidance

PR #75 완료:
- contamination perception at authoritative Core planning boundaries
- hygiene burden from exposure
- immediate emotional discomfort baseline
- location-specific sanitation Memory
- short-window duplicate memory suppression
- avoidance score using current physical exposure + remembered contamination
- deterministic low-exposure outdoor sanitation recommendation
- Save/Load continuity of residue + memory + recommendation
- Bridge API `GetRecommendedOutdoorReliefGridPosition(...)`

현재 경계:
- Core recommendation은 준비됐지만 WorldDirector가 아직 그 recommendation을 실제 emergency-toilet 이동 목표로 소비하지 않는다.
- disease/pathogen/health model은 아직 연결하지 않는다.

## 4. Character / Presentation 병렬 상태

### Character Presentation

완료:
- PR #63 merged
- resident presentation layer
- selection ring / labels
- basic life-stage/sex presentation support

### Deterministic Appearance contract

완료:
- PR #65 merged
- stable resident identity → deterministic appearance projection
- 별도 appearance SaveGame authority 없음

### Character Appearance v1 — PR #67 OPEN

Owner: 다겸 / 다겸 AI

Latest head checked: `aae68dc54bfb4d373ffc2124dd51b143d6038942`

Latest head CI:
- Preflight: PASS
- UE Linux Compile: PASS

Reported/implemented:
- Quaternius CC0 humanoid body assets
- UAL animation assets
- deterministic #65 appearance mapping
- hair / skin / body variation baseline
- minimum Peasant outfit asset integration
- humanoid residents visible in PIE

Still required before DONE:
- final PIE outfit visual confirmation
- explicit same-resident appearance continuity check across restart/load
- review / merge / live-doc sync

Known presentation limitation:
- locomotion is not wired yet; Idle-looking slide remains until Motion Bootstrap.

## 5. 환경 그래픽 규칙 상태

`docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`에 다음 원칙이 canonical로 문서화되어 있다.

`Resident Action → authoritative environmental consequence → Save/Load state → Unreal visual expression → resident perception → changed decision`

반드시 지킬 규칙:
- 그래픽은 authority가 아니다.
- 공간적으로 의미 있는 environmental consequence는 visual representation path를 가진다.
- residue amount/intensity/radius/decay는 visual 강도/범위/소멸과 동기화된다.
- Save/Load 후 authoritative state에서 visual을 재구성한다.
- Android-first이므로 residue마다 무거운 Actor를 1:1 무한 생성하지 않는다.
- pooling / instancing / LOD / culling / clustering을 사용한다.

아직 미구현:
- HumanWaste ground visual/decal/material/VFX
- resource depletion/regrowth visuals
- fire/smoke/scorch feedback
- foot-traffic path formation
- weather-driven contamination visuals
- construction/damage stage visuals

## 6. Open-ended invention 상태

`docs/OPEN_ENDED_INVENTION_v1.md`가 canonical companion이다.

최종 방향:
- 현실 역사에 없던 물건도 허용
- 개발자가 exact recipe를 미리 만들지 않은 구조도 허용
- material + component + connection + geometry/physical property + use result로 기능 판단
- 실패 역시 Memory / Belief / material knowledge로 축적
- 성공한 artifact는 개인 경험 → technique → imitation/teaching → culture로 확산
- 전역 자동 unlock 금지
- 물리법칙을 위반하는 마법식 발명 금지

현재는 **설계 완료 / runtime general artifact engine 미구현** 상태다.

## 7. 즉시 다음 실행 순서

### Jjun lane — 가장 먼저

1. **WorldDirector sanitation avoidance integration**
   - `GetRecommendedOutdoorReliefGridPosition(...)` 소비
   - Core Grid → Unreal world target
   - resident가 실제 recommendation 위치로 이동
   - arrival/use 후 ACK
   - 같은 위치에 authoritative residue
   - 다음 sanitation 행동에서 dirty/memorized 위치 회피

2. **Sanitation Problem Recognition v1**
   - 반복 exposure / discomfort / memory를 문제 인식 입력으로 연결
   - 개인 경험과 관찰/전파를 구분

3. **Primitive Latrine Discovery / Affordance progression**
   - designated area / pit / covering / primitive latrine experiments
   - resource / knowledge / repeatable success requirement
   - 발견 후 실제 authoritative affordance 생성

4. **Environmental Visual Feedback — HumanWaste first**
   - authoritative residue → visible ground feedback
   - accumulation / decay / SaveLoad reconstruction

### Dagyeom lane

1. #67 closeout / merge
2. Motion Bootstrap — Idle / Walk / Jog + orientation
3. World Visual Environment v1
4. remaining Motion & Context — sit/lie/gaze/IK/interactions
5. Observer UX polish / mobile touch / later presentation polish

## 8. 남아 있는 구조적 과제

### A. Unified facility/resource authority — 아직 부분 해결

Core intent completion과 World ACK는 통합됐지만, 모든 실제 World facility/resource가 아직 하나의 exact shared Core target registry로 통합된 것은 아니다.

남은 것:
- facility identity / tier / quality / backing resource를 ACK contract에 더 명확히 연결
- World authored facility와 Core civilization-created facility의 동일 authority path
- target disappearance/path failure/re-resolve semantics 강화

### B. Birth physical position

현재 새 child runtime position은 기본 `{0,0}`에서 시작할 수 있다.

후속 원칙:
- gestational parent / household vicinity에서 authoritative child initial GridPos 결정
- presentation-only spawn ring이 위치 authority가 되지 않음

### C. Dormant legacy `ULLDecisionComponent`

현재 production WorldDirector가 사용하지 않더라도 Blueprint-callable competing chooser가 남아 있다.

후속:
- remove / deprecate / debug-only restrict

### D. Explicit binary snapshot legacy fixtures

Codec은 v1-v4 migration logic을 지원하지만 대표 v1/v2/v3 encoded fixture를 직접 decode하는 dedicated regression coverage는 아직 보완 필요.

### E. Environment → Health / pathogen

#75는 discomfort/hygiene/memory/avoidance까지만 담당한다.

후속:
- disease/pathogen risk
- contaminated water/soil
- cleanup / burial / sanitation effectiveness

### F. Death presentation policy

Core death는 존재하지만 Unreal actor/body/observer 표현 정책은 별도 확정 필요.

### G. Android product validation

아직 남음:
- cheap smoke package path
- Android Cook / Package
- APK artifact
- real device execution
- performance / thermal / memory profiling

장시간 전체 엔진 재빌드를 기본 반복 경로로 사용하지 않는다.

## 9. 구현 우선순위에서 혼동하면 안 되는 것

- #75가 merge됐다고 HumanWaste가 화면에 이미 보이는 것은 아니다.
- 환경 visual 문서가 있다고 visual implementation이 완료된 것은 아니다.
- open-ended invention 문서가 있다고 arbitrary new artifact runtime이 이미 가능한 것은 아니다.
- #67 compile PASS가 곧 merge/DONE을 의미하지 않는다.
- MetaHuman은 Android/mobile validation 이후 upgrade path다. 현재 Track B Quaternius가 baseline이다.

## 10. Documentation audit — 2026-09-15

이번 상태 점검에서 발견한 stale/missing state:
- `tasks/WORK_STATE.md`가 #69 시점에 멈춰 #70~#75 완료 상태를 반영하지 못하고 있었음.
- `tasks/WORK_STATE.md`에 UE compile trigger coverage가 `NEEDS FIX`로 남아 있었으나 #71에서 이미 완료됨.
- `tasks/TEAM_BOARD.md`가 Core ↔ World Execution Sync를 아직 READY_NOW로 표시하고 있었으나 #70~#74로 대부분 완료됨.
- 환경 인지/기억/회피가 follow-up으로만 남아 있었으나 #75에서 구현/검증/merge됨.
- open-ended invention과 environmental visual feedback가 개별 canonical 문서에는 존재하지만, live work-state 문서의 canonical reference map에 빠져 있었음.
- 남은 구조적 과제(통합 facility authority, birth initial position, legacy decision chooser, snapshot legacy fixtures, death presentation, Android validation)를 한 곳에서 확인할 수 있는 current snapshot이 없었음.

조치:
- 이 문서 생성.
- `tasks/WORK_STATE.md` current state로 재동기화.
- `tasks/TEAM_BOARD.md` current queue로 재동기화.
- canonical reference map에 `OPEN_ENDED_INVENTION_v1.md`와 `WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md` 포함.

설계 문서 안의 과거 단계 설명은 historical design context로 남을 수 있으나, **실제 현재 작업 상태 판단에는 이 문서 + WORK_STATE + TEAM_BOARD + GitHub actual state를 사용한다.**

## 11. 다음 통합 체크포인트

가까운 목표는 아래 causal loop를 실제 화면까지 닫는 것이다.

`Need`
→ `Emergency sanitation intent`
→ `Core low-contamination recommendation`
→ `Unreal movement to that exact location`
→ `physical completion ACK`
→ `HumanWaste residue`
→ `resident exposure / Memory`
→ `next location avoidance`
→ `visible environmental feedback`
→ `sanitation problem recognition`
→ `primitive solution discovery`
→ `new actual affordance`

이 루프가 닫히면 LifeLens의 Core AI, physical World, environment, Memory, civilization, presentation이 처음으로 하나의 완전한 인과 시스템으로 연결된다.
