# LifeLens Decision Log

> 목적: 쭌과 AI가 대화 중 확정한 **의미 있는 제품/설계/개발 원칙**을 저장소에 남겨, 새 세션이나 장기 작업 재개 시 다시 설명하지 않아도 되게 한다.
>
> Truth order: **actual GitHub > canonical repository docs > chat/memory/local guess**.

## 문서 역할

- `docs/LIFELENS_SPEC_v1.1.md` — 제품 최상위 요구사항 / 불변조건.
- `docs/DEVELOPMENT_MILESTONES.md` — 장기 개발 순서 / milestone acceptance.
- `tasks/WORK_STATE.md` — 현재 ACTIVE / WAITING_CI / READY_NOW / BLOCKED 상태.
- `tasks/TEAM_BOARD.md` — ownership / locks / Integration Requests.
- `tasks/HANDOFF_LOG.md` — 의미 있는 merge / failure root cause / ownership transition.
- **`docs/DECISION_LOG.md` — 대화 중 합의된 설계·정리·작업 판단 원칙.**

이 문서는 모든 대화 문장을 복사하지 않는다. 향후 구현 선택을 바꾸거나, 세션이 바뀌면 잊기 쉬운 결정만 기록한다.

결정이 기존 Master Spec을 바꾸는 수준이면 이 로그만 수정하지 않고 Master Spec에도 반영한다. 현재 진행상태가 바뀌면 `WORK_STATE`, ownership/lock이 바뀌면 `TEAM_BOARD`도 별도로 갱신한다.

---

# 2026-09-15

## D-001 — 리뷰 요청은 실제 영향 기준으로 판단한다

**결정**
- `Config/**` 등 shared file을 수정했다는 이유만으로 다겸 리뷰를 기계적으로 요청하지 않는다.
- 다겸 소유 영역, 공개 인터페이스, 실제 presentation 동작에 영향을 주거나 상대 작업을 깨뜨릴 가능성이 있을 때만 리뷰/확인을 요청한다.
- 쭌 lane 내부 작업은 정의된 CI/validation gate가 충족되면 쭌 측에서 진행한다.

**배경**
- #96은 production map 연결 + observer camera/config 작업으로 쭌 lane이었고, Dagyeom WorldPresentation 구현을 수정하지 않았다.
- 불필요한 리뷰 요청은 협업 지연과 알림 노이즈만 만든다.

---

## D-002 — 긴 UE Compile 동안 안전한 병렬 작업을 계속한다

**결정**
- 긴 Unreal compile/package가 도는 동안 계속 polling하거나 손 놓고 기다리지 않는다.
- 현재 PR/branch와 파일 충돌이 없는 read-only 조사, 다음 milestone 설계, 독립된 Core 분석을 진행한다.
- long build의 결과가 필요한 merge/closeout만 기다린다.

**예시**
- #96 compile 동안 `SimulationRuleset` / Needs / UtilityAI / snapshot 영향범위를 조사한다.

---

## D-003 — Hardcoding Cleanup의 기본 순서를 유지한다

**결정**
- A: UE runtime Config foundation.
- B: pure Core `SimulationRuleset` + Needs / UtilityAI.
- C: LifeStage / Relationship / Romance / Pregnancy / Family.
- D: legacy projection / Social / Fun / outcome compatibility 정리.
- E: names / assets / content catalogs.

**원칙**
- Core는 pure C++17/표준 라이브러리 기반을 유지한다.
- 안정적 protocol/determinism invariant는 코드에 남을 수 있다.
- 변경 가능한 제품 규칙은 versioned immutable ruleset으로 이동한다.
- Unreal presentation/runtime tuning은 Config/Data Asset 등으로 외부화한다.

---

## D-004 — 출시 전에는 과거 Save/Snapshot 호환을 제품 요구사항으로 취급하지 않는다

