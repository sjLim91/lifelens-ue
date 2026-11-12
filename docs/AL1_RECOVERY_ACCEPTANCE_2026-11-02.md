# AL-1 recovery and local inference checkpoint — updated 2026-11-10

## 2026-11-12 적용 범위 변경 및 최종 인수 우선 기록

- 사용자가 **모든 실기기 검증을 이번 인수에서 제외**하도록 지시했다. 이 조건에서 **AL-1 한 주민의 무개입 Core-native 실제 목표→물리 행동→직접 기억→후속 선택 및 이진 저장 재생은 PASS**다. G1/AL-2~4, 실기기 Qwen 성능·브라우저 자동 모델 실측은 PASS가 아니다.
- 실제 제품 추가 테스트 [#1008](https://github.com/sjLim91/lifelens-ue/pull/1008) main `6afbb592a821b611e8f737c586f531dde3bdc175`, exact-head [Core CI #43164964474](https://github.com/sjLim91/lifelens-ue/actions/runs/43164964474): Core **175/175**, 자연 학습·바이너리 저장 재생 두 시드 및 결정론 PASS. Web WASM #43164964523, Preflight #43164964482, Web Checks #43164964475 SUCCESS(파일 범위상 Web-only job은 skipped).
- 두 증거: 시드 874213954 / 주민 2 / 학습 토큰 70 / 1590→1636분 / 목재 5개, 시드 4242001 / 주민 2 / 토큰 669 / 18900→18990분 / 식량 4개. 두 경우 `binary_roundtrip=1 identical_production_replay=1 actual_learned_harvest=1 firsthand_result_memory=1`.
- [AL-1 최종 범위 인수 기록](AL1_CORE_ACCEPTANCE_2026-11-12.md)을 우선한다. 아래 2026-11-10 `NOT_PROVEN`은 당시 실기기/확대 G1 조건을 포함하던 기록이며, 변경된 AL-1 Core-native 인수 결론을 부정하지 않는다. **86개 계획 단위의 기존 완료 수 6/86은 그대로 유지한다.**


Base: `baa8497fd75f959966a5123e2c535c24eb1c700d` (#996).
AL-1 remains **NOT_PROVEN**; masterplan completion remains **6/86**. No M0 or acceptance thresholds changed.

Current product source: local commit `4de5ebf5b96d97e29c9ef36941bd2251f65357f7`, identical tree to remote `cfafa88691824067ecca6e1b612c7e10120cf149` in [PR #997](https://github.com/sjLim91/lifelens-ue/pull/997). The 2026-11-10 final-source checks below supersede earlier working-tree checks. The natural-run table retains the explicitly identified earlier measurements while a final-source three-seed rerun is recorded separately; it is not silently relabelled as final-head evidence. The original #996 record is [AL1_CAUSAL_ACCEPTANCE_2026-10-29.md](AL1_CAUSAL_ACCEPTANCE_2026-10-29.md).
## Product changes

- Existing single Core Gather decision remembers bounded personally sensed positions, observed stock, time and confidence (radius 6 / four facts; twelve memories; existing 1440-minute evidence horizon). Recall never queries remote stock. A known location rechecked nearby can refute stale stock; a stale remote destination is physically visited before loss becomes an execution failure.
- Recovery candidates use known positions, the same Core access resolver and ground routes, weather/health movement intervals, the original deadline, observed quantity and projected survival pressure. New tokens preserve the original goal and three-attempt limit. No new executor, free resources, seed-specific branches or wider perception.
- Structured personal assessment distinguishes source loss, access, route failure, deadline, witnessed local competition and survival interruption. Predictions are separate from actual extraction outcomes. Remembered competition affects subsequent replanning; expired/interrupted tasks do not become successful harvest memories.
- Competition-only forecast switching requires an alternative with enough observed stock for the remaining goal; the precheck and final selection use the same minimum. Deadline forecasts and actual failure recovery still permit honest partial progress. A controlled executor test covers full-stock selection, retaining the original source when no adequate alternative exists, and partial recovery after actual exhaustion. These fixtures are not natural acceptance evidence.
- Forecast switching requires a feasible known alternative. Without one, the current commitment remains until its original deadline; a current-weather estimate does not guarantee the future.
- The Web bridge now accepts RepeatedFailure only with canonical same-resident recent repeated Core outcome evidence. This fixes a rejected production request type, not proof of model inference.
- The browser selects the same Qwen3-1.7B model with q4f16 arithmetic when `shader-f16` is available and q4f32 otherwise, and reports the actual loaded model ID. Model artifacts use WebLLM IndexedDB caching. This hardware compatibility change is not an inference-success claim.
- A timed-out or disposed browser inference now requests WebLLM interruption. The provider keeps its one engine slot occupied until the actual GPU call settles, so later residents cannot queue hidden work behind a cancelled prefill. Late replies remain invalid and completed requests remove their abort listeners. A submitted GPU dispatch itself cannot be synchronously cancelled; these guards do not turn a slow or failed inference into success.
- The WASM build explicitly accepts `wasmBinary` in Emscripten's incoming module API. The actual generated factory now consumes the Web bridge's supplied bytes instead of fetching another binary; a smoke test rejects every fetch and still creates four residents and advances one Core minute.

## Actual untouched New Game runs

All runs call setupNewGame and step only. No goal, need, source, position or failure injection.

| Seed | Days | Actual failures | Alternative tokens | Same-goal physical completions | Goals achieved | Forecast switches |
|---|---:|---:|---:|---:|---:|---:|
| 874213954 | 365 | 16 | 0 | 0 | 0 | 105 |
| 4242001 | 365 | 6 | 4 | 4 | 3 | 215 |
| 874213955 | 365 | 50 | 6 | 6 | 1 | 290 |

Forecast switches are separately emitted `AL1_CORE_RESOURCE_FORECAST_REPLAN` decisions; they are not actual failed extractions and are excluded from the ten completed natural failure-recovery chains. A physical completion here requires the issued token and original goal link, actual Core Gather activity, inventory gain, source ledger decrease, arrival at the target and a direct personal success receipt. Six of those ten harvests still leave the original goal unmet. The audit's `CHAIN_OBSERVED` summary is not an AL-1 completion verdict.

Concrete achieved witness: seed 874213955, resident 3, goal/token 2360, PlantFood target 8. Source 5223687182841066954 actually exhausted. Token 2361 selected known source 6835049934241821604 within the original deadline 89865. Completion minute 89854, position (-621,1099) -> (-618,1096), personal inventory 1 -> 8, source 94 -> 87. Both direct failure and later direct success memories exist. Other 0 -> 7 examples are partial progress, never goal achievement.

The actual records for that witness include:

```text
seed=874213955 actor=3 goal_id=2360 failed_token=2360 failure_minute=89842
material=PlantFood target_quantity=8 inventory=1 failed_source=5223687182841066954 cause=source-exhausted
candidate=6835049934241821604 observed_minute=89842 remembered=0 confidence=0.6 observed_quantity=94 route_steps=6 travel_minutes=12 remaining_minutes=23 reachable=1 feasible=1 reason=known-stock-within-budget
candidate=7676508658207762473 observed_minute=89700 remembered=1 confidence=0.329295 observed_quantity=93 route_steps=34 travel_minutes=68 remaining_minutes=23 reachable=1 feasible=0 reason=deadline-insufficient
reissued_token=2361 goal_id=2360 expires_minute=89865
result_minute=89854 quantity=7 inventory=1->8 source_stock=94->87 position=-621,1099->-618,1096 goal_achieved=1 direct_success_memory=1
```

The selected source was currently sensed in this witness. The separate seed 4242001 recovery at minute 228122 also completes physically: resident 4, goal 10012, failed token 10012 -> token 10014, target 1, source 6170518871204368220 -> 6539486212521894887; completion minute 228170, inventory 0 -> 2 and source 97 -> 95. The existing tool/batch economics can exceed the one-unit survival predicate; the predicate is not changed after the result.

Seed 874213954 still has no natural completed recovery. Six of its sixteen failed tokens have seven product candidate records, all rejected as `deadline-insufficient`. For example, resident 4 at minute 58874 remembers source 9091508475583309012 from minute 58395, but its actual 44-step route costs an estimated 88 minutes with only 61 minutes left. Earlier diagnostic `remembered-proximity` entries can be older than the eligible 1440-minute planning horizon; their `audit_world_stock` is test-side ground truth and is never given to the resident. Diagnostic knowledge is not automatically a valid product candidate. The remaining failure records do not prove that an eligible alternative existed. No universal recovery claim is made from the two seeds with completed chains.

## 365-day survival and regression observations

The three independent `balance-<seed>.log` files all reach `AUDIT_COMPLETE days=365`. Their day-365 checkpoints are:

| Seed | Living / initial residents | Deaths | Longest continuous sleep saturation (minutes) | Total sleep saturation (resident-minutes) |
|---|---:|---|---:|---:|
| 874213954 | 3 / 4 | Resident 4; one illness death | 270 | 11738 |
| 4242001 | 4 / 4 | 0 | 73 | 152 |
| 874213955 | 4 / 4 | 0 | 309 | 5340 |

These runs completed their observation period; they do not show that all four founders survived in every seed. The exact #996 baseline was independently rerun at `baa8497fd75f959966a5123e2c535c24eb1c700d`: `artifacts/al1-20261109/baseline-996-balance-874213954.log` reaches 365 days with four living founders and zero deaths. The candidate therefore does not preserve that particular survival outcome. `longrun-metrics.json` contains only 874213954 and 874213955 and compares against older baseline `66f623db9413ff626ec03dee433cec482c4e8efc`, not #996; it cannot establish a three-seed comparison to the requested base. The 4242001 figures above were read from its completed raw balance log.

The candidate read-only health trace (`artifacts/al1-20261109/candidate-health-874213954.log`) records resident 4 becoming ill at minute 334800 and dying from illness at minute 447120. At the last daily checkpoint before death (446399), pathogen load was 0.9352, illness severity 1.0 and environmental stress 0.6290; hunger 0.5639, thirst 0.3305 and sleep pressure 0.5289 were below saturation. Repeated positive water-contamination doses are present, including 0.0352 at that checkpoint. `Health.h` is unchanged. These observations identify the illness trajectory; they do not isolate which changed planning decision caused different exposure or establish absence of a regression. The original masterplan explicitly permits deaths and population loss rather than guaranteeing survival, so no needs, mortality or acceptance threshold is relaxed to force four survivors. A final-source rerun and causal diagnosis remain distinct from this historical comparison.

## Single failure-memory ablation

`test_al1_natural_failure_memory_ablation` captures the genuine failed-execution callback boundary from untouched seed 874213955. It removes exactly one canonical direct failure memory; restoring that entry reproduces identical encoded snapshot bytes. World, RNG, goals, positions, beliefs and other memories stay identical. At the first resumed step, the retained branch issues recovery token 2361 while the ablated branch does not choose that recovery source. The retained branch physically gathers 7 units (inventory 1 -> 8, source 94 -> 87), moves, and records a direct success memory.

This snapshot is taken **inside** `step()`, after the failed execution writes its outcome but before pending cleanup/recovery. Restoring it and calling `step()` re-enters the whole tick; the already-updated belief suppresses a duplicate same-minute outcome receipt. Both experimental branches have that same replay behavior. The result demonstrates the failure receipt's causal role at this execution boundary. It does **not** prove normal end-of-tick save/reload continuation, a later independent planning decision after the failure, or both branches' eventual physical outcomes: the test advances the control branch but only asserts its initial different choice and the retained branch's completion. Those distinctions remain acceptance limits.

## Success-memory ablation: 2026-11-09 superseding result

The 2026-11-02 full-suite run was 174/175: the 4242001 ablated branch issued a different source but its physical completion was not recognized. The observer tracked only the first token, although the production forecast planner could issue a successor token for the same goal. The observer now follows only actual Core-issued successors with the same goal ID, material, quantity, creation time and original expiry, a recorded previous token and an incremented attempt. It still requires real movement, inventory/source changes, a direct outcome receipt and distinct final sources in the two branches. No product deadline, stock or success predicate was relaxed.

The 2026-11-09 rerun records both physical branches:

| Seed / resident | Retained-memory result | One-memory-removed result |
|---|---|---|
| 874213954 / 2 | Token 70; source 8371056191655438253; +5 at minute 1636 | Token 70; source 4942088878575105140; +5 at minute 1638 |
| 4242001 / 2 | Token 669; source 7682238751532634410; +4 at minute 18990 | Initial token 669, actual same-goal successor 670; final source 6539486212521894887; +4 at minute 18944 |

In each run exactly one actual direct success memory is removed from an otherwise identical snapshot. This proves the recorded success-memory causal choice and both physical results; it does not extend the narrower failure-memory experiment into proof of later independent failure learning.

## Actual Qwen inference and final-Core physical contrast — 2026-11-10

[The committed evidence receipt](evidence/AL1_QWEN_CORE_2026-11-10.json) contains the actual model input/output, verified official model and llama.cpp hashes, final WASM hashes, snapshot hash, issued token, ledger deltas and direct memory. This is a local **CPU verification path**, using the existing LocalCognitionProvider and real WASM Core; production still automatically uses the one Qwen WebGPU model. No native server discovery, alternative model or second resident controller was added.

The first genuine model response chose `AcquireMaterials` with `targetResident="3"`. The old generation schema allowed this pair while the independent Core-facing parser correctly rejected it. Generation now couples personal/resource intents to null and social intents to a known other resident. Validation remains strict; generated outputs are never rewritten to pass. The prompt states the same target rule.

After that correction, actual official Qwen3-1.7B-Q8_0 inference for seed `874213954`, resident `2`, minute `480` produced `AcquireMaterials`, priority `1`, null target. Core accepted it. The exact recorded input and unmodified output were replayed against the final built WASM and compared with a byte-identical pre-submission snapshot without that proposal:

```text
540: before both positions=(980,-1142), Fiber inventory=0, source stock=98
540: proposal branch issues token=11 Gather Fiber source=5774862701740937008
540: control branch issues token=11 Experiment PrimitiveStorage
554: proposal branch arrives=(973,-1142), actual Fiber inventory 0->3, source 98->95
554: DirectWitness action_experience Civilization:Gather:Fiber success recorded
555: control remains=(980,-1142), Fiber inventory=0, same source stock=98
```

The proposal changed an actual action and physical outcome. It did not itself name a resource node or manufacture stock. Its rationale names Flint, whereas the typed AcquireMaterials strategy lets Core select Fiber; the rationale is not proof of resource necessity or a completed food goal. An earlier resident-1 ExploreOpportunity proposal was accepted but produced no physical difference during its first 240 minutes, and is not counted as causal execution evidence.

The resident-2 CPU inference took **66.15 seconds** under concurrent builds (resident-1 valid inference took 32.05 seconds). This direct verification held simulation time fixed and exceeded the normal 6-second native scheduler budget and 24-second browser budget. It does **not** establish ordinary automatic browser inference. Actual browser q4f32 weights loaded through verified local asset mirrors on software WebGPU, but a valid browser-generated response has not been captured. Browser inference, latency and uninterrupted automatic operation remain acceptance blockers. The fp32 variant is the same Qwen3-1.7B architecture/weights with hardware-compatible arithmetic, not a second model policy or evidence that every device can execute it.

## Saved validation checkpoint

Latest observed local checks supersede the earlier failed test result, without certifying future source changes or remote CI:

| Evidence | Observed result | Scope |
|---|---|---|
| `artifacts/al1-20261110/core-ctest-final.log` | **175/175 pass**, 128.54 seconds | Final product source above, including controlled competition recovery |
| `artifacts/al1-20261110/wasm-bytes-final.log` | **PASS**, real Core, 4 residents, +1 minute, no fetch | Final rebuilt WASM |
| `artifacts/al1-20261110/qwen-final-replay.log` | **PASS**, unmodified real Qwen output causes physical contrast | Final Core replay; not another inference or browser certification |
| `artifacts/al1-20261110/typecheck-final.log`, `build-final.log`, `cognition-final.log` | **PASS** | Final schema, cancellation and Web bundle |
| `artifacts/al1-20261110/web-regressions.log`, `preflight.log` | **PASS** | Full Web runner and 32 Preflight commands; schema-specific checks rerun above |
| `artifacts/al1-20261109/core-ctest.log` | **175/175 pass**, 269.35 seconds | Full native Core suite, including the repaired same-goal success-memory observer |
| `artifacts/al1-20261109/success-memory-ablation.log` | Both branches physically complete in both seeds | Exact witnesses above |
| `artifacts/al1-20261109/wasm-bytes-smoke.log` | **PASS**, population 4, elapsed minutes 1, fetches 0 | Actual generated WASM Core; supplied-byte startup and four-resident New Game |
| `artifacts/al1-20261101/web-final-typecheck.log`, 2026-11-09 | `tsc --noEmit` passes | Local Web typecheck |
| `artifacts/al1-20261101/web-final-build.log`, 2026-11-09 | Vite build completes in 15.61 seconds | Local production bundle; existing CSS/chunk warnings remain |
| `artifacts/al1-20261101/web-final-regressions.log`, 2026-11-09 | Regression runner checks pass | Local Web contracts; not real model inference |
| `artifacts/al1-20261101/preflight.log`, 2026-11-02 08:45 UTC | Recorded validators end in PASS | Earlier candidate preflight; final revision verification remains separate |

Historical failures remain in their original logs: `artifacts/al1-20261101/natural-goal-causality.log` failed on 2026-11-01; later targeted and full-suite runs supersede it. `artifacts/al1-20261101/core-ctest.log` was 174/175 on 2026-11-02 and is superseded by the 175/175 run above. PR #997 is open; final-head CI, merge, Pages release, runtime commit match and fresh deployed browser checks must be read from their actual executions. Local checks alone do not certify them.

Log identities (SHA-256; paths are relative to the local repository):

```text
artifacts/al1-20261101/natural-874213954.log 2c7537450ce33f48332671653a721c80e47042eed6a97b58b12f5a47919403cc
artifacts/al1-20261101/natural-4242001.log a6ad9b9300a761b929b1e0afbdf21a9e3d25ad8c6edc1008e2fb957ea15bd78a
artifacts/al1-20261101/natural-874213955.log 53e2a01dfdc21b2d795d65f6c29c087e136b9c13f31e06454d94f0f9b58c04df
artifacts/al1-20261101/failure-memory-ablation.log 3ed683b11f5d88a287a86cde2a6bc3372f2123381bfd4248d4439f57a4df282e
artifacts/al1-20261101/core-ctest.log 1622973720598d3047641bebc9da4f4abe2cccf143746dc415513ed35e3bf707
artifacts/al1-20261109/core-ctest.log ab0452f0a9aa1df0b7577824464be2c4e1ebd7fee87347e279337bccb1b7f078
artifacts/al1-20261109/success-memory-ablation.log 2d7ac12b7744cfd42fbcf5e35a658b2df0868f5a883da9d5b4579dd96b13a333
artifacts/al1-20261109/wasm-bytes-smoke.log fb32ac3752be791df28ce0d7776795dedf4228fa6b002bb00ce88e5f2bd068ca
artifacts/al1-20261101/web-final-typecheck.log d630f82a35178b039c87c0ce6c0e746d699b7294e9fa4883e3d6b95509598430
artifacts/al1-20261101/web-final-build.log 0a1505eccc8908c92583782fff1f0b87a9cec178f9c287a8b121b8d30db23c7a
artifacts/al1-20261101/web-final-regressions.log 94349d982b934e8ce09b5414c98416236eaea7557050bdc82dd62e68d72a1c6a
```

## Remaining acceptance limits

- Actual Qwen CPU inference, final Core acceptance and a causal physical contrast are demonstrated above. Normal automatic browser WebGPU inference and performance remain unproven; CPU direct invocation does not close that acceptance requirement. No paid API or hosted inference was introduced.
- Survival interruption is recorded and releases the goal; automatic resumption of interrupted tasks is not implemented by this change.
- Partial successful extraction can still end with a goal unmet. No deadline, batch, stock or recovery-attempt limit is increased to hide that result.
- The 4242001 success-memory observer failure is resolved by the identified rerun above. The exact #996 rerun confirms four survivors versus three in the candidate. The recorded candidate death is illness after sustained pathogen exposure; the planning-to-exposure causal difference remains unresolved, so no regression-free survival claim is made.
- Final-source Core/Web/WASM checks and updated Preflight are recorded above. Final-source long-run verification, final-head remote CI, merge and deployment are separate pending integration stages at this checkpoint. This checkpoint does not claim those later stages. AL-1 remains **NOT_PROVEN**, with the original **6/86** completion count and all 86 development units preserved.
