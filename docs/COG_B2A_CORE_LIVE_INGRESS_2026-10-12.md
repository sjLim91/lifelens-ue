# COG-B2a — Core 현재 문맥 재검증 입구 계약

[권한·책임·인과 경계 v1](architecture/AUTONOMOUS_LIFE_AUTHORITY_BOUNDARIES_v1_2026-10-12.md) · [COG-B2 통합 이슈 #689](https://github.com/sjLim91/lifelens-ue/issues/689)

## 범위

\`CognitiveLiveIngressValidation.h\`는 **Core-owned read-only ingress validator**이며 이벤트 저널/행동 실행기 자체가 아니다. 미래의 COG-B2b 저장 수락 API가 호출해야 할 검증 전제조건만 구현한다.

1. WorldSeed/PopulationSeed/WorldGenerationVersion을 현재 Core World와 대조하고 simulation minute/trigger를 검사.
2. \`Simulation::observeCognitiveRequest(actor, trigger)\`로 **Core가 현재 주민 관찰 문맥을 직접 재생성**.
3. Core 현재 문맥의 World-bound stable digest와 **caller-supplied request 전체 digest**를 비교. caller가 가짜 Needs/Memory/Belief를 보내면서 자신의 claimed fingerprint도 맞춰 보내는 공격을 거절.
4. \`validateCognitiveProposal(world, actualCoreRequest, proposal)\`에 실제 생존·대상·allowlist·priority 판정을 최종 위임. 반환값은 \`valid\`/원인 및 현재 digest일 뿐 AI 판단을 world/save/history에 저장하지 않는다.
5. digest에는 Seed, PopulationSeed, generationVersion, actor/minute/trigger, Needs, Personality, Emotion, Memory, Belief, Relationship, allowlist를 타입별로 안정 순서로 포함. 이는 **인증/서명이 아니고 재현·문맥 동일성 검증**용이다.

## 중요한 미구현/금지

- 실제 local LLM의 제안을 Core에 수락·실행시키거나 생존 Utility를 변경하지 않음.
- 중복 accepted input journal, save/load/replay, 별도 Snapshot extension은 **COG-B2b에서 단일 계약**으로 처리. #667/#668 Draft와 중복 저널 금지.
- \`valid=true\`는 '현재 World 문맥에서 typed proposal이 유효함'일 뿐, 해당 제안이 기록됐거나 실제 Core 행동이 발생했다는 뜻이 아님.
- 외부 서버·유료 API·클라우드 불필요. 현재 Core의 Goal/기술/생태계·주민 규칙, ABI 변경 없음.

## 회귀

- 동일 seed/actor/context fingerprint 결정론.
- 다른 WorldSeed/PopulationSeed/generationVersion/시점, 사망/없는 actor, 가짜 Need/Memory/Belief/allowlist, 없는 타겟/틀린 actor의 proposal 거부.
- **자기 위조 request의 fingerprint를 다시 계산한 경우에도 Core의 현재 문맥과 다르면 거부**.
- 검사 20회 반복 전후의 실제 Core snapshot binary와 모든 로그가 정확 동일.
- exact-head Core Tests / WASM / Preflight / Weather Sleep & Era / 전용 COG-B2a CI 전부 확인 후 병합.

## 다음 단계

COG-B2b는 하나의 Core-owned accepted event journal에 world identity + context digest + actor + trigger + minute + typed intent + target + event ID를 기록하고, 모델 재질의 없이 저장/복원 및 replay를 수행한다. B2b Core writer가 **모든 수락 경로에서 이 validator를 통과**하도록 강제해야 하며, Web이 이 검증을 우회해 직접 journal을 수정해서는 안 된다.
