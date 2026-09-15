# LifeLens Hardcoding Policy

LifeLens의 목표는 숫자 리터럴 자체를 없애는 것이 아니라, **변경 가능한 제품 규칙과 표현 튜닝이 구현 코드에 흩어지지 않게 하는 것**이다.

## 1. 분류

### A. Stable protocol / determinism contracts — 코드에 유지

다음 값은 저장 호환성, seed 재현성, 좌표계 또는 직렬화 계약을 정의하므로 임의 외부 설정으로 만들지 않는다.

- World generation version
- deterministic hash/mixing constants and domain separators
- Core chunk coordinate span처럼 생성 규칙의 identity에 직접 포함되는 값
- snapshot/schema version과 명시적인 migration discriminator

이 값을 변경할 때는 일반 튜닝이 아니라 versioned migration으로 취급한다.

### B. Core simulation policy / balance — versioned Ruleset으로 이동

다음 값은 로직이 아니라 시뮬레이션 정책/밸런스 데이터다.

- need decay/rate
- utility curve와 urgency threshold
- life-stage age boundaries/profile multipliers
- relationship / romance / family transition thresholds와 weights
- pregnancy/fertility/gestation tuning
- social/civilization cooldown 및 utility weights

Core는 Unreal 의존성을 갖지 않는다. 최종 구조는 immutable `SimulationRuleset`을 Core 생성 시 주입하고, ruleset identity/version을 snapshot에 고정해 동일 save/replay가 같은 규칙을 사용하게 한다.

### C. Unreal runtime / presentation tuning — Config 또는 Data Asset

런타임 속도, refresh interval, 카메라 framing, presentation 거리/LOD/culling처럼 authority를 만들지 않는 값은 Unreal Config 또는 Data Asset에서 읽는다. 정상적인 튜닝의 source of truth는 Config/Data Asset으로 두고, C++에는 Config 누락 시 조용한 오동작을 막기 위한 동일값 safety fallback만 허용한다. fallback 값은 별도의 독립 튜닝값으로 취급하지 않는다.

### D. Content / asset references — soft reference / catalog

메시, 머티리얼, 맵, 이름 풀처럼 교체 가능한 콘텐츠는 코드 문자열 경로/배열에 직접 박지 않는다. `TSoftObjectPtr`, Data Asset, catalog 같은 content-owned source of truth를 사용한다.

### E. Test fixtures — 테스트 내부 하드코딩 허용

특정 회귀를 재현하기 위한 seed, 좌표, 임계값은 테스트 fixture 안에서는 허용한다. 단 production code의 기본값을 테스트 fixture가 정의해서는 안 된다.

## 2. 변경 원칙

1. 동작 변경과 외부화 작업을 가능한 한 분리한다.
2. 기존 값을 외부화하는 첫 커밋은 behavior-preserving이어야 한다.
3. Core ruleset 변경은 ruleset version/snapshot compatibility를 함께 검토한다.
4. presentation config는 Core authority나 world-generation identity를 바꾸지 않는다.
5. 같은 의미의 값은 한 곳에서만 소유한다.
6. Config/Build/CI, shared types, cross-owner path는 기존 Integration Request / review 규칙을 따른다.

## 3. 현재 적용 순서

1. World/runtime presentation knobs 외부화
2. Core `SimulationRuleset` 도입 및 Needs/Utility AI부터 이동
3. LifeStage / Relationship / Romance / Pregnancy / Family 정책 이동
4. legacy compatibility projection/outcome 상수 제거 (`#93` 이후)
5. hardcoded asset paths/name catalogs를 content catalog로 이동
6. 신규 magic tuning value 유입 방지 validator 추가
