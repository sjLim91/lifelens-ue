# LifeLens COG-PERCEPTION P1 — 개인 기억 출처를 실제 인지 입력에 보존

> 2026-10-12. 자율 인공생명 권한·인과 경계 v1의 "World truth ≠ 개인 기억/신념" 규칙을 COG-3 및 Web Shadow 입력에 연결한 작업.

## 실제 변경

- 기존 Core 개인 MemoryRecord에는 MemorySource (DirectWitness / ToldByOther / Inferred)와 witnessed가 이미 보존되어 있었다.
- 그러나 CognitiveRequest 안의 CognitiveMemoryEvidence 및 Web DTO에는 이 출처가 없어서, 무료 로컬 모델이 전언/추론을 직접 목격한 사실과 구분할 방법이 없었다.
- 이번 변경으로 **Core MemoryRecord -> bounded CognitiveRequest -> WebClientBridge.cognitiveRequestJson -> TypeScript CognitiveMemoryDto -> 로컬 AI**까지 source 및 witnessed를 전달한다.
- 모델 시스템 프롬프트에서 직접 목격과 소문/추론을 구분하며, 후자를 확인된 월드 진실로 취급하지 않도록 한다.
- Core COG-3 수락 입력 저널의 cognitiveRequestFingerprint에 source/witnessed 값을 추가한다. 동일 문장의 기억이어도 출처가 바뀌었다면 이전에 받았던 모델 판단 문맥과 같다고 인정하지 않는다.
- 오래된 Web/WASM read model에는 두 필드가 없을 수 있으므로 TypeScript parser는 **Inferred + witnessed=false**로 보수적으로 치환한다. DirectWitness로 자동 승격하지 않는다. 잘못된 source/boolean 값은 거부한다.

## 의도적으로 변경하지 않는 범위

- 주민이 없던 사건을 목격했다고 설정하거나 거짓 기억을 세계 진실로 바꾸지 않는다.
- 질병·수원·기술·사회행동·건설의 결과, Needs와 물리 실행, World/RNG, SnapshotVersion=3/Binary=8 변경 없음.
- 이 작업은 실제로 존재한 메모리의 출처를 **추가 전달**한다. PerceptionFact의 실재 관찰/시야/청각 제한을 새로 구현하거나 AI 판단을 행동으로 실행하지 않는다.
- COG-3에 이미 저장된 과거 승인 판단의 contextFingerprint는 그 당시 기준의 역사적 입력 식별자로 보존한다. 복원 과정에서 과거 결정을 다시 모델에 묻거나 구버전 fingerprint를 새 값으로 덮어쓰지 않는다.
- B3 조합형 GoalSpec 문서의 "인지 기억에 source가 없다"는 경계는 이 PR이 main에 병합된 시점부터 해소된다. 다만 **출처 전달과 사실 증명은 다르며**, 가설/계획/실제 행동 권한은 여전히 별도 검증이 필요하다.

## 테스트

- Core COG-0: 같은 주민의 직접/전언 MemoryRecord가 bounded CognitiveRequest에 그대로 들어오는지.
- Core COG-3: 같은 내용이라도 MemorySource 또는 witnessed 비트가 다르면 fingerprint가 달라지는지.
- Core Web bridge와 Web WASM: read-only CognitiveRequest JSON serialization/JS 타입 검증/로컬 모델 프롬프트 호환.
- Core Tests, Preflight, Web WASM, 100일+ long-run 원래 시뮬레이션 동등성 검증.

## 다음 작업

World의 실제 감각·지형·물체·동물 사건과 연동되는 typed PerceptionFact를 설계·검증해야 한다. 이미 전해 받은 말/오해를 그대로 믿는 주민 행동도 인과 추적으로 별도 테스트해야 한다. 이것은 script unlock이 아니라 개체마다 서로 다른 불완전한 지식을 기반으로 하는 자율성 기반이다.
