# 절차적 생태계 후속 구현 (#706)

## 1. 의존성
main에 병합된 #703의 자체 나무 3종, 잎·가지·풀·관목·암석·목재 메시를 확장한다. 새 외부 모델, 유료 API/클라우드/에셋은 없다. 기존 주민 리깅/모션을 수정하지 않는다. 비교용 이전 GLB는 QA에만 사용한다.

## 2. 직접 제작한 항목
- 절단면이 있는 목재 그루터기, 같은 나무 ID의 어린 재생 수관.
- 줄기·접힌 5/6장 꽃잎의 야생화, 다면 열매 군집.
- Core v4의 다중 규모 능선, 산지, 골짜기와 암반 급경사, 해안의 모래색 표면.
- Core 고도 17×17 표본에 맞는 지형 메시와 사물 접지.
- 바람/관측 시간의 수면 법선, IOR 1.333 재질, 실제 장면 cubemap/PMREM 반사.

## 3. 변경 파일
Core `Civilization.h`에 목재 노드별 8개 고정 ordinal의 나무 기록(출생 시점, 잔량/용량, 벌목 시점, 재생 세대)을 추가한다. ID는 `nodeId:ordinal`이며 초기 나이는 노드 ID에서 도출하고 mt19937 RNG를 사용하지 않는다. 실제 `ResourceNode.harvest`가 잔량을 차감할 때만 벌목 기록을 생성한다. `regenerateDay`의 실제 보충량을 같은 나무 기록에 배분한다.

`NaturalEcologyLifecycle.h`, `World.h`, `Simulation.cpp`에서 materialized 청크의 적설량을 강설로 누적하고 기온/비로 녹인다. 환경 계산 비용을 제한하기 위해 Core 시간 5분 간격으로 적분한다. 나무 시간은 매 분 갱신한다. 꽃/열매는 계절 위상, 기온, 실제 Fiber/PlantFood 재고에서 계산한다.

`NaturalEcologySnapshotCodec.h`, `NaturalEcologyValidation.h`, 기존 snapshot 소스에서 v8 body 뒤 선택적 확장을 저장한다. 확장이 없는 이전 저장을 읽고, 이전 벌목/적설 이력을 추측하지 않는다. 손상된 나무/적설 기록, 노드 잔량 불일치, 중복/역순 청크를 거부한다. 주민 GUID/규칙/RNG/기존 저장 필드를 변경하지 않는다.

`ContinuousTerrain.h`, `WebClientBridge.*`, `core-types.ts`, `core-bridge.ts`에서 신규 게임 v4 및 고도 lattice/생태 DTO를 전달한다. 기존 v1~v3 지형 수식은 보존한다. 정적 고도 문자열 캐시는 bridge당 최대 256청크로 제한하며 seed/population/generation/좌표를 키로 쓴다.

Web의 resource projection/vegetation/ground detail은 개별 나무 기록과 Core 꽃·열매 수치로 인스턴스를 갱신한다. 그루터기는 실제 벌목 시점이 있을 때만 나타난다. 재생은 기록된 세대/나이를 사용한다. 별도의 성장/눈 타이머나 소비/제작 사건을 만들지 않는다. terrain geometry는 Core 표본과 같은 삼각형 보간으로 모든 접지를 공유한다.

`water-layer.ts`, world scene/renderer는 낮은 해상도 장면 probe를 갱신한다. 물을 캡처에서 숨겨 재귀 반사를 막고, 실패 시 render target/XR/visibility를 복구하며 GPU 자원을 dispose한다. Core 시간/바람의 파문은 geometry/이동 가능 영역을 바꾸지 않는다.

## 4. 시각 검증
기존 비교 fixture에 실제 Core v4 terrain/resource/environment JSON을 생성하는 `ll_ecology_fixture`를 추가했다. desktop/mobile에 Core 지형 및 실제 장면 반사 캡처를 추가한다. 일반 시드 fixture와 Core DTO fixture를 구분한다. GPU 검증 결과/아티팩트는 CI 완료 뒤 기록한다.

