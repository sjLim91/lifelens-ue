# 전 시스템 장기 감사 완료/재개 인수인계 — 2026-10-06

- main 기준 67fbf1e2c133499b2271f885ef066df30efdf665, branch audit/system-balance-causal-20261006, PR #646.
- clone 없이 GitHub connector 사용. WORK_STATE/TEAM_BOARD 변경 없음. 신규 #645는 SocialUtility/tests만 변경, 파일 overlap 없음; 해당 미병합 수정은 baseline에서 제외.
- Core simulation truth/ruleset/확률/utility/비용/속도/행동시간/save schema 변경 없음.

## 완료

- 기존 ll_balance_audit 확장 + observer header + Python run/resume/summarize/compare + JSON/CSV/MD + 기존 workflow 확장.
- native seed874213954/4242001 각각1000일, 대표874 재실행1000일 완료. checkpoint1/7/30/100/365/1000.
- runtime: 1090.245초 /1814.483초 /1132.424초. 전체 parallel wall2236.502초, run 합4037.152초.
- full streamed metrics/events identity PASS(wall/RSS 제외); 6개 save SHA256 byte identity PASS.
- 원본 vs 확장874 day1/7/30/100/365 save identity PASS.
- 기존 native 결정론/snapshot/trade/generation continuity 테스트4개 PASS. Python invariant7개 PASS. Core CMake CTest107/107 PASS; reporting 수정94eec882 Core push PASS, docs3e61fbe PR Core/Web WASM PASS.
- reporting observer7일 대표2회 identity PASS, snapshot day100에서7일 continuation pressure entry=sum resident/max streak 불변식 PASS.
- report docs/audits/LONG_RUN_SYSTEM_BALANCE_AUDIT_2026-10-06.md; checkpoint JSON/CSV, anomaly11필드 JSON 별첨.
- P0 없음; P1 사회/가족 starvation, Yerin illness death, Soyeon 급수 이동 deprivation death. 인구 전체 collapse 미관찰.

## Provenance / 오해 방지

- native1000 evidence 구현 afd922ef264ee0c033ffb4ae13f26e59e481e8cb, 바이너리 build/ll_balance_audit_final SHA256 b8f7607fb1d8ab74989997f7140761b8149624c1730701e6a2bc5692210aa6cf.
- reporting 수정94eec8827e906b5ad55d2268dc57df1c39bd6ce7: pooled critical entries/streak를 resident sum/max로 보정, failure timestamp 제거, legacy action starts/completion ratio JSON. 해당 source의 GitHub Actions full1000 별도 실행 중.
- native raw pooled temporal counters의 mean/histogram/critical minutes는 정상; entries/streak는 latest summary/checkpoint JSON에서 주민별값으로 재구성. raw 파일을 수정하지 않음.
- actor prefix prototype와 completed presentation token0 overcount prototype 폐기; results/invalid-prefix-prototype/results/presentation-cache-prototype은 최종 evidence가 아님.
- 1000일은 출생자 Adult(6570일 후)/다음 세대 reproduction을 검증하지 못함. 출산0은 자녀 성장 실패 증거가 아님.
- global natural quantity는 reachable supply 아님. zero raw water stock은 direct natural water 사용과 다름.
- FirePit/Furnace/CultivatedPlot usage0은 미사용 증거 아님. knowledge subsequent-use300일 경고는 생존 주민만 해당하며 passive knowledge 효과와 구분.
- Soyeon은 Water 탐험 완료 후 Drink 시작, 360분 이동 중 noon thirst1로 사망. 탐험 중 사망이라고 표현하지 않음.
- pure planning probes는 checkpoint 후보 조회이고 실제 scheduler boundary trace 아님; 전후 snapshot bytes 동일.

## 재개/재현

```sh
cmake -S Source/LifeLensCore -B build/core -DCMAKE_BUILD_TYPE=Release
cmake --build build/core --target ll_balance_audit -j2
python -m unittest discover -s Tools/tests
python Tools/audit_system_balance.py run --executable build/core/ll_balance_audit --output longrun-results --seeds 874213954,4242001 --days 1000 --checkpoints 1,7,30,100,365,1000 --workers 2 --determinism-seed 874213954 --resume
```

- /workspace 기존 결과: results/full/{874213954,4242001,874213954-repeat}, driver log longrun-final-driver.log, per-run run.json.
- 기존 native 바이너리로 재개할 때 executable을 build/ll_balance_audit_final로, output을 results/full로 치환하면 완료 실행 hash 일치 시 skip.
- latest reporting source로 새로 빌드하면 binary identity가 달라져 full run 재실행하는 것이 정상.
- 중단 실행은 snapshot에 metric accumulators가 없으므로 처음부터 재실행. source/binary/seed/days/checkpoint 동일 여부 먼저 확인.
- raw metrics/events/long-format CSV/snapshots는 workflow artifact(30일)에 보관. latest reporting full run37403243556에서 두 seed 및 대표repeat artifact 확인.
- 보고서 자료는 저장소에 보존되어 작업환경이 사라져도 GitHub에서 복원 가능. 완료 run JSON은 evidence SHA256도 보고서 JSON에 보존.

## 남은 확인 / 다음 lane

- GitHub longrun workflow37403243556 전체 완료/업로드 상태는 PR Checks에서 확인. native1000 완료와 혼동하지 않는다.
- 최종 docs-only commit의 CI 상태 확인; 과거 PR Core reuse wait timeout은 같은SHA push CTest 성공 뒤 API polling race였음. workflow 임의 수정하지 않음.
- 다음 수정 TOP5: social bootstrap(#645와 별도 감사), 급수 이동, 오염/회복 경계, Tin 접근/채집/smelting 연결, 시설 생활권 수요.
- 모든 튜닝/치료는 별도 PR. main merge/auto-merge/deploy 금지.
