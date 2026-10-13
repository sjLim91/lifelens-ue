# CI 범위 선택과 공통 Web 검사

## 자동 실행

`web-checks.yml`이 PR와 main push의 Web 검증을 담당한다. 의존성 설치, pinned character 준비, 타입 검사, Vite 빌드, 전체 `tests/*/run.mjs` 및 integration entrypoint를 한 번씩 실행한다. Cognitive Shadow UI, ecology lifecycle, character export와 새 grounding/wildlife 계약도 공통 runner에서 유지한다. 기존 Node 22/24 혼재는 공통 Node 24로 통일한다.

기능별 화면은 `Tools/ci_scope.py`가 실제 base/head diff로 고른다. 사회 사건 변경은 social capture만, readout 변경은 공통 회귀만 실행한다. runtime·dependency·공유 지형·알 수 없는 render 파일은 보수적으로 모든 관련 장면을 검수한다. 문서만 바뀌면 Web/Core 빌드가 없고, Core 테스트만 바뀌면 native 검증만 실행한다. diff 실패를 변경 없음으로 처리하지 않는다. 삭제/이름 변경/공백 및 개행 포함 파일명을 NUL 구분으로 처리한다.

`Tools/ci_web.py`가 기존 8개 화면 검수 그룹을 선택해서 실행한다. Playwright는 화면이 있을 때만 설치한다. 환경 화면의 실제 Core DTO fixture는 그 화면이 선택될 때만 생성한다. 새 wildlife capture 파일이 기능 PR에서 추가되면 자동 발견한다. 렌더 준비용 ecology fixture의 native target 빌드는 별도로 필요하며 일반 CTest 전체 재실행을 하지 않는다.

## 제거한 반복

16개 기존 Web 워크플로는 자동 push/PR 실행에서 제외하고 `workflow_dispatch`로 유지한다. 특수 장애를 재현하는 수동 경로와 자산/화면 산출물은 보존한다. Cognitive Agent의 중복 frontend job은 공통 검사로 옮기고 실제 WASM Shadow ON/OFF invariant job은 유지한다. Core/WASM·배포/Pages/probe·저비용 domain Core 검사는 별도 목적이므로 유지한다.

`core-tests.yml`은 PR와 main push에서 native configure/build/CTest/seed 42 결정론 검사를 한 번 실행한다. 개별 branch push와 PR가 같은 코드를 중복 검사하고 서로 기다리던 경로는 제거한다. PR가 갱신되면 이전 Web/Core 작업을 취소한다.

기존 sleep-era의 자동 365일 검사는 Core 검사 안으로 이동했다. Core 행동/상태 소스 변경, 검증 도구 변경, 매일 02:17 UTC 또는 수동 실행에서만 세 seed를 검사한다. Core 테스트 파일과 WebClientBridge DTO 변경만으로 365일 검사를 돌리지 않는다. 기존 Core 빌드의 `ll_balance_audit`를 재사용하고 전체 CMake/CTest를 두 번 실행하지 않는다. 기존 1000일 심층 audit의 별도 명시적 실행 경로는 보존한다.

## 실행 예

| 변경 | 공통 Web | 화면 | Core/365일 |
|---|---|---|---|
| docs만 변경 | 없음 | 없음 | 없음 |
| Web readout | 1회 | 없음 | 없음 |
| social-event-layer | 1회 | social | 없음 |
| resident-grounding/runtime/package | 1회 | 전체 관련 장면 | 없음 |
| Core test | 없음 | 없음 | CTest/결정론, 365일 제외 |
| Core Simulation.cpp | 1회 | 전체 관련 장면 | CTest/결정론/365일 |
| WebClientBridge.cpp | 1회 | 전체 관련 장면 | CTest/결정론, 365일 제외 |
| 야간 Core 스케줄 | 해당 workflow 없음 | 없음 | CTest/결정론/365일 |

## 검증 및 병합

14개 로컬 단위 검사로 실제 Git diff(삭제/이름 변경/특수 파일명), 보수적 기본값, nightly/manual, 실패한 diff, entrypoint 중복 제거, 선택적/future capture와 manual workflow 전환을 확인한다. 전체 YAML 구문과 runner Python 구문을 검사한다. 실제 GitHub 실행은 PR head에 기록한다.

GitHub main branch metadata의 protection.enabled=false, required status contexts=[] 및 rulesets=[]를 확인했다. 필수 검사 이름을 재설정할 작업은 없다. 이 PR은 branch protection이나 Actions 권한을 변경하지 않는다. 향후 보호 규칙을 만들 때 공통 `LifeLens Web Checks / web`와 Core/WASM/preflight 등 필요한 gate를 지정해야 한다.

기능 PR #710/#711의 simulation/presentation 제품 코드는 포함하지 않는다. #711의 기존 procedural-environment workflow 변경은 수동 진단에 유지될 수 있으며, 신규 공통 runner가 추가된 계약/화면 검수 파일을 발견해 자동 검증한다. 배포 workflow는 변경하지 않는다.
