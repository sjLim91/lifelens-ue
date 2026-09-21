# LifeLens Whole-Source Audit Ledger — 2026-09-21

> 이 파일은 전수검사 재개 지점을 위한 **중복 방지용 canonical ledger**다.
> 실제 GitHub main / PR / Actions가 최우선 truth이며, 이 파일은 "이미 본 범위를 또 보는" 일을 막기 위한 실행 체크포인트다.

## Current checkpoint

- main: `18b95d597d7f32c0dd16084b783f1a57534fc283`
- latest merged/landed audit fixes on main:
  - #364 coast nature/resource visibility recovery — merged.
  - terrain relief scale reinforcement v2 — landed on main.
  - opening settlement excessive ecology clearing reduction v2 — landed on main.
- open:
  - #370 CC0 photoreal broadleaf + hero canopy import.
  - #372 Desktop photoreal canopy runtime tier; depends on #370 generated assets.
  - #373 visible-water fallback terrain-burial fix.

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
- visible-water fallback existence **확인 완료**; 현재 남은 문제는 #373의 terrain-clearance 결함.
- coast ecology hard-zero/resource suppression defect **#364에서 수정 완료**.

### C. Terrain / ecology runtime-visibility defects already handled
- flat-looking terrain relief weakness → main의 terrain relief v2로 보강.
- opening settlement vegetation over-clear → main의 ecology clearing v2로 축소.
- coast에서 자연환경/자원 비가시성 → #364 merged.

## CURRENT — 지금 여기서만 이어간다

### 1. Water runtime visibility closeout
- #373 검증/병합 여부 확인.
- 목적: fallback water가 opaque terrain 아래에 묻히지 않는지 보장.
- #373이 green + merge되면 water fallback은 DONE으로 이동.

### 2. Zero-cost production nature asset integration
- #370 asset import gate 결과 확인.
- #370 main 반영 후 #372 runtime consumer 재검증/병합.
- Android cook exclusion / desktop-only hard-reference boundary 유지.
- 이미 탈락한 oversized candidates를 같은 wave에서 다시 probe하지 않는다.

## NEXT — CURRENT 끝난 뒤 순서

1. graphics/materials/platform content boundary residual audit.
2. character appearance / motion / context correctness.
3. lifecycle / family / society continuity residual audit.
4. Observer UI / camera / scale-transition **consumer wiring** audit.
5. Android-first packaging + Windows/macOS native QA + Save/Load + long-run/performance/crash gates.

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
