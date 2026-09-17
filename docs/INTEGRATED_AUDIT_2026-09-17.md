# LifeLens Integrated Audit — 2026-09-17

이 문서는 2026-09-17 시점의 **Jjun + Dagyeom 통합 감사 결과**다.

목적은 세 가지다.
1. 이미 `main`에 끝난 작업을 다시 만들지 않는다.
2. 오래된 Dagyeom 브랜치/PR에서 아직 유효한 부분만 최신 `main`으로 옮긴다.
3. Jjun/Core와 Dagyeom/Presentation을 하나의 통합 로드맵에서 병렬 진행한다.

실제 GitHub `main` / PR / Actions가 항상 최우선 진실이다.

---

## 1. 감사 기준선

감사 기준 `main`:
- PR #125 `Lifecycle Presentation v1` 병합 SHA: `e6e005ba1180a4152476ab9b7a19ad9953c07287`

최근 완료된 correctness / integration chain:
- #115 Lifecycle Core Correctness v1
- #116 Action Completion Unification v1
- #117 Social Communication & Localization v1
- #118 PrimitiveStorage / facilities-tools-technology v1
- #119 Tool Effectiveness + held tool presentation
- #120 DiggingStick / StoneHammer
- #121 Fire / Heat / FirePit v1
- #122 Furnace / Copper Smelting v1
- #123 World / Facility / Obstacle Authority Normalization v1
- #124 Legacy Authority Removal v1
- #125 Lifecycle Presentation v1

현재 유지해야 하는 핵심 authority rule:

> Core / World owns simulation truth. UI / Character / Environment / WorldPresentation is a read-only presentation consumer and must not invent actions, resources, facilities, tools, technology, lifecycle state, or outcomes.

---

## 2. Dagyeom 작업 감사

### PR #100 — LIVE / VALID

Branch: `dagyeom/world-visual-readability-envelope`

판정:
- 현재 #125 `main` 위로 재베이스된 유효 작업이다.
- Core authority를 건드리지 않는 WorldPresentation-only 변경이다.
- 시작 지점 단일 clear radius 대신 정착지 가독성 envelope을 사용한다.
- 0~1200 UU 생활 핵심, 1200~3000 UU 활동 구역, 그 밖은 기존 자연 밀도로 이어진다.
- 기준점은 Core `InitialCenterGrid`를 따른다.
- ResourcePatch authority는 유지하고 presentation scale/thinning만 적용한다.

처리 원칙:
- latest exact head의 required CI가 통과하면 정상 리뷰/병합 대상으로 취급한다.
- PIE/기기 육안 QA는 별도 품질 확인이며 Core correctness blocker가 아니다.

### PR #98 — STALE / SELECTIVE SALVAGE ONLY

Branch: `dagyeom/observer-readability-and-qa-view`

판정:
- 현재 `main`보다 오래된 베이스에서 분기되어 **그대로 병합하지 않는다**.
- 유효 아이디어만 최신 main에 다시 구현한다.

아직 유효한 후보:
- Observer panel readability 개선.
- QA-only `ll.ViewResidents` / `ll.ViewReset` 계열 관찰 명령.

최신 main에서 별도로 해결해야 하는 더 큰 문제:
- Detail overflow는 진짜 scroll이 아니다.
- 읽기 밀도가 높은 Detail/Inspector panel은 generated world 배경에서 가독성 튜닝이 필요하다.

### 오래된 Dagyeom branches

`observer-ui-v2`, `mobile-touch-v1`, `character-motion-v1`, `character-appearance-v1`, `visual-feedback-v1` 등은 직접 merge 대상으로 취급하지 않는다.

원칙:
- 기능이 이미 main에 흡수됐으면 폐기한다.
- 오래된 branch의 구현을 통째로 가져오지 않는다.
- 실제 main에 빠진 기능만 현재 API/authority 계약에 맞게 재구현한다.

이미 main에 흡수된 대표 기능:
- PC/Android observer camera input.
- tap-vs-drag 분리.
- single-finger orbit.
- pinch zoom.
- two-finger pan.
- Quaternius appearance / locomotion bootstrap.
- social talking presentation.
- Core-driven resident detail data.

---

## 3. 현재 main에서 확인된 실제 미비사항

