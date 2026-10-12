# 사회 첫 접촉 계획권 보호 실험 (1시간 기회선)

기존 #657의 15분 unified-only 첫 접촉은 CTest `test_production_new_game` (시작 후 6시간 내 사회 접촉 필수)를 실패하여 메인 부적합.

새 후보는 #656의 지역 사회 bootstrap/아이 실제 귀환 기능을 기반으로, Off-cadence 사회 bootstrap 평가 빈도를
사회적 첫 접촉 OpportunityIntervalMinutes=60, OffsetMinutes=5로 제한한다.
이는 별도의 Goal을 강제하지 않는다. 시민/생존 활동을 무시하지 않으며 기존 personality score/Needs/physical utility veto 동일.
15분 문명 기회는 매번 정상 실행된다. 민감도 검증이 필요한 정책 가정이며 고정 불변 계약으로 선언하지 않는다.

**main 병합 금지:** 6시간 first contact, seed874/424 1000일 social/marriage/birth/adult+infant survival, Determinism, WSE, WASM CI를 모두 계측·비교한다.
