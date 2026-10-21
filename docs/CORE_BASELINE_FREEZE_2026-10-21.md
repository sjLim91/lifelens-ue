# M0-05 Core 실행·저장·Ruleset 기준선 동결 계약 (2026-10-21)

이 문서는 완료율만 올리기 위한 선언이 아니라 이후 PR의 **회귀 거부 기준**이다. 체크포인트 출발 SHA: `2757901ccb9cfc39f9d2e2207a5ca0c9b100cc3f`. 출하되는 새 Core 변경은 이 기준선 및 이후 `main` 전체 CTest를 유지해야 한다.

## 고정 대상

- Core typed snapshot schema: `SimulationSnapshotVersion=3`; 바이너리 저장: `SimulationSnapshotBinaryFormatVersion=8`; `CurrentSimulationRulesetVersion=1`.
- 이 버전 상수 변경은 저장 호환/전환 검증 없는 우발적 변경이 아니라 명시적 설계·마이그레이션 검토가 필요하다. 실제 규칙 수치를 변경하지 않는다.
- `test_core_baseline_freeze`: 시드 `2`, `42`, `874213955`; 새 월드 1일 실행을 독립적으로 두 번 반복하고 Core 전체 이진 스냅샷을 비교한다. 저장·복원 직후 바이너리가 동일해야 하며 **재개 240분 후의 전체 스냅샷**도 복원 없이 진행한 월드와 같아야 한다.
- `core-tests.yml`: Core CTest 전수, seed 42 결정론 재현 `ll_harness`, 변경된 Core의 3개 시드 365일 밸런스/수면 감사. `web-wasm.yml`, `web-checks.yml`, `preflight.yml`의 필수 검증도 유지한다.
- 주요 전수 CTest: 교역·지형·야생·돌봄·생존·출산·스냅샷·인지 관련 기존 테스트를 이 기준선의 필수 기능적 계약으로 둔다. 어느 하나가 실패해도 이 기준선을 통과했다고 하지 않는다.

## 완료 판정 / 유보

PR의 **정확한 HEAD**에서 Core+3시드 365일+WASM+Web+Preflight가 모두 성공하고, 코드가 실제 `main`에 병합되었을 때에만 M0-05 동결 테스트/기준선 검증 완료를 선언할 수 있다. 이 작업은 1000/3000일 자율 문명 인과 검증(M11)이나 전체 LifeLens 완성을 대체하지 않는다. UI 스크린샷 품질을 자동 CI 병합 조건으로 되돌리지 않는다.

무단 버전 증가·저장 포맷 바꾸기·추가 유료 서비스 필수화는 이 PR에 포함하지 않는다.
