# Dagyeom Presentation Handoff — 2026-09-18

> 목적: 쭌 측에서 Dagyeom 소유 Presentation 영역을 **assist 형태로 대신 진행/통합한 범위**를 한 곳에 정리해, Dagyeom이 이미 main에 들어간 기능을 다시 만들거나 오래된 PR로 되돌리지 않게 한다.
>
> Truth order: actual GitHub main / PR / Actions > this handoff > old branch assumptions.
>
> Current main at handoff creation: `ed124a6e68c738793d1989e6063db6c737e86f19`.

---

## 1. Ownership은 바뀐 것이 아니다

기본 ownership은 그대로다.

- Dagyeom: Character presentation / Animation / Observer UI / Camera / Environment visual / WorldPresentation.
- Jjun: Core / AI / Simulation / World authority / Save / Bridge / CI / Android.

아래 PR들은 Dagyeom lane을 영구적으로 Jjun이 가져간다는 의미가 아니다.
사용자 요청에 따라 화면 catch-up을 빠르게 끝내기 위해 **temporary cross-lane assist**로 구현/통합한 것이다.

원칙:
- 앞으로 같은 책임의 수정은 원래 canonical file에서 계속한다.
- 충돌을 피하려고 duplicate HUD / duplicate presentation actor / 우회 source를 새로 만들지 않는다.
- old Dagyeom branch를 whole-file로 현재 main 위에 덮지 않는다.

---

## 2. Jjun assist로 이미 main에 들어간 Dagyeom-lane 작업

### #139 Dynamic Environment Presentation Foundation
Merged: `834c70b2cf4a0f9193d9473c26e737e4752875bb`

Touched:
- `LLDynamicEnvironmentPresentationActor.*`
- `LLLifeLensGameMode.cpp`

Result:
- Core dynamic environment DTO를 실제 WorldPresentation consumer에 연결.
- Presentation이 자체 weather truth를 만들지 않음.

### #140 Visual Catch-up v2
Merged: `576d266015a312afd13384a9f39829cb3f3a0ecd`

Touched:
- `LLDynamicEnvironmentPresentationActor.*`
- `LLWorldPresentationActor.h`
- Unreal module/project metadata.

Result:
- surface wet/snow feedback.
- Niagara hooks.
- post-process weather feedback.
- Android-oriented HISM budget/cull tuning.

### #142 Observer Runtime Chrome v1
Merged: `87b384163b4d73fe2dd8a15f41124ca5c5a76169`

Touched:
- `LLRuntimeObserverHUD.*`
- `LLObserverHUD.h`
- `LLLifeLensGameMode.cpp`

Result:
- runtime top chrome.
- authoritative date/time/season/weather/temp.
- 0x/1x/4x/16x/64x controls.

### #143 Character Context Motion v2a
Merged: `6dccc08f16d83df9d246275aea2d15791faac639`

Touched:
- `LLResidentMotionComponent.*`

Result:
- Gather / Dig / Strike 및 held-tool 기반 contextual motion foundation.

### #144 Observer Detail Scrolling v1
Merged: `1f871077083679d09bd1baa3d0c18973eafc604f`

Touched:
- `LLObserverHUD.*`
- `LLObserverPlayerController.*`

Result:
- PC mouse wheel detail scrolling.
- Android one-finger drag scrolling.
- active detail scroll 중 camera gesture 억제.

### #146 Lifecycle Presentation v1
Merged: `2c6f73dc0875dcade04ac88cc20500abd9bf9de1`

Touched:
- `LLLifecycleEventOverlay.*`

Result:
- birth / growth / pregnancy / death live notices.
- stale save history를 새 event처럼 재생하지 않는 baseline rule.

### #147 Observer Time / Weather / Speed Controls v1
Merged: `ab08fa52070fa0cc0498d71db08519f17007e954`

Touched:
- `LLObserverTimeWeatherOverlay.*`

Result:
- 별도 observer overlay에서도 authoritative time/weather/speed consumer 제공.
- Core time/weather를 재계산하지 않음.

### #148 Dynamic Observer Canopy Visibility v1
Merged: `8a65d172af793ac4999e033c3a6e6231c1c0b343`

Touched:
- `LLWorldPresentationActor.*`

Result:
- 현재 camera -> resident sight corridor의 ambient canopy만 가역적으로 축소/복원.
- resource-patch authoritative tree는 이 처리에서 제외.

### #149 Visual Catch-up v3
Merged: `5c20664e7c854f9c2df33b29c03e38ac8cb00bd5`

Touched:
- `LLDynamicEnvironmentPresentationActor.*`
- `LLWorldPresentationActor.*`

Result:
- authored Niagara asset이 없을 때 packaged build에서도 보이는 lightweight rain/snow fallback.
- authored Niagara가 있으면 기존 Niagara path가 우선.

### #150 Lifecycle Presentation v2
Merged: `3a42d9260fb8054fb6abe97d6405b9447da41612`

Touched:
- `LLLifecycleEventOverlay.*`

Result:
- selected resident persistent family/lifecycle card.
- partner/cohabitation/pregnancy/children/parents/siblings/household.
- 이번 관찰 세션에서 실제 관측된 lifecycle event timeline.
- 과거 LifeHistory를 Presentation이 추정하지 않음.

