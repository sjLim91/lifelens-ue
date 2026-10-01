# 지형 격자 경계 개선 인계

- 기준 main: `e49f2544ad33ea0df95e9a40d755e7dc09fe8d13`.
- 별도 브랜치: `visual/terrain-grid-seams-20261001`.
- 사용자 사진: seed 9567016843, 1일 13:18. 저장본/정확한 카메라는 확보되지 않았다.
- 확인 원인: 실제 3D `WorldScene.terrainColor`가 청크마다 단색 material을 지정하고, `computeVertexNormals()`가 분리된 청크 각각에서 법선을 생성한다. 공통 `terrain-presentation.ts`와 별도 경로다.
- 이전 #563의 공통 팔레트 보간만으로 3D 격자가 해결됐다고 판단하면 안 된다.
- 범위: 실제 3D terrain vertex color의 이웃 보간, 공유 경계 normal 연결, 장식 음영 완화, 이웃 의존 캐시 무효화, 해당 회귀 및 본 문서.
- Core/DTO/store/수계 geometry/자원/시설/이동/지형 높이를 변경하지 않는다. #539/#540과 분리, main 병합·배포 없음.
- 구현 및 자동 검증 완료. [Draft PR #566](https://github.com/sjLim91/lifelens-ue/pull/566), 구현 커밋 `c50d950ce39f6c977c05c4e0e229964bea1bb6c5`.
- 물가 외곽의 큰 계단형 수계 윤곽은 별도 원인이다. Core 수계 계약을 확인하지 않고 임의 해안선을 만들지 않는다.

## 구현
- `terrain-surface.ts`: 실제 3D 팔레트와 chunk-center 색 보간, 5×5 이웃 기반 캐시 키. 선형 색공간으로 색을 보간한다.
- `terrain-geometry.ts`: 위치/UV/index 개수 그대로, vertex color에 연속 색 적용. 공유 경계의 인접 bilinear 지형 기울기를 평균해 normal 연결. world 좌표에 고정한 작은 음영도 매끄럽게 보간.
- `world-scene.ts`: 청크별 단색 tint 제거. 공통 white material × vertex color, 기존 젖음 darkening/roughness 유지. 이웃 식생/수분/암석/고도 변화에도 cache 갱신.
- `terrain-seam-checks.mjs`: 두 축·음수 좌표의 색/법선 경계 일치, 높이/위상 보존, 원점/순서 안정성, 결측 이웃, 실제 WorldScene 캐시 갱신/재사용/폐기 검증.
- 추가 메시/텍스처/네트워크/타이머 없음. 변경된 창에서 geometry 생성 시 이웃 조회·색/법선 연산은 늘어남. GPU draw call 수는 늘리지 않음.

## 검증 결과
- [Web Typecheck 36827069002](https://github.com/sjLim91/lifelens-ue/actions/runs/36827069002): PASS.
- [Preflight 36827068936](https://github.com/sjLim91/lifelens-ue/actions/runs/36827068936): PASS.
- [Web Runtime Resilience 36827069111](https://github.com/sjLim91/lifelens-ue/actions/runs/36827069111): PASS. TypeScript, 기존 Web 회귀, 신규 지형 경계 회귀 5개와 production build 포함.
- 경계 color/normal 동일성은 두 축/음수 좌표/부분 스트리밍 창에서 검사했다. vertex 높이는 기존 sampler와 대조했고 vertex 121개/index 600개를 유지한다.
- 렌더 원점 이동/청크 순서 변경의 안정성, 변경 없는 geometry 재사용, 이웃 식생 변경 시 실제 WorldScene geometry 교체/폐기를 검사했다.
- **실제 모바일 스크린샷 A/B는 미완료.** 테스트 통과를 화면 검수 완료로 쓰지 않는다. 기기 FPS/geometry 갱신 시간도 별도 측정이 필요하다.
- 로컬 `/workspace/lifelens-grid`는 부분 소스 폴더. npm/브라우저 제한 때문에 원격 CI를 사용했다. main/Pages/다른 작업 브랜치는 변경하지 않았다.

## 챗에서 이어갈 순서
1. 실제 GitHub main / #566 / Actions를 재조회한다. 이 브랜치에는 Core 변경이 없으며 감사 브랜치를 merge하지 않는다.
2. 같은 Core 저장 상태·카메라·시간으로 이전 main과 수정본의 땅 표면을 비교한다. 사용자 사진의 seed만으로 정확한 기존 장면 재현을 보장하지 않는다.
3. 경계를 확대하고 카메라 이동/스트리밍 후에도 색·밝기 사각 테두리가 재발하지 않는지 확인한다. 낮/밤/비의 젖음 표현과 물가/식생 위치가 바뀌지 않았는지 본다.
4. 남는 큰 계단형 해안선은 `water-geometry.ts`의 Core waterKind 기반 marching polygon과 지형 교차를 별도 진단한다. 수계 위치/수위/접근점/식생 제외 영역의 일치 계약을 보존하고, 장식으로 물이나 땅을 임의 추가하지 않는다.
5. 기기 검수 결과를 이 문서와 PR에 추가하고 병합·배포는 별도 결정한다. 현재 링크에서 이미 수정이 보인다고 안내하지 않는다.