## 5. 상태 연동 범위
- 실패/0수량 채취는 나무를 베지 않는다. 성공한 목재 채취만 나무 잔량과 벌목 시점을 바꾼다.
- 기존 Core 이동·채집 유효성 검사를 우회하지 않는다. 주민 애니메이션은 유지한다.
- aggregate Wood 잔량은 개별 나무 잔량의 합과 일치한다. 저장 후 나무 ordinal/세대/그루터기를 복원한다.
- 재생량과 자재 인벤토리는 기존 Core 공급량을 보존한다. 새 무료 목재/과일을 추가하지 않는다.
- 눈은 강설이 멈춰도 추울 때 남고, 양의 기온/강우에서 녹는다. reload에서 누적량을 유지한다.
- 실제 재고가 소진되거나 Core 꽃/열매 단계가 0이면 관련 인스턴스를 제거한다.
- 건설·파손·재고 사물은 #703의 기존 실제 Core 연동을 유지한다.

## 6. 검사
로컬 g++ 전체 Core 연결: 신규 생태 테스트, 기존 snapshot codec/save-load/continuous terrain/bridge 검사 실행. Web: 기존 절차적 환경 12개, 시설 10개 및 신규 lifecycle 계약 검사. 신규 계약은 벌목/그루터기/재생/복원/꽃·열매 제거/누적 눈/세부 고도/반사 실패 복구를 확인한다.
CI: CMake/CTest, WASM, Preflight, TS/build, runtime 회귀, 장기 시뮬레이션 및 Chromium SwiftShader 실제 캡처. 최종 정확한 head 결과는 PR에 갱신한다.

## 7. 성능
기존 121청크 fixture에서 나무 수/기존 LOD 삼각형은 유지한다. 그루터기와 꽃/열매는 각 1개의 instancing batch이며 0개일 때 그려지지 않는다. 반사 probe는 desktop 128²/mobile 64², 정지 상태 갱신은 120/240 프레임 간격이다. 초기/지형 변경/카메라 8 world units 이상 이동 때 갱신한다. probe는 한 위치의 실제 반사 근사이며 추가 6회 장면 렌더 + PMREM 처리 비용이 있다.

v4의 terrain geometry는 청크당 121→289 vertices, 고도 JSON payload도 증가한다. local O0 25청크 조회는 최초 약 70ms/캐시 약 54ms, terrain JSON 약 78KB였다. 이것은 최적화 빌드/실제 모바일 시간이나 전체 Core 성능 비교가 아니다. 반사 프레임의 추가 비용과 장치 GPU 검수가 필요하다.

## 8. 기술 범위
개별 나무는 실제 목재 자원 노드의 하위 상태다. 자원 노드 밖의 숲은 기존 Core coverage의 비채취 식생 표현이다. 주민은 기존 노드/접근 지점에 도착해 채취하며 새 개별 나무 클릭/이동 액션을 추가하지 않는다. 나무 잔량과 나이로 손상/재생 외형을 표시하지만 별도의 병충해/화재·낙목 강체 물리·나무별 생리 모델은 없다. 기존 자원 재생 속도를 식물학적 성장률로 재보정하지 않았다.

적설은 materialized 청크에서 누적한다. 이전 저장의 미관측 과거 적설을 재구성하지 않는다. 현재 Web snow material은 관측 중심의 깊이를 coverage로 표현하며, 눈 두께가 실제 충돌 높이/이동 가능한 영역을 바꾸지 않는다.

꽃/열매는 Core의 관측 단계와 기존 PlantFood/Fiber에 해당하며 새 식품 종류/제작/추가 수량을 만들지 않는다. v4는 절차적 지형이며 실제 지구 좌표의 실측 DEM/수문 침식 해석은 아니다. 반사는 단일 위치 probe의 PBR 근사이며 매 수면별 실시간 평면/화면 공간 반사나 유체 해석이 아니다.

## 9. 검토
PR: https://github.com/sjLim91/lifelens-ue/pull/706 . 자동 병합하지 않는다. 정확한 최종 head에서 필요한 CI와 시각 검증이 통과한 뒤 Ready for review로 전환한다.
