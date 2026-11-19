# LifeLens F2-e — 2026-11-19 실제 모바일 화면 리워크

## 출발점

F2-d 실제 모바일 캡처 두 장에서는 줌/장면 구성에 따라 시각 품질이 무너졌다.

- 넓은 관찰: 수목의 반복적인 간격, 큰 단색 원형 바닥 패턴, Core 청크 중심을 잇는 밝은 직선형 수계, 좁은 지형 표현
- 가까운 관찰: 주민에 비해 과도하게 커진 토끼/화로/침구, 단색 배경에 따로 떠 있는 SVG들
- 모든 화면: 세밀한 원화/자연스러운 지형 연결이 부족한 상태

## 이번 변경 (Web presentation만)

1. `fieldVisualSpriteTile`: 지면 카메라 줌과 이미지 확대율을 분리한다. 지면/발좌표/선택 좌표는 원래 Core 좌표로 확대되고, 시설·야생동물·식생은 무제한 커지지 않는다.
2. `fieldContinuousGroundNoise`: 절대 월드 좌표와 WorldSeed에 기반한 다중 주파수 저진폭 안료 변화를 사용해 단색/사각 타일 느낌 완화. 인접 지면색 경계는 연속 혼합.
3. `fieldGroundDetailMarks`: 넓은 빈 타원 패턴을 제거하고 실제 관측된 grass/forest/rock coverage에 기반한 작은 풀·흙·이끼 흔적으로 교체한다. 새 채집 자원은 만들지 않는다.
4. `paintObservedFieldRiver`: Core가 확인한 인접 상/하류 링크의 **동일한 시작·끝점만** 사용해 길이/폭에 제한된 얕은 S자 물결 및 물가 톤을 표현한다. Core 하천 자체의 실제 곡선으로 주장하지 않는다.
5. 2D Canvas만 사용하고 WebGL/추가 모델 다운로드/API를 도입하지 않는다.

## 변하지 않는 것

WorldSeed/주민 GUID/행동 AI/생태계·동물 수/시설 설치 위치/자원/경로/물의 존재/저장 상태는 Core가 유일한 authority다. RPG 레퍼런스의 장소·몬스터·퀘스트 등을 가져오지 않는다. `docs/FIELD_VISUAL_REFERENCE_CONTRACT_2026-11-19.md`를 따른다.

## 검증

- Web Typecheck / full presentation regressions / production build / Preflight / Pages / external probe
- 실제 체감 품질은 배포 화면 비교 필요. 실기기 QA 제외.
- 본 변경은 고품질 신규 일러스트 자산 제작이 아니다. 여러 도상 반복 및 산/절벽의 풍부한 공간감은 후속 그래픽 자산·레벨 오브 디테일 개선이 필요하다. 자동 테스트 통과를 아트 완성으로 보고하지 않는다.
