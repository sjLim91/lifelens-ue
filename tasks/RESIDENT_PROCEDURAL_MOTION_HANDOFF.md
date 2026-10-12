# 주민 절차적 모션 재개 기록

- 기준 main: `bcff5d9c2635762699bcf66cfd5a99475d71e946`.
- PR: https://github.com/sjLim91/lifelens-ue/pull/700 (DRAFT, 병합하지 않음).
- 제품 구현 HEAD: `eaca7280fa3cace3933a0bdfc9db2a5ef5a50390`, CI 검증 진행 중.
- 브랜치: `work/resident-procedural-motion-20261012`.
- 범위/분석/원인/검증/한계: `docs/RESIDENT_PROCEDURAL_MOTION_2026-10-12.md`.
- 최초 Git HTTP clone: 환경 proxy:8080 연결 실패. 동일 commit의 UTF-8 소스를 GitHub connector로 확보했다. 로컬 git checkpoint는 진짜 upstream commit이 아니며 remote에 push하지 않는다.
- 추가 network permission 호출은 응답하지 않아 사용자 중단이 발생했다. 이 경로를 다시 사용하지 않고 connector와 offline dependency로 구현/검증을 이어갔다.
- 로컬 Three r180은 공식 저장소의 build/addon 소스, character.glb는 기존 pinned Git blob을 사용했다. 검증용 TypeScript shim은 Node stripTypeScriptTypes만 실행하며 repo/PR에 포함하지 않는다. typecheck 완료로 주장하지 않는다.
- Chromium 로컬 실행은 sandbox socket 제한으로 실패. Blender actual-skin CPU pose review는 성공. WebGL fixture/CI 검증은 별도다.
- exact next action: draft PR의 exact-head typecheck/build/character/browser 결과를 확인하고 실패를 수정한다. Core/저장/AI 권한은 변경하지 않는다.
- main 병합은 수행하지 않는다. 미완료 시각/모바일/광범위 자동 리깅 범위를 완료로 기록하지 않는다.

- 첫 CI 실패: 기존 수면 후 Eat fixture가 실제 음식 없이 consume을 기대했다. fixture에 실제 PlantFood를 추가하고 이동 중 carry를 기대하도록 수정했다. 실제 pinned UAL1/UAL2를 CI artifact에서 받아 로컬 기존 모션 15개 전체 PASS 확인. runtime 제품 코드는 첫 HEAD와 동일하다.


## 2026-10-12 DONE — #700 병합·실행 미리보기 반영

- 모든 exact-head CI PASS. Core CTest 119개/실패0, 3 seed × 365일 검증 PASS.
- main merge SHA: `5203449a52960923882a5f16379a18e834ea0e62`; #700 squash 병합 완료.
- 제품 preview source SHA: 같은 `5203449`; gh-pages publication SHA: `a74cedd544b61443dd25b3c79e894b1ac875a0ee`.
- Web Preview https://github.com/sjLim91/lifelens-ue/actions/runs/38254855263 PASS.
- External Preview Probe https://github.com/sjLim91/lifelens-ue/actions/runs/38254936779 PASS: launcher/index/hashed JS/WASM 200.
- 실제 최신 제품 실행: https://sjlim91.github.io/lifelens-ue/index.html?v=5203449 .
- 이 항목은 위의 DRAFT/검증 대기/병합 보류 기록을 대체한다. 구현 한계는 기존 보고서를 유지한다.
- 문서 상태 동기화는 제품 코드 변경이 아니며 새 WASM 빌드나 중복 preview 배포를 요구하지 않는다.
