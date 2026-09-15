# LifeLens Open-ended Invention / Emergent Artifact v1

## 목적

이 문서는 LifeLens에서 주민이 **개발자가 미리 정의한 Recipe 목록만 따라 제작하는 것이 아니라**, 필요·호기심·환경·재료·물성·과거 경험을 바탕으로 실험하고, 현실 역사에 존재하지 않았던 형태의 물건이나 도구까지 스스로 만들어 사용할 수 있게 하는 canonical 설계 기준이다.

이 문서는 `docs/CIVILIZATION_PROGRESSION_v1.md`의 Experiment / Discovery / Crafting 방향을 구체화한다.

핵심 원칙:

`Problem / Need`
`→ Observation`
`→ Material selection`
`→ Structural combination`
`→ Experiment`
`→ Physical/functional result`
`→ Failure or useful effect`
`→ Memory / Belief / Technique`
`→ Reproduction`
`→ Naming / Teaching / Cultural adoption`

LifeLens의 발명은 **미리 존재하는 아이템 이름을 해금하는 과정이 아니라, 실제 기능을 가진 artifact가 시행착오를 통해 출현하는 과정**이어야 한다.

---

## 1. 허용되는 발명의 범위

주민은 다음을 만들 수 있어야 한다.

- 현실세계에 존재하는 일반적인 도구와 유사한 물건
- 현실 역사에서 다른 시대/문화에 존재했던 형태와 유사한 물건
- 개발자가 정확한 Recipe로 미리 등록하지 않은 조합
- 현실에서 흔히 쓰이지 않는 구조지만 주어진 물성과 물리 규칙 안에서 기능하는 도구
- 특정 LifeLens 세계에서만 등장하는 독특한 문화적 artifact

예:

`높은 곳의 열매를 따기 어려움`
`→ 긴 나뭇가지 사용`
`→ 실패`
`→ 끝에 무게를 추가`
`→ 섬유로 돌을 결합`
`→ 당기기/치기 실험`
`→ 채집 효율 증가`
`→ 반복 성공`
`→ 해당 사회의 전용 채집 도구로 정착`

그 결과물이 현실세계의 기존 도구와 정확히 일치할 필요는 없다.

---

## 2. 금지되는 방식

다음 방식은 LifeLens의 기본 발명 모델이 아니다.

- `TechLevel >= 3`이면 자동으로 특정 도구 해금
- 모든 캐릭터가 처음부터 전체 Recipe 목록을 알고 있음
- 재료 없이 이름만 가진 아이템 생성
- 실험 없이 확률 한 번으로 완성품 즉시 생성
- 한 사람이 발견하면 전 세계 주민에게 즉시 전역 해금
- 물리적/세계적 근거 없이 마법처럼 기능이 생김

즉:

`새 아이템 이름 생성 → 성능 부여`

가 아니라

`재료 + 구조 + 연결 + 가공 + 사용 방식 → 실제 성능 결과 → 주민이 기능을 학습`

이어야 한다.

---

## 3. Artifact는 이름보다 구조가 먼저다

새로운 도구/artifact의 최소 표현은 이름이 아니라 **구성 구조와 기능적 속성**이다.

예시 데이터 방향:

### Material parts
- Wood / Branch
- Stone / Flake
- Fiber / Rope-like binding
- Bone
- Clay
- Metal
- Hide
- 기타 발견 가능한 재료

### Geometry / role
- Handle
- Edge
- Point
- Head
- Container
- Surface
- Joint
- Counterweight
- Frame
- Flexible section

### Connection
- Loose placement
- Binding
- Slot / insertion
- Adhesion
- Interlocking
- Fastening
- Heat joining
- 기타 문명이 발견한 연결 방식

### Functional properties
- Mass
- Length / reach
- Sharpness
- Hardness
- Flexibility
- Brittleness
- Impact force
- Cutting potential
- Digging potential
- Carrying capacity
- Heat resistance
- Water containment
- Durability
- Stability
- Efficiency

실제 구현은 단계적으로 단순화할 수 있지만, **artifact의 기능은 가능한 한 구성과 물성에서 파생**되어야 한다.

---

## 4. 물리적으로 가능한 세계 안에서만 발명한다

Open-ended라고 해서 제한이 없는 것은 아니다.

허용:
- 역사적으로 실제 존재하지 않았던 형태
- 개발자가 미리 예상하지 못한 조합
- 특정 세계에서만 나타난 독특한 도구

금지:
- 현재 LifeLens 물리법칙에 없는 반중력
- 아무 에너지원 없는 무한동력
- 재료 특성과 무관한 순간이동 장치
- 전제 지식/재료/에너지 없이 현대 기술이 갑자기 출현

향후 LifeLens 세계의 물리 규칙이 확장되면 가능한 발명의 범위도 그 규칙 안에서 확장될 수 있다.

---

## 5. 발명은 문제에서 시작한다

실험 동기는 단순 `Invent()` 랜덤 이벤트가 아니다.

입력 예:

- Hunger / Thirst / Cold / Hygiene / Safety
- 반복적인 작업의 높은 effort
- 자원 접근 실패
- 기존 도구 파손
- 환경 오염
- 운반 한계
- 저장/부패 문제
- 건축 실패
- 관찰한 자연 현상
- Curiosity
- 다른 주민의 행동 관찰

예:

`반복적인 야외 배변 → 오염 증가 → 불쾌감/회피 → 지정 위치 사용 → 구덩이 시도 → 덮기/배수 시도 → primitive sanitation artifact`

발명은 기존 환경 consequence 시스템과 직접 연결된다.

---

## 6. Experiment cycle

한 번의 실험은 최소한 다음 중 일부 결과를 남긴다.

- Success
- Partial success
- No useful effect
- Breakage
- Material loss
- Injury/risk
- Unexpected property discovery
- Secondary use discovery

실패도 학습이다.

예:

`돌 A + 충격 → 산산이 깨짐`

은 단순 실패가 아니라:

- 이 재료는 brittle하다.
- 특정 충격 방향에서 날카로운 파편이 생길 수 있다.
- 손으로 직접 잡으면 위험하다.

같은 Memory/Belief/Material Knowledge의 근거가 될 수 있다.

---

## 7. 기능 검증이 이름보다 우선한다

처음 만들어진 artifact는 주민도 그것이 무엇인지 완전히 이해하지 못할 수 있다.

초기 상태 예:

- Unknown object
- Experimental combination
- Useful for cutting?
- Useful for reaching?

반복 사용을 통해 기능이 검증되면:

- Cutting tool
- Digging tool
- Container
- Hunting tool
- Construction tool

같은 functional concept가 형성될 수 있다.

따라서 내부 artifact identity와 문화적 이름을 분리한다.

---

## 8. 주민/문화가 이름을 붙일 수 있다

artifact가 반복 사용되고 사회적으로 공유되면 이름이 생길 수 있다.

단계 예:

`Experimental Artifact #41`
`→ 민재가 반복 사용`
`→ 다른 주민이 모방`
`→ 공동체에서 기능 인식`
`→ 별칭/명칭 형성`
`→ 문화적 표준 도구`

서로 떨어진 집단이 같은 문제를 다른 방식으로 해결하면 **서로 다른 형태와 이름의 도구**를 가질 수 있다.

향후 언어 시스템이 발전하면 명칭도 문화/언어별로 분화될 수 있다.

---

## 9. 개인 지식과 사회 지식은 분리한다

새 도구를 한 사람이 만들었다고 전 세계가 제작법을 알면 안 된다.

가능한 knowledge level:

- Observed artifact
- Saw successful use
- Understands purpose
- Understands materials
- Can reproduce poorly
- Can reproduce reliably
- Can improve design
- Can teach others

전파 경로:

- 직접 목격
- 모방
- 설명
- 시범
- 공동 작업
- 부모 → 자녀
- 스승 → 학습자
- 기록/교육 체계

발명자의 사망이나 지식 전파 실패로 제작법이 사라질 수도 있다.

---

## 10. Variation / mutation / improvement

도구는 한 번 발견된 형태로 영구 고정되지 않는다.

복제 과정에서 다음이 달라질 수 있다.

- 재료
- 길이
- 무게
- 결합 방식
- 날/끝 형상
- 부품 수
- 제작 정밀도

변형 결과가 더 좋은 성능을 보이면 새로운 variant가 문화에 퍼질 수 있다.

즉 발전은:

`Artifact A unlock → Artifact B unlock`

보다

`A → 복제 변형 → A1/A2/A3 → 실제 성능 비교 → 선택/전파`

에 가깝다.

---

## 11. Multi-purpose artifact

하나의 도구는 개발자가 지정한 한 가지 `ToolType`만 가질 필요가 없다.

예:
- 날카로운 돌: cutting + scraping + drilling 가능성
- 긴 막대: reaching + leverage + defense + digging 가능성
- 용기: water storage + seed storage + transport

주민은 제작자가 의도하지 않았던 **secondary use**를 발견할 수 있다.

이것은 새로운 발명의 출발점이 될 수 있다.

---

## 12. 환경과 그래픽 표현

새 artifact가 실제 세계에 존재하면 Unreal에서도 시각적으로 존재해야 한다.

원칙:

`Core authoritative artifact structure/state`
`→ World physical representation`
`→ Unreal visual assembly`

가능한 표현 전략:

- modular mesh parts
- procedural/parametric assembly
- predefined primitive visual grammar
- material/size/attachment variation
- LOD / simplified distant proxy

중요:
- 그래픽 mesh가 artifact 기능의 authority가 아니다.
- Core 구조/물성이 먼저이고 그래픽은 이를 표현한다.
- Save/Load 후 같은 artifact state에서 동일/호환 visual을 재구성한다.
- Android에서는 완전한 arbitrary geometry 생성보다 modular assembly를 우선할 수 있다.

이 원칙은 `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`와 동일한 authority 철학을 따른다.

