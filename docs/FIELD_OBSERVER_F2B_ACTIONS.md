# LifeLens 현장 2.5D — F2-b 행동별 포즈·실제 도구·연령 단계

기준: `main` 5b152401 (#1051 F2-a).
**Web 2D 현장 관찰 화면만 변경**한다. 모바일 실기기 검증은 이번 작업에서 사용자의 명시적 요청에 따라 제외한다.

## 구현 범위

### Core 인과에 종속되는 행동 포즈

`field-resident-actions.ts`에서 현재 Resident의 `presentation`을 직접 판단한다.
`active=true` 및 `phase=Interacting`이 아니면 신체 활동을 시각적으로 완성했다고 주장하지 않는다.

- `Civilization/Gather` + Core가 지정한 resource node/material과 현재 주민의 target grid 접근 확인 → **채집**.
- `Civilization/Craft` + 실존 facility ID, facility action `Work/Repair`, 실제 목표 근접 → **건설/보수**.
- `Civilization/Craft` + 시설 작업대 근접 또는 Core가 발급한 local technique/token → **제작**.
- `Civilization/Craft` + `CultivatedPlot` 실존 facility ID와 근접, `Plant/Water/Tend/Harvest` → **밭 작업**.
- 실제 시설/저장소 작업 지시 → 기존 일반 작업 자세.
- `Physical` + `Sleep/Eat/Drink/Wash` 보고 시 → **수면/식사/음수/세척 제스처**.
- `Moving` 단계 또는 단순 목표 정보만으로 상호작용 자세를 표시하지 않는다. F2-a의 실제 관측 이동에 한정한 보행 포즈가 우선한다.

작업 동작의 작은 팔·무릎 변위는 **계속 진행 중인 현재 Core 행동의 시각적 포즈**일 뿐 새 자원 소비, 애니메이션 중 자동 채집, 가상 건물 생성 같은 물리 작업이 아니다.

### 진짜 물품만 그리는 규칙

`presentation.artifactTool`을 Core가 실제로 넘기고, `Gather/Interacting` 및 유효한 context token과 유효 내구도/원본 shape·size 부품이 확인될 때만 **원본 artifact parts를 손 위에 렌더**한다.
단순히 주민 inventory가 존재한다는 이유로 장착 도구를 생성하지 않는다.
소지 수량과 착용 상태를 추정해서 빈 허공에 아이템/물통/식량을 띄우지 않는다.
건설 망치나 채집 바구니를 자동으로 생성하지 않는다.

### Core 생애 단계의 화면 크기

`lifeStage` (Baby/Toddler/Child/Teen/YoungAdult/Adult/MiddleAge/Elderly)를 우선 사용하고, 값이 없으면 `ageYears`의 보수적 단계를 적용하여 현장 캐릭터 스케일을 조정한다.
좌표/클릭 포인트는 동일한 실제 Core 위치를 유지한다.
미래의 알 수 없는 life stage를 성인과 동일한 중립 표현으로 복구한다.
이는 출생/성장/사망 로직을 변경하지 않는다.

## 개발 파일

- `web/src/render/field-resident-actions.ts`: 순수 상태 판별, 생애 단계 스케일, Core artifact 부품 2D 표현.
- `web/src/render/field-resident-presentation.ts`: F2-a 8방향/인물 외형에 동작별 팔·무릎·얼굴 제스처 연결.
- `web/src/render/field-observer-renderer.ts`: Core stage에 따라 resident image 크기 조절.
- `web/tests/presentation/field-resident-f2b-checks.mjs`: 허위 행동·허위 소품 차단, 실제 단계 및 도구 구조, 2D 행동 도형 차이, 실제 발좌표/선택 유지 검증.
- `web/tests/presentation/field-resident-f2a-checks.mjs`: 근거 없는 작업 포즈를 승인하던 기존 fixture에 실제 시설/목표 반영.

## 검증 계약

- 전체 Web typecheck, 기존 M0~F2-a, 추가 F2-b 및 Core 인과·모바일 무GPU 회귀, production build, Preflight 성공 후 병합.
- GitHub Pages 배포, 외부 브라우저 WASM Core 초기 부팅 자동 검사.
- **실기기/실사용 모바일 장시간 FPS 테스트는 실행하지 않고 성공으로 보고하지 않는다.**

## 다음 단계

보다 높은 작화의 일러스트 아틀라스, 모든 동물/시설 단계, 주민 애니메이션의 중간 프레임과 실제 작업부위 접촉, Core 자원에 종속되는 소지품/동작 등은 별도 마일스톤. 새 도구/작업을 단지 그럴듯해 보여서 꾸며내지 않는다.
