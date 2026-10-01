# 지형 격자 경계 개선 인계

- 기준 main: `e49f2544ad33ea0df95e9a40d755e7dc09fe8d13`.
- 별도 브랜치: `visual/terrain-grid-seams-20261001`.
- 사용자 사진: seed 9567016843, 1일 13:18. 저장본/정확한 카메라는 확보되지 않았다.
- 확인 원인: 실제 3D `WorldScene.terrainColor`가 청크마다 단색 material을 지정하고, `computeVertexNormals()`가 분리된 청크 각각에서 법선을 생성한다. 공통 `terrain-presentation.ts`와 별도 경로다.
- 이전 #563의 공통 팔레트 보간만으로 3D 격자가 해결됐다고 판단하면 안 된다.
- 범위: 실제 3D terrain vertex color의 이웃 보간, 공유 경계 normal 연결, 장식 음영 완화, 이웃 의존 캐시 무효화, 해당 회귀 및 본 문서.
- Core/DTO/store/수계 geometry/자원/시설/이동/지형 높이를 변경하지 않는다. #539/#540과 분리, main 병합·배포 없음.
- 구현 완료, CI 전 복구 checkpoint. 작업 후 아래에 검증 결과를 갱신한다.
- 물가 외곽의 큰 계단형 수계 윤곽은 별도 원인이다. Core 수계 계약을 확인하지 않고 임의 해안선을 만들지 않는다.

## 구현
- `terrain-surface.ts`: 실제 3D 팔레트와 chunk-center 색 보간, 5×5 이웃 기반 캐시 키. 선형 색공간으로 색을 보간한다.
- `terrain-geometry.ts`: 위치/UV/index 개수 그대로, vertex color에 연속 색 적용. 공유 경계의 인접 bilinear 지형 기울기를 평균해 normal 연결. world 좌표에 고정한 작은 음영도 매끄럽게 보간.
- `world-scene.ts`: 청크별 단색 tint 제거. 공통 white material × vertex color, 기존 젖음 darkening/roughness 유지. 이웃 식생/수분/암석/고도 변화에도 cache 갱신.
- `terrain-seam-checks.mjs`: 두 축·음수 좌표의 색/법선 경계 일치, 높이/위상 보존, 원점/순서 안정성, 결측 이웃, 실제 WorldScene 캐시 갱신/재사용/폐기 검증.
- 추가 메시/텍스처/네트워크/타이머 없음. 변경된 창에서 geometry 생성 시 이웃 조회·색/법선 연산은 늘어남. GPU draw call 수는 늘리지 않음.