### #151 Observer Adaptive Information Density v1
Merged: `e111f7d1e84b55fbee28533553021dc893c674a6`

Touched:
- `LLObserverHUD.cpp`

Result:
- 좁은 모바일 viewport / 먼 camera에서 overview information density 자동 축약.
- 넓거나 가까운 view에서는 richer action summary 유지.

---

## 3. Dagyeom이 직접 만든 작업 중 이번에 통합된 것

### #145 Character Context Motion v2
Author: `STILLofficial`
Merged: `6d6be5f034380987f8e04671d1d2e860c59e5deb`

Jjun 쪽에서 한 일:
- exact-head Preflight / Unreal Compile 확인.
- actual main changes와 file overlap 확인.
- review APPROVE.
- 최신 main과 mergeability 재확인 후 merge.

Dagyeom 구현 내용:
- pending ContextAction directive 우선 read.
- Talk / Learn / GatherPick / StrikeSwing / DigWork / CraftWork / HaulPush / FireTend / CrouchLow / SeatedCare 등의 motion classification.
- #143 Gather/Dig/Strike foundation 보존.

Known quality gaps:
- dedicated sleep lying clip 없음.
- eat/drink/carry-walk 품질은 후속 개선 대상.
- PutToSleep는 현재 seated-care fallback.
- runtime PIE visual QA는 별도 품질 작업으로 남을 수 있음.

이 PR은 **이미 merged**이므로 동일 기능을 새 branch에서 처음부터 다시 만들지 않는다.

---

## 4. PR #98 current-main integration — RESOLVED

PR #98:
`[UI] Observer 가독성 개선 + QA 관찰 명령`

Resolution:
- 원본 #98은 current main보다 크게 뒤처져 wholesale merge하지 않았다.
- 유효한 intent를 최신 main에서 #202로 선택 포팅했다.
- #202는 exact-head Preflight + Unreal Linux Compile PASS 후 merge됐다.
- original #98은 superseded로 close했다.

Integrated through #202:
- panel opacity/readability tuning.
- QA-only `ll.ViewResidents [index]`.
- QA-only `ll.ViewReset`.
- current production observer camera와 QA CameraActor의 tick ownership 충돌 방지.

Status:
- **RESOLVED VIA #202 / ORIGINAL #98 CLOSED**

#98 touched:
- `LLObserverHUD.cpp`
- `LLObserverPlayerController.cpp`
- `tasks/HANDOFF_LOG.md`
- `tasks/TEAM_BOARD.md`

이미 current main에 들어간 관련 변화:
- #144 scrolling.
- #151 adaptive information density.
- 여러 후속 Observer changes.

따라서 #98의 old HUD/controller snapshots를 wholesale merge하면 current-main 기능을 되돌릴 위험이 있다.

#98에서 살릴 후보였던 항목은 #202에서 이미 current-main 방식으로 반영 완료되었다.
동일 기능을 다시 구현하거나 original #98을 reopen/whole-file merge하지 않는다.

---

## 5. Dagyeom이 다시 만들 필요 없는 항목

이미 main에 있으므로 "미구현"으로 취급하지 않는다.

- time/weather/speed observer UI.
- real detail scrolling.
- lifecycle live notices.
- selected-resident lifecycle/family card.
- observed lifecycle timeline.
- adaptive HUD density.
- current camera canopy readability.
- weather presentation foundation.
- packaged rain/snow fallback.
- context motion v2 baseline.

---

## 6. Dagyeom Presentation에서 남은 실제 품질 작업

이제 primary blocker가 아니라 iterative quality lane이다.

Character:
- dedicated lie/sleep/wake.
- carry-walk.
- eat/drink.
- better child/life-stage body proportions and motion.
- animation transition polish.

Observer:
- #98 panel opacity / QA view salvage는 #202로 완료.
- device safe-area/layout QA.
- camera/readability visual QA.
- full historical LifeHistory/deceased presentation once Core read model exposes enough data.

Environment:
- authored Niagara rain/snow/fog systems.
- higher-quality wet/snow materials.
- FirePit/Furnace light/smoke polish.
- Android-specific visual LOD/performance validation.

C1 consumers:
- WorkSurface / SleepingPlace / Shelter presentation should consume Core facility authority as C1-B/C1-C contracts land.
- Presentation must not spawn or complete facilities.

---

## 7. Current Core direction Dagyeom should design against

Current functional mainline:
- #152 C1-A Settlement Facility Authority merged.
- next = **C1-B Autonomous Settlement Need Recognition**.

Expected provider flow:
`Need / environment / crafting pressure`
-> Core Utility decision
-> authoritative facility Plan / DeliverMaterial / Work directive
-> real spatial ContextAction
-> World/Character execution
-> Core ACK
-> operational facility state
-> Presentation refresh.

Dagyeom should render this chain, not invent a parallel build state.

---

## 8. Where to look first after returning

1. `docs/PRESENTATION_WORK_STATE_2026-09-18.md`
2. `PROJECT_STATUS.md`
3. `tasks/WORK_STATE.md`
4. this file
5. `docs/DEVELOPMENT_MILESTONES.md`
6. actual GitHub open PRs / main
7. `tasks/TEAM_BOARD.md` only for ownership/locks/IR.

If any old HANDOFF/TEAM_BOARD line conflicts with actual main or this handoff, **actual GitHub main wins**.
