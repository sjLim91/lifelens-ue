# LifeLens Whole-Source Audit Ledger — 2026-09-21

> 이 파일은 전수검사 재개 지점을 위한 **중복 방지용 canonical ledger**다.
> 실제 GitHub main / PR / Actions가 최우선 truth이며, 이 파일은 "이미 본 범위를 또 보는" 일을 막기 위한 실행 체크포인트다.

## Current checkpoint

> **2026-09-21 runtime correction:** latest screenshots invalidated the earlier visual-closeout assumption. The previously completed checks remain valid as source/contract checks, but Earth/environment runtime visual acceptance is REOPENED under `docs/WORLD_ARCHITECTURE_v2.md`.
>
> **Execution hold:** do not enter character audit and do not implement World v2 until the user gives a new start signal.


- source checkpoint before this docs sync: `255e0e49b6dc64987c7ba195d3674d43a4456908`.
- #370 CC0 photoreal broadleaf + hero canopy import — **MERGED**.
- #372 Desktop photoreal canopy runtime tier — **MERGED** after exact-head Preflight #1360 + Unreal Linux Compile #620 PASS.
- #373 visible-water fallback terrain-clearance fix — **MERGED** after Preflight #1355 + Unreal Linux Compile #619 PASS.
- #364 coast nature/resource visibility recovery — **MERGED**.
- terrain relief v2 + opening-settlement ecology clearing v2 — landed on main.
- Poly Haven mature-canopy provenance is recorded in `Content/Environment/PROVENANCE.md`.
## Resume rule — 반드시 지킨다

1. 세션이 끊겨도 처음부터 다시 검사하지 않는다.
2. 재개 시 `main HEAD`, open PR, Actions만 먼저 reconcile한다.
3. 아래 DONE 범위는 **관련 파일이 이후 commit/merge에서 바뀌지 않은 한 재검사 금지**.
4. 관련 파일이 바뀌었으면 전체 영역을 다시 훑지 말고 **변경 diff + 직접 영향 범위만 재검증**.
5. 이미 확인한 enum/계약/파일 위치를 "혹시 없나" 식으로 다시 찾지 않는다.
6. 현재 항목에서 결함을 찾으면 수정 → cheap gate → 필요한 compile → merge/land → ledger 한 번 갱신 → 즉시 다음 항목으로 이동한다.
7. compile/CI 상태 확인 때문에 전수검사 흐름 자체를 처음부터 되감지 않는다.

## DONE — 다시 처음부터 보지 말 것

### A. Core / authority / prior structural audit
- AUDIT-0A Observer chrome authority.
- AUDIT-0B resident-local environment authority.
- AUDIT-0C regression guards.
- settlement facility authority and settlement need/effect foundation.
- time/calendar/weather authority.
- lifecycle/family/social foundation already covered by prior canonical audits.

### B. Earth / hydrology structural foundation
- Planet / Surface Region / Chunk / Local Surface identity and hierarchy **존재 확인 완료**.
- Local / Regional / Planetary / Orbital / Interplanetary scale enum/contract **존재 확인 완료**.
- deterministic hydrology contract and explicit water-body observation path **구조 확인 완료**.
- visible-water fallback + terrain clearance **#373까지 수정/검증/병합 완료**.
- coast ecology hard-zero/resource suppression defect **#364에서 수정 완료**.

### C. Terrain / ecology runtime-visibility defects already handled
- flat-looking terrain relief weakness → main의 terrain relief v2로 보강.
- opening settlement vegetation over-clear → main의 ecology clearing v2로 축소.
- coast에서 자연환경/자원 비가시성 → #364 merged.

## CURRENT — World v2 설계 완료 / 환경 runtime acceptance 재오픈 / 구현 대기

### Water / nature / terrain
- #373 fallback water terrain clearance — DONE.
- #370 asset import + #372 runtime canopy consumer — DONE.
- Android never-cook boundary contains the desktop-only mature canopy pair.
- Poly Haven provenance for the new pair is recorded and guarded by Preflight.
- platform cook policy records the same exclusions/shared exceptions.

### Source-level residual classification
- authored Rain/Snow Niagara assets are still absent from the repository; the existing primitive precipitation path is an intentional packaged **safety fallback**, not final visual acceptance.
- actual rainfall/snowfall look remains runtime visual QA / future authored-VFX work.
- `island_tree_02` is a sparse desktop hero canopy; Windows/macOS native performance is still a runtime profiling gate.
- these runtime-only quality/performance checks belong to final native QA and do not justify re-scanning completed Earth/hydrology contracts.

### Mandatory handoff boundary
- **Do not start the character appearance/motion/context manual audit until the user is told that the audit has reached the character boundary.**
## NEXT — 사용자 시작 사인 이후에만 진행

1. World v2 contract cleanup / explicit materialized coordinate set.
2. continuous terrain + hydrology + streaming + biome presentation migration.
3. runtime visual acceptance: terrain/water/forest continuity + proxy removal.
4. **그 뒤에만** character appearance / motion / context correctness.
5. lifecycle / family / society continuity residual audit.
6. Observer scale-transition consumer wiring.
7. platform / Save / long-run / performance / crash gates.
## Observer scale 중복 방지 메모

WorldHierarchy의 scale enum과 Planet/Region identity 존재 여부는 이미 확인했다.
따라서 Observer 단계에서는 다시 enum 존재를 찾는 게 아니라 아래만 확인한다.

- 실제 Observer camera/controller가 authoritative scale state를 소비하는지.
- Local ↔ Regional ↔ Planetary ↔ Orbital 전환 시 presentation/culling/input가 올바르게 바뀌는지.
- 현재 구현이 계약만 있고 consumer wiring이 없는 경우에만 미구현으로 기록한다.

즉, **"enum이 있나?"를 다시 검사하지 말고 "runtime consumer가 실제로 쓰나?"만 검사한다.**

## Definition of done for an audit item

- source/contract presence만으로 DONE 처리 금지.
- 결함 수정 시 회귀 guard를 가능한 범위에서 추가.
- C++/UHT/UBT risk가 있으면 Unreal Compile.
- visual/runtime acceptance가 필요한 항목은 compile green과 별개로 runtime QA requirement를 남긴다.
- 완료 후 이 ledger의 CURRENT/NEXT만 갱신한다. 과거 DONE 항목을 재서술하며 다시 훑지 않는다.
