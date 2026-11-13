# M10 관찰 UI·그래픽·모션 구현 체크포인트

## 범위와 권위

M10의 Web presentation 작업이다. 기존 #1017 모바일 관찰 UI/카메라를 보존하며 Core·WASM ABI·저장·자원/관계/아이템 수량·Qwen 실행 정책은 변경하지 않는다. `main` c9a6f322 이후 AL-2 #1018의 Core 변경도 그대로 보존한다.

## 구현

- 주민 상세의 접을 수 있는 **생각과 실제 행동**: 최근 주민 AI 제안과 검증 상태, 활성 Core 행동 번호/발행 시각, Core가 직접 기록한 개인 신뢰 목표 결과, 동일 주민·상대·행동·시각의 믿음 갱신을 구분한다. 제안 시간/의도가 비슷하다는 이유로 제안과 실행을 연결하지 않는다. 원래 DTO에 없는 연결은 미확인으로 표시한다.
- 완료 행동 근거는 정확한 `personal-goal-token:<uint64>`와 `goal_action_feedback / self_trust / Approach|Repair` 및 해당 주민의 직접 목격 기록으로 한정한다. 전언/추론/미래/타인 출처, 잘못된 토큰은 제외하고 중복 제거 후 최대 6개만 표시한다. 목표에 이미 도달한 경우와 진전/미달을 성공으로 합치지 않는다.
- 기본복은 실제 피부 메시의 bind-space 높이와 리그 가중치를 따라가는 표면 표현이다. 남녀/성장/노화·임신 변형과 LOD를 따라가고 손/얼굴/발을 덮지 않는다. 별도 메시·스킨·draw call·텍스처가 없으며 Core에서 제작된 장비나 자원으로 간주하지 않는다. 기존 fallback 해부학 메시에도 적용한다.
- 동물 보간/가까운 rig mixer/먼 LOD 시계를 주민과 동일한 표준 배속에 연결한다. 0배속에서 위치와 걷기 자세를 유지하고, 1/4배속에서 이동·회전·clip 재생이 함께 배속된다. 실제 Core 위치까지만 이동하고 긴 프레임의 실제 시간은 배속 전 100ms로 제한한다.
- **메뉴 → 월드 설정 → 화면 성능 확인**: 최대 300프레임 ring buffer에서 CPU 제출 시간 중앙값/p95, 실제 제출 간격과 FPS, 반사를 포함한 draw/삼각형 수, geometry/texture/program 수, 실제 버퍼 크기/DPR/profile을 읽고 JSON으로 저장한다. 큰 foreground 지연도 FPS에 포함하며 숨김/복귀 경계만 분리한다. 화면 크기 변경 시 다른 뷰포트 측정치를 섞지 않는다. 접힌 동안 UI 타이머와 상세 구독은 실행하지 않는다.
- 물이 시야 밖이면 6면 cube reflection/PMREM을 제출하지 않는다. dirty 상태를 유지해 다시 물이 보일 때 현재 장면을 반사한다. 실제 지형/수계/가시 수면 자체는 변경하지 않는다.
- 개인 목표 평가/행동 경험의 기억·태그·장소·믿음을 한국어로 표시하며 AI가 참고한 실제 경험 원문도 같은 번역 경로를 사용한다.

## 검증

- `npm --prefix web run typecheck`, `npm --prefix web run build`.
- `node web/tests/m10/run.mjs`: 거짓 인과/출처·시간·토큰·학습 배제, 한국어 표시, 실제 WildlifeLayer 정지/4배속/이동 상한, 실제 5종 15개 LOD rig mixer, bounded 성능 계측/긴 foreground 지연/resize/복귀, 실제 남녀 6개 LOD의 기본복과 원래 스킨 가중치, 시야 밖 반사 생략/복귀/GPU 상태 복원.
- 기존 resident-motion, resident-lifecycle, procedural-motion, procedural-environment, grounding-wildlife, imported-tiger, action-context, runtime-resilience, cognitive-agent, input/projection 회귀.
- `web/tests/input/browser-review.cjs`: 실제 Core WASM + 2D fallback에서 393×852 / 320×568 / 915×412 touch / 1440×1000 desktop. 실제 터치 회전/팬/핀치·네이티브 대화상자 focus/닫기·속도/시드 재시작 취소·페이지 넘침.
- `web/tests/m10/browser-review.cjs`: 실제 Core WASM + 소프트웨어 WebGL, 393×852 및 915×412. 주민 4명·욕구·인과 기록의 빈 상태와 가짜 목표 달성 없음, 기본복의 실제 화면, 3D 계측 JSON 다운로드, 가로/세로 fit 및 페이지/에셋 오류 확인. 모델 다운로드를 막기 위해 테스트에서 WebGPU만 미지원으로 설정한다.
- 선택적 브라우저 실행은 Playwright 경로를 `LIFELENS_PLAYWRIGHT_MODULE`로 전달한다. 로컬 이미지/JSON은 `/tmp/lifelens-m10-review`에 생성된다. Web Check의 `Tools/ci_web.py`가 새 `tests/m10/run.mjs`를 자동으로 포함한다. 원격 exact-head 및 배포 상태는 PR Checks/Actions를 따른다.

## 완료 판정 한계

M10 **7개 단위는 진행 중**이며 86개 인수 조건/완료 6개를 변경하지 않는다. UI의 제안→실행 전 출처 연결은 현재 DTO로 완결되지 않는다. 실제 장기 가족/모든 행동 및 종별·지형별 종합 시각 인수는 미완이다. 소프트웨어 WebGL/터치 에뮬레이션은 실제 Android GPU·배터리·발열 측정이 아니다. 이 빌드는 그 기기에서 수치를 남길 수 있는 측정 경로를 제공하지만 실기기 인수 완료를 주장하지 않는다.
