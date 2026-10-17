# LifeLens 시각 제작 Q3 인계·검증 기준 (2026-10-14)

## 확인한 사실
- #719/#720 기존 웹 주민은 자체 절차적 해부학 메시와 외부 CC0 donor skeleton, 실측 접지 IK 사용.
- #721/#722 호랑이·5종 동물은 Core 관측과 현재 Three.js procedural mesh, GPU LOD/동작 사용.
- #724 GitHub Ubuntu runner Blender 4.0.2 설치 및 샘플 .blend/.glb 생성 검증. 고품질 모델 생성 완료는 아니다.
- #723 관찰 화면에서 가까운 나뭇잎 가림 제거. Web Checks 성공 및 실제 화면 검수 후 별도 병합.
- #725 Blender 원본 3종(남·여·호랑이) + 스킨 메시/골격/보행 클립 재현 자동화. 이는 런타임 채택 이전 시제품이다.

## 절대 계약
Core의 시뮬레이션 진실, 성별·유전·연령·행동·위치, 결정론, 저장/복원 규칙 불변. 외부 유료 API/모델 금지. Core의 실제 상태가 관찰 결과를 결정한다.

## 다음 구현 순서
1. Blender 산출물을 turntable/근접·원거리 PNG로 실제 시각 확인하고 기존 Q1과 비교. 사람 얼굴·헤어·눈·코·입·피부·손가락·발가락과 호랑이 털·줄무늬·귀·발·꼬리의 자연스러움 검수. GLB 생성 사실만으로 완료 선언 금지.
2. 웹 주민 기존 CC0 donor 뼈와 RESIDENT_JOINTS, 28개 이상 모션 phase, 나이/성별 변화, IK와 rest bind를 충족하도록 재리깅/리타게팅. 동작 중 외형 재생성 시 현재 애니메이션 pose를 bind mesh로 잘못 복사하지 않아야 함.
3. 호랑이 포함 동물 메시를 기존 WildlifeLayer의 species/sex/near/mid/far InstancedMesh 구조 및 wildlifePart/instanceMatrix 동작과 호환. 무작위 Core 상태 변경 금지.
4. 필요한 경우 실제 GLB를 웹 리포지터리 자산에 패키징하고 실동작 없는 경우 기존 Q1 fallback. CI 임시 artifact만 만들어서는 브라우저에 적용되는 것이 아니다.
5. 브라우저+Android 성능, 관절·지면 관통, 뒤로 걷기, 물리적 이동 중 끊김, Core 지속성/결정론 회귀를 모두 통과한 다음 main 병합.

## 자동 검증 및 전달
- Tools/blender/author_creatures.py: 남·여·호랑이 mesh / armature / skinned weights / walk motion .blend + GLB 생성.
- Tools/blender/validate_creatures.py: GLB 2.0 header, JOINTS_0, WEIGHTS_0, COLOR_0, animation channels 및 모델별 애니메이션 클립 격리 점검.
- Tools/blender/render_creature_review.py: 생성된 .blend에서 Cycles CPU 실제 render PNG 3장 제작. 스크린샷은 외형 평가 입력이며 품질 합격 판정은 아니다.
- Blender workflow artifact lifelens-authored-creature-prototypes는 7일 보존. 프로덕션 채택 시 원본/런타임 산출물 영구 보존 방식 별도 필요.

중요: Blender 스모크 성공 != 모델 제작 완료. 스킨과 클립 존재 != 정상 애니메이션. CI green != 시각 품질 승인. 런타임 미연결은 완료가 아니다.
