# EARTH-0: 명시적 지역 지구좌표/구면 수학 어댑터 (2026-10-12)

## 목적

LifeLens의 기존 **평면 Core GridPos, ChunkCoord, WorldHierarchy**를 그대로 유지하면서,
신규 EarthScenario에서 명시적으로 위경도를 부여한 지역을 지구 구면으로 투영하는
읽기 전용 수학 계층을 만든다.

**아직 실제 자연 지구 데이터, 기후, 지역별 4+4, Globe renderer를 구축한 것이 아니다.**
이 어댑터를 토대로 지구 데이터팩/LOD/관찰자 맵핑을 실제 구현한다.

## 계약

`Source/LifeLensCore/include/lifelens/EarthSurfaceCoordinates.h`

- `EarthGeoPoint`: 위도 [-90,90], 경도 [-180,180]. 출력 경도 [-180,180).
- `earthGlobeUnitVector` / `earthGeoForUnitVector`: 구면 3D 위치의 변환.
- `earthGeoAtLocalOffset` / `earthLocalOffsetForGeo`: 동/북 접평면 방향에 대한 지표면 측지 exponential/log map; 경도 ±180° seam/북남극 통과 시 연속.
- `EarthRegionGeoAnchor`: 실제 region ID + 지리 원점 + 기존 GridPos 원점 + **명시적인 m/grid**. Legacy flat Grid가 자동으로 실제 Earth 위치를 갖는 척하지 않는다.
- `earthGeoForGrid` / `earthContinuousGridForGeo`: opt-in 매핑, 기존 A*·Collision·Plan·Save 코드를 변경하지 않는다.
- `earthCubeUVForGeo` / `earthGeoForCubeUV`: six-face tile addressing 원형. 안정적인 face 벡터만 제공하며 완전한 earth tile 인덱스/저장 ID는 미구현.
- 수학적 투영 반경은 명시된 평균구 반지름(6,371,008.8m)이며 실제 WGS84 타원체 측지 정밀도를 주장하지 않는다. 지역 투영 유효 범위 250km; 전 대륙을 하나의 flat patch로 펼치지 않는다.
- 값은 **관찰/주소 계산용**이고 현재 Core가 실제 지구 자연 해안, 해류, 고도를 이미 알고 있다고 주장하지 않는다.

## 회귀 범위

- 기존 Simulation/Main/Needs/Climate/WorldGenesis/WorldHierarchy, WASM 바인딩, binary snapshot codec, version, RNG state **무변경**.
- 신규 CMake Core test `test_earth_surface_coordinates`에서 적도/남북반구, 부산 인근, ±180° 자오선 통과, 북극 통과, UV 6면, 잘못된 lat/lon, 원격 대상/불가능한 anchor 거부, 음수 청크/좌표, PopulationSeed 격리 검증.
- 기존 `test_world_hierarchy`, Core exact-head, WASM, Preflight와 seed4242001/874213954 1000일 예전 Core state 불변 감사. 관찰 수학만 추가했으므로 기존 World truth·Save 해시 동일해야 한다.

## 다음 작업

1. [EARTH-FLOW #672](https://github.com/sjLim91/lifelens-ue/issues/672) 실제 Earth 자연지리 자료의 라이선스/버전 고정 및 지리 pack.
2. [Globe Observer #329](https://github.com/sjLim91/lifelens-ue/issues/329) Earth region / planet read model + Three.js 구체 렌더러.
3. EARTH-3 2×4 origins, EARTH-4/5 지역 기후·수원·식생과 생활 인과 연결.
4. [COG-4 #670](https://github.com/sjLim91/lifelens-ue/issues/670) 주민이 직접 경험한 지리·환경 차이를 학습해 자발적 선택하는 루프.
