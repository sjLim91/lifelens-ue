# 가족·건강 장기감사 ZIP 보고 도구

기존 `Tools/audit_system_balance.py`를 실행한 GitHub Actions의 ZIP artifact를 읽어서
**출산·사망·자녀별 연령·병원체/잔여물 관측과 ToiletAssist 위생시설 사용**을 분리한다.

- 시뮬레이션 Core, seed, save, balance, ruleset 또는 건강 확률을 변경하지 않는다.
- 하네스 통과/워크플로 green을 인구 및 건강 품질 승인으로 해석하지 않는다.
- `health_transition`의 `local_residue_exposure_after_tick`은 위험 신호지만 정확히 어떤 시설이 오염을 유발했는지는 좌표/경로/dose source 추가 추적이 필요하다.
- 한 ZIP에 repeat를 제외한 seed별 `summary.json`, `evidence_validation.json`, `events.jsonl`, `metrics.jsonl`이 있어야 한다.

사용:
```bash
python Tools/audit_social_family_health.py path/to/longrun-audit-4242001.zip path/to/longrun-audit-874213954.zip --output family-health.json
python -m unittest discover -s Tools/tests -p test_social_family_health_audit.py -v
```

동작 변경 후보에 대한 병합 판정은 기존 main과 같은 seed/시간 범위의 전체 감사 비교,
실제 출산자 생존 및 인과 사건, 결정론 테스트까지 필요하다.
2026-10-12에 발견된 구체 사례는 [결합 감사 PR #652](https://github.com/sjLim91/lifelens-ue/pull/652) 및
[Issue #653](https://github.com/sjLim91/lifelens-ue/issues/653)·
[Issue #654](https://github.com/sjLim91/lifelens-ue/issues/654) 참고.
