# 주민 절차적 모션 재개 기록

- 기준 main: `bcff5d9c2635762699bcf66cfd5a99475d71e946`.
- 브랜치 예정: `work/resident-procedural-motion-20261012`.
- 범위/분석/원인/검증/한계: `docs/RESIDENT_PROCEDURAL_MOTION_2026-10-12.md`.
- 최초 Git HTTP clone: 환경 proxy:8080 연결 실패. 동일 commit의 UTF-8 소스를 GitHub connector로 확보했다. 로컬 git checkpoint는 진짜 upstream commit이 아니며 remote에 push하지 않는다.
- 추가 network permission 호출은 응답하지 않아 사용자 중단이 발생했다. 이 경로를 다시 사용하지 않고 connector와 offline dependency로 구현/검증을 이어갔다.
- 로컬 Three r180은 공식 저장소의 build/addon 소스, character.glb는 기존 pinned Git blob을 사용했다. 검증용 TypeScript shim은 Node stripTypeScriptTypes만 실행하며 repo/PR에 포함하지 않는다. typecheck 완료로 주장하지 않는다.
- Chromium 로컬 실행은 sandbox socket 제한으로 실패. Blender actual-skin CPU pose review는 성공. WebGL fixture/CI 검증은 별도다.
- exact next action: draft PR의 exact-head typecheck/build/character/browser 결과를 확인하고 실패를 수정한다. Core/저장/AI 권한은 변경하지 않는다.
- main 병합은 수행하지 않는다. 미완료 시각/모바일/광범위 자동 리깅 범위를 완료로 기록하지 않는다.
