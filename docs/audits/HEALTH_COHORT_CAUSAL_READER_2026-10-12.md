# 건강·사회 인과 진단: 전체 주민 건강 Cohort ZIP Reader

기준 main: `d494adbc2f9d24912840fa2268ba8647b8da6ace`. #660 인간적인 질병/회복/사망 원칙 후속 구현.

기존 `Tools/audit_social_family_health.py`는 사망 주민의 일일 건강 기록 중심이다. 이 읽기 전용 판독기는 **건강, 병든 후 회복, 병으로 사망, 발병 기록이 없는 주민** 모두를 `resident id` 단위로 연결한다.

## 실행

```bash
python3 Tools/audit_health_cohort.py longrun-audit-4242001.zip longrun-audit-874213954.zip --output cohort.json
python3 -m unittest discover -s Tools/tests -p 'test_health_cohort_audit.py' -v
```

검증 완료된 장기 감사 ZIP의 `events.jsonl`, `metrics.jsonl`, `summary.json`, `evidence_validation.json`을 사용한다. 직접 코어 실행/코어 값 쓰기/보건 확률 조정 없음.

주민별 `illness_onsets`, `recoveries`, `fatal_cause`, 일일 병원체 최고치, 질병 심각도 최고치, 환경 고노출 최대/평균, 섭취 수분 오염량 합산을 표기한다. 종전 같은 질병에서 **발병이 회복보다 먼저 중복 기록**되는 사례를 `duplicated_onset_before_recovery`로 분류한다.

### 해석 제약
- '발병 기록 없음'은 **감염 경험이 없다는 확증이 아니다**.
- 일일 환경 노출은 사람별 오염원 인과 확정이 아니며 실제 위치/폐기물의 source attribution이 없다.
- Event에서 출생 resident=0이므로 모든 출생 아이 ID/동명이인을 연결했다고 주장하지 않는다.
- 건강과 사회행동의 서로 다른 결과를 자연스럽게 검증하는 도구지, 무조건 100% 생존하도록 튜닝하기 위한 게 아니다.
- 다른 seed 다회 실행의 분포를 함께 보고, 단일 seed 생존자 수에 합격선을 고정하지 않는다.

관련 #660, #654, #661.
