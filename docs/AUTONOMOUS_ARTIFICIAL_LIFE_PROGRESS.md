# LifeLens 마스터플랜 86개 개발단위 추적표

> 기준: 2026-10-20 KST. 원본 [자율 인공생명 마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) · 실제 메인/PR/Actions 우선.
>
> 이 표는 **완료 시 반드시 수정하는 단위별 진행 장부**다. 저장만 했거나 PR/테스트가 실행 중인 단위를 완료로 올리지 않는다.
>
> **2026-10-20 검증:** [15개 진행단위별 완료 판정 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md). PR #852의 Core 159/159·365일 CI 통과 및 main 반영 후에도 각 단위 전체 인수 조건은 별도로 판정한다.

## 완료/갱신 계약

1. **✅ 완료**는 해당 개발단위의 **전체 완료 판정이 충족**되고 필요한 구현이 **main에 병합**, 해당 정확한 PR head의 모든 필수 CI가 성공한 때만 선언한다. 해당 열을 `[x]`로 변경하고 PR/merge SHA/CI 링크를 적는다.
2. **🔎 병합 검토**는 exact-head 검증은 모두 통과했으나 PR이 열려 있거나 통합 재검증이 필요한 상태다. 완료가 아니다.
3. **🚧 진행**은 구현·감사·시스템 검증의 일부가 시작됐으나 완료 판정이 아직 성립하지 않은 상태다. **⬜ 미완**은 별도 완료 증거가 없는 상태다. `⚠️ 차단`은 실제 장애 원인과 다음 조치를 기재한다.
4. 새 PR을 만들면 해당 단위를 **🚧 진행**, CI를 통과하고 병합 대기면 **🔎 병합 검토**, main+검증 완료면 **✅ 완료**로 변경한다. 이 규칙을 모든 작업 브랜치에서 적용하고, tracker 단독 PR에는 전혀 무관한 제품 코드/설계 규칙을 섞지 않는다.
5. 완료 수는 **이 마스터플랜의 전체 판정 기준**이다. 기존 부분 기능 존재·문서 작성·단순 CI 성공을 전체 제품 진척률로 과장하지 않는다. 
6. 서로 다른 PR에 의해 tracker 변경 충돌이 나면 코드 본체의 완료 근거를 먼저 확인하고 **최신 main 장부에만 순차적으로 합친다**. 브랜치마다 자기 기록을 무조건 덮어쓰지 않는다.

## 현재 계획 완료 집계

