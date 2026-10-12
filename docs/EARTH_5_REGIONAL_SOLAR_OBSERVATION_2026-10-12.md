# EARTH-5: 지역별 실제 구면 일사량·현지 태양 시각 기반 (2026-10-12)

## 실제 추가 범위

\`EarthSolarObservation.h\`는 Earth's 평균 구면 좌표와 LifeLens 365일 calendar의 날짜/UTC 전역 시간을 결합하는 **읽기 전용 일조 관찰 함수**이다.

- 북반구 춘분 day0, 하지 day91, 동지 day274라는 기존 365일 Calendar phase를 이용. 축 기울기 23.439281도 근사.
- 경도별 태양시(local solar hour)와 위도별 태양고도·일조 길이·태양고도 기반 잠재 일사량 산출.
- 극지 24시간 낮/밤, +180/-180 seam, 북남반구 계절 반전 검증.
- \`SolarHour\`는 정치적 시간대/DST/대한민국 시각이 아닌 **태양 위치 기반 시간**. 대기/구름/기압·공기 미세입자/해류/실제 기후를 재현한 수치가 아니다.
- 일출/일몰 지형 차폐, 대기굴절, 타원체, 윤년과 365.2422 평균 회전주기 등은 후속 모델 범위; 기온·강수·수계가 이 수치와 이미 연결되었다고 주장하면 안 된다.
- **현재 게임 규칙에는 적용하지 않음.** 기존 \`SimulationCalendar\`, \`SimulationClimate\`, \`Needs\`,\`World\`, binary snapshot, RNG, WASM 바인딩과 세계 결과는 그대로다.
- EARTH-4/5 단계에서 지구 위치가 authoritative binding 된 경우에만 기존 EnvironmentConditions의 입력 경로로 단계적으로 이관; 다중 시드 인과 감사 및 모바일 LOD 사전 수행.

## 검증

\`test_earth_solar_observation\`: 춘분 정오, 경도 90도 시차, 날짜변경선 경도 일치, 상반된 계절, polar day/night, deterministic values/invalid input, 음수 시각 정규화. EARTH-FLOW #672 및 확대 관찰 #680과 연결.
