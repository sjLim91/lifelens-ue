# LifeLens V-A1 — 주민별 자체 생성형 외형 1단계 (2026-10-13)

## 목적
같은 유료/무료 인체 모델의 색만 바꾸던 수준을 넘어서, **실제 주민 유전값과 GUID로 생성되는 독립적인 얼굴 윤곽·헤어 실루엣**을 World Observer에 표현한다. 신규 유료 에셋·API·클라우드·캐릭터 GLB 다운로드 **없음**. 기존 CC0 공통 인체 및 검증된 skeleton/motion은 재사용한다.

## 현재 소스 사실
- 생산 Core의 4명 개척민은 `generateInitialFounders`에서 각자 `generateFounderGenetics`로 생성된다. 자녀 `inheritGenetics`도 존재하며 현재 `faceShape/eyePigment/hairPigment/skinTone/heightPotential/buildPotential`가 Web 상세 JSON에 전달된다.
- WorldSession은 고빈도 `residentRuntime` 자료와 상세 `residents` 자료를 ID로 합쳐 Three World에 전달한다. Web 표층에 외형 정보가 완전히 누락된 상태가 아니다.
- 종전에는 1개 인체 GLB를 모두 복제해, 머리카락은 head vertex 위쪽을 칠하는 방식이었다. `hairStyle`은 헤어라인 범위에서만 미세하게 달라져 실제 입체 헤어 실루엣 차이가 부족했다.

## 구현 A1
- `resident-authored-appearance.ts`: Head 뼈가 실제 `SkinnedMesh`의 vertex influence로 검증되는 경우에만 head-local mesh fitting을 계산한다. 실제 머리뼈에만 새 구형 crown 및 개별 세부 곡선 lock을 생성한다. 8종: 짧은 커트, 가르마, 헝클, 레이어, 단발, 짧은 곱슬, 넘김, 긴 머리.
- 머리카락은 **Three.js 기초 `SphereGeometry`/`TubeGeometry`를 소스에서 직접 생성**. 고정 이미지/외부 파츠 없음. 미확인 리그는 떠 있는 가짜 헤어를 생성하지 않고 원래 vertex-scalp fallback 유지.
- `resident-appearance.ts`: Core `faceShape` 유전값에서 `jawScale`, `cheekScale`, `faceLengthScale`를 도출. 기존 per-resident cloned/rest skinned geometry의 헤드 가중치 정점에만 작게 적용하고, 입체 얼굴 외곽을 실제로 변형한다. 원래 `headWidthScale`도 유지한다.
- ID 해시는 8가지 머리 모양 선택에만, 실제 Core 유전 `hairPigment`는 머리 색상·`faceShape`는 얼굴 비율·`buildPotential`는 체격에 연결한다. 색상·유전자 부재 fallback은 기존 결정론 규칙을 사용한다.
- 외형 변경 시 전체 메시를 매 frame 재생성하지 않고 signature가 달라질 때만 파츠 업데이트. 현재 snapshot 뒤늦게 들어오는 유전정보, 개체별 skeleton 분리, 기존 life stage/pregnancy/motion 계층 유지. 기존 hair 파츠 geometry/material은 안전하게 dispose하며 Character/World/Motion/Action/Civilization truth를 절대 바꾸지 않는다.
- `web/tests/characters/run.mjs`: 생성 헤어 실루엣 8종, skeleton 결속, 다른 주민의 소재/유전자 분리, 최신 genetics refresh, 없는 리그 fail-close 검사.

## 선행·변경 경계
- 개발 기준: #712 B5a, #713 B5b를 모두 통과·병합한 main `357892aa`.
- 별도 `feature/visual-genetic-identity-v1-20261013` 브랜치. Core/WASM ABI/GUID/Snapshot/Needs/Decision 로직 **무변경**. Web Observer view 변경만.
- 기존 공통 CC0 body rig를 버리는 것이 아니며 유료 모델과 비교해 photorealism이 우월하다고 주장하지 않는다.

## A2 이후 미구현
- 머리 자람/자르기, 문화별 의복·액세서리·생물학적 수염·체형 성장 정밀화, 피부 노화/흉터의 관측 근거, 부모 닮음의 사진·정면 얼굴 QA, 개별 코·입술·귀 mesh, 근거리 얼굴 표정 morph target은 아직 미구현.
- 근거리 WebGL 실물 GLB 화면 및 4개 개척민 비교 캡처, 모바일 GPU 비용 측정은 CI/browser 수락 전까지 성공을 주장하지 않는다.
- 캐릭터 별 성별 모듈 교체 및 독립 GPU 부하 profiling은 A2~A3 대상으로 둔다.

## 검증/병합 게이트
1. Source-normalized Three.js `character.glb` 실제 Head vertex skinning과 procedural haircut 실루엣이 정확히 결합되는지 브라우저 프레임 및 이미지 비교.
2. `npm run typecheck`, `node tests/characters/run.mjs`, Web build, Character Asset Verification, Web WASM/Preflight CI.
3. NewGame 동일 seed에서 모든 외형 동일 replay, genetics source 미변형, 4 founder 구별성·세대교체 상세 정보 확인.
4. head absence/invalid geometry fail-closed, 재업데이트 시 geometry/material 누수와 animated pose 비누적 테스트.
5. 실제 CI 초록색 확인 후만 main 병합. 
