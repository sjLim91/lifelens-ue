# 주민 욕구 UI 가독성 개선 인계 (2026-10-01)

## 작업 경계

- 기준 main: `0e409b53089739f737856ad508bfbdd99c80e8cb`.
- 별도 브랜치: `ui/resident-needs-readability-20261001`.
- 병행 챗의 Core 밸런스 감사 #540 / 위생 후보 실험과 분리한다. 해당 브랜치를 merge/cherry-pick하지 않는다.
- 수정 소유 범위: `web/src/ui/observer-readout.tsx`, 신규 `resident-needs.tsx`, 신규 `resident-needs.css`, 이 인계 문서만.
- Core, WASM DTO, store, simulation clock, App 모달 흐름, Three.js, 공용 styles.css, tasks/WORK_STATE.md를 변경하지 않는다.

## 구현

- 주민 목록/상세에 동일한 읽기 전용 욕구 막대와 퍼센트 사용.
- ‘높을수록 필요가 큼’을 명시. 수면/위생 100%를 충족 상태로 오해하지 않게 수면 필요/위생 필요/용변 필요로 표기.
- Core의 0..1 값을 그대로 막대 폭으로 표현. 퍼센트만 반올림. 임계치/긴급도/행동 이유를 UI에서 생성하지 않는다.
- 누락/비유한/범위 밖 값은 ‘확인 중’, 미측정 막대로 표시. 0%로 대체하지 않는다.
- 긴 이름/행동명 줄바꿈, 키보드 포커스, 클릭 가능한 카드 ‘자세히 보기’ 단서 추가.
- 애니메이션/타이머/구독/의존성 추가 없음.

## 현재 검증 상태 (2026-10-01 UTC)

- Draft PR: https://github.com/sjLim91/lifelens-ue/pull/541
- 구현 커밋: `1a2f4e0d6ffd36de54b088aec724272b6d27f564`. 이후 인계 문서만 보강했다.
- [Web Typecheck](https://github.com/sjLim91/lifelens-ue/actions/runs/36804740475): PASS (TypeScript + observation feed).
- [Web Runtime Resilience](https://github.com/sjLim91/lifelens-ue/actions/runs/36804740748): PASS. TypeScript, runtime loading/response, continuity, weather, character appearance, touch input, observation feed, resident action context, human traces, fast-forward, terrain/water presentation, **production browser build** 포함.
- [Preflight](https://github.com/sjLim91/lifelens-ue/actions/runs/36804740420): PASS.
- 로컬 실제 TSX를 Babel 변환해 정적 노드로 평가: 욕구 5개, 0/100%, 반올림, 누락/NaN/Infinity/범위 밖 값의 ‘확인 중’ 처리 PASS. React hydration 또는 WASM 실행 검증은 아님.
- **미완료:** 모바일/데스크톱 실제 브라우저 시각 검증. agent-browser CLI 미설치. 대체 Chromium 실행도 sandbox `setsockopt: Operation not permitted`로 실패했다. 스크린샷을 확보했다고 보고하지 않는다.
- 로컬 shell 네트워크의 proxy 연결 실패 및 네트워크 권한 요청 장기 대기로 npm 설치 불가. 같은 요청 반복하지 않는다. GitHub 연결/CI는 정상이다.
- main에 병합/배포하지 않았다. Core 밸런스 실험/감사 브랜치는 변경하지 않았다.

## 로컬 복구 자료 (같은 workspace가 유지될 때만)

- `/workspace/lifelens-ui/`에 수정 파일, `verify.cjs`, `ui-preview.html`이 있다.
- `node /workspace/lifelens-ui/verify.cjs --static-only`로 값 표시 검사 재실행 가능. 이 도구는 현 환경의 Playwright 내장 Babel 경로에 의존한다.
- `ui-preview.html`은 실제 컴포넌트에서 생성한 UI 검증용 고정 표본이다. 제품에 포함되지 않으며 실제 저장본/시뮬레이션 결과가 아니다.
- 네트워크가 되는 별도 환경에서는 저장소를 이 브랜치로 체크아웃해 npm 설치 후 실제 앱에서 검증한다. 로컬 폴더는 부분 checkout이므로 full build 기준으로 사용하지 않는다.

## 중단 후 이어가기

1. actual GitHub main / 이 브랜치 / PR / Actions부터 조회한다. 이 문서는 실행 상태 추정 근거로 사용하지 않는다.
2. 이 브랜치 diff가 위 4개 파일에만 제한되는지 확인한다.
3. 위 구현 커밋의 CI 성공을 확인한다. UI 소스를 추가 변경했을 때 해당 검증을 다시 실행한다.
4. 320/390px 모바일과 데스크톱에서 목록/상세, 0/100%, 데이터 누락, 긴 이름, 키보드 초점을 확인한다.
5. 실제 WASM 구동 확인 전 static fixture를 실제 Core 검증으로 보고하지 않는다.
6. 병합은 병행 Core 작업 상태 확인 후 별도 결정. simulation balance와 합치지 않는다.