---

## 13. Persistence

발명품과 관련 지식은 Save/Load 이후에도 유지되어야 한다.

저장 대상 방향:

- Artifact stable id
- 구성 재료/부품
- 연결/가공 상태
- 주요 물성/내구도
- 위치/소유자/보관 위치
- creator/discoverer provenance
- 사용 기록 또는 학습에 필요한 요약 정보
- 알려진 기능
- 개인별 knowledge/skill state

Save/Load가 artifact를 일반 prefab 이름으로 축약해서 독특한 구조를 잃게 해서는 안 된다.

---

## 14. Determinism

동일 WorldSeed + 동일 상태 + 동일 입력/사건 순서에서는 발명 실험 결과도 재현 가능해야 한다.

랜덤성은 허용하지만 Core deterministic RNG 또는 명시적 deterministic roll을 사용한다.

Unreal frame timing, mesh 이름, Actor 생성 순서가 발명의 성공 여부를 결정하면 안 된다.

---

## 15. 기존 Recipe 시스템의 역할

Recipe 자체를 완전히 금지하는 것은 아니다.

Recipe/Technique는 다음을 표현할 수 있다.

- 이미 학습된 안정적 재현법
- 문화적으로 표준화된 제작법
- 복잡한 공정의 압축 표현

하지만 최초 발견 단계에서는 Recipe가 원인이 아니라 **실험 성공 결과를 기억/표준화한 산물**이어야 한다.

즉:

`Recipe → 발명`

보다

`실험/발명 → 재현 성공 → Technique/Recipe 형성`

이 canonical 방향이다.

---

## 16. Observer 표현

Observer는 모든 내부 계산을 보여줄 필요는 없지만 의미 있는 발명 사건을 읽을 수 있어야 한다.

예:

- `민재가 새로운 재료 조합을 시험하고 있습니다.`
- `민재의 실험 도구가 절삭 작업에서 효과를 보였습니다.`
- `서윤이 민재의 도구 사용법을 모방했습니다.`
- `이 도구의 변형이 공동체에서 반복 제작되기 시작했습니다.`

선택한 artifact에서는 향후:

- 구성
- 재료
- 발견자
- 주요 기능
- 내구도
- 누가 사용법을 알고 있는지
- 문화적 보급 정도

등을 확인할 수 있다.

---

## 17. 구현 단계

Open-ended Invention은 한 번에 완전 자유형 물리 시뮬레이션으로 구현하지 않는다.

### Phase A — property-driven experiment
- Material property 모델 강화
- problem/need에서 experiment intent 발생
- 소수 component role / connection grammar
- 기능 결과 계산
- 성공/실패 Memory 형성

### Phase B — Emergent Artifact
- stable artifact id
- multi-part composition
- durability / wear / breakage
- discovered affordance/function
- personal reproduction knowledge

### Phase C — Cultural reproduction
- imitation / teaching
- variant 제작
- technique standardization
- 지식 손실 / 재발견

### Phase D — Open-ended improvement
- artifact variation selection
- multi-purpose discovery
- 복합 도구
- construction/machine component로 확장

### Phase E — advanced emergent engineering
- 여러 artifact/기계 부품 간 상호작용
- 에너지/힘 전달
- 생산설비
- 새로운 사회별 기술 계통

---

## 18. 완료 기준

최종적으로 LifeLens의 발명 시스템은 다음 질문에 YES라고 답할 수 있어야 한다.

1. 개발자가 정확한 완성품 Recipe를 등록하지 않아도 유용한 새 도구가 출현할 수 있는가?
2. 같은 문제를 서로 다른 세계/주민이 다른 구조로 해결할 수 있는가?
3. 기능이 재료·구조·사용 결과에서 파생되는가?
4. 실패가 다음 실험의 지식으로 남는가?
5. 발명자의 지식이 자동 전역 해금되지 않는가?
6. 다른 주민이 관찰/학습/모방해야 기술이 퍼지는가?
7. 도구가 변형·개선·퇴화할 수 있는가?
8. 발명품이 실제 World에 존재하고 그래픽으로 표현되는가?
9. Save/Load와 세대 교체 뒤에도 artifact/지식 역사가 이어지는가?
10. 현실 역사에 없던 형태라도 LifeLens 물리 규칙 안에서 기능하면 존재할 수 있는가?

---

## Canonical causal loop

`Need / Problem / Curiosity`
`→ Observe environment/material`
`→ Hypothesis`
`→ Combine / shape / process materials`
`→ Experiment`
`→ Physical result`
`→ Success / partial success / failure`
`→ Memory + Belief + Material Knowledge`
`→ Repeated use`
`→ Artifact function recognized`
`→ Reproduction technique`
`→ Teaching / imitation`
`→ Cultural adoption`
`→ Variation / improvement`
`→ New problems and new inventions`

이 루프를 훼손하는 단순 전역 Tech Unlock 또는 Recipe-only 구조를 LifeLens의 기본 문명 발전 모델로 사용하지 않는다.
