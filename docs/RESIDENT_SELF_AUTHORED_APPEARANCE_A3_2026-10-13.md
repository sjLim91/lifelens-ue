# LifeLens A3 — 주민이 만든 원시 섬유 의복을 자체 생성 3D로 연결 (2026-10-13)

## 기준과 권한
- 기반 main: `9eb1163ee6054b0efce29d3803c27fe2081e7369` (A2 #716).
- 별도 브랜치: `work/a3-resident-authored-garments-20261013`, PR #717.
- Core C++만 제작 결과, 소지, 착용 여부의 권한을 갖는다. Web은 화면만 그린다.
- A1/A2 얼굴·헤어·노화, CC0 공통 GLB 리그와 동작을 변경하지 않는다.
- 새 유료 모델, API, 클라우드, 라이선스 자산 없음.

## A3 첫 수직 절편
1. 원시 몸가리개 `PrimitiveWrap` 제작법을 기존 기술 그래프에 등록. 기존 `FiberCordage` 재현 지식과, 실제 개인 소유 섬유 2개 + 끈 1개가 선행 조건.
2. 기존 autonomous 실험/제작 선택 루프가 발견·학습·재생산을 수행한다. 성공/실패와 재료 소비는 Core만 처리한다.
3. `ItemKind::PrimitiveWrap`, `TechniqueId::PrimitiveWrap`, `ExperimentKind::WeavePrimitiveWrap`, `TechnologyId::PrimitiveWrap`, `CapabilityId::WearProtection`를 **enum 끝에만 추가**해 기존 저장값 ordinal을 보존한다.
4. Core observer: 개인 inventory에 `PrimitiveWrap/Fiber`가 수량 양수·내구도 양수로 존재할 때 **현재 자동 착용 정책**으로 `hasWornGarment / wornGarment` 기록을 제공한다. 아직 독립적인 착용 행동 AI나 추위 기반 착의 결정을 구현한 것은 **아니다**.
5. Web DTO `resident.civilization.wornGarment`만 이용해 3D 의복을 만든다. 원재료·의복 지식·시대 단계·GUID만으로 물건을 허위 생성하지 않는다.
6. 실제 SkinnedMesh torso/spine bone과 vertex 가중치를 검사해 검증된 리그에만 몸가리개 실루엣을 부착한다. 닿지 않는 공중 의복은 생성하지 않는다.
7. 천의 색은 자연 섬유 범위, 결속 끈은 제작 recipe에 포함된 재료, 헴 가장자리 마모는 **관측 내구도** 값으로만 조절한다. GUID는 작은 고유 시각 변형에만 사용한다.
8. 동일 garment signature면 geometry/material 재생성 금지; 변경/분실 시 이전 attachment 정리 및 dispose. 캐릭터별 LOD/기존 모션 유지.

## A3 범위 외 / 후속
- 자동 착의 외의 선택 행동, 습기·계절·기온 적응, 물리 보온 효과, 옷 제작 결정에 직접 연결한 자율 욕구, 옷 교환·유통/전용 착의 슬롯, 신발·악세서리·가죽 의복, 손상 사건, 신체 fit 고급 충돌과 임의 도안 발명은 **완료라고 주장하지 않는다**.
- 기존 base GLB의 개인별 색상은 임시 기본 외형일 뿐 Core에서 만들어진 복식이 아니다. 전신 기본 재질과 현대적 팔레트 제거는 후속 별도 개선.
- A3b: Core 자율 착용/해제 정책을 환경·경험·취향에 연결하고 세이브/로드에 안정적 상태로 반영.
- A3c: 새 실물의 재료·구조를 입력으로 받는 형태 파라미터 / 조합 기하 생성기로 일반화.

## 검증·병합 게이트
- Core CMake/CTest/determinism/장기 안정, WASM ABI/web DTO 인과 검증.
- Web typecheck/build, 캐릭터 회귀: 실물 증거·재현·결속·교체·누수·fail-close.
- 실제 pinned GLB 캐릭터 16포즈 export/browser QA 및 Pages Preflight.
- 병합 전 exact PR head CI PASS + 의미적/스냅샷 검증 확인.
- main 병합 전까지 개발 중 상태. 배포는 병합 후 Preview/External Probe 확인.
