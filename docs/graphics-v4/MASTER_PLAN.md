# LifeLens Graphics V4 — Codex 실행 마스터플랜

- 작성일: 2026-11-25
- 기준: `main` / `4b2039780aa39c6bc0580f6519875751bcc74093`
- 실행: 사용자 지정 Codex Astra 6.2 Low. 모델 설정은 사용자가 관리한다.
- 목적: 모바일 게임 수준의 자연스럽고 통일된 2.5D 관찰 화면. Core 및 게임 콘텐츠 변경 금지.
- 상태: 계획 및 핵심 소스 정적 감사. 아직 코드 변경, 테스트 실행, 배포 검증 없음.

## 0. 변경 금지 계약

1. LifeLensCore, WASM 진실값, AI, 행동 선택, 월드 시드, 주민 GUID, 인구, 생태, 지형 높이, 수계, 시설 위치, 저장 데이터, 시간 진행 규칙을 변경하지 않는다. 읽기 전용 presentation adapter만 허용한다. 부족한 정보는 진단으로 남기며 사실인 것처럼 생성하지 않는다.
2. 사용자 레퍼런스는 화풍·환경 밀도·구도·공간감만 참고한다. RPG 몬스터·특수 구역·퀘스트·전투·장소 복제 금지. 현재 사진 속 결함을 재현 대상에 포함한다.
3. 기존 3D/2D/지역 지도/지구 관찰 경로를 검증 없이 삭제하지 않는다. V4는 별도 기능 플래그로 구현하고 마지막 통합 단계에서만 기본 모드를 전환한다. 안전 모드 보존.
4. 무료 로컬 자산 및 라이브러리만 사용. 라이선스/출처 확인. 유료 API/외부 런타임 금지. 물리 실기기 테스트는 제외하고 브라우저 검증 필수.
5. 테스트 삭제·완화, 실패 숨김, fixture를 실제 월드인 것처럼 보고, 임의 오브젝트 배치로 가짜 품질 개선 금지. 자동 검증과 미적 승인 구분.
6. 소스 변경 전 최신 main 재확인, 기준 커밋 이후 변경점 비교. 기존 작업과 충돌하면 최신 코드에 맞춰 범위를 재검토하고 기록. Core 무변경은 diff 검사로 확인.

## 1. 감사 근거와 현재 결함

전체 Git 트리 1,860개 항목을 조회하고 주요 활성 현장 렌더러, 투영, 자산 로더, 주민, 자연환경, 물, 날씨, 엔진 연결, 3D 렌더러, QA 스크립트와 기존 계획서를 정적 검토했다. 전체 그래픽 파일의 본문/바이너리/실행을 모두 검증한 상태는 아니다. G00에서 파일 단위 전수 점검을 완결한다.