**결정**
- 현재 LifeLens는 실제 사용자에게 배포된 버전이 없으므로, 과거 개발 중 SaveVersion / Snapshot binary version과의 하위호환을 보존할 제품 요구사항이 없다.
- 구버전 migration/backward compatibility 코드는 현재 구조를 복잡하게 만들거나 활성 작업과 직접 겹칠 때 제거할 수 있다.
- 새 구조 도입 시 과거 개발 포맷을 살리기 위한 migration layer를 새로 만들지 않는다.

**주의**
- 이름에 `Legacy` / `Compatibility`가 붙었다는 이유만으로 무조건 삭제하지 않는다.
- 현재 Unreal presentation/WorldDirector가 실제로 소비하는 Core→UE adapter는 소비처가 제거되기 전까지 runtime contract일 수 있다.

---

## D-005 — 중간에 발견한 문제는 세 등급으로 판단한다

새 작업 중 발견한 코드 부채/설계 문제를 무조건 미루거나 무조건 즉시 처리하지 않는다.

### NOW — 지금 처리
- 지금 안 고치면 2~3단계 뒤 수정 비용이 크게 증가한다.
- authority/ownership 오류, 구조적 중복, 현재 milestone 기반을 왜곡하는 문제.

### WITH CURRENT WORK — 현재 작업에 포함
- 같은 파일/구조를 곧 다시 뜯어야 하며 현재 milestone을 완결하려면 같이 고치는 편이 더 싸다.
- 현재 작업 목적과 같은 layer/validation scope에 속한다.

### DEFER — 기록 후 보류
- 현재 milestone 품질에 영향이 작다.
- 다른 ownership lane까지 불필요하게 건드린다.
- CI/충돌 위험만 키우고 지금 해결해도 미래 비용 감소가 작다.

**핵심 판단 질문**
> 지금 안 하면 2~3단계 뒤에 더 비싸지는가?

그렇다면 NOW/WITH CURRENT WORK 후보로 본다. 아니면 계획된 milestone을 우선한다.

---

## D-006 — 계획을 유지하되 코드청소를 맹목적으로 미루지 않는다

**결정**
- LifeLens의 milestone 순서는 유지한다.
- 다만 활성 milestone과 강하게 결합된 기술부채는 같이 제거할 수 있다.
- '계획 밖'이라는 이유만으로 미래 비용이 커질 문제를 방치하지 않는다.
- 반대로 '발견했다'는 이유만으로 unrelated cleanup을 새 milestone처럼 확대하지 않는다.

**현재 적용**
- #96 마무리 후 Cleanup B가 기본 다음 단계다.
- B에서 `SimulationRuleset` 도입과 직접 충돌하는 snapshot/save legacy 구조는 필요한 범위까지 정리할 수 있다.
- Social/Fun projection 전체 제거처럼 B와 직접 관계없는 cleanup은 D 단계 또는 별도 적절한 milestone까지 보류한다.

---

## D-007 — 대화로 확정한 결정은 canonical docs에 남긴다

**결정**
- 향후 쭌과 대화 중 구현 방향/제품 원칙/협업 방식이 확정되면, 세션 기억에만 의존하지 않는다.
- 의미 있는 결정은 이 Decision Log에 추가한다.
- 제품 요구사항 변경이면 Master Spec에도 반영한다.
- 현재 상태 변경이면 WORK_STATE에도 반영한다.
- ownership/lock/Integration Request 변경이면 TEAM_BOARD에도 반영한다.

**목적**
- 새 세션에서 사용자가 요구사항을 다시 설명하지 않게 한다.
- AI가 과거 대화 기억만 믿고 실제 저장소와 다른 방향으로 가는 것을 방지한다.

---

## D-008 — 사용자 UI는 한국어 기본, 사회행동은 실제로 관찰 가능해야 한다

