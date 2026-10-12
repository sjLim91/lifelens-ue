# P1 사회·문명 경쟁 인과 실험: 통합 계획 슬롯

2026-10-12, 기준 실험 #656 `67a74fec0d63c20cd5b95de86d9118762024221a`.
#652/#656 seed874의 초기 adult pathogen/illness 데스와 day1~30 Social 12~25%, Civilization 12~14% 경쟁을 수정하기 위한 **실험 브랜치**.

- 첫 접촉이 기존 <0.18 social utility floor에서 막히는 문제는 유지하되, 이 first-contact 예외가 오직 15분 통합 문명/사회 계획 경합에 참여할 때만 허용.
- 5분간격 off-cadence의 독립 first-contact 경로는 중지. 새 종족 특성/Needs/질병/보건 임계값 추가 없음.
- 사회적 행동이 문명 후보를 무시하고 먼저 선점하는 버그 경로를 제거하되, 실제 생존·출산 개선 여부는 CI 1,000일 감사 전까지 UNKNOWN.
- 보육시설 실제 복귀 (#656)와 결합; 이 브랜치는 main merge 금지.

합격: exact-head Core, WASM, Preflight, Weather, 874/424 1000일, actual social/birth/family, infant and founder survival, trade, deterministic replay.
