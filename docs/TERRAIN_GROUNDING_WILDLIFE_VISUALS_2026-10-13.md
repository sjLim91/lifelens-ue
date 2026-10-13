# 지형 접지 보정과 직접 제작 동물 자산

ResidentGroundSurface가 v4 17×17 Core 고도 표본을 네 모서리로 축소하던 문제를 수정했다. 실제 렌더링과 같은 16×16 삼각형 및 a-c 대각선을 사용하고, 기존 10×10 지형도 실제 삼각형을 그대로 샘플링한다. 주민의 권위 있는 X/Z/이동 경로는 바꾸지 않는다.

토끼·사슴·늑대·담수어 메시를 Three.js geometry로 직접 제작했다. 외부 자산이나 유료 API가 없으며 기존 나무·풀·관목·꽃/열매 자산을 유지한다. 네 species batch에 공용 geometry/material을 사용하고 최대 128개를 관측 중심에서 가까운 순서로 표시한다. 동물 ID/좌표/나이는 Core DTO만 사용한다. 누락/구형 runtime 데이터는 동물을 표시하지 않는다.

경량 trace refresh에 wildlife DTO를 전달하며 개체 소멸·월드 교체에서 이전 인스턴스를 지운다. 지형 높이/발바닥 범위로 지상 동물을 접지하고 수면 footprint/시설 내부는 렌더링에서 제외한다. 물고기는 실제 수면 geometry 아래에 몸이 들어갈 깊이가 있을 때만 표시한다. 얕은 수면의 물고기는 가려질 수 있다.

동물 이동은 현재 Core의 일별 위치를 표시하며 연속 보행 애니메이션이나 사냥 표현은 없다. Core 야생동물 관측 PR이 함께 반영되어야 실제 동물이 나타난다. 기존 저장은 해당 Core 계약상 야생동물을 새로 만들어내지 않으므로 신규 v4 게임에서 검수한다.

로컬 검증: 렌더링 메시 raycast 1,144점과 접지 높이 비교, 상세/기존 지형·음수 청크·origin rebasing·범위 밖 좌표 검사 통과. 동물 4종의 결정론/finite geometry/바닥 위치 및 Core-only projection·중복 제거·개체 제거·구형 runtime·복원·dispose 검사 통과. 기존 절차적 환경 12개 및 lifecycle 회귀 통과. Blender CPU geometry diagnostic으로 모델 형태를 확인했다.

시각 갤러리의 관목 import를 실제 procedural-tree-generator export로 수정했다. 물고기 위치는 성장 크기와 실제 geometry 전체 높이를 사용해 수면 위 돌출과 바닥 침하를 방지하고, 얕은 수면에서는 표시하지 않는다. 어린 개체/성체의 실제 수면·바닥 경계 검사도 PASS.

최종 PR head `75459bf9be00befe79521d4ff30eb414e277fef3`의 GitHub Preflight와 공통 Web Checks PASS: 전체 회귀·typecheck·build, 실제 생태 fixture, 모바일/데스크톱 장면과 동식물 WebGL gallery 포함. gallery는 browser 오류 없이 52 draw calls / 12,485 triangles. artifact: https://github.com/sjLim91/lifelens-ue/actions/runs/38384373697 . 실제 새 게임 seed77의 Core 관측 24개 중 화면 레이어 16개 투영을 별도로 확인했다. 로컬 Chromium의 socket 제약은 GitHub 실제 WebGL 검사로 보완했으며 실제 휴대폰 GPU 검수는 별도다.

PR #711은 `6135ea4afb5e9a4dfa163b1ba0c2e4526a130de8`으로 main 병합. Web Preview run 38385310238, Pages deployment 38385364177, External Preview Probe 38385372273 모두 PASS. 화면 source는 `6135ea4`, Core runtime source는 #710의 `db4d931`. 실행: https://sjlim91.github.io/lifelens-ue/index.html?v=6135ea4 . 동물은 신규 v4 게임에서 확인한다.
