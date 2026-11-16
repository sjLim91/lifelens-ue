# Android 앱 내 브라우저 실행 멈춤 수정

사용자 증상: **S23 울트라, ChatGPT 안에서 연 창, 화면과 버튼 전체가 멈춤**. 기준 main `03693a3` / 공개 UI `426d428`. 저장된 세계를 유지하는 실행 안정화 작업이며 M1 및 전체 milestone 수는 승격하지 않는다.

## 확인한 경계와 수정

- 같은 화면 크기·touch/DPR3·Android WebView UA의 실제 공개 3D 앱에서 Qwen worker **0개**인데도 메인 스레드가 **7,297ms** 연속 점유됐고, 후속 읽기는 **8초 응답 제한**에 걸렸다. 소프트웨어 GPU 재현이며 S23 실기기 수치가 아니다. [원시 재현](evidence/embedded-freeze-2026-11-16/before.json).
- Android WebView(`wv`)·ChatGPT UA에서는 GPU 없는 Canvas 2D Core 관찰로 시작한다. 앱 정보를 숨기는 custom tab에는 **`?safe=1`** 복구 주소를 제공한다. 이 주소는 저장 게임·렌더 설정을 덮어쓰지 않는다.
- 호환 경로에서는 세계/아바타 WebGL renderer·WebGPU Qwen·3D 지구본을 준비하지 않는다. 같은 Core 주민·지형·시간·저장 자료를 표시하며, Core agency는 그대로다. 모델 미연결은 성공 추론으로 표시하지 않는다.
- 세계/아바타 렌더러와 Three.js는 필요한 브라우저/화면에서만 동적으로 불러온다. 초기 entry JS를 약 1.52MB→0.52MB로 줄였다. 일반 Chrome/desktop의 기본 3D 선택을 유지하며, 중단된 boot의 늦은 renderer는 할당하지 않는다.

## 검증

- 실제 배포용 빌드, 위 모바일 프로필에서 **50초 연속** Core 08:00→17:54 진행, 주민 버튼 **137ms**, 오류 0, GPU context/worker 0, 무거운 GPU 모듈 요청 0.
- 호환 모드에서 지구본을 열어도 GPU 모듈을 준비하지 않고 안내 후 닫을 수 있다. 실제 IndexedDB 체크포인트 **1080분**의 commit을 기다린 뒤, 같은 세계 seed·시각(18:00)의 frozen reload를 확인했다. [원시 후보](evidence/embedded-freeze-2026-11-16/candidate.json) · [빌드 출처](evidence/embedded-freeze-2026-11-16/candidate-build.json).
- Typecheck/build, runtime resilience **19개** 및 실제 renderer bootstrap/늦은 취소 회귀, cognition **42개**·Shadow **16개** 및 live controller 회귀, 지구 좌표 계약과 관련 구조 검사 통과. Core 소스는 변경하지 않는다.
- 정확한 head의 CI·병합 tree·공개 배포와 복구 주소 검증은 후속 release proof로 기록한다. 물리 S23 실기기 성공을 주장하지 않는다.

## 공개 배포·복구 주소 확인

[#1038](https://github.com/sjLim91/lifelens-ue/pull/1038) UI main `91cae08f8594539a11962f8a01f320447ff02abd`의 tree는 정확한 검증 head `8e39aa0a`와 같다. Web Checks·Web Cognitive Agent·Preflight 세 workflow, Pages Preview/배포/External Probe 모두 성공했다. 공개 entry JS의 git blob과 후보 build SHA가 동일하다.

공개 자동 WebView 호환 경로는 50초 이상 연속으로 08:00→18:24 진행, 버튼 124ms, 오류/GPU context/worker/무거운 모듈 요청 0이었다. 실제 저장 commit 뒤 동일 세계 1109분(18:29) 재접속을 확인했다. UA가 일반 Chrome인 복구 주소도 Core 480→551분 진행·오류 0이었다. [원시 자동 관찰](evidence/embedded-freeze-2026-11-16/public-auto.json) · [복구 주소 관찰](evidence/embedded-freeze-2026-11-16/public-recovery.json) · [배포 출처](evidence/embedded-freeze-2026-11-16/release-proof.json).

사용자 복구 주소: https://sjlim91.github.io/lifelens-ue/?safe=1&v=91cae08 . 기존 멈춘 창을 닫은 뒤 열어 새 코드와 단일 World writer를 사용한다. 저장된 세계를 지우거나 다른 시뮬레이션으로 바꾸지 않는다. M1 6/7 및 기존 milestone 분모/상태는 유지한다.
