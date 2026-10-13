# COG-B5a — 주민별 대안 행동 경쟁·안전 예산 계약 (2026-10-13)

## 범위와 철학

B4a는 목표→단일 Core 행동 후보를 읽기 전용으로 변환했다. B5a는 **하나의 주민 목표에 대해 현실적으로 가능한 대안 방법을 동시에 비교**한다. 이는 실험의 계획/비교 단계이며 실제 행동 실행·지식 창발·성공 경험 기록을 완료한 것으로 표시하면 안 된다.

- `composeAutonomousExperimentOptions(world, relationships, request, goal, budget)`는 기존 `draftAutonomousAffordancePlan`과 같은 현재 Core 문맥 재검증/개인 근거/생존 긴급 차단을 반드시 거친다.
- 현재 Core가 실제로 보유한 `SocialIntent::Approach` 또는 `Repair`가 **각각 점수 > 0**으로 허용되는 상황에서만 대안으로 분기한다. 다른 물리행동 단계는 원래 Core affordance 그대로 유지한다.
- 각 대안은 추상 **effort unit**, 추상 **risk proxy** 정책 상한, 단계 수, 최대 후보 수를 통과해야 반환한다. 이 수치는 실제 시간·이동·재료·의학적 위험·성공 확률이 아니다. 실험을 예약·완료했다고 주장하거나 Core 자원을 차감하지 않는다.
- 후보 순서는 기존 Core SocialUtility 선호 점수와 결정론적인 예산/순서 조건을 사용한다. 이는 성공 예측을 학습한 확률이 아니다.
- 낡은/위조 Cognition context, 부정확한 주민·목표, 자원 부족, 긴급 생존 상황, 사망 대상, 지원되지 않는 재고/기술 목표, 예산 초과는 후보를 만들지 않는다.
- 분기 수가 적거나 (예: 잠을 자는 단일 방법) 대안이 하나뿐이어도 허위 대안을 만들어 내지 않는다. 아무 후보도 없는 경우 `NoSafeCandidate`가 정상적 결과다.

## 인과·권한·호환성

- 코드: `AutonomousExperimentPlanner.h`, `test_autonomous_experiment_planner.cpp`, CMake 테스트 등록만 변경.
- **World/Simulation/UtilityAI/Needs/생태/관계/행동 토큰/저장 포맷/WASM ABI 무변경**.
- AI/모델이 직접 Core 상태나 주민의 비공개 관계를 바꿀 수 없다. 무료 로컬 방식과 동일 seed 재현성을 유지한다.
- `preferenceScore`는 기존 Core social score로, 실제 행동 실행 결과에 의한 성공률이 아니다.
- #693의 Core 수락 저널은 생각의 수락이고 실제 행동 실행이 아니다. #708의 실제 행동 토큰과 #709의 진실 변화 비교는 용도별로 분리해 유지한다.

## 테스트 완료 기준

1. 동일 World/actor/context 20회 반복에서 후보·순서·fingerprint가 완전히 동일하고 Core RNG, minute, Need, trust 불변.
2. 유효한 갈등 상태에서 Approach/Repair 두 방법을 독립적으로 제안. 갈등 해소 후 Repair가 사라짐.
3. 노력·위험·후보/단계 수 예산에 의한 차단. NaN/범위 밖 예산 거절.
4. 위조 Need/Memory, 과거 WorldMinute, 타 주민 명의, 긴급 생존, 사망자 상대 거절.
5. 수면 행동처럼 대안이 하나뿐인 Core affordance는 한 후보만 제안; 이미 달성한 목표는 제안하지 않음.
6. exact-head Core Tests, Preflight, Web WASM, 장기 Core 회귀/결정론 확인.

## B5b~B5d 후속 — 자동 실행·학습의 실제 완성 기준

- **B5b:** 제안된 단일 첫 행동을 *그 시점의 Core*에서 재검증하고 기존 물리 실행 토큰에만 연결. 추가 실행 슬롯/자원 생성 금지. 중단·취소·실패, 실제 이동 경로, 저장 재개 계약 필수.
- **B5c:** Core 실행 토큰과 전후 상태를 엄격 연결해 성공/실패/미달/퇴보를 분리. 주민에게 직접 지각 가능한 결과만 개인 가설 confidence로 학습; 다른 시간의 World 변화는 성공으로 오귀속하지 않음.
- **B5d:** 같은 조건에서 성공·실패·환경 변화별 대안 재선택, 제한된 실험 예산 소모, 100/365/1000일 seed 감사, 관계·수원·생존 행동 회귀. 이후 자원 채집/탐구까지 물리 affordance를 안전하게 확장.

> B5a 병합만으로 '주민이 실제 실험했다' 또는 'GPT급 사고/문명 창발 구현'을 주장할 수 없다.
