# 주민 개인 자원 목표 → 실제 채집 → 개인 경험 → 다음 선택

기준 main: `01c1e12b1039fcb34b14768c2701ff2a0ecf3abc`. PR [#852](https://github.com/sjLim91/lifelens-ue/pull/852).
사용자 요청: 주민 한 명의 자율 AI 최우선·병렬 개발, 클론 없이 GitHub 연결.

## 실행 연결

기존 Core 근접 감지/목표 composer는 목표·후보를 읽기 전용으로 만들었지만
실제 단일 주민 행동 슬롯에 자원 목표를 연결하지 않았다.

`chooseResidentSensedResourceGoal`은 정상 15분 계획 경계에서 같은 주민의
현재 Core 위치를 감지하고 이미 존재하는 typed 물자 목표를 생성한다.
본인이 아직 보유하지 않은 재료만 대상이다. 실제 관측된 유한 자원에서
기존 Gather 후보와 실제 지면 경로를 검증한 후 작은 목표 선호로 순위를
비교한다. 비교 점수는 기존 Core의 성격·역할·협력 보정 및 시설 권한 보호와 같은 척도를 사용한다. 후보 비교만으로 재고·기억·시간·RNG·채집 결과는 바뀌지 않는다. 후보 허용/제외 목록은 최대 12개 관측 노드에만 비례하며 전체 World 노드 제외 목록을 만들지 않는다.

Simulation은 이 결과를 기존 cadenceDecision에 전달한다. 실행은 기존
tryCivilizationDecision → pending token → Core 이동 → 실제 Gather →
결과 기억 경로 그대로다. 이미 ResidentCognition이 선택한 행동, 긴급
생존, 신체 행동, 사회·회피·돌봄, 시설 작업은 이 선호로 바꾸지 않는다.
사람마다 다른 지각과 인벤토리를 다시 확인하며 타인의 센서나 모델
문구를 공유된 지식으로 올리지 않는다.

이 작업은 독립적인 두 번째 AI 루프를 만들지 않는다. 별도의 실행기나
브라우저 권한, 새 snapshot 필드, 가짜 Goal 완료도 추가하지 않는다.
범용 다단계 계획·영구 GoalID/PlanID/StepID 전체 구현 완료는 아니다.

열린 #846의 read-only Affordance/Experiment 후보 변경 파일을 수정하지
않는다. 이번 연결은 실제 Core composer와 기존 Gather를 사용하는 좁은
실행 연결이며, #846의 모든 primitive 조합을 대신 구현하지 않는다.

## 개인 채집 경험

본인의 실제 `resource-node:<id>` Gather 결과 기억만 source/action별로
현재 후보 선호에 반영한다. 정확한 DirectWitness·본인 source/subject·
Core 기록 문구·시각·행동 key를 확인한다. 추론 근접 기억, 전언, 다른 주민,
미래·잘못된 source 또는 성공 예측은 영향을 주지 않는다.

고갈/소실·접근 실패·용기 부족은 서로 다른 작은 음의 선호를 주며 최신
실제 성공은 이를 회복한다. 1일 이내 시간 감쇠, 동일 기록 중복 불증폭,
같은 시각 성공 우선의 결정론적 순서를 유지한다. 선호는 -0.08~+0.03
범위이고 자원 수량·접근 권한·기술·필수 용기를 대신하지 않는다.

기억은 후보 노드마다 다시 읽지 않는다. 한 계획의 실제 source/action
기억만 희소한 임시 index로 만들고 native Gather 후보 및 개인 목표의
재시도에서 재사용한다. World 모든 노드 수에 비례하는 기억 index나
저장 상태는 추가하지 않는다. 긴급 식량의 기존 우선 선택은 보존한다.

## 주민별 실제 인과 감사

paired Core 감사 JSON은 기존 전체 합계를 유지하고 lossless 주민 ID
문자열을 key로 한 residents 투영을 추가한다. 주민별 제안/승인/거절,
pending과 실제 Core 결과, 개인 기억·지식 근거를 구분한다.
승인은 완료가 아니며 시간상 맞는 생각과 행동도 AI 인과의 확증은 아니다.

행동 종류와 outcome code, 토큰·주민·대상·발행/관측 시각을 확인한다.
교육 결과로 돌봄 성과를 올리거나 발행 전 결과·미관측 미래 결과를
받아들이지 않는다. 실제 callback 중복은 주민당 고정 크기 token
watermark로 막는다. callback 이력 전체를 영구 set으로 쌓지 않는다.

## 검증과 남은 범위

- `test_resident_sensed_resource_agency`: 실제 센서→개인 목표→native
  Gather, 읽기 전용 계획/원격 거부/실제 수량 감소/목표 사실 달성,
  미관측·고갈·긴급·사회/시설·기존 cognition 보호, 실제 Simulation
  pending과 binary 재개 및 중복 ACK 거부. 원격/근접 두 공급원이 공존할 때 기존 원격 선택에서 새 개인 목표의 실제 근접 선택으로 바뀌는 production 회귀를 포함한다.
- `test_resident_resource_outcome_preference`: 실제 실패→재생된 자원에서
  개인별 다른 선택→성공 회복, 원인별 차이, 기억 출처/시간/중복 검증,
  실제 memory binary 재개와 희소 index/canonical ID 경계.
- 기존 `test_resident_ai_paired_causal_audit`: 주민별 합계 보존·개인
  격리·시간/kind/token 거부와 결정론적 paired 재실행.

CMake/CTest/결정론/365일·WASM/Web/Preflight는 통합 PR의 정확한 HEAD에서
확인한다. 소스 작성만으로 PASS라고 표시하지 않는다. M1/M2/M3 전체의
완료가 아니라 개인 자원 행동과 검증 공백을 줄이는 구현 단위다.

첫 원격 Core 실행(`31179b74`, [run 39397055502](https://github.com/sjLim91/lifelens-ue/actions/runs/39397055502))은 build 성공, CTest 157/159 통과였다. 새 production fixture는 식량 외 모든 재료까지 고갈시켜 실제 탐색이 우선하는 조건이었으므로, 이후 fixture는 실제 공유 비식량 재고와 지역 수원으로 다른 부족을 통제한다. 수동 저점수 fallback 대신 전체 native unified 선택이 원격 식량을 고르고 동일 입력의 개인 목표가 지역 식량을 고르는 것을 먼저 검증한다. 기존 retry fixture도 실제 실패 이후 native 자체가 다른 source를 고르는 새 요구로 갱신하며, 기존 전략층이 같은 경험을 이중 감점하지 않는지 확인한다. 수정 HEAD 결과는 CI에서 별도 확인한다.
