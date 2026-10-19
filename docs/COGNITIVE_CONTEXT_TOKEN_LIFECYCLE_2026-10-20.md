# #790 — 실제 Core 행동 토큰과 주민 AI 의도 추적 (2단계)

## 이번 추가 계측

기존 주민별 인지 선호 대조 감사에서 다음 세 종류의 실제
`PendingContextActionObservation`을 분리해 본다.

- KnowledgeTeaching: 교육 실행 후보
- Parenting: 실제 의존자 돌봄 실행 후보
- Trade: 주민 간 실제 거래 여정

각 종류마다 `observedStarts`는 처음 발견한 Core 고유 토큰 수,
`temporalIntentTargetMatches`는 실제 Core 토큰의 **행동자·대상·종류**가
그 토큰 발행 전에 수락된 해당 주민의 가장 최근 유효한 인지 의도와
일치하는 수, `noLongerPendingUnclassified`는 후속 샘플에서 토큰이
더 이상 활성 상태가 아닌 수, `replacedByAnotherToken`은 다른 토큰으로
교체된 수다.

**중요한 해석 제약**

- 토큰 소멸은 성공·실패를 증명하지 않는다. 취소, 만료, 생존 우선
  중단, 실제 완료 모두 가능하므로 `Unclassified`이다.
- 토큰/의도의 시간적 일치는 인과적 증명이 아니다. Core가 동일한
  자연 발생 행동을 선택할 수도 있다.
- 교역 토큰은 교환과 귀환의 두 단계로 이어질 수 있다.
- 가구 이주는 별도 Core 정착/가구 경로로 진행하므로 기존 context
  토큰에서 이주 성공을 조작해 집계하지 않는다.
- 인지 승인 ≠ Core 후보 ≠ 수행 ≠ 실제 결과. 사람의 마음이나 지식
  수준 변화도 추가 근거가 있어야 결과로 인정한다.

## 재현 방법

```sh
ll_resident_ai_paired_audit --seed 4242001 --days 100 --policy opportunities
ll_resident_ai_paired_audit --seed 874213954 --days 365 --policy opportunities
ll_resident_ai_paired_audit --seed 4242001 --days 1000 --policy opportunities
```

기존 CTest `test_resident_ai_paired_causal_audit`는 토큰 중복 관측,
교체, 소멸과 최신 인지 대상의 일치/불일치를 분리한다.

## 추가로 남은 완료 조건

정확한 성공률·실패 사유·자원/지식/관계 변화 인과 귀속을
확정하려면 Core가 각 행동에 대해 실제 발행/실행/취소/최종
결과와 동일 `token`이 들어 있는 권위 있는 이벤트를 노출해야 한다.
현재 변경은 절대 이를 꾸며내지 않는다. 따라서 #790은 계속 열린다.
