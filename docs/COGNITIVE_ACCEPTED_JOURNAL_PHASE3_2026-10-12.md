# COG-3 자율 인지 수락 이력 — 저장/재생을 위한 첫 계약 (2026-10-12)

**기존 LifeLens 설계 규칙 미변경.** `docs/COGNITIVE_AGENT_LAYER_v1.md`의 COG-3 사전 조건을 구현하는 독립·무행동 영향 단계. 유료 API/외부 클라우드 불필요.

- Shadow Mode 출력은 기존대로 행동 authority가 없음. 임의 문자열이 Core world state를 직접 편집하는 것을 금지.
- `CognitiveAcceptedJournal.h`: 기존 `validateCognitiveProposal`에서 통과하고 동일 world minute, actor가 맞는 typed proposal만 **선택적으로 수락 이력에 기록**.
- 수락의 provenance: actor, minute, trigger, context fingerprint, backend/model identity, typed intent, target, IEEE754 priority 비트, rationale digest. 말의 내용은 행동 권한이 아니다.
- 실세계 자원, 건강, 가족·질병, 물리 규칙은 변경하지 않음. 기존 MemoryState에 tag versioned structured record로 저장되므로 binary save schema bump 없음.
- 동일 actor+minute+trigger+contextFingerprint의 재수락/재호출 방지, 1명당 최대 32개 accepted events만 보존, 다른 기억은 삭제하지 않음.
- 잘못된 actor, stale, 허위 대상, 허용되지 않은 intent, 우선순위 NaN, 과도한 provenance 길이, memory decode 오류는 거부.
- `test_cognitive_accepted_journal`: Core 검증, 중복 idempotency, 잘못된 제안 거부, 저장·불러오기, 순서/범위 제한, 개인 분리를 검증.

**미구현:** 이 PR은 받아들인 외부 제안을 실제 목표 점수에 반영하지 않는다. COG-4에서 journal recording을 실제 입력 path와 연결하고, snapshot replay/되묻기 금지를 통합 검증한 이후에만 연결한다.

참고 #664 개인 행동 경험, #666 실제 공간 경험에 의한 수원 선택, #660 인간다운 결과. 학습/선택/외형에 대한 사용자의 자율 개발 위임은 simulation truth·기존 설계·determinism 한계 내에서 적용한다.