**결정**
- 일반 사용자용 LifeLens UI의 기본 표시 언어는 한국어로 한다.
- Core의 enum/action/event id는 언어 중립 식별자를 유지하고, Presentation/localization layer에서 사용자 언어로 변환한다.
- `UseToilet`, `Comfort`, `Argue` 같은 raw English identifier를 최종 사용자 UI에 그대로 노출하지 않는다.
- 주민 간 사회행동은 관계 수치만 바꾸고 끝나서는 안 된다. Core에서 실제로 발생한 actor/target/intent/outcome을 Presentation이 접근, facing/gaze, animation, 아이콘/말풍선, Event Feed/History로 관찰 가능하게 표현한다.
- 모든 일상 대화를 큰 말풍선으로 도배하지 않는다. 일상/의미 있는 사회행동/중요 사건의 중요도 단계에 따라 표현 강도를 나눈다.
- 대사는 초기부터 유료 LLM/API에 의존하지 않는다. social intent + personality + relationship + emotion + memory/context를 조합한 deterministic/data-driven 한국어 표현으로 기본 기능이 완전하게 동작해야 한다.
- Presentation은 Core에서 발생하지 않은 고백/다툼/위로/관계변화를 꾸며내지 않는다.

**구현 기준**
- canonical companion: `docs/SOCIAL_COMMUNICATION_LOCALIZATION_v1.md`.
- Jjun/Core lane: social action/event authority, outcome/read contract, presentation에 필요한 deterministic context.
- Dagyeom presentation lane: localization 표시, speech bubble/Event Feed, gaze/body/animation 표현.
- shared contract가 실제로 필요할 때만 Integration Request를 연다.

**목적**
- LifeLens의 핵심 경험을 "내부 숫자가 변하는 시뮬레이션"이 아니라 "사람들이 서로 소통하며 살아가는 사회를 관찰하는 경험"으로 유지한다.

---

## D-009 — 화장실/사적 행동은 Privacy Mask 기반으로 표현한다

**결정**
- 화장실 행동을 위해 의상별 바지 내리기/올리기 애니메이션을 V1 필수요건으로 두지 않는다.
- `Toilet`은 단일 `Sit` 모션이 아니라 `approach -> align -> enter pose -> use loop -> completion ACK -> exit`의 context sequence로 표현한다.
- 사적 구간에는 캐릭터 전체/화면 전체 블러가 아니라 **골반~상부 허벅지 영역의 재사용 가능한 Privacy Mask/Occluder**를 표시한다.
- 모바일 성능을 위해 무거운 full-screen Gaussian blur보다 lightweight material/pixelation/frosted-noise 계열 presentation을 우선한다.
- 실제 변기가 없으면 가짜 시설 상호작용을 만들지 않고 outdoor fallback pose + 기존 sanitation/environment 결과를 사용한다.
- 실제 변기가 있으면 명시적 interaction point/socket/transform에 정렬해서 허공이나 변기 옆에 앉는 표현을 허용하지 않는다.
- V1에서는 성별에 상관없이 indoor seated / outdoor squat-low pose를 공통 fallback으로 사용할 수 있으며, 성별 전용 variant는 이후 개선사항이다.
- Privacy Mask는 Presentation 전용이며 Core 행동의 시작/완료/취소 authority를 가지지 않는다.
- 같은 시스템을 향후 샤워/목욕, 옷 갈아입기, 출산 등 다른 사적 context action에 재사용할 수 있다.

**구현 기준**
- canonical companion: `docs/CHARACTER_CONTEXT_MOTION_v1.md`의 Toilet / sanitation presentation section.
- 목표는 노골적 묘사가 아니라 **관찰자가 상황을 즉시 이해하면서도 의상/리깅 비용을 과도하게 만들지 않는 표현**이다.

---

# 2026-09-16

## D-010 — Observer 카메라는 PC/Android에서 직접 orbit/zoom/pan 가능해야 한다

**결정**
- 현재 production observer camera의 고정 프레이밍은 bootstrap/검증 단계로 보고, 실제 제품 카메라는 사용자가 직접 조작할 수 있어야 한다.
- PC 입력은 `mouse wheel = zoom`, `right-drag = orbit rotate`, `middle-drag = pan`, `left click = 기존 선택`으로 한다.
- Android 입력은 `one-finger tap = 선택`, `one-finger drag = orbit rotate`, `pinch = zoom`, `two-finger translate = pan`으로 한다.
- 모바일에서 touch pressed 순간 선택하지 않고 `Pressed -> movement tracking -> Released`로 tap과 gesture를 구분한다.
- 주민 선택만으로 카메라가 자동 점프하거나 강제 추적하지 않는다.
- zoom distance와 elevation은 제품 tuning 범위로 clamp하고 모든 camera transform 변경은 smoothing한다.
- 카메라 조작은 Presentation이며 Core/World authority와 deterministic simulation을 바꾸지 않는다.

