# 주민 리깅·절차적 모션 기반 구현 — 2026-10-12

## 범위와 기준

분석 시작 main `ddaca28e6caa6587cadf3c529a997007f4846457`, 구현 기준 재동기화 main `bcff5d9c2635762699bcf66cfd5a99475d71e946`.
최신 main의 세 후속 커밋은 Core 인지/물 경험 변경이며 이 변경의 Web 파일과 겹치지 않는다.
이번 PR은 1차 이동 안정화 및 2~4차 공통 기반이다. 전체 요구사항의 최종 시각 품질 완료를 의미하지 않는다.
Core 소스, WASM ABI, GUID, 저장 형식, AI 의사결정과 성공 판정을 변경하지 않는다.
외부 유료 API·에셋·클라우드 의존성을 추가하지 않는다. 공개 저장소의 기존 무료 Linux GitHub Actions를 검증에 사용한다.

## 기존 구조 분석

- 실제 캐릭터는 `character.glb`: 메시 3개, skin 1개, 기본 애니메이션 0개. 단일 비리깅 메시가 아니다.
- 골반/척추/목/머리/좌우 어깨/상완/전완/손/대퇴/종아리/발 18개 공통 관절이 실제 GLB에 존재한다. 기존 손가락 뼈도 유지한다.
- `SkeletonUtils.clone`로 주민별 skeleton을 분리하고 clip은 공유한다. 모델 bbox에서 발 바닥 원점을 맞추고 높이를 1로 정규화한 뒤 기존 phenotype/lifecycle 스케일을 적용한다.
- 현재 제품 경로는 `WorldScene -> ResidentWorldLayer`; `character-layer.ts`는 legacy Canvas 경로이며 이번 변경은 현재 Three World 경로에 집중한다.
- 기존 CC0 UAL1/UAL2 클립 이름을 정확하게 매핑하고 일부 상·하체 track을 합성한다. 실제 눕기/기상은 `ResidentSleepMotion`과 검증된 UAL2 endpoint/97개 보정 샘플을 재사용한다.
- Core `ResidentPresentationDirective`의 active/kind/phase/physicalGoal/targetGrid/objectId/resourceNode/facilityId/targetResidentId/token 등을 읽는다. Moving/Interacting은 실행 상태이며 성공 이벤트가 아니다.
- 관찰자는 Core 좌표를 보간한다. 기존 경로는 두 스냅샷의 Y를 직선 보간해 비평면 지형에서 관통/부유할 수 있었다. 이동 도중 실제 남은 X/Z 방향에서 yaw를 재계산하지 않는 것도 방향 반전의 위험 요소였다.
- 지형은 관찰된 chunk elevation의 10×10 삼각형 메시다. 새 지면 샘플러는 동일한 a-c 대각선과 보간을 사용한다.
- 생산 대상은 기존 `ResidentProductionTargets`의 정확한 ID·종류·잔량·접근좌표·거리 검증을 재사용한다. Core 이동/충돌/대상 예약 규칙을 브라우저에서 다시 만들지 않는다.

## 에셋·리깅 정책

기존 캐릭터/UAL1/UAL2는 Quaternius CC0-1.0 자산이며, 기존 `Tools/prepare_web_character_assets.py`에 고정 commit/blob 해시와 공식 출처가 기록되어 있다.
새 외부 모델을 도입하지 않았다. CC0는 수정/스키닝/재배포를 허용한다. 자체 작성 모션은 저장소 소스 코드이며 유료 모션이 아니다.

`ResidentRig`는 명시적 관절 alias로 Quaternius/Mixamo 관절 및 분리 메시의 이름 붙은 Object3D pivot을 바인딩하고 누락 관절/skin 유무를 보고한다.
이는 자동 retarget 검증이 아니다. 다른 골격에는 해당 모델의 올바른 upright 기준 clip을 `createResidentAuthoredMotions`에 제공하고 축·비율·접촉을 검증해야 한다.

