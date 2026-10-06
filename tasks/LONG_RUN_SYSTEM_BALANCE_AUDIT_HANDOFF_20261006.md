# 전 시스템 장기 감사 재개 기록 — 2026-10-06

- 사용자 범위: 진단 전용, Core truth/규칙/수치/save schema 변경 금지. PR까지만; merge/auto-merge/배포 금지.
- branch: audit/system-balance-causal-20261006
- 확인한 main: 67fbf1e2c133499b2271f885ef066df30efdf665
- 시작 시 열린 PR: 0. WORK_STATE / TEAM_BOARD는 수정하지 않는다.
- AGENTS.md와 지정 문서 확인. CURRENT_MAIN_LONGRUN_AUDIT/WORK_STATE에는 과거 결과가 혼재하므로 새 실행과 분리한다.
- clone 금지 준수: GitHub 연결로 개별 Core 파일을 읽고 작업한다. 로컬 경로 /workspace/Source/LifeLensCore.
- 기존 balance_audit.cpp와 current-main-longrun-audit.yml 확장. 초기 checkpoint 1,7,30,100,365,1000.
- 기본 seed 874213954 / 4242001. 추가 seed는 결과 다양성 확인 후 결정.
- 현재 단계: 원본 Core 파일 수집 및 기존 계측 범위 분석. 아직 새 장기 결과 없음.
- 재개 시 actual main/open PR/branch HEAD/Actions부터 재조회. 로컬 파일/산출물 보존 여부 확인.
- 계획: 시간축 JSONL + checkpoint CSV/JSON/Markdown, per-resident Needs/time budget, resource/facility/chunk/health/knowledge state, causal evidence, invariant/determinism 검증.
- 결과가 없는 항목은 0으로 대신하지 말고 unavailable/unmeasured로 명시한다.
- 기존 --load-snapshot은 simulation만 복원하고 누적 metrics는 복원하지 않음. 재개 구간 scope를 반드시 명시하거나 원점부터 재실행한다.