| ID | 소스 | 확인한 사실 / 검증할 결함 |
|---|---|---|
| A01 | `web/src/render/render-mode.ts`, `web/src/observer-engine.ts`, `web/src/App.tsx` | field/isometric/legacy/three 4개 렌더 모드. `view=2.5d`는 field, `view=region`은 isometric. 코드 기본은 three, 저장 설정과 호환 프로필에 의해 달라짐. 사용자 화면과 정확한 배포 SHA는 미확인. |
| A02 | `field-projection.ts` | x축 정렬, y 깊이 .77, 높이 계수 1.30, 줌 .6~3.6. 고정 높이를 받은 역투영. 실제 경사지형 picking 일관성 검사 필요. |
| A03 | `field-terrain-compositor.ts`, `field-observer-renderer.ts` | Canvas 청크 기반 텍스처; bitmap을 축 정렬 사각형으로 그리는 코드, land에만 meadow 적용, Lake/Ocean은 별도 색. 사용자 화면 사각 수역의 유력 원인이나 재현 검증 필요. 지형 폴리곤 fallback과 텍스처 경로의 경계 비교 필요. |
| A04 | `field-river-presentation.ts` | Core 링크 중심점을 S자 stroke 3겹, 최대 11px로 연결. 하천 표면·수심·합류 geometry 없음. |
| A05 | `field-natural-art.ts`, `field-scene-clarity.ts`, `field-observer-renderer.ts` | coverage 기반 군집, clearance로 장식 위치 제외, 성인 픽셀 크기에 따른 나무 크기, foreground 투명화. 나무 누락은 culling/clearance/asset/occlusion별 계측 필요. |
| A06 | `field-asset-atlas.ts`, `web/public/field/` | 20종 WebP 우선/SVG fallback, 4종 motion sheet, meadow-floor. 수동 artwork bounds, 단순 tint/mirror. 모션 sheet에는 명시적 onerror 처리 없음. |
| A07 | `field-resident-presentation.ts`, `field-resident-actions.ts` | 8방향 heading은 존재하지만 bitmap 호출은 sex/garment/frame만 전달, 좌우 반전 이외 방향별 atlas 미구현. sleep 단일 프레임. 이미지 성공 시 vector fallback보다 먼저 return. 개체 외형, 발/침구 접촉, 행동 전환 별도 검증 필요. |
| A08 | `field-observer-renderer.ts` | Deer/Hare 이외 종은 skip, 동물은 static sprite, depth는 screen y, shadow ellipse. 기존 native wildlife 자산은 별도 경로에 있음. |
| A09 | `field-settlement-illustration.ts`, `field-scene-clarity.ts` | 시설 기반 ground wash, 시설 종류별 sprite; footprint·작업/건설 단계의 통합 표현 검증 필요. |
| A10 | `field-weather-presentation.ts` | Core clock/weather 기반 global tint와 rain/snow screen-space stroke; 광원·그림자·지형/표면 효과의 통합 부족. |
| A11 | `terrain-geometry.ts`, `water-geometry.ts`, `world-renderer.ts` | Three 기반 지형 sampler, geometry builder, 물 surface/flow geometry, frame pacer와 dispose 존재. 재사용 후보이며 품질/계약/비용 검증 후 선택. |
| A12 | `Tools/probe_lifelens_field_browser.mjs`, `.github/workflows/field-visual-qa.yml` | 연결/인구/2종 resident sheet/16개 WebP/36색/스크린샷 16KB 기준의 smoke test. 시각 결함 검출/모드 전환/장시간 성능 QA와 별개. workflow는 PR 및 수동 실행, release WASM 사용. 정확한 소스 SHA와 runtime contract 추적 필요. |

기존 문서: `docs/FIELD_VISUAL_REFERENCE_CONTRACT_2026-11-19.md`, `docs/FIELD_F2E_MOBILE_VISUAL_CORRECTIONS_2026-11-19.md`, `docs/FIELD_VISUAL_REBUILD_V3_2026-11-23.md`.

## 2. V4 기술 결정 (제안; G01에서 실측 확정)

제품 화면은 2.5D 고정 사선 직교 시점. 내부 구현은 기존 Three.js/WebGL의 경량 terrain/water mesh + 저비용 오브젝트/캐릭터, 필요한 경우 방향별 alpha sprite 혼합을 우선 검토한다. 자유 회전 3D 게임으로 복귀하지 않는다. 기존 Canvas 확대/축소만으로 완료하지 않는다. G01에서 고정 카메라 mesh prototype과 수정된 Canvas 삼각형 투영 prototype의 동일 seed 품질/성능 비교 후 하나를 선택, ADR로 잠근다. 동시 생산 렌더러 2개를 만들지 않는다. 무거운 GLB 전부 재사용은 금지; LOD/경량화 검증 후 자산별 선정. 가시 기하와 Core picking 일치가 최우선. WebGL 실패 시 기존 안전 렌더러와 명시적 상태 안내.

목표 파이프라인: Core snapshot → read-only normalized scene → terrain/hydrology → foliage/static assets → facilities/residents/wildlife → light/depth/weather → observer HUD. 하나의 coordinate contract, 공통 scale, 공통 광원, 공통 picking/ground contact. UI 상태와 Core 상태를 분리한다.

