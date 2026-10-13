# LifeLens 자체 절차적 환경 — 구현 및 검증 기록

검토 PR: https://github.com/sjLim91/lifelens-ue/pull/703

기준: 최초 분석 main `8ba73745`, PR parent `3fcc5dd2`. 주민 자체 리그·키프레임·IK 작업 #700 (`5203449a`)이 이미 포함된 상태다. 작업 중 추가된 인지 B3c 변경은 그대로 보존했다. 이 PR은 Web presentation만 수정한다.

## 1. 외부 의존성 분석

| 영역 | 기존 | 이번 변경 |
|---|---|---|
| 나무 | Quaternius CC0 GLB 3개, 합계 1,295,216 bytes, 런타임 원격 로드 | 자체 메시만 제품 경로에서 생성. 기존 loader/고정 출처는 QA 비교용으로 보존 |
| 풀·관목·바위 | 교차 사각형·정다면체 | 구부러진 풀잎, 줄기별 접힌 잎, 좌표 기반 암석 변형 |
| 지형·수계·날씨 | Core DTO 기반 직접 생성 메시/셰이더 | 공유 고도/수계 계약 보존. 지형을 별도 생성하거나 이동 영역을 변경하지 않음 |
| 시설·자재 | 직접 생성 기본 도형과 상태별 조립 | 테이퍼 목재·끝면, 모서리를 다듬은 판재, 불규칙 석재, 열린 용기, 목재 결·섬유 직조 셰이더 |
| 주민·모션 | 고정된 CC0 character/UAL GLB와 #700 자체 모션·IK | 이번 변경 없음. 기존 검증을 그대로 실행 |
| Earth | 내부 구체/지역 변환 + 체크인된 Natural Earth 지리 데이터 | 변경 없음. 지리 데이터는 완성형 3D 에셋이 아님 |

새 유료 API·서비스·에셋을 사용하지 않는다. 비교 GLB 다운로드는 기존 제품의 고정된 자료만 사용하며 테스트 review 폴더에만 저장한다.

## 2. 직접 제작 목록과 공통 시스템

- 나무: 참나무형, 자작나무형, 침엽수형. 각 2개 시드 변형. 테이퍼/굽은 줄기, 밑동 뿌리, 재귀적 가지, 접힌 개별 잎/침엽 잎 무리. 침엽수는 5방향 층별 가지 구조.
- 식생: 길이·폭·굽힘이 다른 풀잎 군집, 다중 줄기 관목. 기존 환경 coverage·군집 배치·자원량 계약에 연결.
- 사물: 통나무/가지/구조 목재, 판재, 암석/광물 표현, 열린 저장 용기, 목재 결과 직조 섬유 표면.
- 시설: 기존 창고·화덕·작업대·침상·쉼터·용광로·농경지 구성 요소와 실제 건설 진행/폐허 표현에 위 메시를 연결.
- 재사용: `AssetMeshBuilder`, 별도 presentation RNG, 참조 수 기반 geometry cache, species template, 인스턴스 배치, 거리/모바일 LOD, 환경 uniform.

수종은 **표현 템플릿**이다. Core가 개별 수종이나 나이를 제공하는 것처럼 표시하지 않는다. 현재 자원량 비율의 크기 변화는 기존 자원 표현 계약이며 생물학적 나이가 아니다.

## 3. 주요 변경 파일

