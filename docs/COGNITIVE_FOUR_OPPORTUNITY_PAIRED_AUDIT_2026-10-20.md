# #790 — 주민 AI 실행 기회 장기 비교 감사

## 검사 방식

기존 `ll_resident_ai_paired_audit`에 `--policy opportunities`를 추가했다.

동일 초기 seed로 두 World를 독립 실행한다. 한쪽은 순정 Core,
다른 쪽은 교육/돌봄/교역/가구 이주 네 가지 의도만 사용하는
**고정 결정론적 대조 입력**을 15분마다 제안한다.

```sh
ll_resident_ai_paired_audit --seed 4242001 --days 100 --policy opportunities
ll_resident_ai_paired_audit --seed 874213954 --days 100 --policy opportunities
ll_resident_ai_paired_audit --seed 4242001 --days 365 --policy opportunities
ll_resident_ai_paired_audit --seed 874213954 --days 365 --policy opportunities
ll_resident_ai_paired_audit --seed 4242001 --days 1000 --policy opportunities
ll_resident_ai_paired_audit --seed 874213954 --days 1000 --policy opportunities
```

테스트하겠다는 지시와 **실제 완료된 장기 데이터는 구분**한다.
완료 결과와 실행 시간/실패 증거는 CI 아티팩트 또는 별도
결과 파일로 확인한 다음에만 기록한다.

## 수치와 해석 경계

- `opportunityProposalInputs`: 각 의도별 **제안 횟수 / Core 승인 횟수**.
  이것은 완료된 행동 수가 아니다.
- `observedKnowledgeTeachingPendingStarts`, `observedParentingPendingStarts`,
  `observedTradePendingStarts`: 실제 Core가 발급한 서로 다른 pending
  토큰 수. **성공·학습·식사·교환의 완료 횟수가 아니다.**
- `observedCivilizationPendingStarts`: 일반 문명 pending을 포함하므로
  가구 이주의 직접 실적을 뜻하지 않는다.
- `civilizationSuccessEvidence`와 `civilizationFailureEvidence`:
  실제 Core 기록에서 읽은 근거; 제안 인과에 전부 귀속할 수 없다.
- 주민 생존 수, 끝 시각, 기술/사실, 경험 기억과 기준 World의
  차이를 통해 장기 변화와 균형을 정량 비교할 수 있다.

## 인정할 수 없는 주장

이 대조 정책은 실제 언어모델이 아니다. 마을 생성과 출생 상황에
따라 CareForDependent 제안 자체가 목표 대상 부족으로 없을 수 있다.
편향되지 않은 대조 실험을 위해 새 가족/지식/자원/파트너를 만들어
넣지 않는다.

원하는 행동의 **미지원 사유별 분류와 승인된 특정 인지 입력→
실제 완료/실패 토큰 귀속**은 이 감사만으로 입증되지 않는다.
미완성 항목을 #790에서 계속 추적한다.
