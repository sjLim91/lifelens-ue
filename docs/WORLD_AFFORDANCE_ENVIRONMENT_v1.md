# LifeLens World Affordance & Environment Consequence Design v1

> 상태: **Canonical product-direction extension**
>
> 이 문서는 `docs/LIFELENS_SPEC_v1.1.md`와 `docs/CIVILIZATION_PROGRESSION_v1.md`를 확장한다. 초기 세계에 현대 시설이 이미 존재한다고 가정하지 않고, 인간의 기본 욕구가 현재 세계에 실제로 존재하는 수단을 통해 해결되며, 그 과정의 부산물과 실패가 환경·건강·행동·지식·문명 발전에 실제 원인으로 남도록 정의한다.

---

## 1. 핵심 원칙

LifeLens에서 `행동 의도(Intent)`와 `행동 수단(Affordance)`은 분리한다.

- Core는 **무엇을 해야 하는지** 결정한다.
- World는 **현재 실제로 존재하는 수단 중 무엇으로 수행할지** 결정한다.
- 필요한 시설이 없다고 침대·화장실·세면대·식탁 등을 자동 생성하지 않는다.
- 더 발전된 수단이 없다면 더 원시적인 수단으로 fallback한다.
- fallback의 불편·위험·환경 부산물은 이후 발견과 문명 발전의 원인이 될 수 있다.
- Presentation/Unreal은 Core authority를 대체하지 않는다.

대표 인과 흐름:

`Need → Intent → Available Affordance 탐색 → 수행 → 결과/부산물 → Environment 변화 → 경험/기억/건강 영향 → 문제 인식 → 실험/발견 → 더 나은 Affordance 제작/사용`

---

## 2. 초기 세계 시설 가정 금지

NEW GAME 초기 세계에 다음 시설이 자동으로 존재한다고 가정하지 않는다.

- 침대
- 화장실
- 샤워기 / 세면대
- 수도
- 식탁
- 냉장고
- 창고
- 현대식 주방
- 하수 시설
- 기타 아직 발견/제작되지 않은 문명 시설

개발 편의를 위한 bootstrap 시설도 정식 제품 경로에서는 자동 생성하지 않는다.

세계에 없는 것은 실제로 **없다**.

---

## 3. Affordance Tier

행동 수단은 다음 우선순위를 사용한다.

1. `Preferred`
   - 목적에 맞게 제작된 정식 수단
   - 예: 침대, 화장실, 세면시설, 정식 식사 공간
2. `Primitive`
   - 초기 제작 가능한 원시적 대체 수단
   - 예: 건초 깔개, 간이 변소, 물통, 단순 화덕
3. `Natural`
   - 자연환경에서 직접 얻는 수단
   - 예: 맨땅, 자연 은신처, 강/샘, 바위/통나무
4. `Emergency`
   - 적절한 World object가 전혀 없을 때의 마지막 행동 방식
   - World object를 자동 생성하지 않는다.
5. `Unavailable`
   - 물리적으로 수행 불가능한 상태

같은 Intent를 지원하는 수단이 여러 개면 Tier를 먼저 비교하고, 같은 Tier에서는 거리/접근성/점유 상태와 안정적 tie-break를 사용한다.

---

## 4. 대표 욕구별 fallback

### Sleep

`침대 → 원시 침구/깔개 → 자연 은신처/안전한 지면 → 맨땅 Emergency 수면`

Emergency 수면은 Preferred 수면과 동일한 품질로 취급하지 않는다.

향후 품질 차이는 회복 효율, 날씨 노출, 안전성, 불편감, 수면 중단 확률 등에 영향을 줄 수 있다.

### Toilet

`화장실 → 간이 변소/구덩이 → 지정 야외 장소 → Emergency 야외 배변`

야외 배변은 단순 애니메이션으로 끝나지 않는다. 후속 환경 상태를 생성해야 한다.

### Hygiene

`정식 위생시설 → 물통/세척 도구 → 자연 수원 → 시설 없는 최소 행동`

물이나 세척 수단이 실제로 없다면 완전한 위생 회복을 허용하지 않는다.

### Eat / Drink

Emergency fallback은 **음식/물을 생성하지 않는다.**

Core/Inventory/World에 실제로 소비 가능한 자원이 있을 때만 현재 위치에서 먹거나 마실 수 있다.

