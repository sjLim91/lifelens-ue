# LifeLens 지구환경·전지구 흐름 관찰 개발 통합 계획 (2026-10-12)

> **상태: 신규 구현 로드맵 / 기존 설계 규칙 불변.**
>
> 구현 추적: [EARTH-FLOW #672](https://github.com/sjLim91/lifelens-ue/issues/672)
>
> 기존 정식 아키텍처: `docs/EARTH_MULTI_ORIGIN_ARCHITECTURE_v1.md`,
> `docs/EARTH_AND_HUMAN_FOUNDATION.md`,
> `docs/WORLD_ARCHITECTURE_v2.md`
>
> 관찰 스케일 전환 요구: [#329](https://github.com/sjLim91/lifelens-ue/issues/329);
> 주민 자발적 AI 연계: [#670](https://github.com/sjLim91/lifelens-ue/issues/670).

## 1. 사용자 목표와 시각적 참조

사용자가 제시한 earth.nullschool.net 같은 모습의 회전 지구를 LifeLens에서
**행성→대륙→지역→마을→주민**으로 확대/축소해 관찰하고,
실제 지구의 자연 지리 위치를 기반으로 각 지역의 바람, 기온, 강수, 습도, 구름,
계절, 해류, 수계, 지형, 식생이 실제 삶에 서로 다른 영향을 주도록 구현한다.

- 실제 자연 지구의 해안·대륙·고도·유역·위도/경도 위치 관계를 우선 사용한다.
- **현대 도시, 국가, 산업, 인구, 도로, 정치 체계는 새 시뮬레이션 시작 시 없음.**
  실제 부산 좌표의 자연 해안 지형이 있더라도 현대 부산 시가지는 없다.
- 바람·해류의 구면 유선(streamlines)은 nullschool에서 얻은 UI 영감일 뿐,
  그 사이트의 코드/이미지/데이터를 복제하는 구현이 아니다.
- 기본 실행은 **생성·진화하는 LifeLens 기후/환경**, '현재 지구 실황'이 아니다.
  관측/재분석 데이터를 추가한다면 데이터 시각·소스·라이선스/버전을 고정하고
  관측과 시뮬레이션을 UI에서 반드시 구별한다.

## 2. 사실의 소유권과 구조

```text
버전이 고정된 실제 지구 자연지리 데이터 (offline Earth pack)
  + WorldSeed / EarthScenario / ClimateModelVersion
  + 전역 SimulationInstant
             |
             v
       Core Earth RegionField
       - Geography: 지표/육해/고도/해안/유역
       - Climate: 지역기온/강수/습도/기압/풍벡터
       - Ocean: 해면온도/해류벡터/염도 등 자료가 존재하는 필드
       - Ecology: 식생/토양/수자원/회복력
             |
          /     \
         v       v
  Core Local Surface     Core Planet / Region Read Model
  - Needs / Health       - sphere LOD data
  - Water / Resource     - overlay scalar/vector samples
  - Utility / Memory     - settlement/origin markers/events
  - Family / Migration              |
          |                         v
          |               Three.js EarthGlobeRenderer
          |               Wind/Ocean/current read-only streamlines
          v
  Web local region / village / resident observer
```

**절대 불변:** Core의 Health, Needs, Utility, Household, LifeStage, Resource,
Civilization, ContextAction, A*/Collision, 저장 결정론의 authority를
Web에 이중 구현하지 않는다. `WorldHierarchy.h`와 `WorldGenesis.h`의
현재 Planet/SurfaceRegion identity를 확장하되 existing local hot path는 보존한다.

## 3. 지역 환경 데이터 계약 (예시, 스키마 초안 — 코드 미적용)

### 3.1 고정 자연지리: `EarthGeographicRegionSample`

- `planetId, surfaceRegionId, earthDataVersion, geoLatDeg, geoLonDeg`
- `landFraction01, macroElevationM, bathymetryM, coastalDistanceKm`
- `slope, watershedId, riverDrainage, freshwaterPotential01`
- `soilWaterRetention01, substrateKind, baselineBiome, naturalVegetationPotential01`
- 각 필드 `available / sourceVersion / resolution` 메타데이터.
  없는 데이터를 임의 고정 숫자로 채우지 않는다.

### 3.2 동적 지역 환경: `EarthRegionalClimateSample`

- `simulationMinute, climateModelVersion, regionId`
- `localSolarHour, solarElevation, seasonalPhase` (위도/경도 및 지구 자전)
- `temperatureC, humidity01, pressureHpa, precipitationMmPerHour, cloud01`
- `windEastMps, windNorthMps` (그 지역 **접평면** 벡터)
- `oceanSurfaceTemperatureC, oceanCurrentEastMps, oceanCurrentNorthMps`
- `snowDepth / surfaceWetness / soilMoisture` (지원하는 수준부터)
- `airPollution`은 **출처/인과 모델이 구현된 경우만** 표시.
  현대 국가의 실측 도시 오염을 문명 0 시드에 이식하지 않는다.

`wind/current`와 scalar 필드는 전구 coarse grid와 active regional 상세 grid를
공통 identity로 참조한다. 미제공 수치는 null/quality flags로 전달한다.

### 3.3 환경 영향 채널

현재 `SimulationClimate` / `EnvironmentalConsequences`의
`airTemperatureC, precipitationIntensity01, humidity01, windIntensity01`,
`heatStress01, coldStress01, travelFriction01`,
`waterReplenishmentMultiplier, plantFoodRegenerationMultiplier`가
Core에 이미 존재한다.

신규 지역 field는 **이 기존 경로의 실제 지역별 입력**으로 어댑트한다.
욕구 감소율, 온도 임계값, 사망률, 출산율 등을 임의 재정의하지 않는다.

장기 확장은 명확한 물리 인과(강수→유출/충전→유역/담수→취수,
습도/기온→환경스트레스, 바람→노출/야외활동, 토양/수분→작물/식생,
계절→자원/이주)를 가지되,
각 연산은 별도 과제의 회귀/결정론 감사 후 단계별로 연결한다.

## 4. Atmosphere / Ocean flow 화면

관찰 레이어에는 다음의 서로 다른 상태를 제공한다.

| 토글 | 입력 | 화면 표현 | 규칙 |
| --- | --- | --- | --- |
| 지표·육해·고도 | versioned Earth pack | 실제 대륙 윤곽, 해안, 고도/해저 | 현대 문명 없음 |
| 바람 | wind East/North field | 접평면 유선/이동 파티클, 속도 범례 | CPU/GPU 시각화만, Core로 역기입 금지 |
| 해류 | ocean current field | 해양 유선/속도, 육지 마스킹 | 물리적으로 해양에서만, 자료 없으면 표시 안 함 |
| 기온·습도 | Core region field | 지구 구체 색상 범례 | 세계 시뮬 시간 기준 |
| 강수·구름·눈 | Core regional weather | 계절/지역 레이어 | 지역 관측치가 있을 때만 표현 |
| 환경/정착 | Core region history | 주민 4+4, 거주지, 교역/이주 사건 | 실제 생성된 시설·행동만 |
| 대기 오염 | 별도 자연/인간 배출·수송 모델 | 조건 충족 시 scalar overlay | 구현 전 임의 색상/숫자 금지 |

바람/해류는 lat/lon 등 간접축의 단순 2D 이동이 아니라 지구 표면
접평면 벡터에서 적분·보간해 범례와 방향을 제공한다.
경도 ±180° seam, 극지 인근, 육해 경계 및 지도의 시점 전환에서
불연속이 없어야 한다. 성능이 부족한 모바일에서는 파티클 수/밀도를
줄이거나 정적인 방향선을 사용하되 **Core 환경값은 동일**해야 한다.

Observer 계층:
`PlanetaryGlobe -> Continental/Regional -> Local Surface -> Settlement -> Resident`.
줌 전환으로 만들어진 새 지형/가상의 다른 날씨는 허용하지 않는다.
기존 local 패치가 정확히 지구의 해당 위치로 매핑돼야 한다.

## 5. 기존 EARTH-0~11과 통합된 작업 순서

### M0 — 현행 AI 기반 안정화 (병렬 의존성)
- [#664](https://github.com/sjLim91/lifelens-ue/pull/664) 경험학습 회귀 및 1,000일 감사,
  [#666](https://github.com/sjLim91/lifelens-ue/pull/666) 실제 대안 수원 선택,
  [#668](https://github.com/sjLim91/lifelens-ue/pull/668) 인지 결정 수락 이력,
  [#671](https://github.com/sjLim91/lifelens-ue/pull/671) 경험 관찰 UI.
- 환경 기능은 AI 전체 완료를 기다릴 필요가 없으나,
  공용 `World/Simulation` 파일에 대규모 충돌을 내지 않도록
  버전/주소/데이터 명세와 전용 read-only renderer부터 분리한다.

### M1 — EARTH-0 + EARTH-1: 자연지구 지오메트리/버전/지역주소
- 실제 지구 공개 라이선스 자료 검토 → 오프라인 경량 `EarthDataPack v1` 생성.
- coarse planet + regional LOD, WorldAddress↔lat/lon↔local tangent 변환
  정확성, 북남극/경도 seam, snapshot backward compatibility 검사.
- 마일스톤 게이트: 기후/도시를 가짜로 생성하지 않고
  자연 해안·산맥이 지구 구면과 local에서 같은 위치.

### M2 — EARTH-2 + [#329](https://github.com/sjLim91/lifelens-ue/issues/329): 진짜 지구본
- Three.js globe observer, local↔regional↔planetary scale handoff,
  camera pan/rotation/touch/mobile LOD, 정착지·활동 지역 Core marker.
- **처음 사용자에게 눈에 보이는 지구본**은 이 단계에서 출시.
- 글로벌 wind/current 텍스처가 아직 없다면 토글은 숨긴다.

### M3 — EARTH-3: 1지역 4명 + 2지역 4명
- 동일 Core 시간, origin-specific 지역주소, 독립 개별 판단/가족·생활.
- A를 보고 있어도 B는 진행; B를 추가해도 최초 접촉 전
  A의 무관한 난수 계열/자연지리 변화 없음.
- 지역 비교: 건강·식수·식생·개발·가족·사회/정착 이유.

### M4 — EARTH-4/5 + EARTH-FLOW: 위치별 환경/흐름 인과
- 지역별 태양/계절/기온/강수/습도/풍향,
  고도·육해/산맥·강수 그림자·연안/내륙 차이.
- 읽기 전용 바람/해류 유선 및 scalar 지도 토글을 단계별 구현.
- Core resident-local weather + 식수·식생·이동/건설/질병 노출
  경로를 연결. 현재 environmental consequences 계약은 유지.
- 주민들은 자기 지역의 실제 경험·인식·기억으로
  물·쉼터·탐험/이주를 다르게 선택한다.
- 자연적 오염, 질병, 사망은 다양한 결과를 허용한다.

### M5 — EARTH-6~11: 탐험·지역 간 이동·접촉·문명·인구 규모
- 여정은 실제 지역 경계/지리/수자원 탐색을 통과, 텔레포트 금지.
- 교역/지식/접촉과 지역 사회 발전은 Core event에서만 발생.
- 4→8→상위 인구는 simulation LOD·성능/결정론을 개별 검증.

**기존 EARTH-0~11의 구조적 순서를 대체하지 않는다.**
기존 명세의 선행 조건을 살리고 EARTH-FLOW를 Earth-4/5의
관찰·인과 개발 묶음에 삽입한다. 초기 Globe는 Earth-2에서 가시화한다.

## 6. 실패 방지 · 검증 게이트

1. **행성 좌표:** lat/lon↔region/chunk↔local roundtrip; 극지와 seam 교차.
2. **지역 차이:** 동일 세계 시간에 다른 경도는 현지 태양 시각이 다르고,
   남/북반구는 계절 위상이 다름. 동일 Earth pack+seed+version 재실행 동일.
3. **사실과 관찰:** 모든 Web overlay 샘플은 Core 또는 고정된 허가 소스
   read model에서 확인 가능; 카메라 전환은 역사에 영향 0.
4. **주민 영향:** 2×4 group의 강수/수원/열/바람/농업/이주 노출 차이가
   실제 Need/자원/행동 로그에서 추적 가능해야 함.
5. **생명·사회:** 다양한 발병/회복/사망/교역·결혼·출산을 인과적으로 검증.
   전원 생존, 반드시 임신, 강제 사회활동을 '성공'으로 오판하지 않는다.
6. **장기 안정:** 100/365/1000일 다중 seed, deterministic snapshots,
   원격 인구 진행, camera origin switching, resource mass balance.
7. **성능/비용:** Android 모바일 저사양 budget을 먼저 측정하고
   구체·유선·식생 LOD를 별도로 조정. 클라우드/API 유료 의존성 0.
8. **데이터 윤리/품질:** 공개 라이선스, 원본/version/attribution/해상도 기록.
   고정된 시뮬레이션을 '현실 지금 시각의 실측'으로 오표기하지 않는다.
9. **레거시 호환:** 현재 seed/기존 save는 이전 WorldGenerationVersion
   재생을 유지. 신규 실제 지구 geography는 명시 버전의 Scenario로 도입.

## 7. 현재 구현 상태 (2026-10-12 기준)

- 구현 완료 기반: `WorldHierarchy.h` Planet/SurfaceRegion identity,
  `WorldGenesis.h` chunk/seed, `EnvironmentalConsequences.h`의
  현재 local weather→Needs/Resource pressure, LifeLens Core/Web Observer.
- **미완:** 고정 실제 지구 데이터팩, globe↔local scale transition,
  globe 위 전구 풍향/해류 시각화, 2×4 origins 동시 simulation,
  지역별 globe 기후 사실 및 Core 이관, 위도/고도 기반 현실 기후 구분.
- 이 문서 자체는 **설계·개발 과정 편입**이며 위 미완 기능이
  이미 배포됐다는 뜻이 아니다. 기능은 별도 PR과 exact-head 검증으로 출시.

## 8. 다음 즉시 착수 가능 단위

- **EARTH-0/1 Feasibility & Data Contract PR:** 주소/데이터팩 스키마,
  저해상도 자연지리 입력·테스트 지오메트리, snapshot/RNG·기존 로컬
  회귀가 없는 정적 관찰 read model부터 시작.
- **독립 Globe Observer PR:** read-only sphere, zoom & region marker;
  실제 Core 지역 좌표를 추후 연결하도록 렌더러 인터페이스를 격리.
  `#329`의 항목을 점진 검증하며 visual dummy globe를
  실제 구현 완료로 속이지 않는다.
- **EARTH-FLOW CI audit:** 위도별 지역 기후 필드/해류 해양 마스크/
  오염 출처·직접 경험 유무·멀티시드 결정론 및 모바일 render budget.

최종 목표: **전지구 환경이 독립적인 장식이 아니라, 관찰되는 한 사람의
생존·기억·삶의 선택과 실제로 연결되는 살아 있는 지구.**
