# World Presentation 2차 — 구현 범위와 복구 기록

기준 main: `467e5e881f6c67e44652ce57db9cb1e4da911339` (#600 포함).
Core 시뮬레이션/기존 WASM DTO는 수정하지 않는다. #593 표현을 재사용/확장한다.

## 1. 실제 구현

- FootTraffic: 실제 관측 wall time, 선택 배속, SimulationClock refresh/tick 계약으로 연속 관측을 판별한다. Core의 cardinal 이동 상한(한 시뮬레이션 분에 최대 한 칸)으로 장거리 점프를 거부한다. 관측 도착점만 표시하고 중간 경로를 보간하지 않는다.
- pause/동일 minute는 누적하지 않는다. 배속 변경과 fast-forward 진입/완료/실패 때 이전 표본을 끊고 기존 자국은 원래 시뮬레이션 시간 감쇠를 따른다.
- 정착지 anchor 선택, 월드 활동 목록 선택, 정확한 대표 시설 선택에서 단일 한국어 정보 팝업을 연다. 인구, 기반 형성, 실제 활동 여부, 시설/저장소 집계, 교역 기록, 최근 관측 전환을 표시한다.
- 선택한 정착지 중심만 지형을 따르는 은은한 강조로 표시한다. 이 원은 영토/생활권 경계가 아니다. 항상 보이는 경계/아이콘/관계선은 없다.
- 대표 저장소가 정확히 연결되는 경우에만 그 저장소의 실제 보관량/품목을 표시한다. 정착지 전체 저장량/잉여량으로 부르지 않는다.
- 실제 재고의 종류와 수량을 #593 저장 적재물에 연결한다. 작은 재고는 낮은 묶음, 혼합/품목 미노출 재고는 중립 묶음, 소진은 적재물 0개다. 최대 4개 묶음이며 재고 종류 변화도 signature에 포함한다.
- 정착지 관측/기반 형성, 교역 활성 전환을 기존 bounded Observer 로그에 연결한다. 초기 로드/누락 DTO/동일 cached payload/같은 상태 반복은 로그를 만들지 않는다. 영구 사건 시각이 없으므로 발견 시각을 사용하며 정확한 역사 사건으로 추정하지 않는다.

## 2. #593 재사용

FacilityLayer/실제 HumanTrace 위치·건설 단계·durability·수리·Ruined 표현, 쉼터 완성 벽, 실제 경작 고랑/작물, 저장 적재물 예산, 실시간 Light 없는 불빛 Sprite, 자원 quantity/maxQuantity 크기 변화, 실제 주민 소지품/교역 이동 표현, FootTrafficLayer 고정 버퍼/감쇠/예산, geometry/material 재사용과 cached signature를 유지한다.

## 3. 발견·수정한 문제

현재 하루는 1440분, 1× 하루는 현실 120000ms, 한 분은 약 83.333ms다. tick은 125ms, 주민 refresh는 500ms다. 정상 observer snapshot 간 변화는 1× 6분, 4× 24분이다. civilization 상세는 4 snapshot마다 갱신되므로 약 2초/1× 24분/4× 96분이며, 이 상세 주기를 주민 보행 cadence로 사용하지 않는다.

#593 `maxGapMinutes=2`와 `maxStepGrid=2`는 정상 1×/4× 이동도 차단한다. 숫자만 올리지 않고 실제 cadence/배속/경과시간 및 이동 상한에 따른 판정으로 대체한다. fast-forward는 명시적인 표본 단절을 추가한다. 저장소의 같은 총량·다른 품목 변경을 기존 signature가 놓치던 문제도 수정한다.

## 4. 실제 사용한 Core/WASM 데이터

`WebClientBridge.cpp`의 `appendCivilizationWorld`와 `LifeLensWebBindings.cpp`의 기존 전달 경로를 확인했다.

| 영역 | 현재 Web까지 도달하는 데이터 | 사용 |
| --- | --- | --- |
| 정착지 | settlements id, gridX/Y(anchor), residentCount, facilityCount, operationalFacilityCount, plannedFacilityCount, storageSiteCount, active, established | 팝업/선택/형성 전환 |
| 시설 | facilities id/kind/state/좌표/durability/linkedStorage/건설·점화·경작 정보, HumanTrace | 기존 실제 월드 표현/정확한 대표 시설 연결 |
| 저장소 | storages id/좌표/totalUnits/inventory | 실제 적재물/대표 저장소 보관량 |
| 교역망 | tradeRoutes id/양 끝 정착지·anchor/partnerCount/exchangeCount/distanceGrid/active | 선택 팝업/활성 전환 로그 |
| 주민 | 실제 gridX/Y, presentation 및 보유 inventory, 개인 migration pressure | 기존 이동·소지품 유지. 이주 압력을 집단 이주로 해석하지 않음 |

정착지 ID는 Core `SettlementNetwork.h`가 정의한 최소 node key다(시설 ID << 1, 저장소 ID << 1 | 1). 대표 노드만 BigInt로 무손실 연결한다. Web에서 clustering/주민 소속/전체 재고를 재계산하지 않는다. Core가 cluster를 병합/분리하면 정착지 ID가 달라질 수 있다. 이것을 이동/멸망 사건으로 해석하지 않는다.

## 5. 부분 구현·미구현 및 정확한 binding 요구

아래 C6 read model은 최신 Core에 존재하지만 현재 `appendCivilizationWorld` 및 Web DTO에는 없다. 타입만 추가하거나 가까운 주민/시설을 묶어 대체하지 않는다.

| 기능 | Core에 있는 구조/필드 | 필요한 binding 및 부족한 권위 |
| --- | --- | --- |
| 가구 이주 | `HouseholdMigrationPlan`: available, householdId, leader, members, origin, target, targetChunk, bottleneckMaterial, leaderPressure01, consensusPressure01 | 이 구조는 후보 계획이다. `Simulation::advanceHouseholdMigration`은 긴급 필요/기존 행동 때문에 실행을 거부할 수 있다. 계획 노출만으로 시작 사건을 만들 수 없다. 실제 실행된 그룹의 안정적인 migrationId, householdId, memberIds, leaderId, origin/target 좌표, startedMinute, phase(이동/도착/취소), sequence가 필요하다. 현재 emit 문자열은 구조화된 Observer 사건으로 전달되지 않는다. |
| lifecycle | `SettlementLifecycleEntry`: settlementId, anchor, residentCount, operationalFacilityCount, storageSiteCount, vitality01, state | state 이름 Inhabited/Vulnerable/Declining/Abandoned 및 minute를 serialize. 전환/재정착 사건 sequence와 occurredMinute가 필요하다. Core enum에 별도 Ruined는 없다. 폐허는 실제 시설 state/durability가 권위이며 lifecycle로 생성하지 않는다. |
| 지식 격차 | `SettlementKnowledgeProfile`: settlementId, anchor, residentCount, awareTechnologyCount, reproducibleTechnologyCount, dominantTechnology, dominantStrength01, specialization01, specialized, technologies | technologies의 technology/technique 이름 또는 안정 ID, awareResidents/reproducibleResidents/practicedResidents/masteredResidents/successfulUses/averageKnowledge01/strength01. 전환 minute/sequence 필요. |
| 생산 특화·잉여 | `SettlementProductionProfile`: settlementId, dominantKind, specialization01, specialized, food01/materials01/toolmaking01/metallurgy01/logistics01, dominantSurplusMaterial, dominantSurplusUnits, surplusMaterialCount | General/Food/Materials/Toolmaking/Metallurgy/Logistics 이름, anchor/residentCount와 관측 minute. 정착지에 소속된 실제 facilityIds/storageIds 필요. 현재 Web에는 소속 목록이 없으므로 전체 시설 강조/재고 합산을 추정할 수 없다. |
| 관계 | `SettlementGroupRelationObservation`: firstSettlement/secondSettlement, firstAnchor/secondAnchor, distanceGrid, crossResidentPairCount, relationshipEvidenceCount, exchangeEvidenceCount, tradePartnershipCount, cooperation01/tension01 및 state | Neutral/Cooperative/Strained/Hostile 이름과 실제 관계 증거(averageBond/Trust/Conflict/Fear/Grudge01 등), 전환 sequence/minute. 교역 활성 자체를 협력 상태로 간주하지 않는다. |
| 교역 움직임·경로 | 기존 `SettlementTradeRouteObservation`은 두 anchor와 교환 증거만 제공 | 현재 실제 이동 task의 residentId, source/destinationSettlementId, outbound/return phase, 실제 cargo, action token. 실제 navigation segments/position samples 없이 anchor 사이 직선을 실제 교역로라고 그리지 않는다. |
| 전체 생활권 범위·저장량 | 현재 cluster 집계만 제공 | 권위 있는 facilityIds/storageIds/residentIds와 소속 변경 시각, 필요 시 권위 bounds. 대표 저장소와 전체 저장량/잉여량을 구분할 것. |

따라서 생산 전문화 강도 조절, 집단 이주 강조/시작 사건, lifecycle에 따른 전체 표현 조절/쇠퇴·방치 사건, 관계 상태 강조/변화 사건, 정착지별 지식 전문화는 **미구현**이다. 정착지 자체의 관측·팝업과 교역 관계 표시는 **부분 구현**이다. 기존 실제 시설·경작·저장·주민 활동·소지품을 보존하고 보행 정상 누적으로 생활권이 자연스럽게 읽히게 한다.

## 6. 모바일 성능

- 매 frame 정착지/주민/관계를 재검색하지 않는다. 정착지 anchor 캐시는 snapshot signature 변경 시에만 갱신한다(최대 64개). 선택/탭 이벤트에서만 pick/focus 작업을 한다.
- 추가 월드 draw call은 선택 때만 LineSegments 1개, 고정 48 segment 버퍼/geometry/material을 재사용한다. 관계선/실시간 Light/Shadow/정착지별 DOM은 추가하지 않는다.
- 팝업 하나, 교역 최대 6개, 최근 사건 최대 3개. 기존 로그 전체 이력 최대 12개를 재사용한다.
- 저장 묶음은 기존 최대 4개/시설. 시설/저장소 조회는 snapshot Map을 사용한다. 기존 시설/재질 캐시와 소지품/자원/불빛 예산을 유지한다.
- 실제 모바일 FPS/발열/메모리 측정은 시각 QA에 남아 있다.

## 7. 자동 검증·시각 QA·복구

추가 회귀: 실제 SimulationClock 1×/4× cadence(인위적인 0→1→2분 입력만으로 판정하지 않음), pause, 시간 점프/관측 공백/teleport, 정상 관측 재개, 실제 저장 종류/양/소진/상한, 전문화/lifecycle binding 부재 시 가짜 표현 없음, 기존 durability/수리/폐허 권위, 무손실 대표 시설 연결, 사건 중복 방지, 선택 강조 geometry/material 재사용.

검증 결과는 PR 설명의 exact-head Actions 링크를 기준으로 기록한다. 로컬 dependency 설치는 network EPERM으로 실행할 수 없으므로 전체 Typecheck/Preflight/Runtime regressions/production build는 GitHub Actions에서 실행한다. 로컬 환경 복제 실패를 반복하지 않는다.

직접 확인: 1×/4× 반복 통행 자국의 누적/감쇠·pause·fast-forward 직후 잘못된 연결 부재, 시설 밀집/작물/적재물로 읽히는 생활권, representative 시설/anchor 선택과 단일 팝업, 모바일 닫기/스크롤/키보드 Escape, 경사지/음수 좌표/청크 경계의 중심 강조 높이, 저장량·품목 변경과 수리/폐허 뒤 외형, 야간 기존 불빛과 주민 손-짐 정렬, 다수 시설 모바일 발열/메모리.

복구: 먼저 actual GitHub main/작업 PR/Actions를 조회한다. 이 문서와 `/workspace/WORLD_PRESENTATION_2_HANDOFF.md`, 로컬 diff를 확인하고 마지막 검증 checkpoint부터 이어간다. Core P0 병행 변경을 되돌리거나 오래된 Core tree를 덮어쓰지 않는다. 원본 main tree에 Web 변경만 올리며 merge/deploy는 수행하지 않는다.
