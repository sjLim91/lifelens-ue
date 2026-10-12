# COG-3 — Core 수락형 인지 제안 저널 (2026-10-12)

## 목적
무료 로컬 모델이 주민 문맥을 보고 제안한 장기 의도를 저장 없이 즉시 행동에 주입하면, 저장/재시작 후 외부 모델 출력이 바뀌어 결정론과 세계 인과가 깨진다. **행동 영향을 주기 전** 수락한 모델 판단을 검증/영구 저장할 독립 Core ledger를 만든다.

## 구현 경계
- 기존 Core의 `buildCognitiveRequest` 원본 문맥과 호출자의 request **전체 상태 지문**을 비교하여 임의 변조·오래된 판단 거절.
- `validateCognitiveProposal` (actor/trigger/intent/target/priority) 검증 통과한 것만 Core 기록.
- provenance: actor, minute, trigger, full context fingerprint, backend identity, intent, priority, target, rationale digest.
- 동일 분에 동일 주민 1건만 수락, 과거 분 재입력/중복 차단. 최대 최근 1,024건까지만 보존.
- snapshot optional binary tail `LLCGJN01`; **0건이면 기존 저장 byte 그대로**. 과거 #661 Core 이전 snapshot을 새 모델 없이 정상 복구 가능.
- 확정 기록은 Core 저장·불러오기 뒤 재질문하지 않고 원본 의도로 보존. 잘못된 ID·목표·순서·우선도 및 손상된 확장 거절.
- 중요: **아직 인지 제안으로 Core 물리/사회/문명 action을 직접 실행하지 않는다.** COG-4에서 기존 Utility/Civilization candidate에 bounded bias로만 반영할 때 적용하고, 매 적용마다 행동·자원·지형·관계 feasibility를 새로 검증한다.
- Appearance/lifestyle/world/Needs/health/social design rules는 변경하지 않음. 외부 서비스 필수 의존성 없음. 유료 API 없음. Web UI 변경 없음.

## 테스트
`test_cognitive_decision_journal`: 실제 NEW GAME 주민의 typed request→Core acceptance, 중복·변조·stale 거절, binary save/load/journal 보존, 미사용 세계 구버전 byte 호환, invalid actor reject. 전체 native Core/WASM/Preflight/sleep gate 확인 뒤 main 병합 판단.

관련 docs/COGNITIVE_AGENT_LAYER_v1.md, #665 자율 AI 로드맵.
