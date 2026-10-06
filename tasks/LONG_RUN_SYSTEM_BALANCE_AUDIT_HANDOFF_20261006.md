# 전 시스템 장기 감사 인수인계 — 2026-10-06

- 저장소 sjLim91/lifelens-ue, main 67fbf1e2c133499b2271f885ef066df30efdf665.
- 브랜치 audit/system-balance-causal-20261006, Draft PR #646.
- 클론 없이 GitHub connector로 파일 확인/브랜치/커밋/PR 생성. WORK_STATE/TEAM_BOARD 수정 없음.
- Core truth, ruleset, deterministic contract, snapshot schema 변경 없음.
- 기존 balance_audit.cpp 확장 + system_balance_audit.h observer + Python 재실행/JSON/CSV/결정론 driver.
- 로컬 g++ Release 빌드, 기존 결정론/snapshot/trade/generation 테스트와 신규 7개 invariant 테스트 통과.
- 원본 하네스와 확장 하네스 seed874 day1/7/30 save bytes 동일. day100/365까지 snapshot bytes 동일 확인 완료.
- 최종 native 실행: results/full/874213954 완료1000일(1090.25초); 4242001 진행; 874213954-repeat 진행.
- checkpoint 1,7,30,100,365,1000; 최종 바이너리 SHA256 b8f7607fb1d8ab74989997f7140761b8149624c1730701e6a2bc5692210aa6cf.
- 진행 로그 longrun-final-driver.log, 실행 manifest 각 results/full/<seed>/run.json.
- 재개: python Tools/audit_system_balance.py run --executable build/ll_balance_audit_final --output results/full --seeds 874213954,4242001 --days 1000 --checkpoints 1,7,30,100,365,1000 --workers 2 --determinism-seed 874213954 --resume
- 중단 실행은 감사 누적량이 snapshot에 없으므로 처음부터 재실행. 완료 실행은 identity/hash 일치 시 건너뜀.
- 다른 환경에서는 CMake Release 기존 ll_balance_audit 빌드 후 executable 경로 치환. GitHub Actions current-main-longrun-audit가 raw evidence/snapshot artifact 보관.
- 최종 문서 docs/audits/LONG_RUN_SYSTEM_BALANCE_AUDIT_2026-10-06.md 작성 예정.

## 이미 확인한 인과 경로/주의

- Social Approach 후보 최대 약 .17, minSocialUtility .18 미달. survival 외 civilization 경쟁도 존재. 임계값 변경 금지.
- 자연수 Drink 오염 dose → noon pathogen 축적 → illness .28 경계 이상 유지 → capacity 감소 → 이동지연. Yerin day145.1667 illness death. 회복 불가 원인을 단순 사망 label로 끝내지 않는다.
- 1000일은 출생자 성인(18*365=6570일) 검증 불가능. 세대교체 미관찰을 실패로 단정하지 않는다.
- Completed civilization presentation token0을 활동/탐험으로 세던 prototype 폐기. final은 active commitment만 관찰.
- 이벤트 [Day N HH:MM] prefix 제거 후 actor 매핑. 이전 prefix prototype 사용 금지.
- 시설 usageCount가 FirePit/Furnace/CultivatedPlot에서 갱신되지 않으므로 zero를 미사용으로 판정하지 않는다.
- 자연자원은 materialized 전체량이며 접근가능량 아님. gross 소비/생산/spoilage/trade 흐름은 unavailable로 보고.
- Core PR CI reuse wait 타임아웃은 같은 SHA push CTest PASS 후 failed job 재실행 요청됨. Core workflow 수정하지 않음.

## 남은 작업

1. 두 seed1000일 + 대표 seed 재실행 완료, byte/event/checkpoint identity 및 evidence validation.
2. 모든 체크포인트 pure planning probe(encode before/after identity), 원본 baseline save 비교.
3. JSON/CSV/Markdown 요약, anomaly 11필드 causal report, A–M 관측 제한과 수정 우선순위.
4. 결과와 최종 인수인계 커밋, PR 설명 갱신/검토가능 전환. main merge/auto-merge/deploy 금지.

## 보고 보정 / 외부 lane

- reporting HEAD 94eec8827e906b5ad55d2268dc57df1c39bd6ce7. pooled Needs critical 진입/streak를 주민별 sum/max로 보정; 이전 raw record에서 mean/histogram/minutes는 정상, entries/streak만 summary에서 재구성한다.
- final reporting binary build/ll_balance_audit_reporting: 7일 대표 seed2회 metric/event/snapshot 동일. native1000 evidence binary는 기존 final(b8f7607...). Core objects/계약은 동일.
- source commit afd922e에서 수행한 Core CMake CTest107/107 통과. 최신 reporting code CI 별도 진행.
- #645가 새로 열림: SocialUtility.h와 테스트3개 변경, audit files overlap 없음. 미병합 patch를 baseline 결과에 포함하지 않음.
- 로컬 최종 보고서 생성기 /tmp/build_audit_report.py, docs/audits JSON/CSV/MD draft 생성됨. 424/repeat 완료 후 latest driver summarize 다시 실행하고 생성기를 재실행해야 함. 현재 draft는 424 day365까지만이므로 final로 발표 금지.
- results/full 전체 run process 유지 중; session11651, longrun-final-driver.log.
