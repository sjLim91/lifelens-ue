# 주민 AI 자원·제작 행동 — Core 기존 affordance 재사용

이 단계는 주민별 무료 LLM 판단을 실제 Core 문명/자원 행동으로 연결하는 후속 단계다. 이전 단계 PR #734의 `submitLocalCognitiveProposal` 저널 승인 경로 위에 구축한다.

## 행동 표

| AI 전략 | Core 실제 후보 | 필수 안전 조건 |
| --- | --- | --- |
| ImproveFoodSecurity | Gather PlantFood | 현재 월드의 실제 식량 노드에 양수 quantity |
| ImproveWaterSecurity | Gather Water | 실제 물 노드 및 기존 용기 보유 조건 |
| AcquireMaterials | Gather | 기존 bestGatherDecisionAtPosition 결정 |
| ExploreOpportunity | Explore | 기존 bestResourceExplorationDecisionAtPosition 결정 |
| CraftUsefulTools | Craft | 기존 recipe/ingredient/technique 규칙을 통과한 직접 도구 제작; 시설 공사 제외 |
| ExpandCultivation | Craft/Cultivation | 재배 기술, 실제 포장/재료·노동 조건 충족 |

## Core 불변식
- 15분 정상 계획 주기에만, 최근 96분 이내 Core accepted 저널에 기록된 개인 선택이 priority 0.55 이상일 때 평가한다.
- 긴급 신체 Needs는 선행하고, 기존 착수 시설 공사 후보는 임의로 중단하지 않는다.
- Core가 생성한 legal candidate만 선호한다. 물/식량/기술을 새로 만들어내지 않는다.
- 채집 시 물 또는 식량으로 좁히더라도 Core 채집 함수의 점수·수량·재료/공간/상태 검증은 그대로 보존한다.
- 목적지가 도달 불가능하면 기존 resolveCivilizationContextTarget 및 pending action 검사가 실패시킨다.
- 옵션을 사용할 수 없으면 AI 선택을 무시하고 기존 Utility/Goal 선택으로 돌아간다.
- 모델 서버가 없는 경우 본 변경은 런타임에서 영향을 주지 않는다.

## 검증
- 새 CTest `test_resident_ai_existing_affordance`는 재료별 filter, 용기 없는 물 채집 거부, 기본 채집 chooser 동등성, 잘못된 타깃/죽은 주민/시간 만료, 실제 NEW GAME Core가 AI 식량 전략을 Gathering pending token으로 연결하고 재료가 실행 이전에 줄지 않는지 검사한다.
- #734 전체 Core + Web + WASM 검증 이후, 이 후속 변경을 결합 검증한 후 main 병합할 것.

## 앞으로
- 이 단계는 목표 6종의 **기존 행동 후보 선택**까지이며, 자유로운 레시피 발명이나 GPT급 일반지능 완료를 의미하지 않는다.
- 실제 행동이 실패/성공하면 기존 Core memory/knowledge/society 결과와 개인 후속 판단 사이의 인과를 관찰 지표로 연결해야 한다.