활성 모델에는 이미 리깅이 있으므로 Blender 재리깅이나 에셋 교체는 불필요하다.
미래의 비리깅 단일 humanoid는 Blender Python으로 armature 생성 → bone heat weights → vertex weight 검증 → GLB export가 가능하지만, 임의 메시의 bbox만으로 관절 위치/위상을 보장할 수 없다.
T/A pose, 축, 좌우, 관절 landmark와 수정 가능한 원본/라이선스가 필요하다. 자동 weighting 실패는 수동 weight/분리 메시 pivot 제작으로 해결해야 하며, 이번 PR은 임의 단일 메시 자동 스키닝 완료를 주장하지 않는다.

## 신규 구현

- 원본 키프레임 생성: 공유된 upright baseline에 gait/crouch/reach/consume/gesture/wash 파라미터를 조합한 33개 semantic clip. 대기, 걷기, 달리기, 앉은 자세, 운반, 식사/음수, 씻기, 용변 자세, 채집, 작업/건설/수리, 제작/배치/보관/꺼내기, 농업, 대화/위로/교육/돌봄.
- 기존 의미에 맞는 vendor clip을 우선하며, 없는 모션은 자체 키프레임으로 보충한다. 수면/기상은 기존 검증 경로를 유지한다.
- run/sit은 재사용 가능한 clip으로 제공한다. 현재 Core에 달리기/좌석 행동 계약이 없으므로 renderer가 임의로 달리거나 없는 의자에 앉히지 않는다.
- 이동: 실제 X/Z 잔여 변위에서 heading 계산, 기존 반전 시 선회 우선/보행 phase·정지 grace 재사용, 이동 중 매 프레임 지면 Y 재계산.
- IK: 고정 비용 analytic two-bone solver, 도달 한계 clamp, 현재 bend plane 유지, 잘못된/nonfinite target 거부. 가까운 stance 발과 도달 가능한 오른손에만 적용한다.
- 발/발목: 지면 높이, near-ground stance 판정, swing clearance 보존, 경사 기울기 제한. 급경사 전체 몸체 무관통 보증은 제외한다.
- 손: 검증된 실제 생산 대상의 기존 표현 높이, 음수/식사 시 자신의 입 위치. 대상이 사라지면 보정 weight를 감쇠한다. 멀리 있는 자원까지 팔을 늘리거나 주민을 옮기지 않는다.
- 시선/자세: 실제 가까운 사회 대상, 실제 sleep need/injurySeverity/fear/joy/introversion 기반 작은 spine·gait·idle/head 보정. 기존 연령/아동 비율/노화/pregnancy 표현을 유지한다.
- 보정 순서: procedural 복원 → lifecycle 복원 → mixer → lifecycle 적용 → procedural 적용. 매번 기준 pose를 복원하므로 누적 변형을 방지한다.
- 물리 행동: 실제 smart object ID/종류/예약자/대상좌표, 자연 수원 chunk의 실제 수계/담수/잔량, 소지한 실제 음식/물과 표시 거리 검증. 정보가 없으면 idle로 안전하게 억제한다.
- LOD: 가까운 화면 주민은 매 프레임, 먼 화면 주민은 15Hz, 화면 밖은 5Hz mixer 갱신. elapsed animation time은 보존한다. 화면 밖 IK를 생략하며 Core 시간/AI에는 영향이 없다.

## 오류별 기록