**구현 기준**
- canonical companion: `docs/OBSERVER_CAMERA_CONTROL_v1.md`.
- `Source/LifeLens/UI/LLObserverPlayerController.*`는 Dagyeom 소유영역이므로 Jjun 구현 시 별도 assist branch + TEAM_BOARD Assist Lock을 사용한다.
- HUD polish/old stacked UI PR 정리와 camera control을 같은 PR에 섞지 않는다.

---

# 2026-09-18

## D-011 — 문명은 현재를 넘어 미지의 미래까지 열린 구조로 간다

**결정**
- LifeLens 문명은 원시 생존/정착에서 끝나지 않고 현대 문명 수준을 지나 현실에 아직 없는 미래까지 발전 가능해야 한다.
- `StoneAge -> BronzeAge -> Modern -> Future` 같은 시간 기반 단일 강제 트리를 Core 진실로 사용하지 않는다.
- 시대/문명 단계는 실제 Knowledge / Capability / Technology / 사회상태를 Observer가 요약한 결과다.
- 같은 시간이 흘러도 세계마다 발전 속도, 경로, 쇠퇴, 지식 소실과 재발견이 달라질 수 있다.
- 미래 기술 역시 자원/에너지/지식/제조능력/조직/유지보수 조건을 실제로 만족해야 한다.

**Canonical companion**
- `docs/OPEN_ENDED_CIVILIZATION_NORTH_STAR.md`.

---

## D-012 — 충돌 회피를 위해 구조를 꼬지 않는다

**결정**
- 파일 충돌을 피하는 것 자체를 아키텍처 목표로 삼지 않는다.
- 같은 책임의 변경은 같은 canonical source/file에 들어가는 것이 맞다면 그 파일을 수정한다.
- 상대 작업과 겹치면 별도 우회 클래스/중복 source를 만드는 대신 상태 확인, 리뷰, rebase/merge로 해결한다.
- "파일이 안 겹친다"보다 "책임이 올바른 곳에 있다"가 우선이다.

**협업 적용**
- Dagyeom active work와 exact file overlap이 없으면 Jjun lane 작업은 정상 진행한다.
- overlap이 있으면 실제 hunk/책임을 확인하고 coordination 후 통합한다.
- stale branch의 whole-file snapshot으로 최신 main을 덮어쓰지 않는다.

---

## D-013 — Jjun PR의 standing merge rule

**결정**
Jjun-owned PR은 다음 조건을 모두 만족하면 매번 별도 확인을 받지 않고 병합할 수 있다.

1. PR exact head를 다시 확인한다.
2. 요구되는 exact-head CI가 모두 green이다.
3. 최신 main / mergeability를 확인한다.
4. Dagyeom의 현재 open PR과 changed filename을 새로 확인한다.
5. unresolved ownership/overlap이 없다.
6. merge 시 `expected_head_sha`를 사용한다.

Dagyeom 소유 변경 또는 실제 overlap/불확실성이 있으면 자동 병합하지 않고 먼저 coordination한다.

---

## D-014 — Presentation Catch-up 이후 C1 Settlement를 본선으로 진행한다

**결정**
- #142~#151에서 time/weather chrome, scrolling, lifecycle visibility, context motion, canopy/readability, weather fallback, adaptive UI까지 1차 Presentation Catch-up을 통합했다.
- 이후 Presentation polishing은 Core 진행을 막는 기본 blocker로 취급하지 않는다.
- 현재 본선은 **C1 Settlement & Subsistence**다.

