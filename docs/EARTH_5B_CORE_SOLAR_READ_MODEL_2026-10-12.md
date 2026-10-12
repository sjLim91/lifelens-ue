# EARTH-5B Core 기반 지역 태양 관찰 모델 출력

#681의 권위 있는 EarthSolarObservation 계산을 #679 EARTH-2C 시나리오에 연결한다.

- `WebClientBridge::earthSolarJson()` / WASM `earthSolarJson` 메서드: WorldSeed, regionId, simulation minute, Core 위경도 및 현지 태양시·태양고도·일조시간 값만 반환.
- 지구 결속이 없는 기존 시드는 `{"available":false}`.
- `regionalWeatherApplied:false`: 해당 실제 지구 태양과 현재 Core의 온도/풍향/자원은 **미연동**.
- World Truth / Health / Needs / RNG / SimulationCalendar / binary Snapshot / 인구 행동은 변경하지 않는다.
- Web UI는 후속에서 `CoreEarthSolar`/WorldMinute/RegionId를 검증한 후 정직하게 표시해야 한다.
- `test_web_earth_scenario_handoff`: 비결속/결속/22분 시계 변화 및 새 Legacy World 전환 검증.
