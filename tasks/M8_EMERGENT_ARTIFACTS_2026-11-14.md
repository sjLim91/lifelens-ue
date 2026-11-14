# M8 — 구조·물성 기반 발명과 발생 문명

사용자 요청: M8-01~08 전체 구현·검증·통합. 필요한 관련 작업은 사전 승인됨.
시작 기준: main `f3c7e4f` (#1029). 과거 #968은 미병합·충돌 상태이므로 합치지 않고 최신 Core 권위 위에 구현한다.

현재 상태: **M8-01~08 전체 8/8 완료**. [PR #1030](https://github.com/sjLim91/lifelens-ue/pull/1030) → main `4640a36cacffb93a71c39de4f0ba17a6434f62d0`. 최종 코드 head `8dcdcbdb1ff0a99085913c804855fd7f4509eba1`의 필수 CI 9/9와 Runtime Release·Pages·External Probe 및 공개 바이트/브라우저 검증을 통과했다. [기계 판독 배포 증거](evidence/m8-2026-11-14/release-proof.json).

## 구현과 인수 경로

| 단위 | 실제 제품 경로 | 검증 |
|---|---|---|
| M8-01 | 재료 물성 → 1~4개 부품의 막대/날/끝/용기/판/끈 + 크기/일체/묶음/끼움 → 베기/파기/운반/가열/내구/질량/작업량. 물·식량 부품, 무결합 날, 다공성 숯 용기를 거부한다. | `test_emergent_artifacts` 구조·물성 대조 |
| M8-02 | 기존 `TechniqueRecipe`는 개인의 실제 지식·전체 재료 bill·기능을 검사하는 호환 adapter로 남긴다. 새로운 구조 ID가 기존 TechniqueId를 전역 해금하지 않는다. | 기존 제작/기술 회귀 + 옛 v8 저장 byte 왕복 |
| M8-03 | 같은 주민의 기존 Core 실험 utility 슬롯에서 실제 보유 재료로 구조 가설을 평가한다. 실패는 원료를 잃고 다음 후보 점수에 영향을 준다. 날카로운 취성 시험은 실제 경미한 부상을 낼 수 있다. | 서로 다른 조합·실패·손실·긴급 생존 차단·위조 ID 거부 |
| M8-04 | 실제 원료/작업 시간을 지불한 기능 시험과 실제 채집 사용이 개인 제작법·신뢰·직접 사용 횟수로 남는다. | 실제 production 토큰/14분/3원료/직접 기억 및 저장 재생 |
| M8-05 | 구조 성능으로 실제 채집 수량/물 운반이 달라진다. 도구는 마모·파손되고 재료를 써서 제한 횟수 보수된다. 가열판은 실제 불 지식·목재 연료·보유 물을 요구한다. 기존 시설 보수/생산망을 보존한다. | 실제 gather/wear/break/repair, 운반 재사용, 연료 없는 가열 거부 |
| M8-06 | 실제 사용한 도구의 근접 시범은 기존 이동/행동 토큰/6분 슬롯을 쓴다. 두 번의 시범 뒤 재현 가능해지고 도제 근거가 된다. 불신/낮은 수용성은 도입을 거부한다. 기존 물리 RecordTablet을 소비한 설계 기록은 직접 넘겨 주고 읽어야 학습된다. | 개인별 지식 분리, 두 시범·중간 저장·도제·물리 기록 전달 |
| M8-07 | 실제 설계별 제작자/생존 지식 보유자/실제 사용/보수/기록과 실제 완공·운영·사용한 시설의 재료 패턴을 읽는다. 기존 사회 역할·교역·기관·정착지 관찰과 함께 제시한다. | 창작 세계 상태 없는 read-only JSON, 사망에 따른 지식 소실, 한글 UI |
| M8-08 | 같은 재료에서 시드별 실제 실험 성공·실패가 갈리고 실패 경험으로 다른 조합을 택한다. 과거 보유자가 사망한 설계를 새로 실험해 성공하면 재발견이다. 무개입 100/365/1000일 인구·실험·사용·지식 소실·저장 수치를 별도로 기록한다. | 8개 통제 시드 + 별도 무개입 장기 census |

## 저장 계약

- 고유 물건 ID, 정확한 부품 구조, 제작자/시각, 내구·사용·보수, 개인 기술·출처·실패/성공, 실제 ledger, 물리 설계 기록을 보존한다.
- 기존 v8 본체와 이전 tail은 변경하지 않는다. 새 상태가 있을 때만 길이로 구획된 `LLARTEND` 최종 확장(version 1)을 쓴다. 새 상태가 없는 저장은 이전 byte 표현 그대로다.
- 로드·직접 restore·저장 모두 중복 물건/설계/소유자, 잘못된 enum/크기/내구/미래 시각, 초과 count/길이, 중복 확장과 잘린 tail을 거부한다. 주민당 물건 32·개인 설계 64·기록 32·최근 ledger 128로 한정한다.
- native의 libstdc++와 WASM libc++는 **기존 mt19937 텍스트**의 끝 index 표기가 다르다. 이 기존 필드 밖의 모든 물리·지식·실행·확장 바이트를 정확 비교하고, 같은 WASM 코덱으로 정규화한 전체 저장본도 비교한다. 같은 backend의 중간 저장 재개는 전체 byte 동일성을 요구한다. raw cross-toolchain 전체 byte 일치로 과장하지 않는다.

## Web 관찰과 실제 화면

- 주민 상세의 **발명과 제작 지식**, 월드의 **실험 설계의 전파와 쇠퇴 / 생활에서 형성된 건축 양식**은 Core JSON만 소비한다.
- 실제 Core Gather 토큰이 선택한 구조만 손 위치에 조립해 표시한다. 단순 보유·걷기·수면·대화는 도구 사용으로 꾸미지 않는다. 동일 구조 캐시와 최대 64개 geometry/disposal을 검사한다.
- `test_artifact_production_replay`가 생성한 실제 Core 결과를 React에 표시해 320/393/915/1440px의 한글·overflow·page error를 확인했다. 이 검사는 통제된 실제 Core 증거이며 무개입 장기 세계나 물리 Android GPU 측정을 대신하지 않는다.
- 이전 사용자 지시의 실기기 제외 및 발열 제외를 유지한다. 유료 모델·외부 API를 제품 필수 경로에 추가하지 않는다.

## 재현 명령

```sh
cmake -S Source/LifeLensCore -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
LIFELENS_ARTIFACT_FIXTURE_DIR=/tmp/m8 build/test_artifact_production_replay
build/ll_invention_audit 874213954 1000
build/ll_invention_audit 4242001 1000
build/ll_invention_audit 874213955 1000
python Tools/build_web_client.py
node Tools/verify_wasm_artifacts.mjs /tmp/m8
npm --prefix web run typecheck
npm --prefix web run build
python Tools/ci_web.py regressions
python Tools/validate_masterplan_tracking.py
```

신규 WASM 증거 생성·동일 재생 검사는 기존 Web WASM CI에 포함한다. 최종 원시 근거와 배포 출처는 아래 인수 결과와 `evidence/m8-2026-11-14/`에 보존한다.

## 통합 회귀와 비용 확인

- 최초 최종 head `e26b077`의 원격 Core **180/181**, Web/WASM/Preflight/Autonomy 및 지구·야생 회귀는 성공했다. 실패 1건은 기존 자연 실패 기억 검사의 seed `874213955`에서 M8 이후 90일 동안 원래의 부분 재고+가능한 알려진 대안 기회가 사라진 것이었다.
- 같은 컴파일러로 원래 main `f3c7e4f` Core만 별도 빌드한 대조는 기존 seed `874213955`의 token 2360→2361, 재고 1→8의 자연 인과 검증을 통과했다. 새 월드에 자원/실패/기억을 주입하거나 기능을 늦춰 그 옛 사건을 강제하지 않았다.
- 새 Core의 **시드 4**에서 같은 90일 관찰 범위와 모든 원래 조건을 유지해 실제 기회를 찾았다. 주민 2, goal/실패 token **1331**, 기억을 유지한 회복 token **1333**, 원천 **8526816152347751445**, 재고 **2→9**, 원천 **108→101**, 실제 이동·직접 성공 기억·정확히 실패 기억 한 건 제거/재삽입의 나머지 snapshot byte 동일성 모두 통과했다. 대조군은 그 회복 원천을 선택하지 않는다. 최종 회귀는 이 자연 시드를 사용하며 **원래 목표까지 실제 달성해야 한다는 assert도 추가**한다. 관찰 기간·재고 양수·기한·이동·수량·단일 기억 차이 조건은 낮추지 않는다.
- 오래된 quality별 원료 스택 때문에 실제 재고와 보유 기능을 반복 스캔하던 비용을 제거했다. 매 제안에서 실제 재고와 보유 기능을 한 번 계산하되, 기존 원료 스택 순서는 중복까지 그대로 보존해 부동소수 동률 선택도 바꾸지 않는다. 지속 cache나 새 authority를 만들지 않는다. 재료/quality/개인 학습/내구가 다른 **192개 입력의 선택 ID와 16진 부동소수 점수가 전부 동일**했다. 로컬 제안 함수 비교의 원시 중앙값 차이는 `proposal-equivalence.json`에 보존하며, 이를 전체 게임/Android 속도 개선 수치로 확대하지 않는다.


## 최종 인수·main·공개 배포

- 구현 PR [#1030](https://github.com/sjLim91/lifelens-ue/pull/1030), 검증 코드 `8dcdcbdb1ff0a99085913c804855fd7f4509eba1`, 실제 main/런타임 `4640a36cacffb93a71c39de4f0ba17a6434f62d0`. [Core CI](https://github.com/sjLim91/lifelens-ue/actions/runs/43566413973)에서 **181/181**, 결정론·기존 AL-1 회복·365일 수면 회귀까지 통과했다. 필수 workflow 9개가 모두 같은 head에서 성공했다.
- [Runtime Release](https://github.com/sjLim91/lifelens-ue/actions/runs/43568657563) · [Pages Preview](https://github.com/sjLim91/lifelens-ue/actions/runs/43568840164) · [External Probe](https://github.com/sjLim91/lifelens-ue/actions/runs/43568903456) 성공. [공개 앱](https://sjlim91.github.io/lifelens-ue/index.html)의 JS/CSS·HTML·WASM·runtime 출처/manifest를 실제 gh-pages Git blob과 비교했다. 공개 WASM으로 native fixture를 실행해 물리/지식 byte와 정규화 전체 저장·중간 재개를 다시 검증했다. 공개 실제 앱의 네 명 새 게임·개인 발명 빈 상태·모바일/가로 화면도 오류 없이 통과했다.
- [배포·CI·브라우저 JSON](evidence/m8-2026-11-14/release-proof.json), [원시 증거 설명](evidence/m8-2026-11-14/README.md), [실제 production 재생](evidence/m8-2026-11-14/native-production-replay.log), [M8 물리/저장 회귀](evidence/m8-2026-11-14/native-unit.log), [WASM 대조](evidence/m8-2026-11-14/wasm-proof.json), [네 화면 크기](evidence/m8-2026-11-14/browser-fixture-proof.json).

### 변경하지 않은 새 게임: 최종 코드 1000일

아래 수치는 실제 `setupNewGame`·`Simulation::step`을 쓰며 시작 원료·주민·행동·기억을 공급하지 않았다. 각 세계에서 100/365/1000일의 native 전체 binary encode/decode/restore 동일성도 통과했다.

| 시드 | 1000일 생존/누적 주민 | 실험/성공 | 실사용/시범 | 시설 | snapshot bytes | 실제 관측 |
|---|---|---|---|---|---|---|
| 874213954 | 2/5 | 1/1 | 700/0 | 74 | 5,466,966 | 100일 생존 지식 보유 1명 → 365일 0명. 제작자 사망 뒤 지식 소실 유지 |
| 4242001 | 4/8 | 0/0 | 0/0 | 34 | 10,394,221 | 시설과 인구는 이어지지만 구조 발명은 발생하지 않은 정체 경로 |
| 874213955 | 4/7 | 2/1 | 818/1 | 34 | 7,203,123 | 성공 설계의 개인 지식 유지·실사용·자율 근접 시범 발생 |

원시 `natural-*.jsonl`과 Release 빌드 출처 `audit-build.json`을 함께 보존한다. 자연 세계에서 관측하지 않은 자율 보수·완성 도제·기록·재발견을 위 표에 추가하지 않는다. 이 기능들은 별도의 실제 물리 Core 통제 회귀(8개 시드 포함)로 검증했다. 발전·정체·퇴보·재발견을 시대 타이머로 강제하지 않는다.

## 완료 범위

M8-01~08을 완료로 변경했다. 기존 M10 및 다른 milestone 상태를 보존하고 전체 장부를 **21 완료 / 20 진행 / 45 미완·미검증 = 86**으로 검증했다. 과거 [#968](https://github.com/sjLim91/lifelens-ue/pull/968)의 충돌 브랜치를 합치지 않고 최신 main 위의 이번 구조 모델로 대체했다. #968은 superseded로 닫고 원래 브랜치는 보존했다. 기존 지시의 실기기/발열 제외를 유지하며 Qwen 추론·물리 Android GPU·다른 milestone 전체 인수를 주장하지 않는다.
