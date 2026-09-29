# AGENTS.md — LifeLens Agent Entry Point

이 저장소에서 작업하는 모든 AI 에이전트는 **이 파일을 먼저 읽는다.**

## 0. Source of truth

우선순위:

`actual GitHub main / PR / Actions > canonical repository docs > 이전 채팅 / 기억 / 로컬 추정`

작업 시작/재개 시:
1. `AGENTS.md`
2. `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md`
3. `docs/LIFELENS_SPEC_v1.1.md`
4. `docs/DEVELOPMENT_MILESTONES.md`
5. `docs/WEB_CLIENT_ARCHITECTURE_v1.md`
6. actual `main` / target PR / Actions
7. `tasks/WORK_STATE.md`
8. 필요 시 audit/team/handoff 문서

## 1. Active architecture

LifeLens의 현재 제품 경로는:

```text
LifeLensCore C++ -> Emscripten/WASM -> Web Observer
                                   -> React/TypeScript/Three.js
```

- `LifeLensCore`가 유일한 simulation authority다.
- Web은 presentation/observer client다.
- Web이 자원, 시설, 행동 결과, 관계, 지형, 미래 상태를 따로 만들어서는 안 된다.
- Core는 React/Three.js/DOM/브라우저 API에 의존하지 않는다.
- Unreal은 active `main`에서 제거되었다.
- 마지막 Unreal 상태는 `archive/unreal-final-20260929`에 보존되어 있다.
- 미래에 Unreal/다른 엔진을 다시 붙일 수 있지만, 그때도 현재 Core 계약에 맞춘 선택적 client로 도입한다.

## 2. Development unit

> **Same purpose + same layer + same validation scope = one milestone-sized PR.**

- 내부 커밋은 작게 나눠도 된다.
- authority가 다른 변경은 억지로 섞지 않는다.
- 기능보다 causality/authority/stability를 우선한다.
- stale branch를 wholesale merge하지 않는다.

## 3. Validation / CI

Core 변경:
- CMake configure/build
- CTest
- deterministic harness

Web/Core 경계 변경:
- Preflight
- Web WASM
- Web Typecheck/build
- Web runtime resilience/contract checks

merge 후:
- Web Runtime Release
- GitHub Pages Preview
- External Preview Probe

Unreal UHT/UBT, Android Unreal APK, UE asset authoring은 현재 제품 gate가 아니다.

## 4. Absolute product rules

- NEW GAME은 성인 4명(남2/여2)에서 시작하고 문명 인프라는 0이다.
- 행동 결과는 실제 이동/자원/시설/경과시간과 인과적으로 연결되어야 한다.
- 이동 없이 원격 채집/보관/건설하지 않는다.
- 음식/물/세척 같은 소비는 실제 보유 자원을 요구한다.
- 수면처럼 시간이 중요한 행동은 경과시간에 따라 상태가 변해야 한다.
- 정착/건물은 경험과 필요에서 발생하며 spawn 좌표를 생활권으로 하드코딩하지 않는다.
- 시대 이름은 관찰용 요약일 뿐 무료 unlock timer가 아니다.
- 지형/수계/생태/날씨는 장식이 아니라 Core 생활 조건이다.
- 일반 사용자 UI는 한국어를 기본으로 한다.
- 유료 API/클라우드/런타임이 없어도 baseline simulation이 작동해야 한다.
- Presentation은 Core에서 발생하지 않은 사건을 꾸며내지 않는다.

## 5. Active ownership

기본 구현 owner는 통합형이다. milestone에 필요하면 다음을 end-to-end로 수정할 수 있다.

- `Source/LifeLensCore/**`
- `Clients/Web/**`
- `web/**`
- Web/WASM adapter
- Web rendering / camera / UI / motion presentation
- Core tests / Web tests / CI
- docs / tasks

시각 QA collaborator가 있더라도 파일 충돌을 피하려고 잘못된 중복 구조를 만들지 않는다.

## 6. State documents

- `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md` — 현재 runtime architecture
- `docs/LIFELENS_SPEC_v1.1.md` — 제품/domain 최상위 요구사항
- `docs/WEB_CLIENT_ARCHITECTURE_v1.md` — Web/Core 계약
- `docs/DEVELOPMENT_MILESTONES.md` — roadmap
- `docs/DECISION_LOG.md` — 지속적 설계 판단
- `tasks/WORK_STATE.md` — 현재 실행 상태

## 7. Interruption / recovery

중단 뒤에는 성공을 추측하지 않는다. actual GitHub 상태를 다시 조회하고 마지막 검증 checkpoint부터 이어간다.

컴파일/CI를 단순 대기하는 동안 무한 polling하지 않는다. 결과가 필요할 때 해당 run 상태와 실패 step/log를 확인한다.
