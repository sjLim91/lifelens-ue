# LifeLens Character Appearance & Presentation Roadmap

이 문서는 LifeLens의 캐릭터 외형/표현 계층을 실제 게임 수준으로 끌어올리기 위한 canonical 실행 로드맵이다.

제품 최상위 기준은 `docs/LIFELENS_SPEC_v1.1.md`, 문명 진행 기준은 `docs/CIVILIZATION_PROGRESSION_v1.md`, 협업 절차는 `docs/INTEGRATION_SPRINT.md`를 따른다.

## 1. 현재 상태 — 중요

현재 LifeLens의 캐릭터 표현은 초기 primitive placeholder 단계를 벗어났다.

현재 production runtime에는:
- 공통 humanoid skeletal mesh 기반 남/녀 실제 인간 body
- 피부/눈/머리 색 variation
- 머리/수염 및 기본 의상
- LifeStage 기반 체격/캡슐/노화 표현
- locomotion BlendSpace + Core context-action animation
- Core `GeneticsProfile` → Bridge → Appearance phenotype 연결
- 선택 링, 이름/LifeStage 라벨 LOD

가 들어가 있다.

다만 얼굴 morph 다양성, Motion Matching/Pose Search, Motion Warping, IK/Control Rig는 아직 production 구현이 없다. 따라서 현재 인간형 표현을 최종 AAA 완성본으로 간주하지 않는다.

## 2. 목표

LifeLens 캐릭터는 아래를 동시에 만족해야 한다.

- 사람이 봤을 때 즉시 '실제 인간 캐릭터'로 인식되는 외형
- 남/녀 초기 주민 4명이 매 NEW GAME마다 서로 다른 외형을 가짐
- 얼굴, 피부톤, 머리, 눈, 체형, 의상 조합을 data-driven으로 구성
- 공통 skeleton/animation 체계를 사용해 행동 표현과 연결
- Core의 행동/감정/관계 상태를 presentation이 표현만 하고 의사결정 authority를 만들지 않음
- 성장/노화/유전 시스템과 이후 연결 가능
- Android 우선, PC/Mac도 같은 데이터 구조 사용
- 유료 API/유료 런타임 서비스 없이 동작

목표 시각 품질은 모바일에서 유지 가능한 상업 게임급 현실형 표현을 지향한다. MetaHuman 전체 런타임을 기본 전제로 삼지 않으며, 성능/용량을 우선해 경량화된 공통 skeleton + modular appearance 구조를 사용한다.

## 3. 핵심 구조

### 3.1 AppearanceProfile

각 주민은 이름과 분리된 안정적인 외형 프로필을 가진다.

예정 필드:
- body archetype / height / proportions
- face variant 또는 morph parameter set
- skin tone/material variant
- eye color
- hair style/color
- facial hair where applicable
- default outfit set
- age/life-stage presentation parameters
- visual seed / provenance

초기 NEW GAME 외형은 `WorldSeed + CharacterId` 기반으로 deterministic하게 생성한다.
Save/Load 후 외형이 바뀌면 안 된다.

### 3.2 Shared Skeleton

- 가능한 한 주민들이 공통 humanoid skeleton을 사용한다.
- animation retarget 비용을 최소화한다.
- body/face/hair/clothing은 modular하게 교체 가능하게 한다.
- primitive → modern civilization까지 의상/장비를 이후 추가할 수 있어야 한다.

### 3.3 Presentation Authority

- Core가 행동/감정/관계/가족/문명 상태의 authority다.
- Character Presentation은 Core directive/read DTO를 읽어 mesh/material/animation/gaze/gesture를 선택한다.
- presentation layer가 별도의 생활 AI나 competing action decision을 만들지 않는다.

## 4. 실행 순서

### Phase A — UI Foundation (#26) — CURRENT

목적:
- Android landscape safe-area
- typography/spacing/touch target
- Observer panel scaling

완료 후 캐릭터 작업의 HUD/선택/라벨 기준이 안정된다.

### Phase B — Character Presentation Foundation (#29)

기존 PR #29를 최신 main 기준으로 reconcile한다.

목적:
- PresentationComponent
- selection ring
- label LOD
- life-stage scale hook
- placeholder body 제거 준비
- Core resident/action identity와 안정적으로 연결

완료 조건:
- 최신 main UHT/UBT/link PASS
- DebugBody/placeholder가 production human mesh로 교체 가능한 구조
- UI/Simulation authority 침범 없음

### Phase C — Character Appearance v1 — HIGH PRIORITY / NEW

#29 직후 시작한다. #30/#36/#38의 시각 polish보다 우선한다.

최초로 실제 인간처럼 보이는 단계다.

구현 범위:
- 공통 humanoid skeletal mesh pipeline
- 남/녀 production base body
- 실제 피부 material
- 얼굴/head variation
- 눈 material/variation
- 머리카락 style/color variation
- 최소 기본 의상 세트
- `AppearanceProfile` data model
- NEW GAME deterministic appearance randomization
- Save/Load appearance continuity
- 기존 Cylinder/Sphere production rendering 제거 또는 fallback 전용화
- selection ring / label은 #29 기능 재사용

완료 조건:
- NEW GAME 4명이 서로 구분되는 실제 인간 외형으로 표시됨
- 2 male + 2 female 초기 조건은 유지되지만 이름과 외형은 매 NEW GAME seed에 따라 달라짐
- load 후 동일 resident 외형 유지
- Mac/Windows editor와 Android에서 동일 resident identity 유지
- 실제 UE UHT/UBT/link PASS
- asset license provenance 기록

### Phase D — Character Motion & Context v1

