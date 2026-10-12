# EARTH-1A: Core 지역↔지구 좌표 명시적 바인딩 계약 (2026-10-12)

기존 Legacy 평면 청크를 임의 지구상의 도시/위치라고 거짓 표시하지 않도록
EarthScenario에 속한 **버전이 고정된 region geographic placement**를 정의한다.

## 구현
- `EarthRegionScenario.h`의 `EarthRegionScenarioBinding`에 WorldSeed,
  WorldGenerationVersion, EarthDataVersion, geography source, Core
  SurfaceRegionCoord 및 Core가 계산한 RegionId, lat/lon 원점,
  Core GridPos anchor와 meters/cell이 함께 들어간다.
- World는 이 데이터를 생성하지 않는다. Scenario/자연지리 데이터팩이
  지역 배치와 실제 지리 원점을 명시적으로 공급해야 한다.
- Catalog에 허가된 동일 버전 팩만 추가하고 지역 중복/타 월드 ID/
  잘못된 위경도 및 현재 Core region 밖의 grid origin을 거부한다.
- `geographicPositionForGrid`는 존재하는 binding이 없으면 false.
  Unknown location, WorldGenerationVersion mismatch, 다른 월드 시드는
  위치 출력 금지. PopulationSeed는 지리 주소 불변.
- 등록 순서와 관계없이 RegionId순으로 저장, RNG·Core World/Save/Schema·
  기존 지형/기후/생존·주민 AI 변경 없음.
- 테스트에 사용한 위경도는 **시나리오 계약의 예시**다.
  부산이나 케이프타운에 현재 주민이 살고 있다는 표기가 아니다.

## 이번 단계의 한계
아직 Core가 World에 영구적으로 `EarthScenarioGeoCatalog`를 붙여서
World snapshot과 동기화하지 않는다. 이는 설계자가 위임하지 않은 기존
save schema/시작 지역 선택 변경을 막기 위한 의도적인 이행 단계다.
구체 지구본에 현재 주민 마커를 자동으로 표시하지 않으며,
2×4 동시 시작/지역별 기후 또는 바람·해류 simulation은 후속 개발.

## 다음
Earth geography pack → scenario geospatial selection → Core World authoritative
binding와 snapshot/replay versioning → Web DTO marker → Globe/Local handoff →
2×4 실제 지역 주민 → 지역별 기후/생태/사회 AI 인과 연결.

관련 #672, #674, #329.