C1 세부 순서:
1. **C1-A Facility Authority** — WorkSurface / SleepingPlace / Shelter의 실제 material/work/persistence. #152 완료.
2. **C1-B Autonomous Need Recognition** — sleep/environment/crafting pressure가 건설 Utility를 만든다.
3. **C1-C Facility Effects & Maintenance** — 실제 sleep/weather/work 효율 및 durability.
4. **C1-D Durable Subsistence** — water/food storage, spoilage, cultivation, seasonal production.
5. **C1-E Emergent Settlement Form** — 실제 사용/동선/제약으로 생활 중심과 공간 분화가 나타난다.
6. **C1-F Early material expansion** — Tin/Bronze는 실제 선행조건 뒤에 진행한다.

**불변조건**
- New Game에 정착 시설을 공짜로 지급하지 않는다.
- 시설 생성/완료 authority는 Core다.
- settlement milestone 이름이 facility spawn trigger가 되어서는 안 된다.

---

## D-015 — Whole-source audit P0 fixes outrank C1-B until stabilized

**Decision**
- 2026-09-18 whole-source audit findings are promoted above the current feature roadmap.
- C1-B remains the next product feature milestone, but it is blocked until AUDIT-0A/B/C are complete and green.

**AUDIT-0A**
- duplicate time/weather/speed UI responsibility must be consolidated to one canonical production surface.

**AUDIT-0B**
- per-minute environmental Need pressure must use each resident's authoritative location/chunk, not one shared start-region climate.

**AUDIT-0C**
- after 0A/0B: Core Tests + deterministic harness + Preflight + Unreal Compile + PIE smoke.

**P1 follow-up**
- WorldPresentation / obstacle collision must eventually consume an explicit materialized chunk coordinate read contract instead of inferring coordinates from count + initial-region ring scanning.

**Reason**
- 0A is a same-responsibility duplication that increases every future UI maintenance cost.
- 0B is a simulation-causality bug that would make Shelter/migration/environment-driven settlement decisions wrong if C1-B were built on top of it.
- Fixing these first is not roadmap drift; it is foundation stabilization.

**Canonical evidence**
- `docs/SOURCE_AUDIT_2026-09-18.md`.

---

## D-016 — Roadmap display is grouped into four macro stages C / D / E / F

**Decision**
- The development roadmap is presented in four large stages for clarity.
- This is a naming/execution organization change only. Previously planned work is not removed.

**Mapping**
- **Stage C — Settlement, Survival & Early Civilization**
  - former C1-A~C1-F.
- **Stage D — Long-Run Simulation & Civilization Engine**
  - former C2 + C3.
- **Stage E — Human Society, Health, Education, Economy & Migration**
  - former C4 + C5 + C6.
- **Stage F — Historical Civilization to Open Future**
  - former F1~F8.

**Rules**
- Detailed acceptance criteria remain in the roadmap and are implemented in manageable PR-sized slices.
- A macro stage must not become one giant PR.
- C/D/E/F labels must never become forced Core era/time unlocks.
- Presentation, PIE/device QA, Android delivery, CI and audit follow-ups remain cross-cutting tracks.

**Immediate application**
- Current work is **Stage C / C-S1 Autonomous Settlement Need Recognition**.

---

# 2026-09-19

## D-017 — Windows PC는 Unreal cinematic renderer를 적극 활용하고 Android와 렌더링 티어를 분리한다

**결정**
- Android가 첫 실제 제품 타깃이라는 원칙은 유지한다.
- 그러나 Android 성능 예산을 Windows PC 그래픽의 품질 상한으로 사용하지 않는다.
- 동일한 Core/World/Save truth를 두고 Windows PC와 Android의 Presentation renderer tier를 분리한다.
- Windows PC 기본 고품질 경로는 DX12 + SM6 + Lumen GI/Reflections + Virtual Shadow Maps + TSR + Nanite eligible assets를 사용한다.
- PC Lumen은 우선 Software Ray Tracing을 baseline으로 하며 Hardware Ray Tracing은 이후 선택형 Ultra tier로 둔다.
- Android baseline은 Lumen/Nanite/VSM에 의존하지 않고 conventional LOD/HLOD/instancing/mobile shading으로 동작해야 한다.
- Nanite를 사용하는 PC asset도 Android fallback mesh/LOD를 잃으면 안 된다.
- Engine primitive 또는 명백한 prototype/low-poly geometry를 production local-view의 자동 fallback으로 사용하지 않는다.