실제 외형이 들어간 뒤 행동이 마네킹처럼 보이지 않도록 한다.

범위:
- locomotion idle/walk/run
- 자연스러운 turn-in-place
- sit / stand / lie / wake
- gaze / head tracking
- interpersonal distance/avoidance presentation
- context interaction animation hooks
- basic IK
- action transition smoothing
- Core Physical/Social/Civilization directive 기반 animation state

완료 조건:
- '없는 의자에 앉기' 같은 context mismatch를 presentation 단계에서 방지
- Core action과 화면 행동이 모순되지 않음
- 주민 4명이 동시에 움직여도 Android target에서 안정적

### Phase E — Observer UX / Mobile / Feedback stack

실제 인간 외형이 확인된 뒤 기존 다겸 stacked PR을 정리한다.

순서:
1. #30 Observer UX Polish
2. #36 Mobile Touch
3. #38 Visual Feedback

이 단계에서 외형/캐릭터가 가려지거나 UI가 과밀해지지 않게 Observer-first 원칙을 유지한다.

### Phase F — Appearance Genetics & Lifecycle v1 — IN PROGRESS

기존 Core Genetics/Lifecycle과 시각 표현을 연결한다.

현재 완료:
- Core 부모 유전 → 자녀 `GeneticsProfile` 상속/variation
- Bridge `FLLCoreGeneticsSnapshot` projection
- 피부/눈/머리색/키/체형 phenotype의 실제 appearance 반영
- LifeStage 성장/노화 크기 및 머리/피부 ageing

잔여 범위:
- 실제 face morph/face-shape asset 연결
- 부모 phenotype/appearance parameter의 얼굴 형태 시각화
- 출생 시 visual identity 생성
- child → teen → adult → elder 단계별 외형 변화
- 키/체형/얼굴 성숙도 변화
- 머리/피부 노화 표현
- family resemblance

중요:
- genetics는 이름이나 actor pointer가 아니라 CharacterId/authoritative Core lineage를 사용한다.
- 기술적으로 가능한 범위부터 점진적으로 확장한다.

### Phase G — Clothing / Equipment by Civilization

문명 진행과 외형을 연결한다.

범위 예시:
- primitive wrap/simple garment
- crafted clothing
- carried tools
- equipped tools/weapons가 아닌 생활/생산 도구
- 직업/역할에 따른 장비 표현
- 후대 기술 수준에 따른 clothing/equipment diversity

전역 시대 unlock이 아니라 주민 개인의 소유/제작/지식 상태를 presentation한다.

## 5. Asset / 비용 원칙

- 프로젝트 런타임 비용 0원을 유지한다.
- 유료 캐릭터 API/클라우드 생성 서비스에 의존하지 않는다.
- 무료 asset이라도 repository에 넣기 전에 재배포/프로젝트 사용 license를 확인한다.
- asset source와 license/provenance를 문서에 남긴다.
- 라이선스가 불명확하면 main에 binary asset을 넣지 않는다.
- 특정 외부 vendor 포맷에 Core data model을 종속시키지 않는다.

후보 asset/pipeline은 구현 시작 시 별도 검토한다. 선택 기준은 품질보다도 Android 성능, license 명확성, 공통 skeleton/LOD/수정 가능성이다.

## 6. Android 성능 원칙

Character Appearance v1부터 다음을 기본 전제로 한다.

- shared skeleton 우선
- material slot 수 최소화
- dynamic material 남용 금지
- LOD 필수
- 머리카락은 모바일 fallback 제공
- 고비용 strand/groom만을 유일한 표현으로 사용하지 않음
- 화면 밖/원거리 animation update 비용 축소
- 주민 수가 세대를 거치며 증가할 수 있으므로 4명 전용 최적화 금지

첫 통합 smoke 목표는 Android Development build에서 4명 주민이 실제 인간 외형으로 표시되고, Observer UI 및 Core simulation과 함께 안정적으로 실행되는 것이다.

## 7. Integration Sprint 새 우선순위

현재 canonical 순서는 다음과 같다.

1. #26 UI Foundation
2. #29 Character Presentation Foundation
3. **Character Appearance v1**
4. **Character Motion & Context v1 최소 세트**
5. #30 Observer UX Polish
6. #36 Mobile Touch
7. #38 Visual Feedback
8. Core + Observer + Human Character integrated runtime verification
9. Android smoke APK
10. Appearance Genetics & Lifecycle 확장
11. Clothing/Equipment civilization linkage
12. deeper civilization production chains 재개

단, blocker/컴파일/SaveLoad/Bridge 결함은 항상 우선 수정할 수 있다.

## 8. 역할

다겸 기본 소유:
- `Source/LifeLens/Characters/**`의 appearance/presentation 성격 변경
- `Content/Characters/**`
- `Content/UI/**`
- UI presentation

쭌 기본 소유:
- Core Appearance/Genetics용 authoritative data가 필요할 경우 `LifeLensCore`/Bridge
- Save/Load
- CI/build
- Core action → presentation contract

공동/충돌 가능 파일은 `docs/INTEGRATION_SPRINT.md`의 ASSIST_LOCK 절차를 따른다.

## 9. 완료 정의

캐릭터 외형 작업은 '에셋이 보인다'만으로 DONE이 아니다.

최소 완료 조건:
- 실제 인간 mesh/material 표시
- deterministic resident appearance
- Save/Load continuity
- Core action과 visual action 일치
- observer selection/label 정상
- age/lifecycle 확장 가능한 데이터 구조
- Android LOD/fallback 존재
- UHT/UBT/link 검증
- asset license/provenance 기록
- 상태문서/HANDOFF 최신화
