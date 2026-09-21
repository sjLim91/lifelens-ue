# LifeLens 프로젝트 상태 정리 — 2026-09-21

> 이 문서는 2026-09-21 기준 전수검사 후반부의 현재 체크포인트와 다음 실행 순서를 고정한다.
>
> 상태 판단 우선순위: **실제 GitHub main / PR / Actions > canonical repository docs > 대화 기억**.

## 1. 현재 체크포인트

- 저장소: `sjLim91/lifelens-ue`
- 기준 main: `4edae40b58a671e84032d17e1e84c39eae18d57b`
- #360 `[Audit] Consume high-speed movement budget across route waypoints`
  - main 병합 완료.
  - 16x/64x 고속 시뮬레이션에서 한 프레임 이동 예산을 단일 waypoint에 버리지 않고 최대 8 segment까지 연속 소비하도록 보강.
- #361 `[Audit] Preserve dependent care after parent loss`
  - Core Tests PASS.
  - Preflight PASS.
  - Unreal Linux Compile PASS.
  - 최신 main mergeability 확인 후 main 병합 완료.
- #362 `[Audit] Keep surface water visible when Water rendering fails`
  - Preflight PASS.
  - Unreal Linux Compile PASS.
  - #361 병합 뒤 최신 main mergeability를 다시 확인하고 main 병합 완료.
  - Unreal Water 표시 경로가 실패해도 실제 수계가 화면에서 완전히 사라지지 않도록 presentation safety surface를 추가.

## 2. 즉시 실행 순서

1. #360 / #361 / #362가 모두 들어간 최신 main을 기준으로 전수검사 다음 구간을 진행한다.
2. 자연환경 소스와 runtime contract를 전수 확인한다.
3. 지형/수계/vegetation이 "구현 존재"가 아니라 실제 화면에서 보이도록 검증한다.
4. 결손 지점에서 현재 asset으로 품질 목표를 달성하기 어렵다면 라이선스가 명확한 비용 0원 asset을 선별해 적용한다.
5. 수정 단위마다 Preflight / relevant tests / Unreal Compile을 통과시키고 안전한 순서로 main에 병합한다.
6. 자연환경 이후 캐릭터/모션 -> 생애/사회 -> Observer/UI -> 플랫폼/Save/최종 QA 순으로 계속 진행한다.

## 3. 전수검사 잔여 우선순위

### A. 지구 / 자연환경 / 수계

- 지형 고도차, 언덕, 산, 계곡, 해안이 실제 runtime에서 읽히는지 확인.
- 강 / 하천 / 호수 / 습지 / 샘 / 해안 / 바다가 Core Hydrology와 동일한 위치에 보이는지 확인.
- 수계가 보이기만 하는 장식이 아니라 담수/염수/이동/생활 resource contract와 일치하는지 확인.
- 나무 / 수풀 / 풀 / 바위 / ground cover가 빈 평면처럼 보이지 않을 정도로 존재하는지 확인.
- biome / 기후 / 토양 / 고도 / 수분에 따른 환경 차이를 검증.
- Local -> Regional -> Planetary -> Orbital 확장 시 지형/대기/지구 규모감이 끊기지 않는지 검증.

### B. 그래픽 / 에셋

- Engine Cube/Cone/Plane 등 임시 primitive가 production local-view에 노출되는지 전수검색.
- 저품질 placeholder를 실제 무료 production asset으로 교체할 수 있는 지점 확인.
- Landscape / PBR material / water / lighting / fog / sky / shadow / reflection 품질 확인.
- Windows + macOS desktop 고품질 tier와 Android lightweight tier를 분리.
- asset LOD / HLOD / instancing / cook boundary를 플랫폼별로 검증.

### C. 캐릭터 / 모션 / 생활행동