**구현 기준**
- canonical companion: `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`.
- `Config/Windows/WindowsEngine.ini`와 `Config/Android/AndroidEngine.ini`에서 platform renderer contract를 분리한다.
- `Config/DefaultEngine.ini`는 universal Android-safe baseline을 유지한다.
- 그래픽 표현은 Presentation이며 simulation/resource/facility/weather truth를 복제하거나 수정하지 않는다.

**이유**
- Unreal Engine을 선택한 핵심 가치 중 하나인 실시간 고품질 렌더링 능력을 활용하면서도 Android-first delivery와 동일한 자율 시뮬레이션을 유지하기 위함이다.

---

## D-018 — Unreal PCG와 Water System을 자연환경 presentation 기반으로 사용한다

**결정**
- LifeLens는 Unreal Engine 5.6 내장 `PCG`와 `Water` plugin을 project-level environment tool로 활성화한다.
- PCG는 나무/풀/바위/ground cover/biome dressing 같은 procedural **presentation placement**에 사용한다.
- PCG가 gameplay resource나 facility의 존재를 독자적으로 결정하지 않는다. interactable/resource state는 Core/World authority를 따른다.
- Water System은 강/호수/해안/바다의 mesh/shading/spline presentation에 사용하지만, 수계의 존재/종류/염도/흐름/가용성은 authoritative Hydrology read model을 따른다.
- Android에서 runtime PCG/고급 water rendering 비용이 크면 bake/cache/LOD/단순 material path로 낮출 수 있다. 동일 world truth는 유지한다.
- Experimental PCG extension/GPU 기능은 별도 validation 전에는 필수 production dependency로 두지 않는다.

**이유**
- 직접 만든 단순 scatter/cube 기반 환경을 계속 확장하는 대신 Unreal-native world-building 기능을 활용해 품질과 유지보수성을 동시에 높이기 위함이다.
- cinematic PC와 mobile Android의 표현 수준을 분리하면서도 한 개의 deterministic LifeLens world를 유지하기 위함이다.

**Canonical presentation doc**
- `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`
- `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`


## D-019 — PC 버전은 Windows와 macOS를 함께 진행한다

**결정**
- LifeLens의 PC 제품 타깃은 Windows 고정이 아니라 **Windows + macOS**다.
- Android와 PC를 별도 게임으로 만들지 않는 기존 원칙과 동일하게, Windows와 macOS도 같은 Core/World/Save truth와 같은 PC 기능 로드맵을 공유한다.
- 플랫폼별 renderer capability 차이 때문에 설정값은 달라질 수 있지만, 기능 진행/문서/QA에서 macOS를 후순위 임시 플랫폼으로 취급하지 않는다.
- Windows는 DX12 + SM6 + Lumen + VSM + TSR + Nanite baseline을 유지한다.
- macOS broad baseline은 UE 5.6의 지원 범위를 기준으로 Lumen software GI/reflections + TSR + Mesh Distance Fields를 사용한다.
- UE 5.6에서 Nanite/VSM은 Apple Silicon M2+ Beta이므로 모든 Mac의 필수 baseline으로 강제하지 않는다. 실제 M2+ 장비 profiling 후 선택형 Mac High tier로 확장한다.
- Desktop presentation code는 기능상 이유가 없으면 `PLATFORM_WINDOWS` 전용으로 작성하지 않고 Windows/macOS를 함께 고려한다.

**구현 기준**
- `Config/Windows/WindowsEngine.ini`
- `Config/Mac/MacEngine.ini`
- `Config/DefaultDeviceProfiles.ini`
- `docs/CINEMATIC_RENDERING_STRATEGY_v1.md`

**QA**
- compile green만으로 PC 그래픽 완료로 간주하지 않는다.
- Windows와 Mac 각각에서 runtime renderer diagnostics + 실제 화면 QA를 수행한다.
