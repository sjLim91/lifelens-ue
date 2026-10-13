# 지형 접지 보정과 직접 제작 동물 자산

ResidentGroundSurface가 v4 17×17 Core 고도 표본을 네 모서리로 축소하던 문제를 수정했다. 실제 렌더링과 같은 16×16 삼각형 및 a-c 대각선을 사용하고, 기존 10×10 지형도 실제 삼각형을 그대로 샘플링한다. 주민의 권위 있는 X/Z/이동 경로는 바꾸지 않는다.

토끼·사슴·늑대·담수어 메시를 Three.js geometry로 직접 제작했다. 외부 자산이나 유료 API가 없으며 기존 나무·풀·관목·꽃/열매 자산을 유지한다. 네 species batch에 공용 geometry/material을 사용하고 최대 128개를 관측 중심에서 가까운 순서로 표시한다. 동물 ID/좌표/나이는 Core DTO만 사용한다. 누락/구형 runtime 데이터는 동물을 표시하지 않는다.

경량 trace refresh에 wildlife DTO를 전달하며 개체 소멸·월드 교체에서 이전 인스턴스를 지운다. 지형 높이/발바닥 범위로 지상 동물을 접지하고 수면 footprint/시설 내부는 렌더링에서 제외한다. 물고기는 실제 수면 geometry 아래에 몸이 들어갈 깊이가 있을 때만 표시한다. 얕은 수면의 물고기는 가려질 수 있다.

동물 이동은 현재 Core의 일별 위치를 표시하며 연속 보행 애니메이션이나 사냥 표현은 없다. Core 야생동물 관측 PR이 함께 반영되어야 실제 동물이 나타난다. 기존 저장은 해당 Core 계약상 야생동물을 새로 만들어내지 않으므로 신규 v4 게임에서 검수한다.

로컬 검증: 렌더링 메시 raycast 1,144점과 접지 높이 비교, 상세/기존 지형·음수 청크·origin rebasing·범위 밖 좌표 검사 통과. 동물 4종의 결정론/finite geometry/바닥 위치 및 Core-only projection·중복 제거·개체 제거·구형 runtime·복원·dispose 검사 통과. 기존 절차적 환경 12개 및 lifecycle 회귀 통과. Blender CPU geometry diagnostic으로 모델 형태를 확인했다.

로컬 Chromium은 sandbox socket 제한으로 시작하지 못했다. 서버 network permission 요청에서 대기가 발생해 중단했으며, 이후 권한 재요청 없이 CPU diagnostic을 사용했다. 실제 WebGL/TypeScript/build/WASM 실행 검증은 GitHub CI와 기기 검수가 남아 있다. WebGL gallery fixture/capture를 기존 CI에 추가했다. CPU diagnostic은 실제 게임 화면 검수가 아니다. 자동 병합하지 않는다.
