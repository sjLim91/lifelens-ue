# EARTH-2B: 지구본에서 두 지역 좌표 직접 선택 (2026-10-12)

EARTH-2 #675 Natural Earth 미리보기에서 **지역 A / B의 실제 위도·경도 후보**
점을 선택할 수 있게 한다. 해당 선택은 브라우저의 일시적 UI state다.

- Three.js Raycaster가 실제 Earth globe sphere mesh의 UV를 반환.
  GeoPoint 변환: lon=-180+u*360, lat=-90+v*180 (texture equirectangular).
- 북/남극, 국제 날짜변경선과 UV 범위/NaN 검사. pointer 이동 7px 이상은
  카메라 드래그로 처리, 짧은 클릭/터치만 지역점 선택.
- 서로 다른 색상의 A/B 마커를 globe mesh 자식으로 부착해 회전 시
  동일 지점을 유지한다. 웹/모바일 전용; 마커 geometry/material 해제.
- 별도 마커는 Core 주민·정착·이동을 표현하지 않는다.
  **실제 생성된 월드 주민 위치가 이 좌표로 자동 변경되지 않는다.**
  UI는 미배치/좌표후보임을 분명히 표시한다.
- 위경도 표시값은 UI 참고값이며 저장/불러오기/WorldSeed 또는
  Physical Simulation truth에 영향을 주지 않는다.
- Core [#674](https://github.com/sjLim91/lifelens-ue/pull/674) Earth
  geodesic API 및 [#676](https://github.com/sjLim91/lifelens-ue/pull/676)
  시나리오 지리주소 계약과 연결하기 위한 관찰 UX의 첫 단계.
- 후속 EarthScenario 신규 월드 시작 선택/버전 고정된 자연지리 팩/두 지역
  실제 4+4 창립집단 활성화는 별도 Core-authoritative 단계에서 구현.

검증: `node web/tests/earth-globe-regions/run.mjs`, 타입 검사/웹 빌드,
기존 지구본 회전/수면/새 월드/영아/관찰 화면 회귀 검증.