- 완료: **3/86** (M0-01, M0-02, M6-01). 단순 계획 단위 완료 지표이지 전체 시뮬레이션 구현률이 아니다.
- 검증 완료·병합 검토: **0** (현재 표시 기준).
- 진행: **17** (M0-03 #790/#820/#824/#836, M0-04 #669/#819/#839, M0-05 #826, M1-01 #822/#828/#832, M1-03 #841, M1-05 #841, M2-01 #844, M2-03 #844, M2-05 #846, M2-06 #846, M2-07 #830/#835/#838, M2-08 #857/#866, M2-04 #879, M2-09 #864/#880, M9-01 #859/#863, M11-03 #852/#854, M11-04 #860/#868).
- 미완 또는 미검증: **66**.
- **병렬 트랙:** AI 실행·인지 감사(M0-03, M1-01, M2-01), 메모리/장기 감사(M0-04, M11-04), 야생/생태(M0-02, M6-01), 시간 지속성(M9-01/02)은 파일 소유 영역을 분리한다. 실제 기능 구현/테스트 착수 전에는 신규 병렬 트랙을 완료·진행으로 미리 올리지 않는다.

## 전체 단위별 진행 장부

### M0 — 진입/검증

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [x] | **M0-01** | #816 교역 양측 기억 exact-head·병합·메인 재검증 | ✅ 완료 | [PR #816](https://github.com/sjLim91/lifelens-ue/pull/816) · main e6061967 · Core/WASM/Web/인지 5 CI PASS |
| [x] | **M0-02** | #814 사냥/식생 소모 코어 PR 충돌·검증 분리 | ✅ 완료 | [PR #814](https://github.com/sjLim91/lifelens-ue/pull/814) · 9 CI PASS · main 22ccf7a1 · Draft 해제 후 병합 |
| [ ] | **M0-03** | #790 14개 의도 전수 지표: offered→accepted→pending→completed/failed/unsupported | 🚧 진행 | [#820](https://github.com/sjLim91/lifelens-ue/pull/820) 실 Core 실행 토큰 감사 main `04a9a477`; [#824](https://github.com/sjLim91/lifelens-ue/pull/824) 14개 intent offered/accepted/rejected main `eac1f47a`; [#836](https://github.com/sjLim91/lifelens-ue/pull/836) Core validator vs commit 거절 4 CI PASS→main `b30d40d6`. 여러 seed 100/365/1000일 실측 인과/미지원 사유 전체 미완 |
| [ ] | **M0-04** | #669 메모리 source/tag/byte·중복·부정합 장기 감사 | 🚧 진행 | [#819](https://github.com/sjLim91/lifelens-ue/pull/819) source/actor/action 기억 감사 main `ac1f3b1a`; [#839](https://github.com/sjLim91/lifelens-ue/pull/839) checkpoint delta main `1f3d560d`; [#850](https://github.com/sjLim91/lifelens-ue/pull/850) 다중 seed별 실측 포함/누락 비교 Preflight 유닛 PASS→main `6495ad56`. [#853](https://github.com/sjLim91/lifelens-ue/pull/853) Core 주민·기억 합계 누락 탐지 Preflight PASS→main `d66da8f5`. 실제 여러 seed 100/365/1000일 결과·해석 미완 [PR #874](https://github.com/sjLim91/lifelens-ue/pull/874) 사실 고유 ID 메모리 출처 분류 Core/WASM/Web/Preflight exact-head CI PASS→main `2d74ac54` 병합; [PR #875](https://github.com/sjLim91/lifelens-ue/pull/875) 실제 checkpoint 주민 기억 유형/전언/중복 통계 Preflight·Web CI PASS 후 main `534909b4` 병합. 실측 1000일 결과 최종 판정 미완 |
| [ ] | **M0-05** | baseline CI·저장 포맷·Ruleset·핵심 회귀 테스트 계약 동결 | 🚧 진행 | [#826](https://github.com/sjLim91/lifelens-ue/pull/826) Preflight/Web Checks PASS→main `8e2aac8b`, pinned 자산 timeout 재시도/원본 hash 검증 보존. 전체 baseline·저장·Ruleset 동결 미완 |

### M1 — 지각/기억

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M1-01** | Core \`PerceptionFact\`: 시야/거리/가림/흔적·청각 채널·시각별 관측 근거 | 🚧 진행 | [#822](https://github.com/sjLim91/lifelens-ue/pull/822) main `0f30df41` 현재 시각 경계; [#828](https://github.com/sjLim91/lifelens-ue/pull/828) 9개 exact-head CI PASS→main `82c43e0f` 주민 실제 Core 위치에서 근접 자원 감지; [#832](https://github.com/sjLim91/lifelens-ue/pull/832) 10개 exact-head CI PASS→main `9a9883e3` Core 근접 감지를 단일 AI 인지 DTO/fingerprint로 연결. [#849](https://github.com/sjLim91/lifelens-ue/pull/849) 실제 관측자 위치/거리 일치 검사 4 CI PASS→main `01c1e12b`. 가림·청각·움직임/실행 검증 미완 |
| [ ] | **M1-02** | \`ResidentMind\` 통합 인터페이스: Needs/Emotion/Personality/Belief/Relation/Goal/Plan 하나의 decision slot | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M1-03** | 개인 기억 출처: DirectWitness/ToldByOther/Inferred와 사건·대상·공간·시간 일관화 | 🚧 진행 | [#841](https://github.com/sjLim91/lifelens-ue/pull/841) 기존 실패 테스트 수정 후 exact-head 9 CI PASS→main `ca0b5a6a`, 근접 자원 신호 추론 기억 및 반복 관측 중복 제한. 소문/사회/도구 전체 기억 출처 불변성은 미완 |
| [ ] | **M1-04** | 에피소드→요약 기억→핵심 신념의 계층적 저장/회상(근거 ID 보존) | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M1-05** | 장소·위험·자원·사람·도구 개인 경험 맵 | 🚧 진행 | [#828](https://github.com/sjLim91/lifelens-ue/pull/828)/[#832](https://github.com/sjLim91/lifelens-ue/pull/832) 실제 Core 개인 근접 센서 main, [#841](https://github.com/sjLim91/lifelens-ue/pull/841) 9 CI PASS→main `ca0b5a6a` 공간/시간 추론 기억. 위험·사람·도구 지도와 LOS·경로는 미완 |
| [ ] | **M1-06** | 호기심·습관·애착·위험감수·상대 신뢰의 동적 갱신 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M1-07** | 자동 무료 로컬 추론: browser WebGPU/loopback provider·스케줄러·신형 Core context 정합 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M2 — 목표/계획

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M2-01** | Core \`NeedGap/Opportunity\` 감지: 생존 위험·자원·관계·호기심·반복 실패 | 🚧 진행 | [#844](https://github.com/sjLim91/lifelens-ue/pull/844) Core/Preflight/WASM/Web 4 CI PASS→main `277b8c6a`: 주민별 근접 관측 기반 호기심/물자 목표 후보, 가짜/고갈된 source 무효화. 실제 GoalSpec 실행/다단계 인과 미완 |
| [ ] | **M2-02** | Core \`GoalSpec\`: 목표 조건·대상·기한·비용·근거·중단 조건 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M2-03** | 주민별 경쟁 목표 후보 생성/순위: 개인 동기·불확실성·환경 증거 | 🚧 진행 | [#844](https://github.com/sjLim91/lifelens-ue/pull/844) Core/Preflight/WASM/Web 4 CI PASS→main `277b8c6a`: 주민별 근접 관측 기반 호기심/물자 목표 후보, 가짜/고갈된 source 무효화. 실제 GoalSpec 실행/다단계 인과 미완 |
| [ ] | **M2-04** | \`Hypothesis\` 후보: 원인 추정·대안 방법·확신과 반증 조건 | 🚧 진행 | [#879](https://github.com/sjLim91/lifelens-ue/pull/879) 주민별 실제 채집 실패 원인 가설·반증 관측 생성 CI 중. 실제 결과 기반 가설 개선·완전한 재계획 미완 |
| [ ] | **M2-05** | 실세계 \`Affordance\` primitive 목록: Observe/Move/Approach/Pick/Carry/Transfer/Combine/Work/Talk 등 구현별 capability | 🚧 진행 | [#846](https://github.com/sjLim91/lifelens-ue/pull/846) 실제 가까이 감지된 물자 목표를 Core Gather read-only 후보로 연결. #844 main 이후 정확히 변경 파일만 재기반, Core Gather 후보 원격 CI PASS 후 main eb3398c5 병합. 경로·소비·실제 실행 전권·다단계 계획 미완 [#852](https://github.com/sjLim91/lifelens-ue/pull/852) exact-head 9 CI(Core 159/159·3 seed 365일 포함) PASS 후 main `18232883` 병합. 완료 조건 중 누락은 [15개 진행단위 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md) 참조 |
| [ ] | **M2-06** | \`PlanCandidate\` 조합 검색: 순서·전제 조건·자원/거리·우회·포기 | 🚧 진행 | [#846](https://github.com/sjLim91/lifelens-ue/pull/846) 실제 가까이 감지된 물자 목표를 Core Gather read-only 후보로 연결. #844 main 이후 정확히 변경 파일만 재기반, Core Gather 후보 원격 CI PASS 후 main eb3398c5 병합. 경로·소비·실제 실행 전권·다단계 계획 미완 [#852](https://github.com/sjLim91/lifelens-ue/pull/852) exact-head 9 CI(Core 159/159·3 seed 365일 포함) PASS 후 main `18232883` 병합. 완료 조건 중 누락은 [15개 진행단위 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md) 참조 [PR #871](https://github.com/sjLim91/lifelens-ue/pull/871) 대안 발급 후 현재 Core·fingerprint·비용·위험 재검증 CI 중, 여전히 단계별 실제 실행·재계획 통합은 미완 |
| [ ] | **M2-07** | 경쟁 계획 평가: 예상 효율·위험·소요/이동·과거 개인 성공률 | 🚧 진행 | [#830](https://github.com/sjLim91/lifelens-ue/pull/830) 개인 경험 기반 read-only 대안 순위 main `d8c09fd2`; [#835](https://github.com/sjLim91/lifelens-ue/pull/835) 같은 주민·상대·사회 행동의 개인 Core 경험을 실제 unified agency 선택에 반영 4 CI PASS→main `cc4df1b9`; [#838](https://github.com/sjLim91/lifelens-ue/pull/838) Core/Preflight/WASM/Web 4 CI PASS→main `d40e1521` 지각 컨텍스트/목표 계획 호환. 물리 거리/다단계 자율 실행·인과 검증 미완 [#852](https://github.com/sjLim91/lifelens-ue/pull/852) exact-head 9 CI(Core 159/159·3 seed 365일 포함) PASS 후 main `18232883` 병합. 완료 조건 중 누락은 [15개 진행단위 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md) 참조 |
| [ ] | **M2-08** | \`ExperimentBudget\`: 한시적 탐색·안전한 실험·자원 제한/긴급 생존 선점 | 🚧 진행 | [PR #857](https://github.com/sjLim91/lifelens-ue/pull/857) 실제 근접 자원의 거리별 계획 effort/risk 예산 검증을 작성·Core 157/157 및 Preflight/WASM/Web CI 통과 후 main `2ede302f` 병합. [PR #866](https://github.com/sjLim91/lifelens-ue/pull/866) 생리 압력 상향 시 선택 실험 후보 제한·자가회복 후보 유지 Core/365일·WASM/Web/Preflight CI PASS 후 main `d132ef62` 병합. 실행 시 예산 소모·장기 안전 전체 미검증 |
| [ ] | **M2-09** | 실패/환경변화 시 부분 재계획·가설 수정·기존 목표 유지/포기 | 🚧 진행 | [PR #864](https://github.com/sjLim91/lifelens-ue/pull/864) 주민별 직접 관측한 실패 위치의 선택적 Goal 재시도 회피와 실제 성공 복구 회귀, Core 159/159·3 seed 365일·WASM/Web·Preflight CI PASS 후 main `d1a79ec4` 병합. [#880](https://github.com/sjLim91/lifelens-ue/pull/880) 실패 장소에서 실제 대체 채집 성공 회귀 CI 중. 전체 다단계 재계획·가설 수정 미완 |

### M3 — 실행/학습

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M3-01** | 모든 허용 cognition intent의 지원/차단/기회 분류 표와 Core 검증 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M3-02** | \`GoalID → PlanID → StepID → ContextToken → OutcomeID\` 유효 인과체인 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M3-03** | 실제 local+원격 행동의 성공/실패/무효/중단 provenance 공통화 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M3-04** | 행동 실패 원인 분류: 접근불가/도구없음/자원고갈/대상부재/거부/질병 등 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M3-05** | 주민별 행동·대상·장소·방법 성공 경험 반영 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M3-06** | 계획 중 저장/복원·중복 ACK·새 모델 응답·월드 변경 안전 처리 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M4 — 생명/생존

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M4-01** | 식사·취수·수면시간·배설·위생·체온 Need 감소/회복의 시간적 사실 감사 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-02** | 유한 식량 생산→운반→저장→소비→부패, 물 용기·오염·식수 회복 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-03** | 시설/사물의 사용 예약·충돌·접근 경로·중복 공사·수리·재활용 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-04** | 질병·상처·통증·체온·회복·치료·사회적 돌봄 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-05** | 임신·출산·영유아 의존·성장·노화·사망·상속/보호·세대교체 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-06** | 주민 도구/직업·성별/나이/특성 표현이 실제 Core 개인 능력과 일치 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M4-07** | 100/365/1000일 생존·사회 시간 예산과 역설적 loop 감시 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M5 — 사회/문화

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M5-01** | 타인 지각·언어/제스처·신뢰도별 실제 정보 전달 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-02** | 친밀·연애·갈등·회피·화해·동반/돌봄 관계의 지속 행동 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-03** | 두 사람 이상 공동 목표/작업 분담·동의/거절·갈등 조정 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-04** | 가족·양육·결혼·가구·보호자·부모 사망 후 후견 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-05** | 개인 지식·도제 교육·관찰 학습·전언 전파·유실/재발견 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-06** | 노동·저장·소유권·분배·교역·이주 계약 및 갈등 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M5-07** | 관습·규범·직업/기관·문화의 **사후 관찰 분류** | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M6 — 동물/생태

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [x] | **M6-01** | #814 사냥/초식 실제 식생 소비의 Core 합치기 및 자원 ledger | ✅ 완료 | [PR #814](https://github.com/sjLim91/lifelens-ue/pull/814) · Core 유한 먹이·사냥/현장 식사 성공·실패/실행 토큰/저장 회귀 · main 22ccf7a1 |
| [ ] | **M6-02** | 종별 지각·허기·영역·경계/도주/추적·독립 생물 agent | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-03** | 포식자/피식자 추격·실제 접촉·부상·사망·먹이 섭취 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-04** | 주민 위협 지각→회피/보호/치료, 포식자도 위험·스트레스·기아 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-05** | 사냥→시체/육류/가죽/뼈→운반·보관·부패·조리·식사 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-06** | 서식지별 수용력·계절·식생·물·번식·이동·멸종/복원 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-07** | 반복 먹이 제공·동물 경계/신뢰·울타리·사육·유전적 세대 변화 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M6-08** | 야생 행동·사냥·가축·위협의 Web 관찰/애니메이션 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M7 — 지구/지역

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M7-01** | 공개 라이선스 자연지리 EarthDataPack/version/해상도·출처 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-02** | Planet/Region/Chunk/Local 지구 구면↔지면 연결, seam·극지 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-03** | 지구본 확대→지역→정착지→개인, 한 Core 월드의 연속 렌더 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-04** | 실제 위치에 4+4 시작 집단·독립 지역주소·시드/기원 보존 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-05** | 위도·고도·계절·태양시·지형에 연결된 지역 기온/강수/바람 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-06** | 유역·담수 공급/갈수·토양·침식/습윤·자원 재생 지형 결속 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-07** | 접평면 바람·해류·해수면·기상 관찰 지도(지원 field만) | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M7-08** | 실제 경로 지역 경계 통과·탐사·교역·이주·정보 전파 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M8 — 발명/문명

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M8-01** | 물질 속성/부품/형상/결합/내구/열·에너지의 typed artifact 모델 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-02** | 현재 registry recipe를 실제 재료·도구·기술 capability 어댑터로 분리 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-03** | 주민의 새로운 조합 가설·소규모 물리 실험·실패/파손/부상 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-04** | 결과의 기능 측정(베기/파기/운반/가열)→개인 기술 지식화 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-05** | 실제 도구/시설 보수·재사용·생산망·기술 숙련/개선 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-06** | 기록/모방/교육/도제·독립 발명/기술 도입/거부 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-07** | 경제·제도·문화·건축 양식/정착지 형성의 **발생 관찰 모델** | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M8-08** | 다양한 실험 가능한 seed에서 발전·정체·퇴보·재발견의 분기 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M9 — 월드 지속성

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M9-01** | authoritative save+journal+wall timestamp 및 중복 실행 방지 lease | 🚧 진행 | [PR #859](https://github.com/sjLim91/lifelens-ue/pull/859) IndexedDB 원자적 체크포인트 시각·Core 분 역행 방지, Preflight/Web PASS 후 main `8da0cbfa` 병합. [PR #863](https://github.com/sjLim91/lifelens-ue/pull/863) Web Locks 단일 Core writer·중복 탭 차단/안전 종료 구현, Web Checks·Preflight exact-head PASS 후 main `26fcdfb5` 병합. 다중 기기 동기화·journal·모바일 실제 기기 검증 미완 [PR #872](https://github.com/sjLim91/lifelens-ue/pull/872) 동일 시간 서로 다른 스냅샷 충돌 거부 Preflight/Web PASS 후 main `78baa435` 병합. 모든 기기/프로세스 authoritative journal은 미완 |
| [ ] | **M9-02** | 재접속 결정론 catch-up: 실제 Core tick/이벤트·중간 체크포인트 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M9-03** | 장시간 빨리 따라잡기용 안전한 coarse simulation/LOD | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M9-04** | 상주 무료 로컬 실행기(PC/홈 서버 등) + browser read-only 연결 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M9-05** | Android 제약을 반영한 포그라운드/제한적 백그라운드/복귀 정책 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M9-06** | PC↔Android 단일 권위 월드 동기화/백업·충돌/단절 복구 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M10 — 관찰/그래픽

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M10-01** | 지구본↔정착지 관찰 중심 가로 UI, Seed·시간·속도/이동/줌 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-02** | 주민별 팝업: 욕구·성격·성별/연령·기억·가설·계획·상대·가족 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-03** | 인과 타임라인: 생각(제안)→승인/차단→실제 시도→결과→학습 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-04** | 체형/성장/성별 외형 차별화·해부학적 절차 메시·일관 rig | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-05** | 걷기/후진/시선/거리/작업/수면/부상/배설/돌봄 행동 모션 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-06** | 호랑이/늑대/사슴/토끼 실제 종별 anatomy rig/behavior motion | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M10-07** | 강·숲·색상·날씨·흔적·시설/파손/수리의 Core truth 기반 렌더 LOD | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

### M11 — 검증/확장

| 완료 | 단위 | 계획에 정의된 작업 | 상태 | 근거 / 후속 |
|---|---|---|---|---|
| [ ] | **M11-01** | exact-head Core CTest·WASM·Web Build/Typecheck/Preflight | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M11-02** | WorldSeed/PopulationSeed/generation/ruleset 및 snapshot/backward codec 검사 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M11-03** | 100/365/1000일 다중 seed 및 일부 3000일 장기 실행 | 🚧 진행 | [#852](https://github.com/sjLim91/lifelens-ue/pull/852) 365일 seed 874213955 수면 포화 42633분 > 14905분 기준 실패·Draft 유지; [#854](https://github.com/sjLim91/lifelens-ue/pull/854) 주민별 기여 원인 분석 Core/Preflight PASS, main 2c5dceff 병합. #852는 최신 main 충돌 해소 후 장기 회귀 재검증 진행 중. 장기 다중 seed/3000일 안정성 미완 [#852](https://github.com/sjLim91/lifelens-ue/pull/852) exact-head 9 CI(Core 159/159·3 seed 365일 포함) PASS 후 main `18232883` 병합. 완료 조건 중 누락은 [15개 진행단위 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md) 참조 |
| [ ] | **M11-04** | 수치 계측: 개인별 목표 생성/시도/완료/실패원인/학습 후 선택 변경 | 🚧 진행 | [PR #860](https://github.com/sjLim91/lifelens-ue/pull/860) 주민 AI 승인·Core 성공 분리 보고 및 가짜 인과 방지 테스트 exact-head CI 진행 중. [PR #868](https://github.com/sjLim91/lifelens-ue/pull/868) 주민별 실제 성공/실패와 집계 정합성·lossless ID 검증 Preflight/Web PASS 후 main `e3c3fb09` 병합. 주민별 목표 인과·학습 이후 장기 선택 변화 전체 미완 [#852](https://github.com/sjLim91/lifelens-ue/pull/852) exact-head 9 CI(Core 159/159·3 seed 365일 포함) PASS 후 main `18232883` 병합. 완료 조건 중 누락은 [15개 진행단위 감사](AUTONOMOUS_ARTIFICIAL_LIFE_15_UNIT_COMPLETION_AUDIT_2026-10-20.md) 참조 [PR #873](https://github.com/sjLim91/lifelens-ue/pull/873) 동일 시드 개인별 1·7·100·365일 Core 관측 비교 CI PASS 후 main `eadb592a` 병합. [PR #876](https://github.com/sjLim91/lifelens-ue/pull/876) 실제 Core 1/7일 시드 고정 비교 감사 CI PASS→main `6abb861f` 병합, 목표·실패·학습의 전 인과 증명 미완 |
| [ ] | **M11-05** | no-LM, browser LM, PC loopback LM A/B 실행 프로파일 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M11-06** | 비용/성능: 4→8→32→100→300→1000 주민, 동물·메모리·LLM 토큰/시간/지연 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M11-07** | 실제 Android 입력/메모리/GPU 렌더링·발열·장시간 observer QA | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |
| [ ] | **M11-08** | 회귀 dashboard 및 사용자용 진척률: 실구현/실행경로/검증/배포 분리 | ⬜ 미완 | 기반 구현은 [마스터플랜](AUTONOMOUS_ARTIFICIAL_LIFE_MASTERPLAN_2026-10-20.md) 참조. 완료 판정 미검증 |

## 출하 변경 기록

| 날짜 | 단위 | 이전→현재 | 검증/조치 |
|---|---|---|---|
| 2026-10-20 | M0-04 | 🚧 진행 유지 | #853 실제 checkpoint 주민·기억 총합과 resident 개별 기록 불일치 실패 검증, Preflight PASS→main d66da8f5 |
| 2026-10-20 | M11-03 | ⬜ 미완→🚧 진행 | #852 실제 365일 시드 수면 포화 회귀로 main 차단. #854 주민별 기여 원인 분석 신규 PR CI 중, 허용 임계치 불변 |
| 2026-10-20 | M1-01 | 🚧 진행 유지 | #828 정확한 HEAD Core/Preflight/WASM/Web 및 야생/지구 감사 PASS, main 82c43e0f. #832 Core-bound perceived-resource DTO·fingerprint CI 진행 중; M1 전체 미완 |
| 2026-10-20 | M1-01 | 🚧 진행 유지 | #822 exact-head 4 CI PASS, main 0f30df41. 시간 인과 유입 차단 병합; 공간/시야 지각 전체 미완 |
| 2026-10-20 | M0-05 | 미완→🚧 진행 | #826 캐릭터 리소스 다운로드 일시 timeout/503만 제한 재시도, 404/hash/size는 fail-closed. 검증 진행 중 |
| 2026-10-20 | M0-03 | 🚧 진행 유지 | #836 validator·commit 거절 단계별 14 intent 입력 구분 exact-head 4 CI PASS→main b30d40d6; 전체 완료 아님 |
| 2026-10-20 | M1-03/M1-05 | 🚧 진행 유지 | #841 fixed HEAD 4ca19aad의 9개 CI PASS 후 main ca0b5a6a. 개인 근접 추론 기억·12개 제한·동일 스캔 중 중복 재발견 회귀 해결 |
| 2026-10-20 | M2-01/M2-03 | 🚧 진행 유지 | #844 fixed HEAD 33d6d51f 4 CI PASS, main 277b8c6a. 실제 Core 주변 신호 기반 개인 자원 호기심/물자 목표 후보. 실전 실행 여전히 별도 |
| 2026-10-20 | M0-04 | 🚧 진행 유지 | #850 독립 seed별 100/365/1000일 메모리·snapshot 실측 누락/범위 비교·오프라인 테스트 Preflight PASS 후 main 6495ad56. 실제 seed 장기 측정 원인 해석 미완 |
| 2026-10-20 | M1-01 | 🚧 진행 유지 | #849 동일 Core node ID로 관측 거리/관측자 위치 위조 시 goal evidence 거절, 4 CI PASS→main 01c1e12b. LOS·청각 종단 구현 미완 |
| 2026-10-20 | M2-05/M2-06 | ⬜ 미완→🚧 진행 | #846 (선행 #844) 실제 감지 자원 목표를 현재 Core 재료 노드 Gather 후보로 형상화·차단 회귀 CI 준비; 이동/집행 성공이나 전체 primitive 조합 완료 아님 |
| 2026-10-20 | M2-07 | 🚧 진행 유지 | #838 exact-head CI 4개 PASS 후 main d40e1521에 Core 감지 컨텍스트/목표 계획 검증 호환 패치 병합. GoalSpec 실행 루프 미완 |
| 2026-10-20 | M1-03 | 🚧 진행 유지 | #841 Core 157개 중 메모리 크기 순환 테스트 1개 실패 원인 진단, 스캔 안 재발견 중복 제거 커밋 4ca19aad 재검증 진행 |
| 2026-10-20 | M2-01/M2-03 | ⬜ 미완→🚧 진행 | #844 자기 Core 근접 센서로만 물자/호기심 목표 후보 발생, Core 자원 ledger 실존/고갈 검증과 단위테스트 CI 진행. 전체 실제 행동 루프 미완 |
| 2026-10-20 | M2-07 | 🚧 진행 유지 | #835 주민 firsthand 자기 감정 경험을 실제 단일 Core 사회 행동 선택에 적용 exact-head 4 CI PASS→main cc4df1b9; 전체 완료 아님 |
| 2026-10-20 | M1-03 | ⬜ 미완→🚧 진행 | #841 감지 근접 자원을 '직접 목격'으로 오판하지 않게 Inferred provenance·시각·좌표를 저장; CI 중 |
| 2026-10-20 | M1-05 | ⬜ 미완→🚧 진행 | #828/#832 Core 위치 검증 센서 main, #841 개인 자원 경험 장소 메모리 CI 중. 위험·사람·도구 맵 미완 |
| 2026-10-20 | M0-04 | 🚧 진행 유지 | #839 실제 오프라인 메모리 100/365/1000일 집계 분석 기능 단위테스트/Preflight/Web PASS→main 1f3d560d. 다중 seed 실측 미완 |
| 2026-10-20 | M1-01 | 🚧 진행 유지 | #832 exact-head CI 전부 PASS→main 9a9883e3. LOS·청각·온전한 개인 지각 계약 미완 |
| 2026-10-20 | M0-04 | 🚧 진행 유지 | #839 오프라인 실제 100/365/1000일 개인 기억·스냅샷 증가율 대조 툴/회귀 검증 중; 없는 날짜를 완료 표시하지 않음 |
| 2026-10-20 | M2-07 | 🚧 진행 유지 | #838 주민 지각 센서를 가진 인지 요청의 합법적 read-only Core 목표 후보 거절 회귀 수정 CI 진행; 실제 집행 권한은 Core 별도 |
| 2026-10-20 | M2-07 | 🚧 진행 유지 | #835 실전 Core unified social action choice까지 firsthand actor/partner-specific outcome feedback 반영, exact-head CI 진행 |
| 2026-10-20 | M0-03 | 🚧 진행 유지 | #836 Core validator 거절과 validator 통과 후 commit 거절 분리, 14개 intent 카탈로그 전수 검사 CI 진행 |
| 2026-10-20 | M2-07 | 🚧 진행 유지 | #830 exact-head Core 장기 검증 포함 4 CI PASS→main d8c09fd2. Core personal mood outcome 기반 read-only 계획 순위 조정 검증; 전체 목표 실행/계획 동작 미완 |
| 2026-10-20 | M2-07 | ⬜ 미완→🚧 진행 | [#830](https://github.com/sjLim91/lifelens-ue/pull/830) 직접 경험에 따른 사회적 계획 경쟁 순위 수정·CI 검증 중. 범용 효율/시간/이동·실제 실행은 미완 |
| 2026-10-20 | M0-03 | 🚧 진행 유지 | #824 14개 의도 입력 범위 감사 exact-head 4 CI PASS (Web 네트워크 timeout job 재실행 성공), main eac1f47a. 전체 100/365/1000일 근거는 별도 |
| 2026-10-20 | M0-05 | 🚧 진행 유지 | #826 pinned 리소스 다운로드 일시적 네트워크 실패 retry, hash/size fail closed: Web/Preflight CI PASS, main 8e2aac8b |
| 2026-10-20 | M1-01 | 🚧 진행 유지 | #828 bounded Core 직접 위치 기반 근접 자원 감지, 개발/CI 검증 중. 뷰/모델 합류·line-of-sight 미완 |
| 2026-10-20 | M0-03 | 🚧 진행 유지 | #820 exact-head 4 CI PASS→main 04a9a477. 후속 #824 전체 14개 전략 순회 감사 CI 검증 중; 단위 전체 완료 아님 |
| 2026-10-20 | M0-04 | 🚧 진행 유지 | #819 exact-head 4 CI PASS→main ac1f3b1a. 장기 100/365/1000일 시드별 분석 전까지 미완 |
| 2026-10-20 | M1-01 | 미완→🚧 진행 | [#822](https://github.com/sjLim91/lifelens-ue/pull/822) 시각 인과 경계 회귀 구현/CI 검증 중. 공간/지각 모델 미완 |
| 2026-10-20 | M0-02 | 🔎 병합 검토→✅ 완료 | #814 HEAD 81fa7aca CI 9개 PASS, Draft 해제·main 22ccf7a1 |
| 2026-10-20 | M6-01 | 미완→✅ 완료 | 같은 #814: 초식 실제 먹이 소모/사냥/현장 식사; 가죽·뼈·육류 보관은 M6-05 후속 |
| 2026-10-20 | M0-03 | 🚧 진행 유지 | AI 승인→실제 Core 결과 read-only evidence [#820](https://github.com/sjLim91/lifelens-ue/pull/820) 검증 중 |
| 2026-10-20 | M0-04 | 🚧 진행 유지 | 행동 기억 출처/byte별 성장 [#819](https://github.com/sjLim91/lifelens-ue/pull/819) 검증 중 |
| 2026-10-20 | M0-01 | 검증 중→✅ 완료 | #816 Core+WASM+Web+인지+Preflight 전체 통과, main e6061967 |
| 2026-10-20 | M0-02 | 진행→🔎 병합 검토 | #814 기존 정확한 head의 9개 CI 통과, Draft/후속 main 통합 검토 필요 |
| 2026-10-20 | M0-03 | 미완→🚧 진행 | #790 기존 통합 AI 실행 공백 감사 이슈, 14개 intent 측정으로 확장 예정 |
| 2026-10-20 | M0-04 | 미완→🚧 진행 | #669 Memory/Snapshot 증식 원인 추적 이슈 |

**설계 권한 유지:** 기존 물리·사회·생애 설계 규칙 변경은 자동 완료 또는 묵시 승인 대상으로 보지 않는다. 저장 규칙/호환, 무료 사용 및 무가공 Core truth를 유지한다.
