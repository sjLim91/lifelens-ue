# 주민 행동 모션 완성도 패스 — 2026-10-02

작업 시작 main `70969179766369cccc912b3cdc66d2b8d5fdf0d1` (#607); PR 작성 전 main `558eabd3eafee1adc8120504dc49a44c2eadded6` (#610) 재확인. 이 사이 Core 파일 3개 변경과 이번 변경은 겹치지 않는다. Source/LifeLensCore/**, AI/Needs/자원/사회/이동 authority, WASM DTO는 수정하지 않는다. PR만 작성하며 merge하지 않는다.

## 1. 기존 문제 원인

- 실제 UAL2 GLB에는 `LayToIdle`이 있었지만 sleep 코드와 과거 구조 검사는 수면 클립이 없다고 가정했다. Idle를 정지하고 발 피벗의 visual을 86도 회전한 후 신체 두께 비율로 들어 올렸다. 이는 실제 skinned mesh 하단이나 진입/종료 포즈를 반영하지 못했다.
- Gather는 PlantFood 외에는 Interact였고, Craft/Experiment/Work/Repair도 같은 work였다. Interact의 실제 포즈는 가리키는 제스처여서 일반적인 재료 손 작업으로 확장하기 부적절했다.
- Walk의 local playback rate와 mixer clock 양쪽에 simulation speed가 들어가, 실제 이동 속도에 비해 보폭 주기가 과도하게 빨라질 수 있었다. 저장된 이동 속도는 다음 위치 snapshot까지 배속 변경을 반영하지 않았다. pause에도 잔여 보간 속도로 이동할 수 있었다.
- update에서 target Vector3를 매 프레임 clone했고, 과거에 한 번 보인 주민의 skeleton/action을 무제한 보존했다. 물주기 소품은 Core Interacting이 먼저 도착하면 시각적 도착 전 기울어진 용기를 보여 줄 수 있었다.

## 2. 행동 → semantic motion → 실제 포즈

이동 및 Core Moving이 항상 우선한다. stationary 행동은 active/Interacting에서만 시작하며 사회 행동은 실제 가까운 target resident가 필요하다. 클립은 정확한 이름으로 선택하며 substring 검색으로 무기/죽음 클립을 끌어오지 않는다.

| Core 행동 | semantic | 실제 motion / 안전한 근사 |
|---|---|---|
| Moving | walk | Walk_Loop |
| DeliverMaterial/Store/Trade 이동 + 실제 운반물·손 attachment | carry | Walk_Carry_Loop; 없으면 Walk_Loop |
| Sleep | sleep | LayToIdle 역방향 진입, 0초 누운 포즈 유지, 정방향 기상 |
| Eat / Drink | consume | Consume; 없으면 Interact |
| 자연 수원 Wash + 실제 target grid | wash | crouch 하체 + PlantSeed 손 작업; UAL2 없으면 Fixing 손 작업 |
| Sink/소지한 물 Wash + 실제 target | interact | 기존 중립적인 hand interaction 근사 |
| 실제 Toilet / 지정 위생 장소 / target 있는 emergencyFallback | crouch | Crouch_Idle_Loop |
| Gather Wood | gatherWood | Idle 하체 + Harvest 상체, 작은 팔 방향 조정: 서서 손으로 가지/떨어진 나무 수집 |
| Gather Stone / Flint / CopperOre / TinOre | gatherMineral | Fixing_Kneeling: 무릎을 대고 손으로 채취 |
| Gather Clay | gatherClay | Farm_PlantSeed: 땅을 짚고 채취, 0.8 tempo |
| Gather Fiber | gatherFiber | Crouch 하체 + Harvest 상체: 낮은 식물 채집 |
| Gather PlantFood | harvest | Farm_Harvest 유지 |
| facility Work / 건설 | construct | Fixing_Kneeling |
| facility Repair | repair | Crouch 하체 + Fixing 상체, 0.8 tempo: 쪼그린 수리 자세 |
| Craft | craft | Idle 하체 + Fixing 상체: 서서 손 작업 |
| Experiment | experiment | Idle_FoldArms_Loop: 대상 관찰/검토 근사 |
| Fuel | fuel | Crouch 하체 + PlantSeed 상체: 낮은 곳에 넣는 손 작업 |
| Ignite | ignite | Crouch 하체 + Fixing 상체, 0.6 tempo: 낮은 곳에서 손 작업 |
| LoadSmeltCharge | loadFurnace | Crouch + PlantSeed, 0.9 tempo |
| CollectCharcoal / CollectMetal | collect | Farm_Harvest: 낮은 곳에서 회수 |
| Store / DeliverMaterial 도착 | store | Farm_PlantSeed: 낮은 저장소에 놓는 근사 |
| Retrieve | retrieve | Farm_Harvest: 낮은 저장소에서 집는 근사 |
| CultivatedPlot Plant | plant | Farm_PlantSeed |
| CultivatedPlot Water + 실제 물·용기 | water | Farm_Watering + 기존 실제 inventory 용기 |
| CultivatedPlot Tend | tend | Crouch 하체 + Harvest 상체 |
| CultivatedPlot Harvest | harvest | Farm_Harvest |
| 일반 Social / Approach | talk | Idle_Talking_Loop |
| Social Comfort | comfort | Yes: 절제된 동의/반응 gesture, 0.65 tempo |
| Social Repair | reconcile | Idle_Talking_Loop, 0.65 tempo |
| KnowledgeTeaching / Parenting Educate·Discipline | teach | Idle 하체 + Interact 상체: 실제 상대를 향한 설명 gesture |
| Parenting Feed·Bathe·ToiletAssist·Hold·HealthCare | care | Crouch 하체 + PlantSeed 상체: 낮은 상대에게 손을 내미는 근사 |
| Parenting Comfort·PutToSleep | comfort | Yes, 0.65 tempo |
| Parenting Play | talk | Idle_Talking_Loop |

독립적인 의료/육아/실험/세척 authored clip은 없다. 표의 근사는 directive의 행동을 읽기 쉽게 하는 자세이며 실제 아이를 들어 올림, 도구 장착/사용, 점화 성공, 물질 생산 등의 새 사실을 만들지 않는다. 특히 TreeChopping_Loop은 발견했지만 장착·사용 authority가 없어 선택하지 않는다. 기존 inventory 소품은 실제 양수 소지량에만 근거한다. 무기 공격/Death/table 높이 pickup을 지면 채취로 사용하지 않는다.

## 3. Sleep 구현

실제 GLB 및 Blender 포즈 검사를 통해 LayToIdle의 0초는 지면에 누운 자세, 마지막 1.5333333초는 서 있는 자세임을 확인했다. visual hierarchy 회전 없이 skeleton을 실제 포즈로 움직인다. 진입은 시간을 역으로 진행하고 수면 중 action time=0을 정확히 고정한다. 기상은 정방향으로 진행한 뒤 support offset을 천천히 해제한다. 모든 다른 action을 진입 시 stop하므로 수면에 locomotion이 섞이지 않는다. 수면/기상 동안 root yaw를 고정하며 Core 위치의 보간은 여전히 Core target에만 따른다.

공유 모델에 대해 97개 sample의 실제 skinned vertices를 한 번 측정한다. 하단 Y와 X/Z 중심 보정표로 지면 침투 및 clip body drift를 제거한다. per actor/per frame bounding-box/vertex 검색을 하지 않는다. 작은 외형 변형·표본 보간을 위한 0.045 world-unit clearance를 둔다. operational SleepingPlace는 실제 bedding 높이 0.41을 사용하고, ground/shelter는 footprint 3×3 terrain 표본의 최고 높이에 맞춘다. support는 수면 진입 시 latch하여 반복 snapshot에 의해 꿀렁거리지 않는다.

UAL2 실패 시 pinned LayToIdle의 실제 두 endpoint pose를 가진 42,276-byte local TS derivative를 쓴다. 전환은 두 실제 pose 간의 단순 보간이며 native 기상 동작보다 덜 정교하지만 실제 누운 포즈를 유지한다. 두 library 모두 실패하면 동일 derivative의 standing endpoint가 neutral idle fallback이 된다. 특정 action은 의미가 가까운 UAL1 clip으로 내려가고 layer는 계속 렌더링한다.

## 4. 이동·facing·#605 호환

현재 interpolation budget, walk grace, yaw smoothing, gait bias는 유지한다. 이동 budget을 1× 값으로 저장하여 preset 변경을 즉시 반영하고 playback의 local gait rate에서 선택된 simulation speed를 한 번 제거한다. mixer 시간과 실제 이동은 같은 1×/4× 배율을 갖는다. pause에서는 둘 다 정지한다. 큰 방향 반전은 먼저 회전하여 뒤로 걷는 translation을 억제하고 gait는 실제 표시 속도로 계산한다.

Social은 실제 target actor, Gather는 directive가 가리키는 실제 ResourceNode center, 시설/보관은 실제 facility/storage ID의 Core 좌표를 바라본다. ResourceNode의 accessGrid는 이동 위치로 그대로 유지한다. Node가 없으면 Core target grid만 사용하고, target도 없거나 자기 위치와 같으면 방향을 만들지 않는다. 수원 행동도 주어진 target grid만 사용하며 새 수원 위치를 추정하지 않는다. #605 vegetation/ground patch/quantity projection과 resource proxy 파일은 변경하지 않았다. Wood 둥근 proxy를 추가하지 않았다.

기존 한글 action cue와 사회 connector 의미/색을 보존한다. connector와 pouring prop도 실제 시각적 도착 전에는 진행 중 표시를 억제한다.

## 5. 자산·라이선스·재현

추가 외부 다운로드 자산 없음. 기존 Quaternius CC0-1.0 UAL1/UAL2/character만 사용한다. sleep endpoint derivative와 composed motions도 이 고정 CC0 데이터에서 얻은 presentation 산출물이다.

- UAL2 공식 출처: https://quaternius.com/packs/universalanimationlibrary2.html
- 라이선스: https://creativecommons.org/publicdomain/zero/1.0/
- character mirror: programasweights/avatar @ ddd5fc34a445bcded3cf9836607aaeebc19a5c78, blob b3fd79533fdb9fcedd077744f7e120920eb6cc97
- UAL1 mirror: Seyamalam/blood-league-kickoff @ aa02a4e6d8337a0604d2da131bcbbeb1f01badf0, blob 4fccf561b9b2ef73f611efe21981ef8739080065
- UAL2 mirror: richardanaya/metaverse-avatar @ 84fd636910bf713099010efbab7f3c84550f4bcb, blob dc684c2a664927964307e8eb7b27b0000ebf6a18

기존 pipeline `Tools/prepare_web_character_assets.py`가 Git blob, GLB 및 external URI를 검사한다. derivative 재현은 `node Tools/extract_web_sleep_pose.mjs`; `--check`는 파일을 덮어쓰지 않고 검증한다. 새 유료 API/자산/런타임은 없다.

## 6. 성능 영향

actor당 기존 12개 슬롯 대신 실제 unique action은 최대 20개다. semantic alias는 같은 AnimationAction을 공유한다. library composition과 sleep calibration은 layer 생성 시 한 번, action binding은 actor 생성 시 한 번 수행한다. 주민별 asset 다운로드나 추가 캐릭터 skeleton은 없다. calibration용 임시 skeleton clone 1개는 측정 후 유지하지 않는다.

update의 Vector3 clone을 제거했다. 32명에 동일 snapshot/update를 반복했을 때 actor/mesh/action 및 다운로드 횟수는 증가하지 않았다. 보이지 않는 skeleton cache는 최대 32개이고 eviction/dispose에서 mixer root, 소품 geometry/material, cue texture를 해제한다. Node CPU update 측정 32명은 median 약 0.55–0.58 ms, p95 약 0.75–1.39 ms였다. 이 측정은 모바일 GPU/브라우저 프레임 성능을 입증하지 않는다. GLB 3개의 기존 다운로드 크기 11,517,676 bytes는 유지되며 local derivative만 추가된다.

## 7. 검증 및 한계

- 실제 pinned GLB 3개 local hash 검사, 정확한 clip 목록 전수 확인.
- TypeScript typecheck, Vite production build PASS.
- 신규 실제 AnimationMixer/clone skeleton 회귀 15개 PASS: 전체 semantic matrix, deterministic 반복, 도착 guard, 물주기 소품 guard, social/resource facing, ground/bedding/slope/age·외형 extrema sleep, wake continuity, pause, heading reversal, 1×/4×·carry tempo, action/mesh/download cache, inactive cache eviction, UAL2/모든 library 실패.
- 기존 character 10개, action-context 29개, presentation 64개 PASS. 주민 모션/행동/공간/시설 structural checks PASS.
- 실제 skinned mesh의 motion contact sheet를 Blender CPU 렌더하여 수면·작업 자세를 검토했다. `node web/tests/resident-motion/run.mjs` 후 `blender --background --python web/tests/resident-motion/render-review.py`로 재생성한다. poses.json은 CI artifact로 보존하고 review/는 Git에 넣지 않는다.
- 전용 mining/wood hand gathering/repair/care authored mocap, hand IK, 상대 신체를 실제로 잡는 연출은 추가하지 않았다. native clip composition과 tempo/stance 구분이다. 특히 전용 실험 동작은 observation 근사다.
- 실제 모바일 WebGL·게임 화면의 최종 시각/터치/GPU 검수는 아직 수행하지 않았다. CPU contact sheet와 tests를 그 검수로 보고하지 않는다.

## 8. 변경 파일·충돌 가능성

핵심: resident-world-layer.ts, resident-semantic-motion.ts, resident-motion-library.ts, resident-sleep-motion.ts, resident-sleep-pose.ts, resident-props.ts. 최소 연결: world-scene.ts, lifelens-contract.ts의 obsolete sleep constants. 검증: resident-motion tests/renderer, 기존 action-context/presentation fixtures, resident/action structural validators, character-assets-check workflow, endpoint extractor 및 기존 character pose exporter의 world-presentation-config ESM dependency 처리. 문서: 이 문서와 전용 recovery 파일.

기존 main/다른 Web presentation 작업과 충돌 가능성이 큰 파일은 resident-world-layer.ts, resident-semantic-motion.ts, resident-props.ts이고 shared 연결은 world-scene.ts, lifelens-contract.ts, character-assets-check.yml이다. Source/LifeLensCore/** 및 공통 tasks/WORK_STATE.md는 변경하지 않는다.

## 9. 실제 발견한 전체 animation clip 목록

아래는 추측한 이름이 아니라 pinned binary GLB JSON에서 추출하고 local Git blob hash로 검증한 목록이다.

### character.glb — 0 clips

애니메이션 없음.

### ual1.glb — 43 clips

`A_TPose`, `Crouch_Fwd_Loop`, `Crouch_Idle_Loop`, `Dance_Loop`, `Death01`, `Driving_Loop`, `Fixing_Kneeling`, `Hit_Chest`, `Hit_Head`, `Idle_Loop`, `Idle_Talking_Loop`, `Idle_Torch_Loop`, `Interact`, `Jog_Fwd_Loop`, `Jump_Land`, `Jump_Loop`, `Jump_Start`, `PickUp_Table`, `Pistol_Aim_Down`, `Pistol_Aim_Neutral`, `Pistol_Aim_Up`, `Pistol_Idle_Loop`, `Pistol_Reload`, `Pistol_Shoot`, `Punch_Cross`, `Punch_Jab`, `Push_Loop`, `Roll`, `Sitting_Enter`, `Sitting_Exit`, `Sitting_Idle_Loop`, `Sitting_Talking_Loop`, `Spell_Simple_Enter`, `Spell_Simple_Exit`, `Spell_Simple_Idle_Loop`, `Spell_Simple_Shoot`, `Sprint_Loop`, `Swim_Fwd_Loop`, `Swim_Idle_Loop`, `Sword_Attack`, `Sword_Idle`, `Walk_Formal_Loop`, `Walk_Loop`

### ual2.glb — 43 clips

`A_TPose`, `Chest_Open`, `ClimbUp_1m_RM`, `Consume`, `Farm_Harvest`, `Farm_PlantSeed`, `Farm_Watering`, `Hit_Knockback`, `Hit_Knockback_RM`, `Idle_FoldArms_Loop`, `Idle_Lantern_Loop`, `Idle_No_Loop`, `Idle_Rail_Call`, `Idle_Rail_Loop`, `Idle_Shield_Break`, `Idle_Shield_Loop`, `Idle_TalkingPhone_Loop`, `LayToIdle`, `Melee_Hook`, `Melee_Hook_Rec`, `NinjaJump_Idle_Loop`, `NinjaJump_Land`, `NinjaJump_Start`, `OverhandThrow`, `Shield_Dash_RM`, `Shield_OneShot`, `Slide_Exit`, `Slide_Loop`, `Slide_Start`, `Sword_Block`, `Sword_Dash_RM`, `Sword_Regular_A`, `Sword_Regular_A_Rec`, `Sword_Regular_B`, `Sword_Regular_B_Rec`, `Sword_Regular_C`, `Sword_Regular_Combo`, `TreeChopping_Loop`, `Walk_Carry_Loop`, `Yes`, `Zombie_Idle_Loop`, `Zombie_Scratch`, `Zombie_Walk_Fwd_Loop`
