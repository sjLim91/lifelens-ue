# TASK 02 — LifeLensCore: Unreal 무관 시뮬레이션 라이브러리 + 콘솔 하네스 + 테스트

브랜치: `task/02-core-sim`
선행: TASK_01 (core-tests.yml 존재)
참고: SPEC 8, 13, 23, 24, 25, 26, 27, 29, 31절 / `docs/BUILD_STRATEGY_v1.2.md`

## 목표

SPEC의 시뮬레이션 로직을 Unreal에 의존하지 않는 표준 C++17 정적 라이브러리로 만든다. 콘솔 하네스가 "개발 캐릭터 1명이 작은 집에서 하루를 산다"를 텍스트 로그로 출력하고, 유닛 테스트가 각 시스템을 검증한다. 이 라이브러리는 Termux(Android clang)에서 그대로 빌드되어야 한다.

이 TASK의 범위는 **골격 + Needs + Utility AI 최소 구현**이다. Emotion, Memory, Relationship은 인터페이스만 잡고 구현은 이후 TASK로 미룬다.

## 제약

- 엔진 헤더, `UObject`, `TArray`, `FString`, `FName` 사용 금지. `std::vector`, `std::string`, `std::unordered_map`만.
- 외부 의존성 없음. 테스트 프레임워크도 헤더 하나짜리(예: doctest 단일 헤더를 `tests/` 에 포함) 이상은 쓰지 않는다.
- 난수는 `std::mt19937_64`를 `WorldSeed`로 초기화해 사용한다. 같은 seed → 같은 로그가 나와야 한다(결정론).
- 시간은 실제 시간이 아니라 시뮬레이션 tick(분 단위 정수)으로 관리한다.

## 디렉터리

```
Source/LifeLensCore/
  CMakeLists.txt
  include/lifelens/
    Ids.h            # CharacterId, HouseholdId, ObjectId (uint64)
    Needs.h
    Personality.h
    Emotion.h        # 인터페이스만
    Memory.h         # 인터페이스만
    Relationship.h   # 인터페이스만 (다차원 구조체 정의는 포함)
    Character.h
    SmartObject.h    # 추상: 종류, 위치(간단한 격자 좌표), 예약 상태, Needs 효과
    World.h          # 시간, seed, 캐릭터/오브젝트 컨테이너
    UtilityAI.h      # Goal 후보 + 점수 계산
    Planner.h        # Goal → Action 시퀀스
    Simulation.h     # tick 진행 진입점
  src/               # 위 헤더 구현
  harness/
    main.cpp         # ll_harness
  tests/
    test_needs.cpp
    test_utility.cpp
    test_determinism.cpp
```

## 구현 명세

### Needs (SPEC 23절 중 물리 욕구 5개만)

`Hunger, Thirst, Sleep, Bladder, Hygiene` 각각 0.0~1.0. tick마다 증가율은 캐릭터의 Physical Trait(신진대사, 수면 성향)로 보정한다. 임계값(예: 0.7 이상)에서 Utility 가중치가 급격히 커지는 곡선을 쓴다(선형 금지).

### Personality (SPEC 24절 전체 차원, 값만)

14개 차원을 0.0~1.0으로 보관. 이 TASK에서는 Utility 점수 보정에 `Introversion`, `Conscientiousness`만 실제로 사용하고 나머지는 저장만 한다. 생성은 `std::normal_distribution(0.5, 0.15)` 후 clamp (SPEC 13절: Uniform 금지).

### SmartObject (추상)

```
enum class ObjectKind { Bed, Toilet, Sink, Fridge, Chair, Table, Sofa };
struct SmartObject {
  ObjectId id; ObjectKind kind; GridPos pos;
  std::optional<CharacterId> reservedBy;
  NeedsDelta effectPerTick;   // 사용 중 tick당 Needs 변화
  int useDurationTicks;
};
```

이동은 격자 맨해튼 거리 × 이동 tick으로 추상화한다. Navigation은 이 TASK 범위가 아니다.

### Utility AI (SPEC 26, 27절)

Goal 후보: `Eat, Drink, Sleep, UseToilet, Wash, Idle`. 각 Goal 점수 = Needs 곡선 값 × Personality 보정 × 시간대 보정 × (해당 Smart Object 사용 가능 여부). 최고 점수 Goal을 선택하되, SPEC 28절을 위해 상위 2개 사이에서 작은 확률로 2위를 선택하는 노이즈를 넣는다(결정론 유지: seed 기반).

### Planner (SPEC 27절 예시 흐름)

Goal → `[FindObject, Reserve, MoveTo, Use, Release]` 액션 시퀀스. 사용 가능한 오브젝트가 없으면 실패 → 다음 tick에 재평가(무한 반복 방지: 같은 Goal 연속 실패 3회면 해당 Goal 점수에 일시 페널티).

### Simulation

`step()`이 1 tick(1분) 진행: Needs 갱신 → (AI 주기: 5 tick마다) Utility 재평가 → 현재 Action 진행 → 이벤트 로그 발행. 이벤트 로그는 콜백으로 밖에 내보낸다(하네스는 stdout, Unreal은 나중에 화면).

### 콘솔 하네스 출력 형식

```
[Day 1 07:12] 개발캐릭터 기상 (Sleep 0.12)
[Day 1 07:14] 개발캐릭터 → Toilet 이동 (Bladder 0.81)
[Day 1 07:19] 개발캐릭터 Toilet 사용 시작
[Day 1 07:23] 개발캐릭터 Toilet 사용 종료 (Bladder 0.05)
...
[Day 1 END] Hunger 0.31 Thirst 0.22 Sleep 0.08 Bladder 0.10 Hygiene 0.35
```

옵션: `--days N --seed S --tick-log` (tick-log는 매 tick Needs 출력).

## 테스트 (최소)

- `test_needs`: 24시간 방치 시 각 Needs가 1.0에 도달하고, 오브젝트 사용 시 감소한다.
- `test_utility`: Hunger 0.9 / 나머지 0.2 일 때 `Eat`이 선택된다. 모든 오브젝트 예약 상태면 `Idle`로 떨어진다.
- `test_determinism`: 같은 seed로 두 번 3일 실행 시 로그가 완전히 동일하다. 다른 seed면 다르다.

## Termux 검증 절차 (성준이 폰에서 실행)

```
pkg install git clang cmake make
git clone <repo> && cd lifelens-ue
cmake -S Source/LifeLensCore -B build && cmake --build build -j
./build/harness/ll_harness --days 1 --seed 42
```

이 절차를 `Source/LifeLensCore/README.md`에 그대로 적는다.

## 완료 조건

- [ ] `core-tests.yml` 통과.
- [ ] Termux에서 위 절차가 그대로 성공한다 (성준이 확인, 스크린샷을 PR에 첨부).
- [ ] 하네스 하루 로그에서 캐릭터가 하루 동안 기상 → 화장실 → 세면 → 식사 → … → 취침을 **시간표 없이** Needs만으로 수행한다 (SPEC 77절).
- [ ] 같은 Goal이 5회 이상 연속 반복되는 구간이 없다.
- [ ] 같은 seed 재실행 시 로그가 동일하다.
