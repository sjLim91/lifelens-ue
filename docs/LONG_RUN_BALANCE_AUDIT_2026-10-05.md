# LifeLens 전 시스템 장기 밸런스 / 인과 감사 — 2026-10-05

## 목적

이 문서는 개별 기능의 통과 여부가 아니라 **LifeLensCore의 여러 subsystem이 장기간 동시에 작동할 때 최초로 균형을 무너뜨리는 causal bottleneck**을 찾기 위한 감사 보고서다.

이번 lane은 진단 전용이다. Needs decay/threshold, birth/death, 질병 확률, 탐험 반경, 시설 비용, trade threshold, Utility weight, 이동속도/행동시간, cap, save/snapshot format과 Web simulation truth를 변경하지 않는다.

## 기준선 / 병렬 작업 경계

- 감사 시작 main: 6f9a877485b7b8fc2b82297ff42d09802b4cbfb8 (#643 Cognitive Shadow Mode)
- 병렬 Core PR: #642 C6 교역 여정/귀환 persistence
- #642 변경 파일과 이 PR 변경 파일의 overlap 목표: **0**
- tasks/WORK_STATE.md: 충돌 위험 때문에 수정하지 않는다.
- TEAM_BOARD: 감사 시작 시 저장소에서 해당 파일을 찾지 못했다.

## 감사 구성

### Seed

Canonical: 4242001, 874213954

추가 deterministic coverage: 1357911, 2718281, 3141592

### Checkpoint

- day 1 / 7 / 30 / 100 / 365 / 1000

### 결정론

seed 4242001을 day100까지 동일 조건으로 2회 실행하고 다음을 비교한다.

1. wall-clock 필드를 제외한 모든 AUDIT_* metric line identity
2. day100 encoded snapshot byte identity (SHA-256 포함)

## authoritative observation 원칙

감사기는 Simulation::observe*, persistent Core state, Simulation::onEvent, 기존 snapshot codec만 읽는다. 별도의 상태를 simulation truth로 되먹이지 않는다.

정확히 노출되지 않는 값은 임의 추론값을 만들지 않고 **P3 observability gap**으로 남긴다. 특히 material별 inter-settlement trade transfer, 명시적 spoil/loss event, migration의 전용 minute-level presentation phase는 현재 authoritative read-model 노출 여부를 별도로 기록한다.

## 산출물

자동 workflow가 seed별 raw log/snapshot과 다음 통합 파일을 만든다.

- long-run-balance-audit.json
- long-run-balance-audit.csv
- long-run-balance-audit.md

regression hard-fail은 deterministic mismatch, crash/save failure 같은 사실 기반 invariant에 한정한다. 사회시간 비율·시설 수·출산 수 등에 임의의 '좋은 숫자'를 하드코딩하지 않는다.

---

## 실행 결과

아래 영역은 이 branch의 5-seed/day1000 실행 결과로 같은 PR에서 갱신한다.

### P0 / P1

실행 결과 반영 예정.

### 정상 판정 영역

실행 결과 반영 예정.

### day1000 causal timeline

실행 결과 반영 예정.

### 활동시간 비율

실행 결과 반영 예정.

### Resource bottleneck

실행 결과 반영 예정.

### Exploration / chunk growth

실행 결과 반영 예정.

### Population / health

실행 결과 반영 예정.

### Knowledge / civilization dead-system

실행 결과 반영 예정.

### Performance / scale

실행 결과 반영 예정.

### 다음 수정 우선순위 TOP 5

실행 결과 반영 예정.
