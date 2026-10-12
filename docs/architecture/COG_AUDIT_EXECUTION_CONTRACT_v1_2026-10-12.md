# COG-AUDIT — 자율성/인과 계측 실행 계약 v1

> 상태: 구현 선행 계약. 기준: 최신 main 및 `docs/architecture/AUTONOMOUS_LIFE_AUTHORITY_BOUNDARIES_v1_2026-10-12.md`. 이 문서는 코드 구현 완료를 뜻하지 않는다.

## 목적

현재 Core가 실제로 만든 선택과 결과를 기록하여, AI가 생존 루프에 갇혔는지, 개인 경험으로 방법을 바꿨는지, 사회적 학습이 일어났는지를 검증한다. **행동 정책·Needs·World RNG·Snapshot 계약은 변경하지 않는다.**

## 불변 경계

- Core 이벤트만 측정. Web Shadow 제안은 별도 진단 채널이며 실제 선택에 포함하지 않는다.
- 읽기 전용 observer는 simulation state를 수정하지 않고, 이벤트 관찰 유무에 따라 결과가 달라져선 안 된다.
- actorId, WorldSeed/RegionId, simulation minute, source action, attempt ID, outcome, witness provenance를 사용하되 개인의 미관찰 WorldTruth를 AI에 누출하지 않는다.
- 생존 Goal의 점수나 임계치를 조정하지 않는다.
- 저장 형식 `SimulationSnapshotVersion=3` / binary format `8`을 이 단계에서 변경하지 않는다.
- 수집 용량과 이벤트 수에 상한을 두고 이벤트 순서/정렬을 안정화한다.

## 이벤트 분류

1. `need_trigger`: 실제 생리적 압력과 선점 원인.
2. `goal_candidate`: 기존 Utility/Cognitive 후보; Shadow 후보는 별도 namespace.
3. `goal_selected`: 실제 Core 선택과 선택 근거.
4. `action_attempt`: Core가 실행을 시도한 행위/대상/장소.
5. `action_outcome`: 성공/실패/중단/원인 및 실제 자원 비용.
6. `learning_update`: 직접 경험에 따른 방법별 믿음/기대치 변경.
7. `social_transmission`: 목격/전언/모방 출처와 전달 내용.
8. `world_ecology_delta`: Core가 실제 발생시킨 자원·환경·동물 변화.

미구현 계층의 이벤트는 0건으로 기록하고 가짜 이벤트를 생성하지 않는다. 동일 attempt 중복 기록은 금지한다.

## 필수 집계

- 시간 비중: 생존/사회/탐색/실험/대기/이동/불명. 합계와 누락 시간을 함께 출력한다.
- 개인별 선택 다양성: 목표·행동·대상 종류와 동일 실패 반복률.
- 생존/사회 경쟁: 긴급 Needs 선점률, 사회행동 진입/완료/중단률.
- 경험 학습: 실제 실패 이후 방법/수원/대상 전환 비율과 성공률 변화.
- 지식 출처: 직접 목격/전언/추론 구분, 미관찰 사실 누출 건수.
- 생태/문명: 유한 자원 소모·재생, 실제 채집/재배/교역/시설/동물 사건; 미구현은 별도 표기.
- 시간: 100/365/1000/3000일, seed별/주민별/지역별, 저장-복원 전후.
- 결정론: 계측 활성/비활성 실행의 Core snapshot/event hash 일치.

## 검증 게이트

1. 기준 main의 대표 seed와 재현 가능한 장기 하네스 수집.
2. 계측 off/on 동시 조건에서 Core 결과 일치 증명.
3. 생존/사회 행동 비중과 반복 실패를 수치로 설명.
4. 저장/복원 및 일수 건너뛰기에서 중복·누락 없는 시간 범위 확인.
5. 모든 수치의 출처가 실제 Core 사건인지 확인.
6. 검증 실패 시 병합 보류, 원인/수정/재검증. 기존 Core truth/Need 튜닝으로 계측 실패를 덮지 않는다.

## 다음 단계 연결

COG-AUDIT의 결과로 COG-3 저널 후보 #667/#668 중복을 비교하고 하나의 권위 계약으로 통합한다. COG-L2 #666은 생존 Needs 회귀 없이 방법 선택만 변경해야 한다. ECO-A2a #686의 독립 codec을 World Snapshot 통합 완료로 간주하지 않는다.

이 단계의 핵심 산출물은 **실제 선택 → 실제 실행 → 실제 결과 → 실제 학습**의 인과적 관찰 가능성이다.