## 3. 마일스톤과 작업 순서

각 G번호는 작은 PR/커밋으로 분리 가능. 같은 파일을 동시에 수정하지 않는다. 선행 gate 실패 시 후속 통합 금지. 계획/테스트/자산 inventory는 독립 병렬 가능. 아래 파일명은 현재 존재하는 수정 후보이며 새 모듈의 이름은 G01 ADR에서 고정한다.

| ID | 범위 | 산출물 및 인수 기준 |
|---|---|---|
| G00 | 전수 감사 및 baseline | `git ls-files`, import/reachability 분석으로 `web/src/render/**`, `character-layer.ts`, engine/input/runtime/UI/CSS, public field/vendor, Tools, tests, workflows, 그래픽 docs 분류. 각 파일/자산별 활성 경로, 역할, 위험, 유지/재사용/교체/격리, 근거, 테스트를 CSV/MD로 작성. 바이너리 치수/alpha/pivot/atlas frame/GLB triangles/textures/animations/license 확인. 실제 Core seed 고정 baseline 캡처 및 frame metrics. 기존 화면 사각형 재현과 paint pass 추적. |
| G01 | 좌표/카메라/렌더러 선택 | `field-projection`, `isometric-projection`, `world-projection`, `camera-ground-projection`, `terrain-geometry`, `water-geometry`, `world-renderer`, `observer-engine` 비교. 작은 실험 장면에서 지형 높이/picking/zoom/pan/성능 검증. ADR, scene adapter, capability flag, resize/dispose 계약. 기존 모드 유지. |
| G02 | 지형 전면 교체 | 공유 경계 vertex/삼각형 mesh 또는 동등한 warp raster. Core elevation/biome 혼합, 연속 UV, 다중 주파수 재질, 경사/능선/계곡, 실제 기복만 표시. 격자/사각형/반복 무늬 노출 금지. 좁은/넓은 줌과 청크 이동 seam 검증. |
| G03 | 수계 전면 교체 | `water-geometry`, `water-layer`, `water-surface-material`, `field-river-presentation` 재검토. Lake/Ocean union 경계, shore transition, 높이·물 footprint 준수, river mesh/ribbon 연결·합류·물가·흐름. coarse Core 자료만 있으면 표현 보간은 허용하되 진실값 변경 금지; 실제 없는 수역 생성 금지. 사각 평면/파란 경로선 제거. |
| G04 | 통일 아트/자산 파이프라인 | `public/field`, `public/vendor`, `field-asset-atlas`, `procedural-asset-kit`, `world-visual-style`. 동일 조명/시점/색감/물리 비율의 style sheet, manifest(id, source, license, pivot, bounds, variant, LOD, action, direction). 자동 alpha/크롭/프레임 검사. 무료 기존 자산 선별 후 부족한 자산 제작. 임시 에셋은 완료 금지. |
| G05 | 식생/바위/자원 | `field-natural-art`, `field-scene-clarity`, `vegetation-*`, `tree-asset-layer`, `procedural-tree-*`, `ground-detail-layer`, `natural-resource-projection`. Core coverage 기반 seeded 군집과 실제 자원 구분. 종/크기/실루엣 변형, 숲 그늘/하층 식생, 카메라 이동 시 배치 안정, depth/culling/occlusion 디버그. 물/시설 침범 금지. |
| G06 | 주민 외형/모션 | `field-resident-*`, `resident-*`, `character-layer`, vendor characters. GUID 기반 외형 유지, 나이/성별/복장/체형, 8방향 표현 또는 동등한 rig rotation, 이동/정지/수면/식사/채집/제작/건설/육아/사회 행동 등 실제 Core 상태 매핑. foot/hip/bed anchor, 전환/stride, click hitbox. 겹침은 물리 위치 보존하며 occlusion/표시로 처리; Core 동일 좌표라면 원인 기록. |
| G07 | 야생동물 | `wildlife-layer`, `wildlife-motion`, `native-wildlife*`, `field-observer-renderer`, vendor wildlife. Core species inventory 전체 확인, 현재 지원 종 전부 정확한 asset/size/pose, 방향과 이동, 물고기 수면 관계, 사망/사냥 상태가 Core에 있으면 대응. 미지원 명시 및 fallback은 다른 종으로 위장 금지. |
| G08 | 시설/문명/생활 흔적 | `facility-*`, `field-settlement-illustration`, `human-trace-layer`, `foot-traffic-layer`, `settlement-*`, `production-*`, `artifact-presentation`. Core 시설 종류/좌표/건설 진행/생산/농경/저장/불/연기/거주지와 일치. 발자국/흙길은 실제 활동의 표현만. 미완 시설을 완성 건물로 표현하지 않는다. |
| G09 | 빛/날씨/계절 | `field-weather-presentation`, `weather-layer`, `atmosphere-layer`, `surface-consequence-layer`, `environment-surface-presentation`. 통일 광원, 부드러운 접지/차폐 그림자, 밤 가독성, Core 비/눈/구름/안개/계절/젖은 지면 지원 상태에 따라 표현. 날씨 임의 생성 금지. |
| G10 | 카메라/관찰 UI/지도 | `camera-input`, `globe-input`, `App.tsx`, `observer-shell.css`, `observer-controls`, `observer-readout`, `region-map-preview`, `earth-globe-preview`, `illustrated-region-map`. 현장/지역/지구의 좌표와 선택 상태 일치, 터치 pan/pinch, 줌 앵커, 주민 추적, 세로/가로/PC, HUD 축소, 한국어, 오류 모드 명시. 지역/지구 지도가 현장처럼 보이는 것을 목표로 하지 않는다. |
| G11 | 모바일 성능/수명주기 | `render-frame-pacer`, `render-performance`, `device-performance`, `observer-engine`, `world-renderer`, runtime resilience. spatial culling, atlas/material 공유, instancing/LOD, bounded cache, DPR cap, throttling, dispose, context loss, session/hidden-tab. Core tick이나 AI 품질을 낮추는 방식 금지. |
| G12 | 통합/배포/정리 | 기존 Web/Presentation/Input/Runtime/Core gate 모두 통과, 새 시각 gate 및 비교 캡처 검토. V4 기본 모드 전환, safe/region/globe 유지. 중복 코드 삭제는 reachability 및 regression 근거 확보 후 별도 커밋. GitHub Pages 배포 SHA, WASM contract/version, 실제 외부 URL, asset network error 확인. 미적 승인 없으면 '기술 검증 완료/시각 승인 대기'로 명시. |

