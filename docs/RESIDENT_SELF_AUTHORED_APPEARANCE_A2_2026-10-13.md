# LifeLens V-A2 — 자체 생성 얼굴 세부 형상 및 생애 외형 (2026-10-13)

## 위치
Web Observer / Three.js 주민 표현 전용. #715 V-A1 이후. Core Genesis/Genetics/Birth/AI/Needs/save-load/WASM ABI/DTO **변경 없음**. A2는 유료 모델·외부 API·새 외부 에셋을 사용하지 않음.

## 실제 데이터 경계
- `GeneticsProfile.faceShape`는 현재 Core의 유일한 얼굴 형태 유전 축. 턱/볼/얼굴 길이(기존 A1)·새 코 돌출·입 너비가 모두 이 축의 함수이며, GUID 해시로 소량만 개체별 시각 표현의 차이를 만든다. **Core가 코·입술용 새 유전자를 소유한다고 주장하지 않는다.**
- `Resident.lifeCondition.appearanceAgeFactor`는 `lifeConditionForAge`로 Core가 정한 외형 나이 지표. 모발 회색화·근접 얼굴 잔주름에 사용. 부상·흉터·헤어컷 등 관측에 없는 사건을 합성해 표시하지 않음.
- `lifeStage`는 모발 기하학적 길이/볼륨의 성장 단계별 비율에 사용. 연령층과 입력 스냅샷이 동일하면 정확히 같은 스타일 유지. 모션·노화 임신 체형 구조는 기존 유지.

## A2 신규 구현
- `resident-appearance.ts`: Core의 연속 외형 나이값으로 흰머리 점진 혼합, 유아/유년/청소년 모발 길이 계수, Core 얼굴 유전값 기반 `noseScale`/`mouthWidthScale`; hash salt는 가족 유전 계약 대신 작은 개체 표현 차이.
- `resident-authored-appearance.ts`: 모델의 실제 `Head` 정점 skinning에서 구한 범위를 재사용하고, **실제 Eyes mesh bounding box가 유효한 얼굴 앞면 방향을 제공하는 경우에만** 코/입윤곽을 Three.js 소스 기하학으로 생성·Head bone 하위에 부착. 노화 계수에 따른 2개 잔주름 메시는 역치 이후만 표시. GLB의 위치·눈을 신뢰할 수 없으면 얼굴 디테일 fail-close, 기존 A1 모델 보호.
- `resident-world-layer.ts`: 신규 actor와 늦게 오는 정밀 유전·lifeCondition 변경 시 동기화, 동일 profile replay/스냅샷 캐시 유지. 얼굴 미세기하는 카메라가 9 WorldUnit 이내·화면에 보이거나 주민 선택 때만 렌더(머리 실루엣은 유지).
- `web/tests/characters/run.mjs`: 실제 3D Head+Eyes 진단 fixture로 성별-independent source self-generation, 결속/비대칭, 가족 유전 영향, 나이/성장 구분, LOD, geometry 갱신 비누적·fail-close, 브라우저 외형 CI.
- `web/tests/characters/export-review.mjs`: **실제 pinned GLB**의 Head/Eyes를 사용한 16개 포즈에서 자체 생성 헤어/얼굴도 스키닝·mesh export. 실제 좌표 마커가 동작하지 않으면 CI에서 즉시 실패하도록 해 시뮬레이션 모델과 단위 테스트 사이의 간극을 막음.

## 경계·추가 작업
- A2는 영화급 안면 morph/근육, 개별 눈꺼풀·귀 morph, 환경 유발 흉터, 체형 질병 변화, 시대별 제작한 의복, 애니메이션에 맞춘 얼굴 표정을 **완료하지 않음**. 정확한 코/입 랜드마크가 추가되면 A3에서 리깅·표정까지 확장.
- runtime ageFactor가 결측이면 기존 나이값의 결정론적 fallback을 사용하며 성별을 바꾸거나 새 주민 정체성을 만들지 않음.
- 렌더러는 옷 생산/착용, 헤어 컷, 상처 치료를 창작해 Core에 역기록하지 않는다.
- 근접 face meshes는 draw-call 증분이 있으므로 먼 주민 LOD를 필수로 유지하고 Android 실기 성능 QA를 추후 수행.

## 병합 조건
- GitHub 최신 exact-head CI의 Web Checks(Web typecheck, 캐릭터 회귀, 실제 GLB 16포즈, 브라우저) + 해당 PR의 다른 필수 체크 **전부 PASS**.
- 실제 4명 개척민/다른 세대 외형 비교는 렌더 캡처가 있으나 개체성의 미적 성공은 별도 사용자 눈 검수를 남긴다. 이 점을 성과로 과장하지 않음.
