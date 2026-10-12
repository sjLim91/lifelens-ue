# EARTH-5C — 실제 Core 태양 관찰을 Web 지구본에 노출

- Source: EARTH-5B #682의 Core WASM `earthSolarJson()`. 기존 주민 World/Needs/AI/Weather/Calendar/Snapshot/RNG 변경 없음.
- 경로: `LifeLensCoreBridge.earthSolar()` → `WorldSession.refresh()` → `ObserverStore.earthSolar` → 지구본 현재 실제 Core 지역의 태양 정보.
- 월드 시드, generation version, region ID, simulation minute, 위도/경도를 모두 현재 WorldOverview와 EARTH-2C Core 시나리오에 맞춰 검증한 경우만 표시.
- 미결속 legacy world, 데이터 누락/버전 불일치/이전 시간 데이터/비정상 값은 `available:false`로 즉시 차단한다. Web에서 태양시간을 추정하여 생성하지 않는다.
- 표시: 지역 현지 태양시(태양 기준, 법정 표준시 아님), 태양고도, 천문학적 일조 시간, 지평선 위/아래. 지역별 기온·비·바람·주민 행동에는 적용되지 않음을 반드시 명시.
- 기존 지구본 지역 A/B 선택과 동일 시뮬레이션 주민 생활 확대 경로를 유지.
- Contract: `cd web && node tests/earth-solar-observation/run.mjs`, Web Typecheck/Build, Core WASM, 기타 Earth LIFE gate.
- 지역별 실제 기후·낮밤 렌더링·동물 habitat 등은 후속 PR로 분리.
