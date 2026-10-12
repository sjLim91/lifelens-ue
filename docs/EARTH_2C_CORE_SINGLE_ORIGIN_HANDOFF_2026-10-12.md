# EARTH-2C: Core-authorized single-origin globe→actual LifeLens observer handoff

2026-10-12 사용자 정정: 지구본이 별도 장식 화면이 아니라 **확대하면 주민의 실제 생활이 보여야 한다.**

## 범위

- 새 `WebClientBridge::newEarthGame(seed,lat,lon)`은 사용자가 명시적으로 고른 실제 육지 위치만 받는다. 1° Natural Earth land-mask가 Ocean/Unknown/invalid로 판정하면 기존 게임을 파괴하지 않고 거부.
- 기본 `newGame`은 기존 방식·기존 시드와 완전히 동일하며 Earth binding 없음.
- 승인된 Earth 시작은 **기존 Core 4명 신생 월드를 생성**하고 founder runtime grid + SurfaceRegionID를 실제 선택 위치에 opt-in 바인딩. RegionID/WorldSeed/GenerationVersion과 묶여 다른 월드 지형을 해석하지 않는다.
- `earthScenarioJson()`은 Core 인증된 월드 시드·실제 founder 위치·선택된 위경도·Natural Earth 데이터 버전·지역 ID를 반환하고, **`terrainGeographicallyConditioned:false`, `metricScaleProvisional:true`, `snapshotPersistence:false`**를 반드시 표시한다.
- Browser는 Core가 돌려준 binding이 없는 일반 월드에서 지도 위 마을 마커를 만들 수 없다.
- Planet→Region→Living Area 관찰 전환은 Core가 해당 지역을 연결한 경우에만 허용. 기타 지역은 자연지리만 보여주고 존재하지 않는 주민 생활을 렌더하지 않는다.
- 현재 생활환경의 procedural terrain은 완전한 실제 Earth 지형·산맥·하천/해안이 **아직 아니다**. 물리적 의미를 지닌 지역 terrain-conditioning, 기후, 멀티 origin 4+4, snapshot-persisted scenario는 EARTH-3/4/5에서 별도 구현한다.
- 이 API는 런타임-only **초기 관찰 위치 매핑 계약**이며 아직 영구 저장된 full Earth Scenario가 아니다. 재생 시작 시 같은 explicit seed+geo 입력이 필요하다.
- Core 수면·인과/이벤트·RNG를 바꾸지 않는 read-only binding, 질병/출산/자원 강제 없음.

## 검증

`test_web_earth_scenario_handoff`는 결속 없음/명시 입력/육해 거부/입력 오류 거부/다른 월드 독립/Reset/Legacy vs Earth observation baseline의 **일치하는 생존 인구/개인/원시지형/22분 행동 기록**을 검증.

WASM, Core, Preflight, 지구 지역 주소/육해/좌표 CI 및 Snapshot 결정론 모두 통과 후 Web 지역 전환 코드를 병합.
