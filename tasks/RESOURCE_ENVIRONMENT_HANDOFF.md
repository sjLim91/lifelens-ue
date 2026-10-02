# Web 자연 자원 / 환경 통합 — PR #605 검증 완료

- PR: https://github.com/sjLim91/lifelens-ue/pull/605
- 브랜치: `work/web-resource-environment-20261002`
- 상태: HOLD / 사용자 요청대로 PR만 생성. main merge 금지.
- 최종 동기화 main: `705d917ba4ebce9585d9748cfe4e102a17ee81b2` (#606). 시작 후 #601/#604/#606의 Core-only 변경을 비교했고 Web/수정 파일 overlap이 없다.
- 마지막 검증 코드 HEAD: `d1dbdb0b7cf426b5eaf825ed8b252237d7ac6631`.
- 이후 closeout/main 동기화 commit은 이 문서/WORK_STATE와 main의 Core-only 진전을 반영한다. 이 PR의 Web 제품 코드는 동일하다. 최신 CI 근거는 PR 본문에도 기록한다.

## 원인 / 구조

일반 숲과 지면 식생은 coverage만 받고, 별도 spatial target layer는 accessGrid에 자원 프록시를 그렸다. quantity 0의 프록시는 사라져도 환경은 변하지 않았다.

`NaturalResourceProjection`은 `civilization.resources`에서 immutable grid 기반 영역과 presentation 후보를 만든다. 고갈 노드도 영역에 포함한다. 근처 coverage 후보는 자원 영역에서 억제하고, 고정 seed/id 후보의 수/크기를 quantity/maxQuantity로 조절한다. 같은 재고/seed면 동일한 결과이며 별도 성장 clock/history/state는 없다. renewable/regenerationPerDay는 DTO 상태일 뿐 Web에서 재생을 계산하지 않는다.

- Wood: 기존 GLTF tree asset 및 foliage palette 유지. full stock은 성목, 감소 시 개체 감소/일부 작은 수목, zero는 해당 소유 영역에 성목 없음. 회복 시 같은 후보에 어린 나무부터 성목 복구. 그루터기 state는 저장하지 않으며 빈 공간으로 벌목을 표현한다.
- PlantFood/Fiber: 관목/풀 기존 instance 사용. 실제 자원 영역의 herbaceous baseline을 억제하고 quantity 감소/회복에 따라 수/크기 변화.
- Stone/Flint/CopperOre/TinOre: 기존 rock instance와 광물별 tint 사용. stock 감소 시 개체 수/크기 감소, zero는 해당 자원 instance 제거.
- Clay: 기존 rock batch에 넓고 낮은 적갈색 노출 지면 형태. quantity 비례 수/크기와 zero 제거.
- 동일 재질 영역이 겹치면 가장 가까운 node가 소유하며 id로 동률 결정. 실제 다른 재질은 함께 존재 가능.
- 자원 없는 coverage 지역 유지. 후보별 water/facility guard와 Core accessGrid clearance 적용. 경계/음수 chunk 높이 샘플링 유지.
- HumanTrace ResourceUse는 실제 관찰된 지면 흔적/선택 정보로 유지하며 현재 stock을 trace에서 추론하지 않는다.
- Core AI/채취/재생/이동/시설/Needs 규칙 변경 없음.

## 성능

- 개별 Object3D 추가 없음. 기존 InstancedMesh/GLTF instance buffer 재사용.
- 별도 자연 자원 mesh 9개 제거. 새 draw call 없음.
- 모바일 per-chunk 최대: 나무6, 풀14, 관목4, 돌5. global capacity도 기존 그대로.
- 광물 슬롯을 노드별로 먼저 나눠 특정 Stone 노드가 Clay/광석 예산을 독점하지 않게 한다. 고갈 슬롯을 baseline에 재배정하지 않는다.
- 동일 resources reference + viewport에서 projection index 재사용. 동일 내용의 새 snapshot은 signature로 GPU 갱신 생략.
- Wood stock signature와 ground stock signature 분리: 광물만 채취되면 숲 instance buffer는 갱신하지 않는다.
- 500ms observer cadence 변경 없음. 자원 갱신은 terrain geometry rebuild를 호출하지 않는다.
- 실제 모바일 GPU timing/GLTF 화면 품질은 측정하지 않았다. 자동 검증은 실제 renderer matrix/instance 자료를 검사하고 tree asset 네트워크 loader만 stub한다.

## 수정 파일 (14)

- `web/src/render/natural-resource-projection.ts`: 새 공통 projection/cache.
- `vegetation-layer.ts`, `ground-detail-layer.ts`: 자원 후보/서명 소비, 기존 asset/palette/budget 유지.
- `authoritative-spatial-target-layer.ts`: 자연 자원 프록시 제거, accessGrid helper/sanitation 유지.
- `world-scene.ts`: civilization resources 공급 및 갱신 분리.
- `world-presentation-config.ts`: 영역/후보/크기/clearance/광물 tint 상수.
- `human-trace-layer.ts`: 현재 재고 authority와 지면 흔적 역할 명시 (동작 변경 없음).
- `web/tests/presentation/resource-environment-checks.mjs`, `run.mjs`: 15개 신규 회귀 통합.
- `Tools/validate_web_authoritative_spatial_targets.py`: 자원/environment 계약 보강.
- `Tools/validate_web_presentation_v2_environment.py`, `validate_web_world_activity_observation.py`: 새 setTerrain projection 인자 검증.
- `tasks/RESOURCE_ENVIRONMENT_HANDOFF.md`, `tasks/WORK_STATE.md`: 복구 정보.

## 검증

로컬:
- 순수 projection 회귀 12/12 PASS (Node24 type stripping, 외부 dependency 없음).
- authoritative spatial/ecology/v2 environment/world activity validator PASS.

코드 HEAD d1dbdb0의 PR CI:
- Preflight: https://github.com/sjLim91/lifelens-ue/actions/runs/37018917517 — PASS.
- Web Typecheck: https://github.com/sjLim91/lifelens-ue/actions/runs/37018917095 — PASS.
- Runtime Resilience: https://github.com/sjLim91/lifelens-ue/actions/runs/37018916969 — PASS.
- Runtime Resilience에는 typecheck, 전체 presentation 64/64 (신규15개), runtime/continuity/weather/character/input/feed/action/trace/fast-forward 회귀 및 production build 포함.
- full/감소/zero/회복/결정성, baseline 유지, 경계/음수 좌표, 물/시설/accessGrid 회피, proxy 제거, 모바일 budget, 광물 간 예산 공유, 종류별 cache를 검증.
- Core 소스 변경 없으며 별도 Core/CTest/WASM rebuild는 이 Web presentation-only PR의 필수 path gate로 실행되지 않음.

## 실패 / 복구 / 충돌 주의

- shell git clone은 proxy 접속 실패. 추가 네트워크 권한 요청을 사용자가 취소했다. 다시 권한 요청하지 않는다. GitHub connector로 source snapshot/branch/commit/PR 작업했다.
- 초기 Preflight 실패는 두 기존 validator의 이전 `setTerrain(window)` 문자열 요구였다. 새 projection 계약으로 보강하여 최종 PASS.
- 로컬 `/workspace/lifelens`는 connector source snapshot이다. 저장소 clone으로 착각하지 않는다. 원격 브랜치가 authoritative checkpoint다.
- 초기 main 8a5d26d 이후 #601을 비교하여 새 main 기준을 반영했다. 해당 Core 기술교육 수정과 파일 overlap 없음.
- 충돌 가능: WorldScene, vegetation/ground-detail/spatial-target layer, world-presentation-config, presentation run.mjs, 관련 validators, WORK_STATE. 최신 main/PR 파일을 다시 비교하고 필요한 변경만 보존한다.
- 재개 시 actual main/PR head/Actions를 조회하고 이 코드 HEAD와 비교. 제품 코드가 같으면 위 CI 근거 재사용. 코드가 바뀌면 관련 회귀/CI만 다시 수행. 사용자 승인 없이 merge하지 않는다.
