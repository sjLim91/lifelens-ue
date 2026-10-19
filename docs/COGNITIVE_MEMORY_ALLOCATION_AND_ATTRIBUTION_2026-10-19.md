# 인지 요청 복사 비용과 기억 출처 계측 — 2026-10-19

#800 위에 쌓은 COG-5 성능/읽기 전용 계측 단위다.
이 변경은 기억을 삭제·요약하거나 주민의 관계/학습/행동 규칙을 바꾸지 않는다.

## 실제 비용과 수정
기존 buildCognitiveRequest는 전체 MemoryRecord의 what/where/tags를
CognitiveMemoryEvidence로 복사·정렬한 뒤 대부분을 버렸다. 이제 함수
내부 읽기 전용 view를 정렬하고 기존 최신 직접 경험·반복 실패 보존
규칙을 그대로 적용한 뒤 선택된 DTO의 문자열만 소유 복사한다.
뷰는 요청 밖으로 나가지 않는다.

기존 main의 함수 본문을 고정한 비교 기준으로 0/1/2/3/8/32/100개의
기억 예산, 최근 직접 경험과 두 실패 후보를 비교했다. 모든 요청 지문이
동일했다. 8천 개 장문 기억/8개 context 테스트의 동적 할당 총량은
8,939,427 → 839,925 bytes(약 90.6% 감소)였다. 이는 해당 요청 생성의
할당량이며 전체 월드 크기·모바일 FPS 개선 수치로 해석하지 않는다.
원본 기억과 수락/실행/저장 규칙을 수정하지 않는다.

## 기억 폭증의 원인 분해
기존 1000일 ll_balance_audit의 주민 checkpoint에 memory_attribution을
추가한다. 전체 entries, UTF-8 텍스트 bytes, DirectWitness/ToldByOther/
Inferred, 실제 직접 목격, 미래 timestamp, 나이 구간, primary tag별
개수/텍스트량, 완전히 동일한 중복 record를 관찰한다.
해시 충돌은 전체 record 비교로 확인하고 주민 상태는 쓰지 않는다.

text bytes는 문자열/태그의 실제 UTF-8 payload 합계이지 벡터 capacity,
메타데이터 또는 전체 snapshot bytes의 추정 합격 수치가 아니다.
primary tag는 64개 종류와 나머지 묶음으로 출력 크기를 제한한다.
기존 snapshot_bytes/activity_minutes/사회 사건 계측과 같은 checkpoint의
결과를 비교해 직접 학습 기록과 사회 사건 증폭을 구분할 수 있다.

Refs #669, #665, #670. 1000일 실제 결과와 더 큰 인구의 CPU/메모리
측정 없이 대규모 문명 성능이나 주민 추론 품질을 완료했다고 하지 않는다.