수단이 없는 것과 자원이 없는 것은 별도 실패 원인이다.

### Rest / Sit

`의자/침대 → 통나무/바위 → 지면`

시각적으로 존재하지 않는 의자에 앉는 연출을 하지 않는다.

---

## 5. Emergency fallback의 비용

Emergency는 편의 기능이 아니라 문명 미발달 상태의 실제 비용이다.

- Preferred와 동일한 효율을 주지 않는다.
- 위험/불편/환경 부산물/시간 비용이 있을 수 있다.
- 행동 성공 여부와 욕구 회복량은 수단 품질에 따라 달라질 수 있다.
- 반복되는 Emergency 사용은 주민의 경험과 기억에 남아야 한다.
- 반복되는 문제는 Curiosity/Need/Observation/Experiment 입력이 될 수 있다.

따라서 Emergency를 사용했다고 해서 문제가 완전히 사라지는 구조를 피한다.

---

## 6. Environmental Residue

행동의 부산물은 필요할 경우 실제 World state로 남긴다.

최소 v1 대상은 생활 폐기물과 위생 부산물이며, 이후 다른 환경 부산물로 확장할 수 있다.

Environmental Residue는 단순 VFX가 아니다.

Core authoritative state가 가져야 할 최소 정보 방향:

- `ResidueId`
- `ResidueType`
- `WorldLocation` 또는 world-space/grid reference
- `Intensity / Amount`
- `Radius / InfluenceRange`
- `CreatedAtSimulationTime`
- `SourceResidentId` 또는 source event (필요 시)
- `DecayRate`
- `ContaminationRisk`
- `Odor/Discomfort` 계열 영향값
- `WaterContaminationPotential` 등 매체 전파 가능성

정확한 데이터 구조는 구현 단계에서 비용을 고려해 최소화하되, authority와 Save/Load를 잃지 않는다.

---

## 7. 야외 배변 환경 결과

Emergency/Natural 야외 배변이 완료되면 최소 다음이 가능해야 한다.

`Toilet intent 완료`
`→ 해당 위치 위생 부산물 생성`
`→ 같은 장소 반복 사용 시 intensity 누적`
`→ 주변 위생/불편/건강 risk 증가`
`→ 주민이 해당 장소를 인지하거나 회피`
`→ 정착지/수원과 너무 가까우면 추가 문제 발생 가능`

중요:

- 배변 위치는 가능하면 정착지 중심, 수원, 식량 보관지에서 멀리 선택한다.
- 주민이 아직 환경 위험을 이해하지 못한다면 완벽한 장소 선택을 보장하지 않는다.
- 개인 경험과 문화적 지식이 쌓이면 더 안전한 장소를 선택하도록 발전할 수 있다.

---

## 8. 누적 / 확산 / 감소

환경 잔류물은 영구 고정값이 아니다.

향후 영향 요인:

- 시간 경과
- 비 / 건조 / 온도
- 토양/물 접촉
- 청소
- 매립
- 소각
- 배수 시설
- 자연 분해

예:

`야외 배변 반복 → 환경 부산물 누적 → 비 → 주변 확산 가능 → 수원 접근 시 수질 위험 상승`

초기에는 과도하게 정밀한 모델 대신 게임플레이에 필요한 수준의 추상화된 contamination model을 사용한다.

---

## 9. 주민에게 미치는 영향

환경 상태는 주민 시스템에 입력될 수 있어야 한다.

연결 대상:

- Hygiene
- Health risk
- Emotion / discomfort
- Memory
- Avoidance / path preference
- Social conflict
- Household / community concern
- Curiosity / problem solving

예:

`특정 장소에서 반복 불편 경험 → 기억 형성 → 해당 장소 회피`

`환경 문제 반복 → 연관성 추론 → 배변 장소 분리 시도`

---

## 10. 문명 발전과의 연결

위생 기술은 시대 레벨이 올라가서 자동 해금되는 것이 아니다.

가능한 emergent chain 예:

`방광 욕구`
`→ 야외 배변`
`→ 반복적인 환경 문제`
`→ 문제 관찰`
`→ 정해진 장소 사용`
`→ 구덩이/매립 시도`
`→ 간이 변소`
`→ 청소/폐기 관습`
`→ 배수 개념`
`→ 발전된 화장실/위생 시스템`

