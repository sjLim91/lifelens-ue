Core 날씨·반복 보행·HumanWaste·불시설 상태가 지면과 시설에 읽히도록 Web 표현을 연결합니다. 기존에는 비/눈이 공중 particle에 치우치고, 보행·오염은 얇은 단색 patch, 불시설은 flame/glow 중심이라 실제 환경 결과가 약했습니다.

## Authority와 경계

- 작업 시작·재개·완료 준비 시 remote main을 조회: `4f3715e8b0983e8109397ab9eeb2d18c55514470`. 이후 새 main 변경 없음. 최신 main 기반이며 merge하지 않습니다.
- 실제 weather authority는 요청에 언급된 `DynamicEnvironment.h`가 아니라 `SimulationClimate.h::DynamicEnvironmentObservation`입니다. 기존 `WebClientBridge → DynamicEnvironment → WorldScene`의 `surfaceWetness01`, precipitation type/intensity, airTemperatureC, windIntensity01를 소비합니다.
- HumanWaste는 `HumanTraceReadModel → WebClientBridge`의 실제 `gridX/gridY`, `amount`, `intensity`, `radiusTiles`를 사용합니다. 이 DTO에는 age/kind 필드가 없으며 현재 Residue는 HumanWaste만 투영합니다. age나 decay를 추측하지 않습니다.
- 불은 시설 trace의 `state`, `active`, `lit`, weathering은 기존 civilization facility detail의 `durability`를 id/kind/위치 검증 후 사용합니다.
- 보행은 기존 `ObservedFootTraffic` session-local 실제 이동 관찰 메모리입니다. mark 위치·strength·fade·teleport/time-gap 계약은 유지합니다.
- **Core/World decides reality. Web only presents reality.** 새 저장 상태, 자원, road, terrain type, contamination, fire spread, AI/Needs/progression 변경 없음. Snow/puddle/scorch는 현재 observation에서 재구성되는 presentation이며 save하지 않습니다.

## 구현

| 범위 | 결과와 경계 |
|---|---|
| Wet ground | 기존 terrain vertex palette를 보존하고 material darkening/roughness를 갱신. 바위·목재·석재도 wetness 연결. 지면 roughness 최저 0.55, grass는 Lambert 유지하여 플라스틱 반사 방지. Core wetness 감소 시 원복. |
| Puddle | 관측된 height sampler에서 낮고 평평한 후보를 seed/chunk로 결정. 실제 water footprint·시설·steep/invalid/incomplete terrain 제외. 최대 128 patch를 단일 preallocated geometry로 지면에 맞춰 표시. wetness opacity를 따르고 current snow가 덮으면 가림. Water ResourceNode/음용/세척/navigation/save가 아님. |
| Traffic + mud | 동일 mark에 strength × wetness 기반 어두운 tint/opacity·작은 width 증가·roughness 감소. centerline/방향/방문 기록을 변경하지 않고 dry로 정확히 복구. single-pass shared draw call. |
| Snow surface | 실제 양수 Snow intensity + cold temperature + wetness에서 stateless coverage 계산. 현재 약한 눈도 얕게 읽히도록 표시 floor 적용. terrain/rock/시설 upward normal에만 shader modifier. Rain/따뜻함/강수 없음이면 즉시 0. depth/history/melt timer 없음. water material은 제외. |
| HumanWaste/residue | radiusTiles에 비례한 irregular soil footprint, intensity 기반 색/alpha, amount 기반 최대 8 작은 dark-soil clusters. 기존 trace geometry·picking draw call 안에 합침. DugPit의 작은 radius/intensity 자체가 작은 오염으로 보임. Core가 decay/removal하면 표시도 약화/제거. water contamination DTO가 없어 shoreline 오염을 추론하지 않음. |
| Smoke/scorch | Operational + active + lit FirePit/Furnace에서만 전역 최대 192 smoke points(8/facility). simulation minute와 seed로 동일 결과, windIntensity만 사용하고 drift 방향은 cosmetic. operational/ruined fire footprint에 최대 64 static/state-dependent scorch instances. 연료량·누적 사용일·화재 확산 추측 없음. |
| Facility weathering | 기존 durability condition-band/cache/ruined debris 보존. dry condition baseline에 wetness와 upward snow modifier 적용. 습한 동안 durability refresh가 발생해도 이중 darkening하지 않음. 마지막 instanced bedding 제거 시 GPU instance dispose도 보장. |

Shelter precipitation clipping은 낮은 우선순위 항목으로 보류했습니다. 기존 camera-centered rain/snow pool을 유지하며 raycast/collision이나 별도 강수 시스템을 추가하지 않았습니다.

## 파일

- 새 렌더링 모듈: `web/src/render/environment-surface-presentation.ts`, `surface-consequence-layer.ts`, `facility-emission-layer.ts`.
- 기존 연결: `world-scene.ts`, `ground-detail-layer.ts`, `foot-traffic-layer.ts`, `human-trace-layer.ts`, `facility-layer.ts`, `world-presentation-config.ts`.
- 테스트: `web/tests/environment-consequences/{run.mjs,fixture.html,fixture.ts,browser-review.mjs,.gitignore}`, 기존 `web/tests/presentation/run.mjs`의 asset stub에 새 snow setter 추가.
- CI: `.github/workflows/environment-consequences-check.yml`.
- 문서/복구: `docs/WORLD_ENVIRONMENTAL_VISUAL_FEEDBACK_v1.md`, `tasks/WORK_STATE.md`, `tasks/ENVIRONMENT_CONSEQUENCE_HANDOFF.md`, `tasks/HANDOFF_LOG.md`, 이 PR 본문 checkpoint.
- 새 외부 asset 없음. 기존 primitive/material/shader 사용. 기존 asset provenance/라이선스 계약 변경 없음.