의존성: G00 → G01 → G02 → G03. G04는 G01 이후 병렬로 자산 준비 가능. G05~G08은 G02/G03/G04 이후 모듈별 병렬, 공통 파일 변경은 순차 통합. G09/G10은 scene contract 고정 후 독립 진행 가능. G11은 단계마다 측정하고 통합 후 최종 확인. G12는 모든 선행 단계 후.

## 4. QA 및 인수 계약

- `cd web && npm ci` (lockfile 있는 경우; 없으면 저장소 CI의 설치 방식), `npm run typecheck`, `npm run build`. 기존 `web/tests/tests.json`, workflow 및 preflight를 읽고 실제 명령을 G00에서 고정한다. 존재하지 않는 npm test script를 가정하지 않는다.
- 새 QA는 screenshot 16KB/색상 수/asset count만으로 통과하지 않는다. geometry seam, shoreline correspondence, river connectivity, picking, direction, pose/asset missing, render mode, resource cleanup을 독립적으로 assert.
- seed 고정 실제 Core WASM 장면: 초원/숲/호수 또는 해안/하천/기복/정착지/낮/밤/비/눈을 데이터 근거가 있는 장면에서 캡처. 찾지 못한 상태는 미검증으로 보고. synthetic fixture는 별도 라벨.
- 뷰포트 360×800, 412×915, 915×412, 1440×900 CSS px. 각 주요 장면 wide/mid/close, pan/zoom 전후. 동일 seed/시간/카메라 baseline/after. 이미지 비교와 사람이 판별 가능한 검토표. 스크린샷의 사각 수역/잘린 나무/검은 placeholder/비정상 투명 경계/스케일 오류/방향 오류/겹침 점검. 정상 스프라이트 투명 배경은 허용.
- 10분 이상 브라우저 지속 관찰 + 반복 모드 전환/resize/zoom. 제안 목표: 모바일 에뮬레이션 평균 30fps 이상, p95 frame interval 50ms 이하, 초기 warm-up 후 메모리/geometry/texture/cache 개수 안정, 오류 0. CI 하드웨어와 실제 모바일 성능은 구분. G00에서 측정 환경/기준 고정하고 미달은 원인/수치 기록. 무조건 달성했다고 보고하지 않는다.
- Core read-only 입력 replay로 동일 snapshot identity/좌표/행동/시설/수계/시간/저장 상태 보존, presentation RNG 독립, renderer mode 전환으로 simulation restart/추가 writer 금지. 테스트 fixture만으로 실제 Core 연결 검증 대체 금지.
- 배포본 SHA 확인, Pages build 완료 및 외부 접속 probe, 새 브라우저 프로필과 기존 localStorage 양쪽 확인. 실패 시 이전 검증된 기본 모드 유지/롤백.

