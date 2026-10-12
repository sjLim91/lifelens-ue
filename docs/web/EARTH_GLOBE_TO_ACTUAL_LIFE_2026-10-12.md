# EARTH-2C — 행성 → 지역 → 실제 주민의 생활 관찰

사용자 정정(2026-10-12): 별도 지구본 팝업은 목적이 아님.
지구를 돌려서 확대하면 **해당 위치에 실제로 생활 중인 사람을 관찰**해야 한다.

Core PR #679과 본 Web PR은 새로운 지구 시나리오를 **명시적으로 시작한 경우**
위치가 Core에 등록된 초기 한 지역에 한해서 실제 관찰 전환을 제공한다.

- 상단 버튼은 **지구 관찰 / 주민 관찰** 전환. 지구는 월드 뷰포트를 채우는
  같은 관찰 단계이며, Core의 ticking/시간은 이 전환으로 중지되거나 재시작되지 않음.
- Natural Earth land data는 실제 대륙/해안 윤곽만 나타냄.
- A/B 터치 핀은 단순 후보. Core가 `newEarthGame`을 수락하기 전에는
  마커/주민/시설이 그 지역에 있다고 **표시할 수 없다**.
- 현재 기본 평면 Core 월드는 지구 좌표가 없으므로 Planetary 화면에서
  직접 주민에게 확대할 수 없음. 육지 후보를 고르고 확인 후
  **새 월드를 만드는 명시적 사용자 동작**이 먼저 필요함.
- Core는 WorldSeed/WorldGenerationVersion/SurfaceRegionId/FounderGrid/
  Natural Earth coarse land-mask로 신생 월드의 지구 원점을 검증. 이후
  `earthScenarioJson`에서 같은 월드의 Core-authoritative 지리 정보만
  반환함. Browser는 mismatched seed/edition, 이상 좌표, 허구 source,
  불명확한 datum을 fail-closed 방식으로 숨김.
- 같은 Core 월드를 **지구 화면 ↔ 기존 주민/건설/사회 활동 관찰 화면**에서
  계속 볼 수 있음. 다시 지구본으로 돌아가도 기존 주민 상태·배속 유지.
- 현재는 단일 지역 4명. 원하던 1위치4명/2위치4명은 EARTH-3 후속.
- **정확도 제한을 숨기지 않음:** 현재 로컬 지형/기후/수원/식생은
  지구 위치 실제 자연환경으로 조건화되지 않았다. 좌표 연계는 구현됐지만
  실제 부산의 산맥/강/바다를 그리는 단계는 아직 아니다. 메트릭 스케일은
  임시이며 지구 시나리오 위치를 binary save에 지속시키는 기능도 미완이다.

테스트는 `web/tests/earth-life-navigation/run.mjs` + WASM/Core + Web
Typecheck/Build/기존 화면 CI. 미지원 이전 WASM은 새 Earth 기능을
fail-closed로 비활성화하며 기존 Web 관찰은 그대로 유지한다.

다음 최우선: Core 로컬 지형을 선택된 좌표의 실제 지구 육해/고도/수문
데이터로 조성하고, 두 지역을 한 개 Core simulation authority로 동시
실행하는 EARTH-3/4/5. 그 이후 Planetary→Regional→Local 스케일을
단순 화면 전환에서 실제 연속 LOD로 진화시킨다.