| 파일 | 역할 |
|---|---|
| `web/src/render/procedural-asset-kit.ts` | 시드 RNG, tube/leaf/blade/rock/plank/vessel 생성, 재료 결 |
| `web/src/render/procedural-tree-generator.ts` | 3종·변형·LOD tree geometry와 수명 관리 cache |
| `web/src/render/procedural-tree-layer.ts` | 공유 메시 인스턴싱, 카메라 거리 LOD, 관측 계절·바람·눈 uniform |
| `web/src/render/vegetation-layer.ts` | GLB 생성 경로 제거, 기존 자원·접근·수계·시설·접지 계약 유지 |
| `web/src/render/ground-detail-layer.ts` | 자체 지표 메시와 상태 기반 uniform, 고정된 자원 슬롯 형태 |
| `web/src/render/natural-resource-projection.ts` | stock 변화에도 유지되는 presentation slot key 추가. DTO/자원 상태 변경 없음 |
| `web/src/render/facility-layer.ts` | 기존 조립·재고·내구도에 자체 구성 요소/재질 연결 |
| `web/src/render/environment-surface-presentation.ts` | snow modifier와 재료 결의 셰이더 cache key 합성 |
| `web/src/render/terrain-dressing-signature.ts` | biome/온도/수분 변경 시 템플릿 갱신 |
| `web/src/render/world-scene.ts` | 기존 환경을 식생 uniform에 전달, 카메라를 tree LOD에 전달 |
| `web/tests/procedural-environment/` | 결정론·접지·상태·수명·성능 검사 및 기존 GLB 비교 장면 |
| `.github/workflows/procedural-environment-check.yml` | typecheck/build, 환경·모션 회귀, desktop/mobile 브라우저 캡처 및 artifact |

주민 GUID, Core RNG, 저장 형식, WASM ABI, 행동 선택/결과, 주민 리깅·모션 소스를 수정하지 않았다.

## 4. 시각 비교

첫 자체 수관은 기존 모델보다 성겨 보여 잎 크기/밀도와 침엽수 층별 가지를 보강했다. `export-review.mjs`와 Blender `render-review.py`로 **기존 GLB의 텍스처까지 포함한** 비교 화면을 생성했다. Blender 화면은 형태 진단이며 Three.js 런타임의 최종 품질 판정은 아니다.

브라우저 fixture는 동일 seed/관측 상태/카메라에서 baseline, forest, autumn, winter, harvest, construction, operational, ruin을 1280×900 및 390×844로 캡처한다. GPU 셰이더 오류를 실패로 처리한다. CI artifact와 결과는 PR에서 확인한다. 화면 검토와 실제 기기 성능이 승인되기 전 외부 에셋 관련 코드를 삭제하거나 main에 병합하지 않는다.

## 5. Core 상태 연동 범위

- `forestCoverage01`, grass/shrub/rock/wetland coverage → 실제 배치 밀도.
- 실제 Wood/Fiber/PlantFood/Stone/광물/Clay 노드 quantity/maxQuantity → 기존 고정 슬롯의 감소·제거·회복. 자원 없는 인벤토리로 식생을 만들지 않음.
- Core accessGrid 및 shared water/facility footprint → 채취 접근 위치에 장식물을 배치하지 않음.
- 시설 trace의 actual progress·delivered materials·state와 일치하는 detail durability → 구성 요소 조립·마모·폐허. 주민이 도착했다는 이유로 완성/수리하지 않음.
- 실제 storage inventory → 종류·수량에 따른 적재물. 바뀐 메시도 기존 bounded pile count와 데이터 연결을 사용.
- 관측 calendar season, windIntensity, simulationMinute, 현재 cold Snow → 가을 색·겨울 잎 밀도·바람·눈 표면 표현. 별도 성장/적설/융해 clock 없음.

고도는 기존 공통 sampler 그대로 사용한다. 실제 resource 위치와 접근 위치를 옮기지 않으며 주민 hand/foot anchor도 변경하지 않는다.

## 6. 테스트/빌드

로컬: 절차적 환경 11개, 기존 facility construction 10개, 환경 preflight, diff whitespace 검사 통과. 새 검사는 모델 결정론, 서로 다른 형태, finite/index/bounds, LOD 비용, 공유 cache dispose, 변경 없는 GPU upload 억제, stock 고갈/회복, origin rebase 접지, 관측 날씨 reset, 침상 support bounds, 셰이더 cache 합성을 검사한다.

