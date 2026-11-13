# M10 전체 완료 작업

사용자 요청은 M10-01~07 전체 완료다. 이미 배포한 M10-03에 한정하지 않는다. 발열 검증은 제외하며 기존 머지·배포 사전 승인을 유지한다. 최신 통합 main은 `e22063bd`(#1026 생존 중단 후 저장된 계획 재개를 보존)이고 Core 권위·실제 데이터·무료 baseline·기존 저장을 보존한다.

## 같은 Web presentation 단위에서 완료할 범위

- [x] M10-01: 모바일 가로/세로 관찰 UI, 지구→지역→실제 생활의 왕복, 시드·속도·회전/이동/핀치. 지구본 자동 회전 제거, 입력이 있을 때만 그리기, 다중 터치 전환·취소 정리와 가로 화면 배치를 보완한다.
- [x] M10-02: 실제 욕구·성격 14필드·생애·기억·믿음/가설·현재 실행 계획·목표 후보·상대·가족. 후보/승인/발행/완료를 섞지 않으며 기존 읽기 전용 Core API를 사용한다. Core material enum에서 한국어 자원 이름을 생성한다.
- [x] M10-03: #1023 main `6dcfa886`, #1027 main `451642bd`의 실제 승인→토큰/교환 사실→결과→학습 및 저장·재접속 인과 기록을 보존한다.
- [x] M10-04: 남녀·아동·청소년·성인·노년의 실제 생산 rig/피부 메시·기본복·외형 변이·LOD를 확인한다. 검증용 생애 입력은 fixture라고 표시하며 제품의 새 게임 인구를 바꾸지 않는다.
- [x] M10-05: 실제 생산 모션의 이동/방향 반전/시선/작업/수면/부상/용변/돌봄, 지형 접지와 0/1/4배속을 확인한다. 보이지 않던 화면을 다시 열 때는 최신 Core 위치에 재배치하고 가짜 이동/발자국을 재생하지 않는다.
- [x] M10-06: 호랑이·늑대·사슴·토끼 및 기존 물고기의 원본 rig/LOD, 걷기·도주·추격·섭식·급수·휴식·정지를 확인한다. Core 위치·행동 이외의 사냥/먹이/목적지를 만들지 않는다.
- [x] M10-07: 강·숲·색상·날씨·흔적·시설 진행/파손/수리의 실제 데이터와 LOD를 검증한다. 모바일 프로필의 실제 CPU 프레임·FPS·그리기/삼각형·그래픽 객체/버퍼 기록·내보내기를 제공한다. 숨겨진 생활 화면의 GPU 작업을 중단하고 구형 캐릭터 경로의 RAF·리소스·캐시를 정리한다.

## 검증 원칙

실제 Core 통합 검사에서 기존 `ObservedLocalResource` 목표 근거가 bridge에서 `Unknown`으로 직렬화되어 유효한 목표 후보 전체가 거부되는 결함을 확인했다. Core exporter의 이름과 Web reader를 같은 enum으로 연결하며 물리 상태·규칙·RNG·저장 형식은 바꾸지 않는다. 읽기 전용 native/생성 WASM 회귀로 확인한다.

Web 전체 타입/빌드/회귀와 생산 자산의 형태·모션 검사를 실행한다. 브라우저에서는 실제 Core 앱과 별도 표시용 rig fixture를 구분하고 320×568, 393×852, 915×412를 검사한다. 소프트웨어 GPU 수치를 물리 Android GPU 수치나 발열 측정으로 표시하지 않는다. 기존 86개 인수 조건은 유지하며 실제 통과·병합·배포 근거가 마련된 항목만 완료로 반영한다.

현재 상태: 전체 M10-01~07 구현·통합 검증·main `b4a426bb` 병합·공개 배포 검증 완료.

## 최종 인수 근거

코드 PR #1028 head `9cf25392af667d37bc73c45cb08f182e5d5dd575` → main `b4a426bb5254eccceba71d49b2d85918f1db119c`. [Core Tests](https://github.com/sjLim91/lifelens-ue/actions/runs/43384850614) · [Preflight](https://github.com/sjLim91/lifelens-ue/actions/runs/43384850557) · [Cognitive Agent](https://github.com/sjLim91/lifelens-ue/actions/runs/43384850514) · [Web WASM](https://github.com/sjLim91/lifelens-ue/actions/runs/43384850549) · [Web Checks](https://github.com/sjLim91/lifelens-ue/actions/runs/43384850527). [Runtime Release](https://github.com/sjLim91/lifelens-ue/actions/runs/43386525057) · [Pages Preview](https://github.com/sjLim91/lifelens-ue/actions/runs/43386737776) · [External Probe](https://github.com/sjLim91/lifelens-ue/actions/runs/43386868352).

| 검증 | 실제 결과 / 재현 명령 |
|---|---|
| Native Core | CMake Release + CTest **179/179**, deterministic harness 및 365일 회귀 CI PASS |
| Core/Web 경계 | 실제 CI 생성 WASM의 `Tools/verify_wasm_causal_journal.mjs` native checkpoint 재생·유효 목표 enum PASS. 조회 전후 binary snapshot 동일 |
| 타입·빌드·회귀 | `npm run typecheck`, `npm run build`, `Tools/ci_web.py regressions`, cognition/Preflight PASS |
| 실제 모바일 UI | `web/tests/m10/full-browser-review.cjs` UI_ONLY: 실제 Core 성격 14필드·목표, 320/393/915px 지구 및 지역 생성/동일 시드 복귀, 모바일 프레임 비용 JSON 저장 PASS |
| 카메라/기존 UI | `web/tests/input/browser-review.cjs` 및 `web/tests/m10/globe-touch-review.cjs`: 320/393/915/1440px, 실제 이동/회전/양방향 핀치/손가락 전환/선택 오작동 방지·일시정지/4배속 PASS |
| 생애·사람 모션 | 실제 생산 rig 표시 fixture **12 생애×성별 + 14 행동**, 별도 resident motion **21** / procedural motion **13** 회귀 PASS |
| 야생동물 | 실제 원본 skin **4종×6행동=24** 표시 fixture, 기존 물고기 포함 5종/15LOD·0/1/4배속·접지 회귀 PASS |
| 환경/자원/시설 | procedural-environment 12개 및 lifecycle, grounding-wildlife 6개 회귀와 기존 Core 시설 계약 CI PASS |
| 장시간 누적 원인 | 실제 WebGL 3회 생성/숨김/일시정지/재개/새 월드/해제. active 1 + inactive 최대 32, 유령 표시 없음, dispose 후 context lost, 늦은 load 재생성 없음 |
| 공개 배포 | 실제 공개 JS Git blob·WASM commit 일치, 인과 타임라인 새로고침 유지·페이지 오류 0, 모바일 지구/실제 지역 왕복 PASS |

원시 구조화 결과: [빌드 UI/프레임 기록](evidence/m10-2026-11-13/ui-acceptance.json), [공개 앱 UI/프레임 기록](evidence/m10-2026-11-13/public-ui-acceptance.json), [별도 생산 rig 표시 fixture](evidence/m10-2026-11-13/visual-acceptance.json), [실제 WebGL 수명](evidence/m10-2026-11-13/final-lifecycle.json), [지구본 실제 터치](evidence/m10-2026-11-13/globe-touch-acceptance.json), [생활 카메라/UI](evidence/m10-2026-11-13/camera-ui-acceptance.json), [소스/CI/배포 근거](evidence/m10-2026-11-13/release-proof.json).

실제 Core 앱 검사와 표시 전용 fixture를 구분한다. fixture가 Core의 자연 출생·사냥·작업 성공을 증명하지 않는다. 소프트웨어 WebGL의 FPS는 물리 Android 성능 수치가 아니며 비교 기준선 없는 향상률을 제시하지 않는다. 기능 인수와 별개로 물리 Android GPU/FPS·Qwen 추론 품질은 미측정이다. 발열 측정은 사용자 지시로 제외한다.
