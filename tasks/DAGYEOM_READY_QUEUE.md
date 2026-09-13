# Dagyeom READY NOW Queue

이 문서는 다겸(STILLofficial) / 다겸 측 AI가 **Core/API 대기 때문에 유휴 상태가 되지 않도록** 유지하는 실행 큐다.

원칙:
- `READY NOW`에는 지금 바로 시작 가능한 작업만 둔다.
- Core/Simulation/Build 쪽 API가 필요한 작업은 `BLOCKED-BY-JJUN`으로 분리한다.
- 다겸 측 AI는 작업 시작 전 `tasks/WORK_STATE.md`와 이 파일을 읽는다.
- READY NOW가 0개가 되면 `NEEDS_ASSIGNMENT` 상태로 보고, 제품 SPEC에서 다겸 소유 범위의 다음 작업을 즉시 채운다.
- 다른 PR과 겹치는 파일이 많으면 새 브랜치를 stacked branch로 분리하고, 기존 PR에 기능을 계속 쌓지 않는다.

## READY NOW

### DQ-01 UI Foundation / Android landscape
- 추천 브랜치: `dagyeom/ui-foundation-v1`
- 소유 범위: `Content/UI/**` 중심, 필요 시 UI 전용 코드
- 목표:
  - Android landscape safe-area 기준 정리
  - typography / spacing / icon / 상태표현 token 정리
  - 작은 화면 overflow / scroll / tab density 기준 정리
  - 한국어 표시를 고려한 font fallback 구조 준비
- Core API 의존: 없음
- PR #17 의존: 낮음

### DQ-02 Character Presentation v1
- 추천 브랜치: `dagyeom/character-presentation-v1`
- 소유 범위: `Source/LifeLens/Characters/**`의 presentation 영역 + `Content/Characters/**`
- 목표:
  - 선택 주민 nameplate
  - LifeStage badge
  - selected / focused visual feedback
  - 카메라 거리별 정보 밀도/표시 LOD
  - Observer 관점에서 과도한 UI 없이 현재 주목 대상을 식별 가능하게 만들기
- Core API 의존: 없음 또는 현재 LifeStage/이름 수준
- PR #17 의존: 낮음

### DQ-03 Observer UX Polish v1
- 추천 브랜치: `dagyeom/observer-ux-polish-v1`
- PR #17 파일 의존성이 있으므로 가능하면 `dagyeom/observer-ui-v2` HEAD에서 stacked branch로 시작
- 목표:
  - World → Quick → Detail 전환의 시각적 위계 정리
  - 뒤로가기 / 선택해제 / 빈 공간 탭 동작 일관화
  - main view는 전체 상황을 빠르게 읽고, 상세는 선택 후에만 노출되는 Observer-first 원칙 강화
  - 데이터가 없는 탭에서 가짜 값을 만들지 않고 명확한 empty-state 제공
- Core API 의존: 없음

### DQ-04 Mobile Touch v1
- 추천 브랜치: `dagyeom/mobile-touch-v1`
- 목표:
  - Android 터치 hit target 확대
  - landscape safe-area / notch / 화면 가장자리 대응
  - tab 전환, 스크롤, 패널 닫기 동작 점검
  - 작은 화면에서 텍스트 겹침/잘림 방지
- Core API 의존: 없음

### DQ-05 Visual Feedback v1
- 추천 브랜치: `dagyeom/visual-feedback-v1`
- 목표:
  - 주민 선택/해제
  - 관찰 레벨 전환
  - 현재 관심 대상 강조
  - 상태 변화가 있을 때 화면을 지저분하게 만들지 않는 비침투적 피드백
- Core 값을 새로 만들거나 Simulation 로직을 변경하지 않는다.
- Core API 의존: 없음

## BLOCKED-BY-JJUN

아래 항목은 다겸 쪽 할 일이 없는 것이 아니라 **쭌 측 Unreal read API 제공 대기**다. 다겸 측에서 Core/Simulation을 직접 수정하지 않는다.

1. Relationship 13차원 + 대상 주민 ID/이름 읽기 API
2. Emotion 세부 차원 + valence/arousal/intensity 요약 API
3. SocialIntent(Approach/Repair/Comfort/Avoid) + 대상 주민 API
4. 가족 요약: Partner / Parents / Children / Siblings + 혼인/동거/임신 상태
5. World overview 집계: Households / Couples / Married Couples / Pregnancies / Major Events
6. 최신 Core read DTO를 Unreal BlueprintPure/USTRUCT 형태로 노출하는 Bridge/API

쭌 측은 위 항목을 준비하면 이 문서에서 해당 항목을 READY NOW로 승격시키고, 다겸 측 AI가 바로 UI binding 작업을 시작할 수 있게 한다.

## 현재 다겸 진행상태

- `dagyeom/observer-ui-v2`
- PR #17 `[UI] Observer HUD v2: LEVEL 0 overview, LEVEL 1 quick inspector, LEVEL 2 detail tabs`
- latest known HEAD: `3b578c67528c564064c73a76c1eb4f16f25e489f`
- Structural Preflight: PASS
- 실제 UHT/UBT / 화면 동작 검증: 대기
- PR #17이 검증 대기라고 해서 다겸 전체 작업이 BLOCKED인 것은 아님. 위 READY NOW 큐에서 별도 작업을 계속 진행한다.
