# 주민 자발 학습 결과 관찰 — Web Readout v1

**목적:** 사람이 어떻게 선택을 바꿨는지를 관찰하는 LifeLens 방향에 맞춰 Core Memory로 실제 확인된 경험을 주민 상세 상단에 별도 표시. 브라우저는 시뮬레이션 truth를 만들지 않는다.

- `action_experience` 태그가 있는 Core의 직접 경험 기록에 대해서만 식사/급수/수면/배변/위생 행동 성공·실패를 표시한다. #664 이후 데이터가 실제 제공될 때만 렌더.
- `environment`+`contamination`와 grid 위치가 있는 Core firsthand Memory만 실제 위생 위험 경험으로 표시.
- 주민별 최신 시각 기준 3개. 동일 행동+결과 또는 위치는 가장 최신 표본만 보임. 소문·추론·부정확한 타입·미래/음수 시각 데이터는 제외.
- 섹션 제목 "직접 경험으로 배운 것"은 Core 기억의 의미이며 외부 언어모델의 실제 사고/행동 권한을 뜻하지 않는다.
- 기존 Memory, Belief, Cognition Shadow UI는 유지. 주민이 이 기억으로 다른 수원을 선택하는 인과는 별도 #666 코드 검증 대상.
- Web UI/디자인 프레젠테이션만 변경, Core physics/인구·성장·건강·가족 설계 규칙 불변.

테스트: `cd web && node tests/resident-experiences/run.mjs` + `npm run typecheck` + Web build/preflight.
