# EARTH-1B: 실제 Natural Earth 육해 참조 필드 (2026-10-12)

**기존 LifeLens 설계 규칙 변경 없음.** EARTH-0 #674 구면 좌표,
EARTH-1A #676 explicit Core region origin binding 이후의 정적 자연 지구 데이터 계약.

## 출처·생성
- `EarthNaturalLandMask.h`는 Natural Earth Physical Land 1:110m GeoJSON 127개 자연 지형 폴리곤에서 생성된 **1° 격자 셀 중심점** 육지/바다 비트마스크.
- Upstream 고정: `nvkelso/natural-earth-vector@693f11422f4e08d2da4566b854dda53eb7c39fb3`
  `geojson/ne_110m_land.geojson` (Public Domain).
- Web에서 쓰는 `web/public/earth/ne_110m_land.geojson`와 동일 source/version.
- 360×180=64,800개 셀, packed 8,100bytes, 16,200자리 hex constexpr.
- 기준 좌표: 셀 중심 위도 `-89.5+y`, 경도 `-179.5+x`.
  폴리곤의 외부/내부 ring 교차 테스트·경도 seam ±360 shift로 rasterization.
  남극 단일 대륙 폴리곤은 남극점을 둘러싸는 360° 순환형이므로 경도별
  해안선에서 극점 방향으로 채우는 특례가 **데이터 rasterization 단계**에만 적용.
- 명시된 데이터 버전 `ne110m-land-centers-1deg-v1` 및 source commit 표기.

## 엄격한 용도 제한
- 참조 API `earthNaturalLandClassAt({lat,lon})`: Land / Ocean / Unknown.
  합법적인 EarthGeoPoint 외에는 Unknown. ±180° 날짜변경선 동일.
- 대륙 윤곽의 **개략적인 지구 참조**이다. 해안/섬 1도 미만 정보가 소실될 수 있음.
  특히 부산과 같은 복잡한 해안 셀은 오인식 가능.
- **이 자료로 기존 Core의 지형, 담수/염수, 자원, 정착지, 이동경로, 생존,
  농업, 질병, 기후를 바꾸지 않는다.** Freshwater≠Land. 주민의 경험/지식
  획득에도 이 coarse map을 즉시 알려주지 않는다.
- 2개 다른 지역의 Earth coordinate를 **명시적 시나리오 등록**할 준비 단계.
  실제 2×4 Origin Core activation, 지리 버전의 Snapshot save, Core Web DTO,
  지역 기후·해류 방향은 아직 구현 전.

## 검증
- 4개 대륙의 내륙 셀 / 대양 / 남극 / 경도 ±180° / 잘못된 위경도 / 1° grid.
- opt-in EarthRegionGeoCatalog과 연결해 좌표 → Natural Earth 셀 조회.
- World minute/RNG/chunks 무변화. exact-head Core/CMake/WASM/Preflight,
  기존 long-run 100/365/1000일 unchanged.
- 원본 소스가 달라지면 업스트림 commit, 데이터 버전, bit pack을 같이 변경하고
  샘플 및 지리 artifact diff 검증 필요.
