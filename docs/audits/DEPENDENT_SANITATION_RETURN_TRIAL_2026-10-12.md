# 의존 아동 ToiletAssist 물리적 복귀 회귀 후보

실험 기반: #652 사회 첫 접촉 + #651 위생시설 escort.
이번 수정은 #653의 주 원인 후보인 **사용 후 아동의 위생시설 체류**를 막는다.

- 픽업 위치를 기존 `PendingContextAction.civilization.facilityTargetPos`에 저장한다. Parenting 태그에서만 사용하므로 일반 문명 action/교역 authority 불변.
- `civilization.quantity==2`는 보육 위생시설 사용을 완료한 후 실제 물리적 복귀 여정이다.
- 영아와 caregiver의 실제 Core navigation step 일치. Return phase는 기존 PendingContextAction/네비게이션 snapshot codec에 의해 이어서 재생된다.
- 현장 배설물 기록/시설 useCount와 보육 need 개선은 **현장 도착 후 한번** 실행. 복귀 중 재실행하지 않는다.
- 도착 전에는 pending 계속 유지하여 새 보육행동이 기존 여정을 덮어쓰지 않도록 한다.
- 물리적 완료/저장 복구를 확인하는 단위 테스트 추가.
- 아직 route failure, critical survival preemption, 실제 영아 health 시드 결과 분석이 필요하며, 이 브랜치는 main merge 금지.
