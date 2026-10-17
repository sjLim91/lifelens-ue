# 호랑이 원본 확보와 Web Observer 통합 — 진행 중 / 병합 금지

## 현재 판정

이 개발 단위는 **미완료 / 현재 시각 품질 Gate FAIL**이다. 공개 CC BY 원본
두 개와 원본 GLB, 편집 가능한 Blender 검수 파일, 실제 4방향 PNG, UTC 다운로드
기록과 SHA-256을 확보했다. 실제 웹 연동, 발 접지 검증, Android 실기 성능은
아직 미확보다. 제품 Core와 렌더 경로는 변경하지 않았다. 외부 유료 API/모델/
클라우드 작업을 사용하지 않는다. 아래 실제 결함이 해결되기 전 병합하지 않는다.

## 확보 자산 및 직접 이미지 검수 결과

| 자산 | 제작자 / 허가 | 원본 SHA-256 | 실제 검수 |
| --- | --- | --- | --- |
| [Tiger](https://www.sloyd.ai/free-3d-models/model/tiger-7m4ap2k1) | TopoForge / CC BY 4.0 / generated with Sloyd | `f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be` | 8,298,328 bytes, UV 포함, 28,973 vertices / 39,200 triangles. 통나무와 본체가 연결됨. 골격·클립 없음. 그대로 런타임에 쓸 수 없음. |
| [A Ferocious Tiger Figurine With A Muscular Body](https://www.sloyd.ai/free-3d-models/model/a-ferocious-tiger-figurine-with-a-muscular-body-inzht46x) | a Sloyd creator / CC BY 4.0 / generated with Sloyd | `b62a99cbe6d41e2f669e4c717e52f7c24f448623479d73ada1508dd7db4953e8` | 6,470,204 bytes. 실제 PNG가 이족 보행 humanoid 캐릭터임을 확인해 제외. |

두 모델은 신규 생성 API를 호출하지 않고 제작자가 게시한 공개 GLB 링크로
직접 받았다. `docs/asset-reviews/tiger-topoforge/`와
`docs/asset-reviews/tiger-ferocious/`에 `source.glb`, `source-review.blend`,
4방향 PNG, 원본 URL·UTC·파일 해시·제작자·라이선스·표기 문구 JSON 및 CREDITS를 보존했다.
다운로드 도구는 위 두 SHA-256과 다른 파일을 받으면 중단한다.

TopoForge source는 UV seam 때문에 index connectivity가 1,078개로 나뉜다.
좌표가 같은 정점을 분석상 연결하면 실제 geometry component는 3개이며 본체와
통나무가 같은 큰 표면이다. UV 조각을 독립 오브젝트로 간주해 삭제하지 않는다.

### 첫 받침 분리 실험 — 불합격

원본 corner UV를 유지하고 seam의 중복 위치만 연결한 뒤 graph cut으로 받침을
분리했다. voxel/remesh/새 primitive 몸체는 만들지 않았다. 실제 결과는
`body-*.png`, `body-extraction.blend`, `body-extraction.glb`, `body-extraction.json`에 있다.
첫 결과는 뒷다리 일부를 잃고 앞발 아래 나무 조각이 남아 **불합격**이다.
20,437 faces 제거, 67 boundary edges 잔존이라는 구조 수치도 함께 기록했다.
그 결과에 골격을 붙여 웹에 도입하지 않는다. 원본 paw anchor를 실제 orthogonal
진단 렌더에서 확인하고 절단 경계를 보정하는 단계다.

Blender 성공은 원본/검수 산출물 생성 성공만 뜻한다. 실제 Observer 화면,
새 모델의 스킨 변형/행동 연결/접지 및 실기 FPS 합격은 아직 아니다.

## 원본 후보 확인 (2026-10-14 UTC)

| 후보 | 출처 / 제작자 | 확인한 내용 | 도입 판정 |
| --- | --- | --- | --- |
| White Tiger (RIGGED ANIMATED) | https://sketchfab.com/3d-models/white-tiger-rigged-animated-9dd099d283e54f99b7cbd40b531b1a29 / MotionStreamStudios | 검색 색인에 CC Attribution, 33.1k triangles, 22k vertices. 설명이 아래 원본을 명시한다. 현재 원본 페이지 조회는 403. | 원본 저작자·라이선스 체인과 실제 파일 미검증. 사용 보류. |
| White Tiger의 명시된 원본 | https://sketchfab.com/3d-models/tiger-51ed5186afb04487ae6adb51f8ffd09b | 위 파생 모델 설명에서 관계 확인. 제작자/정확한 라이선스/실제 다운로드 미확인. | 미확인 상태로 재배포하지 않는다. |
| Tiger/Lion (Base Mesh Rigged) | https://blendswap.com/blend/27158 / LazyGraph | 원본 페이지 CC0, UV, quad topology, 기본 spine/leg IK rig. https://blendswap.com/blend/27158/download 에서 로그인 요구를 직접 확인했다. | 익명 확보 불가. 로그인 우회하지 않는다. 완성 PBR/동작 자산도 아니다. |
| Tiger rebuilt | https://sketchfab.com/3d-models/tiger-rebuilt-6b5d14b2de984ffcb00ee00e404ad208 / kenchoo | 설명이 두 모델의 혼합을 명시한다. | 다중 원본 체인 검증 필요. 단일 CC BY 표시만으로 도입하지 않는다. |
| Ares rigged and HQ | https://blendswap.com/blend/12238 / zingzong | CC BY-NC | 제외. |
| Merlin Tiger Rig | https://merlin3d.gumroad.com/l/TigerRig | 무료 표시만으로 상업 이용/재배포 허용을 확정할 수 없다. 관련 제작자 게시물에 개인·비상업 용도 제한이 보인다. | 명확한 허용 라이선스 확보 전 제외. |

위 초기 후보들의 모델 다운로드 시점·파일 SHA-256은 **미확보(null)**다.
확보한 두 Sloyd 원본의 기록은 앞 절과 각 source-record.json을 따른다. 웹페이지나
API 응답의 해시를 모델 파일 해시로 대신하지 않는다. 최종 CC BY 자산은
모델·텍스처·애니메이션 각각의 제작자, 원본/파생 URL, 정확한 라이선스 버전,
변경 내역과 표시 문구를 기록하고 배포 화면의 크레딧에서 접근 가능하게 한다.

## 실제 렌더 경로 분석

- `web/src/render/wildlife-layer.ts`: Core `TerrainWindow.wildlife.animals`를 읽어
  ID별 `Visible` 상태를 유지한다. grid 좌표를 월드 좌표로 변환하고 목표 위치로
  보간하며 heading을 실제 이동 방향으로 계산한다. 성숙도 scale, 수면/지형 및
  시설 footprint 필터, 128개 제한, near/mid/far(11/25 world units) 배치를 가진다.
  현재 동작은 `InstancedMesh`에 전달한 phase/motion/behavior 셰이더 변형이다.
  실제 스킨 골격/AnimationMixer 경로는 없다.
- `web/src/render/wildlife-quality-surface.ts`: species별 implicit volume 기반
  외형과 LOD를 만든다. 이 조형을 다시 정교하게 반복하는 것을 최종 제작 전략으로
  선택하지 않는다. 파일 로딩 실패나 원거리에는 기존 fallback이 필요하다.
- `resident-world-layer.ts`: character GLB와 UAL1/UAL2를 로드하고 skeleton을
  개체별 복제한다. authored anatomy/hair/face, 수면 calibration, procedural
  afterMotion 및 실제 속도 기반 gait 보정이 있으므로 GLB URL만 바꾸면 안 된다.
- `resident-rig.ts`: 의미별 bone alias와 rest quaternion, two-bone IK를 사용한다.
  동일한 bone 이름만으로 rest orientation과 bind matrix가 호환되지는 않는다.
- `resident-anatomical-body.ts`: donor의 skeleton에 새 parametric geometry를 만든다.
  MakeHuman 원본을 채택하면 이 교체 경로가 원본 토폴로지를 다시 덮지 않도록 한다.
- `resident-authored-appearance.ts`: head skin influence로 attachment를 맞춘다.
  새 eye/hair mesh를 덮거나 중복 생성하는 경로를 실제 clone/animation에서 검사해야 한다.
- `resident-motion-library.ts`: 정확한 클립 이름으로 행동을 매핑하고 특정 본의
  상체/하체 track을 합성한다. 새 골격의 변환 행렬을 고려한 리타게팅이 필요하다.

## #727 재사용 범위

`author_makehuman_cc0_trial.py`는 pinned MakeHuman OBJ/target의 형태를 파싱하고
실제 body vertex만 사용하며 UV/skin/eyes/hair 제작 모듈을 연결한다. 그러나
골격과 가중치는 LifeLens가 새로 만들고 독립 보행 Action을 작성한다. 기존
웹 모션의 실제 리타게팅, 주민 유전/성장 및 모바일 검증은 미완료다.
재사용 대상은 출처 핀, 원본 형태/UV 파싱, helper 제외, eye/hair 데이터와
검증 로직이다. 18본 생성과 독립 보행 성공을 원본 rig 보존이나 웹 리타게팅
완료로 판정하지 않는다. 호랑이 단위 완료 후 별도 사람 단위로 진행한다.

## 호랑이 통합 구현 조건

1. 출처 체인과 무료/재배포 조건을 통과한 실제 파일 확보 후 SHA-256과 UTC 기록.
2. 원본 topology/UV/PBR/skin/animations를 Blender에서 검사하고 좌표축, 지면
   pivot, 단위, 원본 clip name과 root motion을 확정한다. 주황색 털은 원본 UV에서
   흰 배/눈 주변/검은 줄무늬를 보존하며 제작한다.
3. WildlifeLayer가 이미 계산한 ID/위치/heading/scale/behavior를 스킨 actor에
   전달한다. 모델 clip의 root motion이 Core 이동을 덮지 않도록 한다. Core가
   제공하지 않는 행동을 임의로 만들어 표시하지 않는다.
4. 근거리 스킨 actor 수를 제한하고 mid/far에는 검증된 LOD 및 기존 배치를
   유지한다. 한 ID에 fallback과 새 메시가 동시에 표시되지 않아야 한다.
5. 로딩 실패, 부적합 rig/clip, 제거된 ID, 청크 변경, dispose 중 완료된 로딩을
   검사한다. 모델 오류로 기존 동물이 사라지거나 GPU 자원이 누적되지 않아야 한다.
6. 같은 조명/카메라에서 기존/새 모델 4방향·근경·모션 변형을 실제 웹에서 비교한다.
   발 접지/미끄러짐, 귀/눈/꼬리/털 부착 및 행동 전환을 검수한다.
7. Core/Web 회귀, production build, 실제 Observer 화면, Android 실기와
   desktop 성능을 확보한다. 인구/동물 수/해상도/기기/GPU/측정 구간/프레임 분포를
   기록한다. headless software GPU 또는 모바일 viewport는 Android 실기 결과가 아니다.

## 복구 checkpoint

로컬 환경은 proxy:8080 연결 실패로 Git clone/npm/모델 다운로드를 실행하지 못한다.
GitHub 커넥터를 통한 source GET은 작동한다. 별도 branch의
`LifeLens Tiger Source Evidence` Actions가 public API에서 3개 후보의 metadata와
익명 download endpoint 응답을 짧은 timeout으로 조회하고 JSON artifact를 남긴다.
인증 토큰을 넣지 않고 viewer 내부 파일을 추출하지 않는다. 원본 체인 검증 전
서명된 파일 URL을 저장/접근하거나 runtime 자산을 도입하지 않는다.

현재 비어 있는 근거: `.blend`, `.glb`, 모델 파일 SHA-256, 웹 비교 이미지,
애니메이션/접지 실측, Android FPS, 최종 CI. 이 항목이 채워지기 전 Draft 유지.