## 5. Low 모델 운영 지시

매 작업 시작 전 이 계획, `AGENTS.md`(존재 시), 관련 계약/기존 테스트를 읽는다. 한 작업에 하나의 기능 목표. 임의 구조 변경/범위 확대 금지. G00 결과와 ADR을 기준으로 파일 범위를 확정. 작업을 작은 독립 커밋으로 나누고, 각 커밋마다 검증한 뒤 진행표 업데이트. 실패한 테스트는 원인 해결; 변경한 기대값은 계약 변경의 근거 없이는 허용하지 않는다. 테스트가 통과하지 않거나 환경 차단이면 구체적인 차단 원인과 재현 명령을 남긴다. 독립 작업은 계속하되 선행 의존 작업을 억지로 완료하지 않는다. 실제 장시간 실행 또는 백그라운드 작업을 했다고 허위 보고하지 않는다.

권장 기록 파일: `docs/graphics-v4/MASTER_PLAN.md`, `AUDIT_INVENTORY.csv`, `ADR-001-RENDERER.md`, `PROGRESS.md`, `QA_MATRIX.md`, `ASSET_MANIFEST.json` (저장소 규칙에 맞춰 경로 조정 가능). 각 G 단계 진행 상태 NOT_STARTED/IN_PROGRESS/BLOCKED/VERIFIED/VISUAL_REVIEW_PENDING/ACCEPTED. 근거: SHA, 변경 파일, 명령+결과, 캡처 경로, 미검증 항목.

### Codex 최초 입력

이 문서를 작업 기준으로 저장소에 등록하라. 최신 main과 기준 SHA의 차이를 확인하고 G00부터 시작하라. 전수 감사에서는 그래픽 관련 파일의 목록만 작성하지 말고 본문, import graph, 실제 활성 경로, 자산과 테스트를 확인하여 파일별 판정을 기록하라. 사용자 캡처 결함을 재현하고 baseline을 저장하라. G01에서 렌더러 선택 ADR을 확정하기 전에는 대규모 자산 교체/기존 렌더러 삭제를 하지 마라. 이후 의존성 순서대로 작업하고, 각 단계 gate 및 증거를 남겨라. 시뮬레이션 Core는 변경하지 마라. 기존 사용자 레퍼런스는 아트 방향만 참고하고 콘텐츠는 가져오지 마라. 기존 승인 및 저장소 정책에 따라 PR/병합/배포를 진행하고 실제 검증 결과만 보고하라.