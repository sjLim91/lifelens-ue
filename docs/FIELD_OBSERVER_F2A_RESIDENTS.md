# LifeLens 현장형 2.5D — F2-a 주민별 외형과 관측된 이동 방향

기준: `main` fed012a431 (#1050, F1-b).
범위: 현장(Canvas 2D) 주민 **표현 계층만**. LifeLensCore, WASM, Core AI, 상태·저장·인과 계약 변경 없음.

## 지난 모바일 화면과의 차이

기존 F0/F1에서는 모든 남성 주민과 여성 주민이 두 개의
정면 SVG 이미지만 사용했다. 같은 방향으로 서 있고 Core에서
실제로 걷거나 작업해도 정면 마커만 보여, 주민의 삶을 관찰하기
어려웠다. F1-b에서 시야를 확보했으므로 이번에는 **사람 자체**를
개인별·방향별로 구분한다.

## 실제 구현

- `web/src/render/field-resident-presentation.ts`
  - `fieldResidentPalette()`: 현재 Resident의 영구 GUID와
    `genetics.skinTone`, `genetics.hairPigment`,
    `genetics.heightPotential`, `genetics.buildPotential`로부터
    팔레트/체형을 **결정적으로** 선택한다. 머리 스타일과 복장도
    GUID에서 안정적으로 정하며 매 갱신 새로 랜덤하게 뽑지 않는다.
    유전값이 없는 경우에만 GUID 기반 시각적 기본값을 사용한다.
    UI용 색상 해석일 뿐 외형의 과학적 유전 확정 결과로 표시하지 않는다.
  - `fieldHeading()`: 실제 그리드의 x/y 이동 방향을 8방향으로
    분류. X+동쪽, Y+남쪽(탑다운 화면 기준). 초기 방향은 Core가
    현재 `Moving` 목표를 명시한 경우에 한해 그쪽을 응시할 수 있다.
  - `FieldResidentMotion`: 이전 실제 관측 좌표와 현재 Core
    관측 좌표를 비교. 충분히 짧은 실제 이동이 발견된 직후에만
    330ms 이하의 제한적인 보행 포즈 적용. 단순한 이동 목표·의도
    또는 정지 프레임만으로 걷고 있다고 애니메이션하지 않는다.
    긴 비관측 간격, 장거리 도약, 재시작·빠른 시간 진행은
    보행으로 표현하지 않는다. 캐시는 주민 목록 기준 prune/reset.
  - `drawFieldResident()`: 원래의 동일한 성별 정면 SVG 대신
    8방향 후면/측면/정면·대각선 표정과 개별 피부·머리·옷 색,
    움직이는 팔/다리, Core가 보고한 `Physical/Sleep`,
    `Civilization/Interacting`에만 따른 잠자기·작업 포즈를
    Canvas 위에 직접 그리는 벡터 캐릭터.
    도구·채집 결과·물리 이동을 가상 생성하지 않는다.
- `web/src/render/field-observer-renderer.ts`
  - **기존 Core 기반 좌표·선택 링·클릭 픽킹 유지**.
  - 기존 2D Canvas 스프라이트 아틀라스·시야 확보·전경/후경
    깊이 정렬 유지.
  - 실제 Canvas에서만 벡터 캐릭터를 표시. 구형/경량 Canvas
    대체 경로는 기존 원본 주민 SVG로 안전하게 돌아간다.
- `web/src/observer-engine.ts`
  - 월드 새로 만들기·장기 빨리감기 전후 `resetMotion()`;
    관측 누락에서 이전 좌표로 잘못 걸었다고 표시하는 문제 방지.

## 검증

`web/tests/presentation/field-resident-f2a-checks.mjs`
- 8방향/정지·목표만으로 보행 생성 금지.
- 실제 좌표 이동만 보행·유효 시간 제한, 장거리 도약 억제.
- Core가 보고한 수면/작업 단계만 포즈 반영.
- 주민별 GUID/유전값 결정성, Core DTO 불변.
- 앞/뒤/측면에 실제 다른 벡터 얼굴 표현.
- 기존 필드 선택/주민 오버레이/Canvas 대체 표시 유지.
- F0·F1·F1-b·전체 Web regression, Preflight, typecheck, build 필수.

## 아직 완료하지 않은 사항

- 이 버전의 8방향은 직접 그리는 2D 벡터 퍼펫이다. 원본 전문
  일러스트로 구성된 **8×다중프레임 아틀라스**와는 구분한다.
- 더 풍부한 행동별 키프레임(밭 갈기, 제작, 건설, 소비),
  소지품·도구 연동, 아동/노인 전용 모델, 모든 동물 종,
  대형 암반·숲의 최종 미술 수준은 별도 후속 작업이다.
- 실제 갤럭시 기기의 시각 QA·긴 시간 FPS를 자동 검사만으로
  승인했다고 주장하지 않는다.
