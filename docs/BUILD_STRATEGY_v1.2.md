# LIFELENS — 빌드 / 검증 전략 v1.2

이 문서는 `LIFELENS_SPEC_v1.1.md`의 72절(빌드 전략), 73절(Android Build Pipeline), 74절(FAST TEST / FULL BUILD), 75절 P0, 76절(Phase 완료 조건)을 대체한다. 충돌 시 이 문서가 우선한다.

## 배경 — 왜 바꾸는가

기존 파이프라인은 Unreal Engine 자체를 CI에서 소스 컴파일했다. 이것이 빌드 시간의 대부분이었고, 무료 러너 환경에서는 캐시로도 해결되지 않는다(엔진 산출물이 캐시 한도를 넘는다). 또한 개발 환경이 폰 + GitHub 저장소뿐이라 "코드 수정 → 확인"의 유일한 경로가 전체 빌드였다.

v1.2는 세 가지를 바꾼다.

1. 엔진은 컴파일하지 않고 Epic 프리빌드 이미지를 받아 쓴다.
2. 시뮬레이션 코어를 Unreal 밖으로 분리해 초 단위로 검증한다.
3. APK 빌드를 "확인"이 아니라 "마일스톤 산출물"로 격하한다.

## 검증 계층 (빠른 것부터)

| 계층 | 무엇을 검증 | 소요 | 실행 위치 | 트리거 |
|---|---|---|---|---|
| L0 | LifeLensCore 컴파일 + 유닛 테스트 | 수십 초 | Termux(폰) / CI | 수정할 때마다 |
| L1 | 콘솔 하네스 하루 시뮬레이션 로그 | 수십 초 | Termux(폰) / CI | 로직 변경 시 |
| L2 | Unreal 프로젝트 컴파일만 (쿡 없음) | 5~10분 | CI | UE 모듈 변경 시 |
| L3 | Android FAST TEST APK | 30~60분 | CI 수동 | 마일스톤 |
| L4 | FULL BUILD | 1시간+ | CI 수동 | 릴리스 |

원칙: 상위 계층에서 잡을 수 있는 문제를 하위 계층에서 발견하면 그것은 프로세스 실패다. 로직 버그를 APK에서 처음 발견했다면 L0/L1 테스트가 부족한 것이다.

## 엔진 공급 방식

- 이미지: `ghcr.io/epicgames/unreal-engine:dev-slim-<버전>` (Epic 계정을 GitHub에 연결한 계정의 토큰으로 pull). 정확한 태그는 작업 시점에 GHCR에서 확인하고 `docs/`에 기록한다.
- 엔진 버전은 프로젝트 전체에서 하나로 고정하고 `LifeLens.uproject`의 `EngineAssociation`과 일치시킨다.
- Android SDK/NDK 버전은 해당 엔진 버전의 `Engine/Extras/Android/SetupAndroid.sh`가 요구하는 값을 그대로 쓴다. 임의로 최신 NDK를 설치하지 않는다.

## 캐시 정책 (GitHub actions/cache, 저장소당 10GB 한도)

- 캐시 대상: `Intermediate/`, `DerivedDataCache/`, `Saved/Cooked/`
- 캐시 키: 엔진 버전 + `Source/**` 해시 + `Content/**` 해시 (restore-keys로 부분 일치 허용)
- 캐시 대상 아님: 엔진 이미지 자체(한도 초과), `Binaries/`(재생성이 싸다)

## FAST TEST 정의 (재정의)

- 맵: `Content/Maps/L_FastTest.umap` 하나만 쿡 (`MapsToCook` 고정)
- 캐릭터: 개발용 1명, 저품질 메시
- 텍스처 포맷: ASTC 하나
- 아키텍처: arm64-v8a 하나
- 빌드 구성: Development, Pak 암호화 없음
- 비활성 플러그인: 사용하지 않는 모든 플러그인 (특히 온라인/에디터 전용)

## Phase 완료 조건 (재정의)

각 Phase는 다음을 모두 만족해야 완료다.

1. 해당 Phase의 로직이 `LifeLensCore` 테스트로 커버된다.
2. 콘솔 하네스에서 해당 동작이 로그로 확인된다.
3. `core-tests.yml`이 `main`에서 통과한다.
4. **P4 이후의 Phase에 한해** FAST TEST APK가 폰에서 실행되고 크래시 없이 해당 동작을 보여준다. P0~P3는 APK 없이 L0~L2로 완료 판정한다.

## Phase 순서 조정

SPEC 75절의 P0 "Android APK 성공"은 마일스톤이 아니라 **인프라 작업(`tasks/TASK_01`)**으로 이동한다. 실질적인 첫 Phase는 다음과 같다.

- P0' — `LifeLensCore` 골격 + 콘솔 하네스 + 테스트 (TASK_02)
- P1' — Needs 시스템 (코어)
- P2' — Utility AI + 간단한 Plan 실행 (코어, Smart Object는 추상 인터페이스로)
- P3' — 개발 캐릭터 1명이 하루를 살아가는 로그 (코어)
- P4' — 위 코어를 Unreal 모듈에 링크, FAST TEST APK에서 같은 하루가 화면에 보임

즉 SPEC의 P1~P4(집, 캐릭터, Navigation, Needs, Utility AI)는 코어에서 먼저 텍스트로 완성한 뒤 Unreal에 붙인다. 이 순서가 폰 단독 개발 환경에서 유일하게 반복 가능한 루프다.

## 개발 환경 옵션

- 폰 단독: Termux + clang + cmake + git → L0/L1 전부 가능
- CI: L0~L4
- 원격 PC(있을 경우): Unreal Editor + Play In Editor → L2~L3 를 초 단위로 대체. 이 옵션이 생기면 FAST TEST APK 빈도를 더 줄인다.