| 재현 조건/문제 | 원인 또는 기존 상태 | 수정/검증 |
|---|---|---|
| 이동 반전에서 뒤로 이동 | snapshot heading과 실제 보간 방향의 불일치 위험 | 실제 잔여 변위 yaw + 기존 선회 우선 유지, 첫 반전 프레임 후진 없음 자동 검증 |
| 외부 모션 누락 시 보행 없음 | 자체 walk fallback 없음 | 외부 두 라이브러리 실패를 주입하고 실제 actor authored walk 검증 |
| 비평면 지형 이동 시 관통/부유 | root Y가 endpoint 사이 chord | 매 프레임 렌더 삼각형 높이 사용, raycast 일치 및 이동 중 Y 검증 |
| 짧은 정지/전환에서 튐 | 동작 reset 재진입 위험 | 실행 중인 action의 무조건 reset 제거, 보정 pose 복원/누적 방지 검증; 실제 영상 QA 남음 |
| 크기 불일치 | 이미 main에서 bbox/phenotype/lifecycle 정규화 | 기존 코드 재사용, 실제 GLB·외형 회귀 검증; 새 모델별 scale QA 필요 |
| 멀리서 채집 | access cell과 source 중심은 다를 수 있음 | 기존 생산 target guard 유지, 손 IK 도달 범위 밖 skip; 긴 access 거리의 clip 의미 정합은 후속 |
| 없는 물에서 음수 | Physical directive만으로 consume 선택 | 실제 수원/소지 물/object 및 거리 검증, dry chunk에서 실제 actor idle 검증 |
| 수면·침상 불일치 | main의 skeletal sleep/bed support 경로 존재 | 해당 경로 보존, 수면 actor 회귀; 기존 전체 UAL sleep suite는 CI 실행 |
| 전환 순간 자세 튐/누적 | additive overlay를 다음 pose에 다시 더함 | 정확한 직전 mixer pose 복원, 100회·pause 검증 |
| 이동 방향/시선 불일치 | interaction heading과 travel heading 구분 필요 | 이동 heading 우선, 실제 근거리 사회 target 시선만 적용 |

## 검증과 한계

로컬 Node 24의 내장 TypeScript 변환으로 실행한 신규 모션 테스트 12개 PASS. 이것은 semantic TypeScript typecheck가 아니다.
실제 pinned UAL1/UAL2를 확보한 기존 모션 15개 PASS. 기존 외형 10개, 행동 문맥 27개(+추가 cultivation/inventory guards), runtime resilience 18개, resident continuity 13개 PASS.
구조 검증: resident motion/action context/authoritative spatial targets PASS.
PR 첫 HEAD의 실제 TypeScript typecheck와 runtime resilience CI PASS. Core native g++ 컴파일 및 determinism/save-load/snapshot codec/context completion 회귀 PASS. CMake/CTest 전체와 WASM/build는 로컬 도구/네트워크 부재로 CI 검증 대상이다.

실제 GLB skin 변형을 bake하고 Blender CPU로 걷기/중립/경사/음수/수면 포즈를 렌더해 확인했다. 이 결과는 게임 WebGL 화면이나 동영상의 최종 수락을 대신하지 않는다.
로컬 Chromium은 sandbox socket `setsockopt: Operation not permitted`로 종료했다. WebGL 데스크톱 960×720/모바일 390×844의 14개 캡처 harness를 기존 character CI에 추가했다. CI 결과 확인 전 화면 검증 완료를 주장하지 않는다.

300 actor × 60 frame의 먼 거리 LOD CPU 테스트는 약 177~421ms/전체 60프레임(약 3~7ms/frame)이었다. 관측 fixture이며 실제 모바일 FPS/GPU draw-call 상한의 증거가 아니다.
주민별 기존 skeleton/material 비용은 유지하고 관절 참조·quaternion snapshot을 추가한다. 모션은 공유되나 skeleton은 instancing하지 않는다.

남은 범위: 임의 비리깅 단일 모델 자동 스키닝/retarget 수락, 실제 Core의 run/seat/backstep/strafe 계약, 수면 진입의 현재 clip pose와 다중 clip blend 정밀 QA, 급경사 full-body/pelvis contact, 보행 거리별 완전한 발 고정, 실제 협력 action outcome별 제스처, Android GPU/1000명·장기 전체 회귀.
손 IK는 대상 표현 높이/본인 입으로 제한한다. object DTO에는 정확한 손잡이·좌석·발 슬롯이 없어 임의 정밀 affordance를 생성하지 않는다.
Core만 성공/실패/중단과 상태 변화를 결정하며, renderer는 directive 종료/대상 유실에 따라 중립·회복 transition으로 전환한다. 렌더 완료를 Core에 success로 전달하는 코드가 없다.
