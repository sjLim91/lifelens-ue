# LifeLens 2.5D 전환 — M0 Canvas 아이소메트릭 월드 베이스

작성: 2026-11-18. 기준 `main`: 0419a681 (#1045). 목표: 3D 전체 교체 중 첫 **실행 가능한** milestone.

## 구조 결정

Core -> WASM -> WorldSession -> Web Observer 권위 구조는 그대로 둔다.
`?view=2.5d`에서 WebGL 컨텍스트를 만들지 않는 별도 Canvas2D 아이소메트릭
렌더러가 현재 월드의 *실제* Core 데이터를 관찰한다.

현재 P0/M0은 **병행 전환**이다. 3D 기본/비상 Canvas는 M1~M4 기능 이식과
검증을 위해 남긴다. 기능 동등성 없이 3D를 삭제하거나 2.5D를 유일한 제품
경로라고 선언하지 않는다. M5 이후 정식 기본값 전환을 검토한다.

## 작동 기능

- `web/src/render/isometric-projection.ts`: 45° 고정 쿼터뷰. Core chunk/grid
  인덱스를 결정적 무원근 screen 좌표로 투영. 반대 변환은 실제 지형을 샘플링해
  화면 pan/zoom 기준점을 구한다.
- `web/src/render/isometric-canvas-world-renderer.ts`: 실시간 Core `TerrainWindow`
  17×17 elevationSamples에 근거한 고도 표면. 저사양 Canvas에서 상세 패치의
  그리기 해상도를 제한하는 화면용 LOD. 주변 청크 대표 고도만 복사하지 않는다.
- 수계는 Core `waterKind`와 `hasDownstream`이 존재할 때만 물 표면/
  흐름을 그린다. 방향, 소스, 소진 상태를 임의 추정하지 않는다.
- 화면 외형용 숲/바위 실루엣은 Core vegetation coverage 기반 *장식*이며
  실제 독립 원물 자원 노드로 해석하지 않는다.
- 현재 Core 좌표의 주민을 GPU 없는 2.5D 마커로 표시하고 탭/클릭 선택한다.
  실제 HumanTrace의 유효한 시설·자원사용·환경 흔적, 실제 활성 정착지만
  표시·선택 가능하며 Core에 없는 시설을 추가하지 않는다.
- 원 손가락 탭, PC 왼쪽 드래그, 핀치 및 휠 줌을 지원한다.
  이 모드만 터치와 왼쪽 드래그로 카메라를 이동하며, 3D의 기본 조작은 보존.
- React UI 상단에서 2.5D/기존 3D 보기 전환 가능.
  `?view=2.5d`, `?view=3d`, `?view=legacy` URL과 설정으로
  rendering만 전환하며 월드 Seed/Core/저장물은 변경하지 않는다.
- Android WebView에서도 `?view=2.5d` 경로는 WebGL 없이 사용할 수 있다.
  `?safe=1`은 기존 안전한 Canvas 비상 경로를 강제한다.
- 2.5D 렌더러 오류는 기존 Canvas rescue로 전환되어 WASM 시뮬레이션을 계속 관찰한다.

## 인수

`web/tests/presentation/isometric-m0-checks.mjs`에서 다음을 검증한다.

- world-to-screen 및 screen-to-world 회복, pan과 고도
- 실제 Core 17×17 고도 샘플 변경 반영, 원본 DTO 불변
- 실제 Core downstream edge 없을 때 가짜 하천을 잇지 않음
- 실제 주민과 HumanTrace에 대해서만 마커 생성/클릭 가능
- URL 전환 시 월드 Seed, safe 파라미터, hash 불변
- 전체 Web 타입검사·회귀테스트·빌드 및 Preflight
- 병합 시 GitHub Pages와 외부 WASM Core browser boot 검사

## M1~M5 미완료

- M1: 고도 상세 입체감을 가진 자연환경/수계/절벽, 생태 자원 정밀화
- M2: 주민과 동물 8방향 2.5D sprite, 상태별 animation, 움직임 연속성
- M3: 시설 2.5D 자산/공사/재고/컷어웨이 완전 이전
- M4: 관찰 UX / 모바일 프레임 비용 및 고밀도 월드 렌더 최적화
- M5: 모든 화면 기능 검증 후 2.5D 기본값 전환 및 3D legacy 정리

이 PR만으로 '라이프렌즈 전체 2.5D 전환 완료'라고 주장하지 않는다.