### A. Knowledge transmission physical authority — Jjun

현재 civilization knowledge witness/teaching은 실제 물리적 거리/만남을 요구하지 않는다.

문제:
- discovery/craft event 후 모든 살아있는 주민이 witness 후보가 될 수 있다.
- hourly teaching도 teacher/learner의 실제 만남 없이 선택될 수 있다.
- 여러 정착지가 생기면 원격 기술 전파가 발생할 수 있다.

필요한 방향:
- witness는 실제 spatial proximity / encounter 조건을 요구한다.
- teaching은 Social/Context interaction처럼 실제 접근/만남을 필요로 한다.
- 지식 결과는 Core ACK 이후에만 확정한다.

### B. Character Context Motion v2 — Dagyeom

현재 Core Context Action과 WorldDirector ACK 체인은 연결돼 있지만 motion 표현은 아직 v1 fallback이다.

현재 presentation mode 중심:
- None
- Interact
- Gather
- Build

현재 Gather 등 일부 행동은 generic `Interact` fallback을 사용한다.

이미 repo에 존재하는 활용 가능한 animation 예:
- `Sitting_Enter`
- `Sitting_Idle_Loop`
- `Sitting_Exit`
- `PickUp_Table`
- `Fixing_Kneeling`
- `Interact`
- `Idle_Talking_Loop`
- `Death01`

다음 표현 세분화 후보:
- Sit / Stand / Lie / Wake
- PickUp / Carry / Use
- Gather / Cut / Chop / Dig / Strike
- Craft / Build / Fire / Smelt
- Parenting care
- sanitation interaction

규칙:
- Character Presentation은 Core directive와 실제 target을 소비한다.
- animation이 결과를 만들거나 ACK를 우회하지 않는다.

### C. Observer readability + real scrolling — Dagyeom

현재 main의 Observer detail data 자체는 풍부하지만 긴 Detail content는 실제 scroll container가 아니다.

필요:
- 실제 scroll/overflow interaction.
- generated world 배경 위 panel 가독성 재조정.
- QA camera helper가 필요하면 production camera와 분리된 debug-only path로 제공.
- Core에 존재하는 lifecycle/history/deceased data를 living-only compatibility projection과 혼동하지 않는다.

### D. Emotion Runtime Integration — Jjun provider + Dagyeom presentation

Core에는 emotion dimensions / event delta / decay 구조가 이미 있다.

부족한 부분:
- Needs pressure / relief.
- work success / failure.
- repeated frustration.
- parenting/care outcome.
- environmental threat/contamination.
- lifecycle loss/grief.
- civilization discovery/craft success.

원칙:
- New Game founders는 중립 상태에서 시작한다.
- 실제 사건만 emotion을 바꾼다.
- UI가 보기 좋게 만들기 위해 감정을 임의 생성하지 않는다.

### E. Lifecycle event presentation — Dagyeom

#125로 완료된 것:
- `bAlive=false` resident는 living physical projection에서 제외되어 actor/runtime cleanup.
- LifeStage 변경에 맞춘 body/capsule scale synchronization.

남은 것:
- birth/growth/death event visibility.
- deceased/history inspection은 Core observer/history DTO를 사용.
- 필요 시 death animation/event presentation.
- 현재 Quaternius adult skeleton uniform scaling은 v1 기능 표현이며 child anatomy 품질의 최종안이 아니다.

### F. Civilization Phase 2 — Jjun provider -> Dagyeom presentation

기존 완료:
- no-free-infrastructure New Game.
- PrimitiveStorage.
- SharpFlake / StoneCuttingTool / DiggingStick / StoneHammer.
- FirePit / Heat / Charcoal.
- Furnace / CopperOre / CopperMetal / CopperSmelting.

다음 후보:
- SleepingPlace.
- Shelter.
- WorkSurface / workbench precursor.
- TinOre.
- Bronze / bronze tool progression.
- 시설에 따른 실제 Needs / work efficiency 변화.

새 시설은 항상 Core progression을 통해서만 생성한다.

### G. Cleanup / performance — Jjun

- `SimulationSnapshotCodecLegacy.cpp` 및 pre-release save policy와 맞지 않는 dead compatibility source 최종 정리.
- old Preflight structural marker/legacy API assertion 제거.
- long catch-up simulation에 per-frame simulation budget 도입.
- residue / HISM refresh 비용 장기 profiling.

