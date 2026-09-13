# AGENTS.md — LifeLens 작업 규칙

이 저장소에서 작업하는 모든 AI 에이전트(Codex / Astra / Claude Code 등)는 이 파일을 먼저 읽는다.

## 기준 문서

- `docs/LIFELENS_SPEC_v1.1.md` — 제품 설계 기준. 요구사항을 사용자에게 다시 묻지 말고 이 문서를 기준으로 진행한다.
- `docs/BUILD_STRATEGY_v1.2.md` — 빌드/검증 전략. SPEC 72~76절(빌드 전략, Phase 완료 조건)과 충돌하면 **이 문서가 우선**한다.
- `tasks/` — 실제 작업 지시서. 번호 순서대로 진행한다. 지시서 하나가 끝나기 전에 다음 지시서를 시작하지 않는다.

## 절대 규칙

1. **Unreal Engine 소스를 컴파일하지 않는다.** 엔진은 Epic 프리빌드 컨테이너 이미지로만 가져온다. `Engine/Build/BatchFiles/Build.sh`나 `Setup.sh`, `GenerateProjectFiles.sh`를 엔진 저장소에 대해 실행하는 워크플로우를 작성하지 않는다.
2. **APK 빌드는 확인 수단이 아니다.** 로직 검증은 `Source/LifeLensCore` 의 콘솔 하네스와 테스트로 한다. Android 패키징은 `workflow_dispatch`(수동)로만 실행한다.
3. **`LifeLensCore`는 Unreal에 의존하지 않는다.** `#include "CoreMinimal.h"` 같은 엔진 헤더, `UObject`, `TArray`, `FString`을 코어에서 사용하지 않는다. 표준 C++17만 사용한다. 이 라이브러리는 Termux(Android clang)에서도 컴파일되어야 한다.
4. 캐릭터 이름을 ID로 쓰지 않는다. 모든 캐릭터는 GUID(코어에서는 `uint64_t` 또는 128bit 구조체)를 가진다.
5. Relationship을 숫자 하나로 표현하지 않는다. SPEC 31절의 다차원 구조를 따른다.
6. 무료 범위를 벗어나는 서비스(유료 CI, 유료 API, 유료 클라우드)를 필수 의존성으로 넣지 않는다.

## 작업 방식

- 계획만 서술하지 말고 실제 파일을 생성/수정한다.
- 한 번의 작업은 하나의 지시서 범위를 넘지 않는다. 지시서에 없는 기능을 "겸사겸사" 추가하지 않는다.
- 변경 후 반드시 해당 지시서의 "완료 조건"을 스스로 확인하고, 확인 방법(실행한 명령과 결과)을 커밋 메시지 또는 PR 본문에 적는다.
- 워크플로우 YAML을 수정할 때는 무거운 잡(패키징) 이전에 반드시 Preflight 잡(도구 버전 확인, 디스크 확인)이 실행되게 한다.
- 에러가 나면 로그 전체를 요약해 원인을 특정한 뒤 고친다. "다시 실행해 보세요"로 끝내지 않는다.

## 커밋 / 브랜치

- `main`은 항상 코어 테스트가 통과하는 상태를 유지한다.
- 지시서 단위로 브랜치를 만든다: `task/01-ci-prebuilt-engine`, `task/02-core-sim`, ...
- 커밋 메시지 접두어: `core:`, `ue:`, `ci:`, `docs:`

## 디렉터리

```
lifelens-ue/
  AGENTS.md
  docs/
    LIFELENS_SPEC_v1.1.md
    BUILD_STRATEGY_v1.2.md
  tasks/
  Source/
    LifeLensCore/        # 순수 C++ 시뮬레이션 (Unreal 무관)
      include/
      src/
      harness/           # 콘솔 관찰 하네스
      tests/
      CMakeLists.txt
    LifeLens/            # Unreal 게임 모듈 (LifeLensCore를 링크)
  LifeLens.uproject
  Config/
  Content/
  .github/workflows/
    core-tests.yml       # push마다, ~1분
    android-package.yml  # 수동 실행만
```