로컬 셸 네트워크/Chromium socket 제한 때문에 npm 전체 설치와 브라우저 검증은 GitHub CI로 보완한다. PR에는 전체 TypeScript/build, 64 presentation 회귀, 환경 9개, 기존 모션·시설·runtime resilience 검증이 연결되어 있다. 정확한 최종 head의 CI 결과는 PR Checks를 기준으로 한다. 이전 실패나 이전 head 통과를 최종 결과로 취급하지 않는다.

## 7. 성능 측정

고정 11×11 전체 숲 CPU fixture, tree-only static 버퍼 계수:

| 항목 | Desktop | Mobile |
|---|---:|---:|
| 나무 수 | 1089 | 726 |
| 기본 distant LOD 삼각형 합계 | 1,804,884 | 1,165,008 |
| geometry attribute/index bytes | 2,178,432 | 589,680 |
| instance matrix/color bytes | 4,939,776 | 2,580,480 |
| 비어 있지 않은 tree batch | 12 | 12 |

Desktop 가까운/먼 배치를 동시에 사용하면 최대 24 tree draw calls가 가능하다. Mobile은 낮은 LOD 한 종류만 생성한다. 캐시는 동일 템플릿 geometry를 공유하고 마지막 owner가 해제되면 GPU 리소스도 해제한다. 관측 상태가 같으면 matrices를 다시 upload하지 않는다.

기존 GLB의 수종별 삼각형은 6,257 / 5,625 / 4,065개다. 이 값만으로 프레임 향상을 주장하지 않는다. foliage overdraw와 가까운 LOD 비율도 중요하다. CPU 시간은 머신/JIT에 따라 달라지므로 `cpu-metrics.json`을 원자료로 남긴다. 기존 대비 frame median/drawcalls/geometry/texture는 동일 카메라의 `browser-metrics.json`에 기록한다. SwiftShader 결과는 실제 모바일 GPU FPS가 아니다. Core simulation 속도는 이 fixture로 측정하지 않는다.

## 8. 미구현 및 Core 확장 요구

- 개별 나무 ID/수종/나이/건강/분기 손상/벌목 성공 기록이 없으므로 개별 벌목·그루터기·떨어진 목재·부러진 가지를 임의 생성하지 않는다. 현재 Wood 집합의 감소 표현까지만 제공한다.
- 실제 적설 depth, soil moisture의 공간장, 바람 방향장, 시간에 따른 biomass 상태가 없으므로 적설 누적/융해나 독립 식생 성장·고사 시뮬레이션을 만들지 않는다.
- 꽃/열매의 성장·채취 단계, 새로운 울타리/주거 확장/도구 제작·해체 사건은 별도 Core 계약이 필요하다. 현재 시설/재고의 표현 확장 범위만 구현했다.
- 산맥·절벽·해안의 신규 Core 지형 생성, 수면 물리 반사, 파괴 물리, async chunk 생산은 이 PR에 포함하지 않았다. 기존 지형/수계/젖은 지면/눈 셰이더는 유지했다.
- 실제 모바일·데스크톱 장기 GPU/메모리·주민 상호작용 영상 검수는 남는다. 자동 anchor 검사와 기존 motion 회귀는 기기 검수의 대체가 아니다.

따라서 이 PR은 2차 지시서 전체의 최종 완료 선언이 아닌, 자체 생성 기반과 현재 Core 상태 연동의 검토 가능한 구현 단위다. 위 미구현 상태를 renderer에서 가짜 사건으로 채우지 않는다.

## 9. 병합

PR #703은 CI/시각·성능 검토를 위한 Draft로 생성했다. 자동 병합을 요청하지 않는다. 충돌 유무와 최종 검증·품질 상태는 PR에 갱신한다. main과 다른 개발자의 변경은 전체 파일 복제나 stale branch merge로 덮어쓰지 않는다.