### H. Later large systems

기존 MASTER 일정에서 유지:
- health / disease / pathogen.
- contaminated water / soil.
- premature illness / accident mortality.
- exploration / migration.
- multiple households and settlements.
- jobs / economy / trade.
- social institutions / politics.
- multi-generation civilization.
- long-run population and society stability.

---

## 4. 통합 실행 순서

Android/APK Gate B는 사용자가 2026-09-17에 명시적으로 보류했다. 재개 요청 전에는 roadmap에서 삭제하지 않고 **PAUSED** 상태로 유지한다.

기능 개발의 현재 순서:

1. **Canonical docs reconciliation**
   - 이 audit 문서.
   - `tasks/WORK_STATE.md`.
   - `docs/DEVELOPMENT_MILESTONES.md`.
   - `tasks/TEAM_BOARD.md`.
   - #125 Lifecycle Presentation assist lock release.

2. **Dagyeom PR #100 World Readability Envelope closeout**
   - latest exact-head required CI 확인.
   - 통과 시 병합.

3. **Knowledge Transmission Spatial Authority v1 — Jjun**
   - witness / teaching physical encounter requirement.
   - deterministic Core tests.

4. **Character Context Motion v2 — Dagyeom**
   - existing assets first.
   - Core directive consumption only.

5. **Observer Readability + Real Scrolling — Dagyeom**
   - #98은 selective salvage만.
   - 최신 main에서 새 구현.

6. **Emotion Runtime Integration v1 — Jjun + Dagyeom consumer**
   - survival/work/family/environment/lifecycle causal wiring.

7. **Lifecycle Event Presentation v2 — Dagyeom**
   - birth/growth/death/history visibility.

8. **Civilization Phase 2 — Jjun -> Dagyeom**
   - sleeping/shelter/work surface.
   - bronze progression.

9. **Cleanup + Long-run Performance**

10. **Health / Disease / Premature Mortality**

11. **Migration / Multiple Settlements / Economy / Society**

12. **Android Gate B — PAUSED until user resumes it**

---

## 5. Parallel ownership rule

이제부터는 별도의 두 로드맵을 운영하지 않는다.

하나의 통합 로드맵 안에서 lane만 나눈다.

### Jjun lane
- Core
- AI / Utility / Decision
- Simulation
- World authority
- Save/Load
- Bridge/read contracts
- deterministic/Core regression
- Build/CI/Android/config

### Dagyeom lane
- Character presentation
- Animation/context motion
- Observer UI
- camera/visual UX
- Environment/maps
- WorldPresentation

### Cross-lane rule
- provider contract가 필요한 기능은 Jjun이 Core/read DTO를 먼저 확정한다.
- Dagyeom은 그 contract를 소비해 표현한다.
- 상대 lane 직접 수정이 꼭 필요하면 Integration Request 또는 scoped Assist Lock을 먼저 기록한다.
- Jjun은 `dagyeom/*` branch에 직접 push하지 않는다.
- 오래된 branch를 재활용하기보다 최신 `main`에서 필요한 변화만 다시 구현한다.

---

## 6. Validation strategy

Android가 보류된 동안 기본 gate:
- Core 변경: Core Tests + deterministic harness + Preflight.
- Unreal C++ 변경: Preflight + Unreal Linux Compile.
- Presentation-only 변경도 C++이면 Unreal Linux Compile을 통과해야 한다.
- docs-only 변경은 불필요한 Unreal compile을 새로 강제하지 않는다.
- exact-head rule을 유지한다.

장시간 Unreal job은 정상 진행 중이면 반복 polling/restart하지 않는다.

---

## 7. 이 문서 이후의 canonical 관계

- 상세 감사/누락 근거: `docs/INTEGRATED_AUDIT_2026-09-17.md`
- 현재 실행 상태: `tasks/WORK_STATE.md`
- 큰 개발 순서: `docs/DEVELOPMENT_MILESTONES.md`
- ownership / locks / IR: `tasks/TEAM_BOARD.md`
- 지속적 설계 판단: `docs/DECISION_LOG.md`

앞으로 기능을 시작하기 전에 위 문서와 실제 GitHub 상태를 대조한다.