## Budget와 모바일

- 추가 최대 **3 draw calls**: puddle 1 + scorch 1 + smoke 1. Snow/wet material modifier는 새 geometry/draw call 0. Traffic은 1 draw call, residue는 기존 1 draw call 유지.
- puddle 최대 128 × 16 triangles, scorch 64 instances, smoke 192 points, residue 64 traces × 최대 8 clusters, traffic 기존 512 marks. rain 1600/snow 700 기존 particle cap 그대로.
- Layout signature/cache 유지. 날씨/눈은 material/uniform 갱신, puddle은 opacity, smoke는 bounded observer-minute buffer 갱신. per-frame height scan/geometry/Object3D 생성 없음. 반복 refresh의 scene children/GPU geometry 안정성과 dispose 검사.
- 실제 모바일 GPU FPS 측정은 하지 않았습니다. 390×844 SwiftShader WebGL fixture는 모바일 viewport/readability·GPU shader·draw budget 검증입니다.

## 검증과 A~F 결과

- Product HEAD: 3cac0a9f5f29011efff21ec88cc4ae585edeb0c3
- [최종 Web environmental CI](https://github.com/sjLim91/lifelens-ue/actions/runs/37123316493): typecheck, build, 새 환경 회귀 **9**, 기존 presentation **64**, HumanTrace **11**, weather **7**, resident motion **15**, primitive bedding/Korean era suite PASS.
- 로컬: typecheck, 환경 회귀 9, presentation 64, HumanTrace 11, weather 7, runtime resilience 18, resident continuity 13 PASS. 의존성은 CI artifact에서 가져와 로컬 network blocker를 해결했습니다.
- A dry / B rain+wet / C rain+traffic / D cold Snow / E HumanWaste+contained profile / F active FirePit+Furnace를 **390×844와 1280×900**에서 캡처했습니다. 같은 seed/minute/camera, real layer + read-only synthetic Observer DTO fixture입니다. 실제 Core 장기 시나리오 실행으로 주장하지 않습니다.
- [Screenshots + regression/browser metrics artifact](https://github.com/sjLim91/lifelens-ue/actions/runs/37123316493/artifacts/11273383579): 12 PNG, shader/browser errors 0, 모든 반복 refresh에서 scene children/GPU geometry 안정. 비+active fire 동시 fixture에서 추가 pool draw calls = 3 확인.
- 직접 캡처 검토 후 snow 위의 puddle 점을 가리고, 얕은 snow 표현·wet ground·흙 cluster·활성 smoke를 확인했습니다. fixture 재실행: `npm install --no-save playwright && npx playwright install chromium && node tests/environment-consequences/browser-review.mjs`.

| 390×844 fixture | A dry | B rain | C mud | D snow | E residue | F fire | Rain + fire budget |
|---|---:|---:|---:|---:|---:|---:|---:|
| 전체 draw calls | 46 | 47 | 48 | 46 | 46 | 50 | 52 |
| 새 pools의 draw calls | 1 | 2 | 2 | 1 | 1 | 2 | 3 |

| 1280×900 fixture | A dry | B rain | C mud | D snow | E residue | F fire | Rain + fire budget |
|---|---:|---:|---:|---:|---:|---:|---:|
| 전체 draw calls | 59 | 60 | 61 | 59 | 59 | 63 | 65 |
| 새 pools의 draw calls | 1 | 2 | 2 | 1 | 1 | 2 | 3 |

## 호환·충돌·재개

- #605: `NaturalResourceProjection`과 quantity/depletion/regrowth masking 변경 없음. weather refresh가 scenic tree/resource proxy를 재생성하지 않음. 기존 자원 연동 회귀 PASS.
- #611: resident motion/facing/LayToIdle 변경 없음, 회귀 PASS.
- #612: Core sleep destination/weather-aware sleep, primitive bedding geometry/support, civilization era 변경 없음. 기존 regression PASS. facility climate modifier와 제거 시 dispose만 연결.
- 현재 open #613/#614는 Core performance lane이며 이 렌더링 pass와 분리됩니다.
- 이후 main에서 충돌 가능: `world-scene.ts`, `facility-layer.ts`, `ground-detail-layer.ts`, `human-trace-layer.ts`, `foot-traffic-layer.ts`, `world-presentation-config.ts`, feedback docs 및 공용 `tasks/WORK_STATE.md`/`HANDOFF_LOG.md`. 재개 때 실제 main/branch/PR/Actions를 다시 조회하고 변경된 main 위로 rebase할 것.
- 클론 없이 GitHub connection으로 branch/tree/commit 생성. 안전 checkpoint와 CI run은 handoff에 기록. 초기 local network 호출이 장시간 멈춰 사용자 중단 후 복구했고, 성공을 추측하지 않고 GitHub 상태와 artifact로 검증했습니다.
- **PR까지만 생성. main merge / auto-merge 설정 금지.**
