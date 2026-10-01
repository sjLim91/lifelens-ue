# 맵 표면 시각 개선 인계 — 2026-10-01

## 경계 / 복구 지점
- 기준 main: `7aedd14aa6c2c0f3145bc04ee526d86119d0b4ed`.
- 작업 브랜치: `visual/map-surface-polish-20261001`.
- 목적: 고도별 지형색의 불연속 완화, 밤/안개에서 수면이 주변과 따로 밝게 보이는 문제 완화.
- 변경 소유 범위: `web/src/render/terrain-presentation.ts`, `water-surface-material.ts`, 전용 맵 표면 회귀 검사와 기존 presentation runner 연결, 이 문서.
- 병행 #561/#562의 물 운반/용기/생존 밸런스, #540 감사와 분리. Core/DTO/store/App/공용 CSS/WORK_STATE/다른 렌더 레이어는 변경하지 않는다.
- 시각 색상·조명만 변경. 지형 높이, 수계 모양/수위/흐름, 통행·자원·시설·실행 결과는 기존 Core 계약 그대로 유지.
- main에 병합·배포하지 않는다. 신규 Draft PR로 검토한다.

## 상태
- 작업 범위 확정. 구현 및 검증 예정.
- 중단 후 actual GitHub main/branch/PR/Actions부터 확인. 이 문서의 상태를 성공 추정에 사용하지 않는다.
- 로컬 환경 `/workspace/lifelens-map`은 부분 소스이며 full checkout이 아니다.
- 이전 작업에서 shell npm 네트워크 및 Chromium 소켓 제한이 확인됐다. 같은 장기 권한 요청을 반복하지 않고 GitHub CI로 타입/빌드/회귀를 검증한다.
- CI 성공과 실제 WebGL 시각 검증은 구분해 기록한다.
