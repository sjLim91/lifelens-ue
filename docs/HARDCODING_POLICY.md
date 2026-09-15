# LifeLens Hardcoding Policy

LifeLens의 목표는 숫자 리터럴 자체를 없애는 것이 아니라, **변경 가능한 제품 규칙과 표현 튜닝이 구현 코드에 흩어지지 않게 하는 것**이다.

## 1. 분류

### A. Stable protocol / determinism contracts — 코드에 유지

다음 값은 저장 포맷, seed 재현성, 좌표계 또는 직렬화 계약을 정의하므로 임의 외부 설정으로 만들지 않는다.

- World generation version
- deterministic hash/mixing constants and domain separators
- Core chunk coordinate span처럼 생성 규칙의 identity에 직접 포함되는 값
- snapshot/schema version과 format discriminator

출시 후 실제 사용자 save/replay가 존재하는 기준선이 생기면 이 값의 변경은 일반 튜닝이 아니라 명시적 compatibility/migration 판단 대상이다.

**현재 pre-release 예외:** 아직 실제 사용자에게 배포된 LifeLens 제품 save가 없으므로 개발 중 과거 snapshot/save 포맷의 migration/backward compatibility는 제품 요구사항이 아니다. 현재 구조를 더 복잡하게 만드는 개발 전용 migration 코드는 보존하지 않으며, 현재 포맷을 직접 갱신한다.

### B. Core simulation policy / balance — versioned immutable Ruleset

다음 값은 로직이 아니라 시뮬레이션 정책/밸런스 데이터다.

- need decay/rate
- utility curve와 urgency threshold
- life-stage age boundaries/profile multipliers
- relationship / romance / family transition thresholds와 weights
- pregnancy/fertility/gestation tuning
- social/civilization cooldown 및 utility weights

Core는 Unreal 의존성을 갖지 않는다. `Simulation`은 생성 시 immutable `SimulationRuleset`을 값으로 주입받고 실행 중 이를 변경하지 않는다. 저장 시 ruleset version만 기록하는 것이 아니라 실제 ruleset 값 전체를 snapshot에 고정해 같은 save/replay가 같은 규칙을 계속 사용하게 한다.

Unreal runtime은 Config/Data Asset에서 변경 가능한 제품 튜닝을 읽어 Core ruleset 값으로 변환한다. Core의 동일 숫자 기본값은 Config가 누락되거나 잘못된 경우를 위한 safety fallback이며 정상 튜닝 source of truth가 아니다.

### C. Unreal runtime / presentation tuning — Config 또는 Data Asset

런타임 속도, refresh interval, 카메라 framing, presentation 거리/LOD/culling처럼 authority를 만들지 않는 값은 Unreal Config 또는 Data Asset에서 읽는다. 정상적인 튜닝의 source of truth는 Config/Data Asset으로 두고, C++에는 Config 누락 시 조용한 오동작을 막기 위한 동일값 safety fallback만 허용한다. fallback 값은 별도의 독립 튜닝값으로 취급하지 않는다.

### D. Content / asset references — soft reference / catalog

메시, 머티리얼, 맵, 이름 풀처럼 교체 가능한 콘텐츠는 코드 문자열 경로/배열에 직접 박지 않는다. `TSoftObjectPtr`, Data Asset, catalog 같은 content-owned source of truth를 사용한다.

### E. Test fixtures — 테스트 내부 하드코딩 허용

특정 회귀를 재현하기 위한 seed, 좌표, 임계값은 테스트 fixture 안에서는 허용한다. 단 production code의 기본값을 테스트 fixture가 정의해서는 안 된다.

## 2. 변경 원칙

1. 동작 변경과 외부화 작업을 가능한 한 분리한다.
2. 기존 값을 외부화하는 첫 커밋은 behavior-preserving이어야 한다.
3. Core ruleset 변경은 ruleset version/snapshot determinism을 함께 검토한다.
4. 실제 배포 기준선 이전에는 개발 전용 구버전 migration layer를 새로 만들지 않는다.
5. 실제 배포 기준선 이후에는 save/replay compatibility 정책을 명시적으로 결정한 뒤 format/ruleset version을 변경한다.
6. presentation config는 Core authority나 world-generation identity를 바꾸지 않는다.
7. 같은 의미의 값은 한 곳에서만 소유한다.
8. Config/Build/CI, shared types, cross-owner path는 기존 Integration Request / review 규칙을 따른다.

## 3. 현재 적용 순서

1. World/runtime presentation knobs 외부화
2. Core `SimulationRuleset` 도입 및 Needs/Utility AI부터 이동
3. LifeStage / Relationship / Romance / Pregnancy / Family 정책 이동
4. legacy compatibility projection/outcome 상수 제거 (`#93` 이후)
5. hardcoded asset paths/name catalogs를 content catalog로 이동
6. 신규 magic tuning value 유입 방지 validator 추가

## 4. Cleanup B 적용 상태

Cleanup B의 목표 구조는 다음과 같다.

`DefaultGame.ini -> ULLCoreBridgeSubsystem Config -> SimulationRuleset value -> const Simulation::ruleset_`

- Needs/UtilityAI의 변경 가능한 제품 튜닝은 `DefaultGame.ini`가 정상 source of truth다.
- Core default 값은 pure C++ 테스트/독립 실행과 Config failure를 위한 fallback이다.
- snapshot은 실행 중인 ruleset 전체 값을 저장한다.
- load는 snapshot에 저장된 ruleset으로 Simulation을 먼저 구성한 후 상태를 복원한다.
- 개발 중 v1~v6 snapshot migration 및 Unreal SaveGame v1 replay migration은 미배포 pre-release 부채이므로 제거한다.
- Social/Fun projection, LifeStage/Relationship/Romance/Pregnancy/Family의 나머지 하드코딩은 각 계획된 후속 Cleanup 단계에서 처리한다.
