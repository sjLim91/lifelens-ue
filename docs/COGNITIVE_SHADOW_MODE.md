# Cognitive Agent Shadow Mode

## 목적과 authority

COG-0~2 및 #641의 typed request/proposal, free-local provider, bounded scheduler를 이용해 주민의 실제 Core 문맥과 인지 제안을 비교한다. 기존 Utility AI와 Core simulation truth는 그대로 유지한다. Core/WASM ABI, 교역·정착지·이주, snapshot/save에는 변경이 없다. 결과는 Web 메모리 안에서만 존재하며 저장·replay·행동 적용 경로가 없다.

현재 WASM에는 Core proposal 실행 검증 ABI가 없으므로 UI의 검증 통과는 기존 proposal schema, Core가 제공한 allowedIntents, 현재 살아 있는 대상 확인을 뜻한다. Core 실행 승인이나 accepted cognition event를 뜻하지 않는다.

## 사용

주민 상세의 접힌 고급 관찰을 열고 무료 로컬 OpenAI-compatible 서버의 루프백 주소와 모델 이름을 입력한 다음 관찰을 켠다. 기본 OFF이며 설정은 저장하지 않는다. 루프백 외 주소, URL 자격 증명, 리디렉션은 허용하지 않는다. 한국어 짧은 rationale을 요청하며 한국어 이유가 없으면 등록된 안내 문구를 표시한다.

수동 관찰 또는 선택 주민 자동 관찰을 사용할 수 있다. 자동 관찰은 기존 Core refresh에서만 평가하고 매 프레임 호출이나 별도 timer/cron은 없다. 주민, 관찰 시각, 당시 실제 행동, 제안 의도/대상/이유, Personality·Emotion·Memory·Belief·Relationship·Needs 요약, 관찰용 검증과 실패 사유를 표시한다. 현재 실제 행동은 최신 Core 상태로 별도 표시한다. 유효 제안 뒤 실패가 발생하면 최근 유효 제안과 최신 실패 사유를 함께 확인할 수 있다.

## 호출·메모리 budget

- 동시 1건, 대기열 0건. 기존 CognitionScheduler를 경유한다.
- 전역 실제 시간 최소 15초 간격(최대 분당 4회). 토글·월드 reset·모델 교체로 우회하지 않는다.
- 자동 관찰은 마지막 표본 이후 simulation 30분 진행 조건도 요구한다.
- timeout 6초, 기존 simulation stale 허용창 96분. 표본 시각을 명시하며 stale 제안과 문맥은 표시하지 않는다.
- 최대 32명 × 최근 3건, 총 96건. Core의 bounded context만 보관한다.
- OFF에서 context/provider 호출 없음. 고속 진행 중 취소·호출 중단.
- 주민 삭제/사망, 대상 소멸, 시간 역행, world identity 변경 시 무효화한다. 같은 seed 새 게임도 epoch reset으로 이전 응답을 차단한다.
- reset/dispose는 AbortSignal을 무시하는 제공자도 스케줄러 호출을 즉시 종료한다.

## 검증

`node web/tests/cognitive-agent/run.mjs`는 기존 17개 회귀와 Shadow 실패/생명주기/budget 검증을 수행한다. `shadow-ui.mjs`는 주민 상세 기본 접힘, 한국어 레이블, 요구 문맥과 실패 표시를 확인한다. world-lifecycle 테스트는 새 월드 refresh 이전 관찰 reset을 확인한다.

GitHub Actions는 production typecheck/build 및 실제 production Core WASM 비교를 수행한다. `shadow-core.mjs`는 seed 4242001 / 874213954 각각 독립 Core 인스턴스를 OFF/ON으로 1440분 진행하고 25개 checkpoint마다 9개 JSON 출력의 정확한 일치를 비교한다. OFF에서는 cognition context를 요청하지 않고 ON에서만 실제 Core context로 25개 typed proposal을 생성한다. 주민 전체 출력의 Needs·행동·위치·inventory·관계·감정·기억·신념 및 문명·월드 출력을 확인한다. save 바이너리는 WASM에 save ABI가 없어 직접 비교하지 않는다. 저장 API 접근 없는 구조와 기존 save 코드 무변경으로 경계를 유지한다.

## 협업 범위와 후속 단계

main 0d90d07edc263f9b5b56c0e69dfaa91142aa2d40 (#640/#641), open PR #642, TEAM_BOARD/WORK_STATE 확인 후 시작했다. #642의 Core 교역 여정/귀환 persistence 파일 10개와 변경 경로가 겹치지 않는다. 공유 WORK_STATE와 Core persistence 문서도 수정하지 않는다.

후속 단계는 실제 무료 로컬 모델로 문맥 충실도/지연/거절률을 관찰하고, 별도 Core 설계에서 accepted event persistence/replay 및 행동 authority 연결을 검토하는 것이다. 이번 PR에서는 실행 authority를 연결하지 않는다.
