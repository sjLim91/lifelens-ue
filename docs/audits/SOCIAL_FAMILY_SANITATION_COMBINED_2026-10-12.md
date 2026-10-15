# 사회·가족·위생 결합 1000일 인과 감사 — 2026-10-12

> **최종 판정: FAIL, main 병합 금지.** Core/Preflight/WASM/Weather/1000일 감사 워크플로 성공은 제품의 인구·질병 품질 통과를 뜻하지 않는다.

## 근거와 재현
- 통합 Draft [PR #652](https://github.com/sjLim91/lifelens-ue/pull/652), 감사 head `c4ca4a4ee5f34f5bbb20b6527deda007fd136f8c`.
- 기준 main `19aa2ad371767779bddcb25b8ee5fc886423d0d9` (#646).
- 결합: 미병합 #648 off-cadence local first-contact + #651 아이 ToiletAssist 실제 위생시설 이동, #651 테스트 include 수정 반영.
- [Actions 38188360468](https://github.com/sjLim91/lifelens-ue/actions/runs/38188360468) — seed별 artifact `longrun-audit-4242001`, `longrun-audit-874213954`. 두 seed 완료, 대표 seed 결정론 재실행 및 evidence_validation passed; 별도 exact-head Core/Preflight/Web WASM/Weather 모두 green.
- 근거는 1000일 `summary.json`, `metrics.jsonl`, `events.jsonl`과 `ContextActionRuntime.cpp`, `Simulation.cpp`를 대조한 것. Actions artifact 30일 만료 전 이 보고서를 영구 보존한다.

## 수치 비교

| Seed | main #646 1000일 | 결합 #652 1000일 | 판정 |
|---|---|---|---|
| 874213954 | 생존 3/4, 출산0, social0 | 생존 **1/4**, 출산0, illness 사망3 | 심각한 초기 성인 생존 회귀 |
| 4242001 | 생존 3/4, 출산0, social0 | 성인4명 생존, 총 8명 출생 포함, **영아4명 모두 사망** | 사회·혼인 활성화, 다음 세대 생존 실패 |

#652 checkpoint:
- seed874: day1 4/4, day7 4/4, day30 3/4, day100 2/4, day365 1/4, day1000 1/4. day1000 married0, births0, facilities13, chunks14, activeSettlements0.
- seed424: day1 4/4, day7 4/4, day30 4/4, day100 4/4, day365 4/4(혼인2/임신2), day1000 4/8(출산4/사망4, 혼인2, 시설21, chunks5).

### 874: 성인 질병 사망 (elapsed day)
1. Hayun(id4): day21.17, illness.
2. Taeyun(id1): day30.17, illness.
3. Siwoo(id2): day107.17, illness.

현재 결합 후보에서 infant/ToiletAssist가 없는 상태로 early illness collapse가 발생했다. #651 child sanitation 이동만 수정해선 seed874 문제가 해결되지 않으며, #648 사회 활성화 후 생존·시설·물 위험 경쟁을 별도 분석해야 한다.
성인 1/2/4 early health_transition에는 반복되는 `pending_water_dose_before_tick`와 pathogen 증가가 보인다. 직접 수원·오염 잔여물·물리 행동 시간 예산의 분해가 필요하다.
seed874 day30 social active 약12.21%, civilization 약12.85% vs 원래 main day30 social0%, civilization 약36.53%. 이는 연관이지, 사회활동 단일 원인의 증명은 아니다.

### 424: 영아 4명 사망

| 자녀 | 출생 elapsed day | 사망 elapsed day | 사망 원인 | 일일 local residue exposure 평균 | 시설 ToiletAssist 횟수 |
|---|---:|---:|---|---:|---:|
| Minho(id5) | 494.17 | 508.17 | illness | 0.985785 | 41 |
| Minho6(id6) | 499.17 | 501.17 | severe deprivation | 0.995800 | 5 |
| Jaeyun(id7) | 835.17 | 854.17 | illness | 0.987222 | 55 |
| Seungmin(id8) | 837.17 | 879.17 | illness | 0.983224 | 123 |

영아 4명 중 생존 0. 아이들에게는 실제 Feed·Sleep·Bathe·ToiletAssist가 반복됐으며, 보육 스케줄이 아예 비어 있었던 것은 아니다.
영아 id5/7/8은 첫 관측일부터 `local_residue_exposure_after_tick` 0.95~1.0 전후의 지속 고노출이 관찰되고 `pending_water_dose_before_tick=0`인 날에도 pathogen이 증가했다. 특히:
- id5 day495.17 exposure .985, pathogen 0→.121.
- id7 day836.17 exposure .991, pathogen 0→.121.
- id8 day838.17 exposure .971, pathogen 0→.113.
- 시간 흐름에 따라 id5/7/8 pathogen≈1, illness≈1 수준이 지속됨.

한편 `during assisted toilet care via sanitation site` 이벤트가 총 **224회**이고, 이 이벤트들은 같은 환경 잔여물 record id96으로 병합된다. seed424의 지정 sanitation site useCount는 day495에 7154, 주변 HumanWaste 잔여량은 관측 상한에 가까운 약100이다.

## 코드 구조에서 확인되는 위험

현재 #651의 `Simulation::advancePendingContext`는 아이를 픽업해 실제 site까지 escort하고, `ContextActionRuntime.cpp`의 ToiletAssist 완료는 아이의 위치를 `resolvedPosition`(해당 site)에 둔 후 action을 clear한다. **독립적인 실제 return leg가 없다.** 이후 다른 보육행동이 아이를 주 생활 위치로 자동 복귀시킨다는 계약도 이 흐름에서는 확인되지 않는다.

따라서 지독하게 오염된 sanitation site에 아이가 체류하면서 local exposure가 높아지는 **강한 인과 후보**가 있다. 다만 이 보고서의 숫자는 daily Health 관측치이며 site-vs-home 좌표와 분리된 dose 상세까지 증명하지는 않는다. 현 단계에서 "100% site가 원인"이라고 단정하거나 pathogen 숫자를 내려 통과시켜서는 안 된다.

## 다음 독립 수정

- [Issue #653](https://github.com/sjLim91/lifelens-ue/issues/653): 영아 안전복귀·위생시설 체류 정책/물리 이동/저장·재생 계약 구현. nursery source 분해.
- [Issue #654](https://github.com/sjLim91/lifelens-ue/issues/654): seed874 day1–30 사회 첫 접촉·정착지 시설·직접 자연수 pathogen 연쇄를 분리하여 분석.
- 양쪽 수정은 제품 동작을 변경하는 독립 PR로 구현하고, 기존 main 대비 same seed 100/365/1000일 생존/가족/교역 회귀를 필수로 점검.
- 출생 다음 세대 성인 도달까지 6570일이 필요하므로 1000일만으로 다음 세대 재생산이 검증됐다고 표현하지 않는다.

**판정: #652 Draft 유지. #651 단독도 장기 결합 성공 증거 없음.** main merge, auto-merge, 제품 배포 모두 금지.