- 시작 남2/여2 절차 생성, GUID / Seed / 이름 / 성격 / 외형 영속성.
- 걷기 / 뛰기 / 앉기 / 눕기 / 수면 / 식사 / 위생 / 화장실 등 행동과 실제 환경/소품의 context 일치.
- 허공에 앉기, 대상 없는 작업 자세, 잘못된 방향 정렬 제거.
- StateTree / Smart Object / Motion Matching / IK / Motion Warp / navigation 연결 검증.
- 고속 시뮬레이션에서도 route / obstacle / avoidance가 깨지지 않는지 검증.

### D. 생애 / 가족 / 사회 시뮬레이션

- Needs -> Emotion -> Personality -> Memory/Belief -> Utility -> Goal/Action 연결.
- 관계 / 연애 / 결혼 / 동거 / 임신 / 출산 / 육아 / 성장 / 노화 / 사망 / 세대교체.
- 부모 사망 / 보호자 상실 / dependent care 연속성.
- Household 분리/합가, 가계도, 유전, dead/stale reference 정리.
- 별거 / 이혼 등 아직 연결이 약한 생애 사건 확인.

### E. Observer / UI / 카메라

- 주화면은 간결한 전체 개요 유지.
- 선택 주민 상세에서 Needs / Emotion / Memory / Relationship / Family / event history를 확인.
- 중요 사건 추적과 시네마틱 카메라.
- PC mouse / Android touch gesture 충돌과 safe area.
- Local / Regional / Planetary / Orbital 관찰 스케일 전환.

### F. 플랫폼 / Save / 최종 QA

- Android APK 최우선 실기기 패키징 / 실행.
- Windows / macOS native build 및 runtime visual QA.
- Save -> 종료 -> Load 후 GUID / 가족 / 관계 / WorldSeed / 시설 / 수계 보존.
- 장시간 / 16x / 64x simulation.
- CPU / GPU / memory / thermal / crash / RHI 검증.
- clean checkout -> build -> package -> launch 재현성.

## 4. 무료 에셋 사용 원칙

무료 에셋 사용은 허용하며, 시각 품질 향상에 실질적으로 도움이 되면 적극 활용한다. 다만 **무료 다운로드 가능 여부와 사용 허가 여부는 별개**이므로 저장소에 들어오기 전에 라이선스를 확인한다.

허용 기본 범위:
- CC0 / Public Domain.
- 상업적 게임 배포를 허용하는 명시적 permissive license.
- CC-BY 등 attribution이 필요한 경우 attribution 의무를 실제로 지킬 수 있고 출처 기록이 남는 경우.

금지:
- Non-Commercial / Personal Use Only.
- Editorial Only.
- 라이선스가 없거나 모호한 다운로드.
- 다른 게임/제품에서 추출된 ripped asset.
- 재배포/패키징 권리가 불명확한 asset.

모든 신규 외부 asset은 최소 다음을 기록한다:
- asset / pack 이름.
- 제작자 / 제공처.
- 원본 출처.
- 라이선스 종류와 확인일.
- attribution 요구 여부.
- LifeLens 내 사용 경로와 용도.
- Shared / Desktop / Mobile 플랫폼 분류.
- 수정/최적화 여부.

세부 정책은 `docs/ZERO_COST_ASSET_POLICY_v1.md`를 따른다.

## 5. 완료 판정

코드가 존재하거나 CI가 green인 것만으로 시각 기능 완료로 보지 않는다.

환경/캐릭터/UI 영역의 완료는 다음을 함께 요구한다:
- authoritative Core/World contract 일치.
- Preflight / relevant Core tests.
- Unreal compile.
- package/cook validation이 필요한 경우 성공.
- 실제 runtime에서 눈에 보이는 결과 확인.
- 플랫폼별 품질/성능 기준 충족.

특히 자연환경은 **“소스에 있다”가 아니라 “실행 화면에서 지형, 나무, 수계, 대기와 생활 맥락이 실제로 읽힌다”**를 완료 기준으로 한다.
