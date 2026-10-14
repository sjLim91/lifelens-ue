# Q6 — 자연스러운 인체 모델의 CC0 토폴로지 평가 (별도 실험)

## 이유
순수 voxel remesh + 절차적 단면 방법으로 실제 골격 메시를 생성했고 #727 Blender CI green이지만, 관찰자 실물 렌더는 여전히 인형 같은 눈·손·얼굴/머리카락과 어깨 연결이 남아 있다. 동일 조형 방식만 반복하지 않고 충분한 원본 인체 토폴로지/표정을 가진 CC0 메시를 비교 평가한다.

## 저작권·재현성
- **원본** MakeHuman Community 공식 https://github.com/makehumancommunity/makehuman, pinned commit `a8bc2d54ff0ac92e78ff71431b1023eda42bf482`.
- 입력은 MakeHuman 기본 `makehuman/data/3dobjs/base.obj` 및 공식 `makehuman/data/targets/macrodetails/` 폴더의 남녀 young 평균 근육/체중·caucasian-young morph target 네 개.
- 공식 LICENSE: https://github.com/makehumancommunity/makehuman/blob/master/LICENSE.md — 코어 그래픽 자산 CC0, 소스 프로그램 AGPL. **프로그램 소스, MPFB GPL Python 코드는 복사/번들/실행하지 않음.**
- LifeLens 자체 OBJ/target 파서만 제작. 네트워크가 필요한 곳은 GitHub 임시 CI의 CC0 자산 다운로드뿐. 유료 API, 사용자 PC 설치 불필요.
- CI 아티팩트만 생성하며 Web Observer 의존성으로 추가하지 않는다. Runtime 교체 미진행, 기존 Q1 fallback 보존.

## 테스트 계약
- 산출물: `resident-female-cc0.blend`, `resident-male-cc0.blend`, 대응 GLB 및 전신·얼굴 렌더.
- GLB 2.0/실제 메시/남녀 형태 차이/18 본/1개 보행 클립 검사.
- 결과 이미지에서 성별 외형 차이·얼굴 현실감·손발 토폴로지를 기존 Q5와 비교. 실제 CC0 베이스가 더 자연스럽더라도 자동 승인하지 않고 현재 리깅의 정상 보행/IK/정체성·성장·성별 바인딩과 모바일 성능/배포를 검증하기 전 main 병합 금지.
- 동일 성별/연령이더라도 Core 유전/개인 파라미터 차이가 실제 외형으로 이어지도록 모델 생성 매핑을 별도로 구현한다.

## 기술 제약
- MPFB2 최신은 Blender >=4.2가 필요하다. 프로젝트의 현재 Blender 4.0.2 CI에 무작정 애드온을 설치하지 않고 CC0 베이스 메시·타깃만 직접 해석한다.
- Blender 측 형태/리그/렌더 확인을 해도 기존 CC0 donor + Three.js Web Observer 동작에 즉시 사용되는 것은 아니다.
- 호랑이 등 동물에는 MakeHuman 인체 메시를 전용하거나 가짜 동물 형상에 사용하지 않는다.
## Q8 — UV/skin-material enhancement (experimental)
- MakeHuman original `base.obj` UV coordinates are preserved by LifeLens-owned OBJ parsing; face-only original CC0 topology uses matching UV seams, no planar approximations.
- Pinned graphical-source repository `s20220526/makehuman-assets` commit `8cf9645b975a98eea056b140df11a1d278da0d10`, under its repository CC0 asset grant. Only two young light-skinned diffuse maps are downloaded from Git LFS media endpoint for this initial neutral comparison; additional skin tones should be parametrized by identity later.
- Each input PNG is signature/size checked before Blender processing to reject LFS pointer files; embedded in `.blend` and `.glb` via Image Texture Base Color, with physically plausible roughness/subsurface.
- Source: https://github.com/s20220526/makehuman-assets and https://static.makehumancommunity.org/makehuman/faq/are_makehuman_files_free.html
- The visible 13,380 body vertices are a morphology/truth visualization only. Adult age/growth, identity/genetics, retarget to current Web Observer, actual deployment and performance remain open, so neither main nor production models change.