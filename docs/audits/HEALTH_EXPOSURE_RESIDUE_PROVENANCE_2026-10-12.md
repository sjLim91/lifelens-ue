# 장기감사 건강 노출 위치·폐기물 물리적 출처 계측 — 2026-10-12

기준 main: `543cb073c4d4c74c0af763007fbbe914b6981554`, 후속 #660, #653, #654.

## 목적
기존 Health 코어 감사에는 주민별 감염 심각도, 지역 오염 점수만 있고 **주민의 실제 위치와 폐기물 contributor의 신원**을 알 수 없었다. 그 결과 영아 4/4 사망이 특정 위생시설 근처 누적 오염의 구조적 문제인지, 일반 감염·회복 편차인지 분리하기 어렵다.

## 구현
`Source/LifeLensCore/harness/system_balance_audit.h`의 매일 `health_transition` 관측 레코드에만 아래 필드를 추가:
- `position_x`/`position_y`: 이 tick에서 Core가 제공하는 실제 주민 위치
- `top_residue_id`: 해당 지점 노출에 기여한 폐기물 가운데 가장 큰 기여량을 가진 실제 record ID, 0은 없음
- `top_residue_source`: 그 record에 마지막으로 기록된 생성 주민
- `top_residue_distance`/`top_residue_contribution`/`top_residue_amount`: 중심거리와 선형 감쇠를 적용한 단일 기록의 기여량, 기록의 현재 잔존량

`Tools/audit_health_cohort.py`는 이를 optional `last_position`/`last_top_residue` 필드로 읽는다. 기존 ZIP에도 후방 호환.

**해석 상의 제한:** 최대 기여 기록은 인과적으로 감염시킨 개체라는 의미가 아니다. 여러 residue 합의 누적·물 섭취·개인 면역·실제 체류시간은 별개이며, 기록의 sourceCharacter는 동일 타일에서 재퇴적되면 최근 사용자를 가리킬 수 있다. 다중 seed·일일 trace와 함께 검증해야 한다.

## 안전성
Sim/World/Health/Needs/Utility/RNG/save schema를 전혀 변경하지 않는 observer-only 추가. 동일 seed에서 Core truth snapshot byte와 deterministic contract가 동일해야 한다. audit JSONL에 새 read-only 키가 생기는 것은 의도된 진단 자료 확장.
