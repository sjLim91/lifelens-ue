# Web runtime / cognition review — 2026-10-05

## 기준과 범위

- 실제 main: `f79b27df8160541174ba564d116556d96b6310fb` (#638 merge).
- #638의 새 고속 진행 캡처/사망 기록 구현이 main에 있는 것을 확인했다.
- 열린 #639는 Core 교역 여정/persistence 작업이다. 해당 Core/codec/test 파일은 수정하지 않는다.
- 읽은 영역: runtime architecture/roadmap/ownership, Core snapshot codec/health/cognitive validation, WASM loading/bridge/response contracts, simulation clock, world session, observer engine/store/feed, App 입력 gate, resident/world rendering lifecycle, local cognition provider/scheduler/tests와 GitHub Actions.
- main의 Preflight, Typecheck, Web Preview, External Preview Probe는 조회 시 PASS였다. Core 저장/건강/장기 균형의 전체 native run이나 실기기 시각 QA를 이번 Web 수정의 검증으로 주장하지 않는다.

## 재현된 결함과 수정

### 새 월드 lifecycle — `work/web-world-lifecycle-audit-20261005`

1. paused/4x 상태에서 새 월드를 만들면 ObserverStore는 1x로 돌아가지만 SimulationClock와 renderer는 이전 배속을 유지한다. 새 월드가 UI와 실제로 다른 속도로 진행될 수 있다.
   - 이전 시간 accumulator를 먼저 초기화하고 새 store의 기본 배속을 clock/renderer에 전달한다.
2. ObserverStore.resetWorld가 selectedSettlementId를 지우지 않고 engine도 renderer의 정착지 선택을 명시적으로 초기화하지 않는다.
   - 두 경로에서 새 세계의 선택 상태를 초기화한다.
3. 고속 진행 중 버튼은 비활성화되지만 seed input의 Enter 및 programmatic action은 새 월드를 호출할 수 있다. chunk loop가 yield한 동안 Core 세계가 바뀌면 서로 다른 세계를 하나의 고속 진행 작업으로 진행/요약할 수 있다.
   - engine의 createWorld에서 fastForwardRunning을 검사한다.

검증: 실제 production resetWorld 및 createWorld orchestration에 대한 회귀에서 선택 잔존과 paused clock 불일치를 수정 전 FAIL로 확인, 수정 후 PASS. 0x/4x→new-world 속도 일치, renderer 선택 초기화, accumulator 순서, 진행 중 호출 무효화, 기존 WorldSession 고속 진행/사망/조회 실패 회귀 및 fast-forward structural validator PASS.

### 인지 비동기 lifecycle — `work/cognitive-agent-lifecycle-audit-20261005`

1. reset/dispose는 AbortSignal만 보낸다. 제공자가 abort를 무시하면 active caller와 실행 슬롯은 timeout까지 남는다.
   - scheduler 자체 abort 결과를 Promise.race에 포함해 caller를 null로 완료하고 timer/listener/slot을 정리한다. 늦은 제공자 응답은 수락하지 않는다.
2. simulationMinute가 NaN/Infinity 또는 요청 시각 이전으로 돌아간 경우 기존 로직은 제안을 수락할 수 있다.
   - 유효하지 않거나 되감긴 컨텍스트는 null로 거절한다.
3. simulationMinute callback이 throw하면 execute의 Promise가 reject되고 submit의 Promise는 완료되지 않는다.
   - 컨텍스트 오류도 null로 완료하며 다음 요청을 처리할 수 있게 한다.

검증: abort를 무시하는 제공자 reset 회귀는 수정 전 FAIL/수정 후 PASS. 전체 기존+추가 인지 suite 17개 PASS: queue/concurrency bounds, timeout, stale/out-of-order, schema/Core bridge, loopback provider, reset/dispose 즉시 완료, invalid/rewound time, throwing-context recovery.

## 검증과 복구

- 두 변경은 별도 목적/검증 범위라 독립 PR로 관리한다. 서로의 수정 파일 및 #639 Core 파일과 겹치지 않는다.
- runtime PR: 기존 Runtime Resilience suite에 새 lifecycle 회귀를 연결한다. exact-head Typecheck/Preflight/Runtime Resilience/production build 결과를 확인한다.
- cognition PR: 기존 Web Cognitive Agent workflow에서 17개 suite/Typecheck/production build를 확인한다.
- GitHub 직접 연결 API로 commit/branch/PR을 생성한다. 크론/스케줄을 추가하지 않는다.
- 오류/수정되지 않은 영역에 대한 완전 무결성 보증이나 새로운 C6/C7 완료 선언은 하지 않는다.