각 단계는 필요, 환경, 지식, 재료, 개인 경험, 사회적 전파의 결과로 나타나야 한다.

`WorldTechUnlocked = true` 하나로 전역 해금하지 않는다.

---

## 11. Knowledge / Culture 연결

환경 문제에 대한 지식도 개인 상태에서 시작한다.

예:

- 특정 장소가 불쾌하거나 비위생적이라는 사실을 직접 경험
- 다른 주민의 행동과 결과를 목격
- 환경 문제와 장소 사용의 상관관계를 추정
- 더 안전한 장소 선택 방법을 학습
- 다른 주민에게 전달
- 공동체 규범으로 발전

따라서 위생 문화도 `Memory / Belief / Witness / Rumor / Teaching / Imitation` 기반으로 확장한다.

---

## 12. Core / Unreal Authority 경계

### Core authoritative

장기적으로 Core가 소유해야 하는 것:

- residue 존재 여부
- residue type / amount / decay
- contamination 상태
- 주민에게 미치는 simulation consequence
- Save/Load continuity
- knowledge/health/civilization 입력

### Unreal World / Presentation

Unreal은 다음을 담당한다.

- 실제 위치 표현
- decal/mesh/particle/VFX 등 시각화
- 오염 범위 디버그 표시
- action animation / movement / interaction
- Core state를 읽어 presentation 갱신

Unreal-only mutable state가 Core의 환경 authority를 대신해서는 안 된다.

---

## 13. Save / Load

환경 잔류물이 simulation consequence를 가진 순간부터 Save/Load 대상이다.

저장 후 다시 시작했을 때:

- 환경 상태 위치가 사라지지 않는다.
- 누적량이 초기화되지 않는다.
- decay 시간 흐름이 일관적이다.
- 관련 주민 기억/건강 상태와 모순되지 않는다.

Presentation-only VFX는 재생성할 수 있지만 authoritative residue state는 복원되어야 한다.

---

## 14. 구현 단계 경계

### PR #66 — World Affordance Fallback v1

목표:

- 자동 bootstrap 생활시설 생성 제거
- `Preferred → Primitive → Natural → Emergency` 선택 구조
- 실제 존재하는 수단 우선
- 시설이 없어도 가능한 Intent는 Emergency 행동으로 수행
- World object를 암묵적으로 생성하지 않음
- 주민별 active affordance tier read 가능

**#66은 환경 residue authority까지 구현한 것으로 간주하지 않는다.**

### Next — Environmental Residue v1

최소 목표:

- 야외 배변 완료 사건을 환경 consequence로 연결
- Core-owned residue model
- 위치 / 양 / 강도 / decay 최소 상태
- 중첩/누적
- Save/Load
- Observer/World read path
- 주변 Hygiene/Health/Discomfort에 사용할 수 있는 query contract

### 이후

- 날씨/물/토양 영향
- 청소/매립/폐기
- 위생 지식과 발견
- 간이 변소/화장실/배수 progression
- VFX/시각적 흔적

---

## 15. Acceptance principles

- 침대가 없는데 침대가 생겨나지 않는다.
- 화장실이 없는데 화장실을 쓰는 척하지 않는다.
- 음식/물이 없는데 Emergency가 자원을 생성하지 않는다.
- 야외 배변은 후속 환경 consequence를 만들 수 있는 실제 사건이다.
- 더 좋은 시설이 생기면 AI가 기존 Emergency보다 그것을 우선한다.
- 같은 세계를 Save/Load해도 환경의 결과가 이어진다.
- 환경 문제는 주민의 경험과 문명 발전 원인이 될 수 있다.
- 플레이어가 직접 Tech Tree 버튼을 눌러 위생 기술을 해금하지 않는다.

---

## 16. 제품 방향 요약

LifeLens의 문명 발전은 "시설이 없어서 불편하다"는 사실을 숨기는 것이 아니라 그 불편을 simulation에 남기는 데서 시작한다.

**시설 부재 → 원시 행동 → 실제 부작용 → 관찰/기억/문제 인식 → 실험/지식 → 더 나은 시설 → 문화/문명**

이 인과 연결을 유지하는 것이 World interaction, Environment, Health, Knowledge, Civilization 구현의 공통 기준이다.
