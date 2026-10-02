# 주민 모션·소지품·유전 외형 표시

기준 main: `03a7866ef63c367082d2df0bb3ed05dc1bf97a26`. PR #590, Web presentation 전용.

## 원인과 수정

- 재배의 Plant/Water/Tend/Harvest와 Trade가 semantic motion 선택기에 빠져 있었다. 실제 Interacting/목적지/상대가 확인되는 경우에 연결한다.
- 실제 용변 emergencyFallback + target이 있으면 crouch, 자연 수원 직접 행동이 아닌 실제 Wash 목적지에서는 interact를 사용한다. 사실이 없는 행동은 idle 유지.
- 기존 운반은 DeliverMaterial 문자열만으로 빈손 carry를 선택했다. 이제 실제 인벤토리의 해당 자재와 손 attachment가 확인되어야 carry를 선택한다. Store 이동도 같은 조건을 적용한다. Gather/Retrieve를 아직 획득하지 않은 자재의 운반으로 꾸미지 않는다.
- 물주기는 CultivatedPlot/Water/Interacting과 실제 물 및 SimpleContainer를 모두 확인해야 Farm_Watering과 손의 용기를 함께 표시한다. 정보가 없으면 neutral hand interaction으로 내려간다.
- 이전 모션 재진입은 중간 재생 시간을 이어 받아 전환이 어색했다. 새 행동 선택 시 reset 뒤 fade-in한다. 수면의 기존 안정된 누운 자세/frozen mixer는 보존한다.

| Core 사실 | 표시 |
|---|---|
| CultivatedPlot / Plant | Farm_PlantSeed |
| CultivatedPlot / Water + 실제 물·용기 | Farm_Watering + 손 용기 |
| CultivatedPlot / Harvest | Farm_Harvest |
| CultivatedPlot / Tend | 기존 Fixing_Kneeling 근사 자세 |
| Trade + 가까운 실제 상대 | 기존 Interact |
| 실제 자재를 보유한 DeliverMaterial/Store 이동 | Walk_Carry_Loop + 손 사이 자재 표본 |
| 양수 inventory item | 허리 주변 소지품, 최대 3종 |

지원 소지품: DiggingStick, StoneCuttingTool, StoneHammer, BronzeAxe, BronzePick, SimpleContainer, SharpFlake, Cordage, FuelBundle, RecordTablet.
지원 운반 자재: Wood, Stone, Flint, Clay, PlantFood, Water. 다른 자재는 미확인 모양을 만들어 넣지 않는다. 도구의 소지는 실제 장착/사용을 뜻하지 않는다. 내구도 0인 도구도 양수 inventory면 소유물로 존재한다. 수량이나 퀄리티의 변화를 장식으로 위조하지 않는다.

## 외형 데이터

- heightPotential → 연령대별 키 범위, buildPotential → 폭/두께.
- skinTone/hairPigment/eyePigment → 연속 색상, faceShape → 작은 머리 폭 변형.
- sex → 기존 평균 키/폭 + 제한된 어깨·골반 비율. 복장 색상은 성별과 독립된 ID seed이다.
- 유효한 0..1 유전 수치를 사용하며 범위를 clamp한다. 미관찰/NaN은 기존 ID fallback을 유지한다.
- 처음 경량 snapshot 이후 전체 genetics가 와도 appearance를 갱신한다. source geometry/material에서 다시 적용해 누적 변형/색 오염을 막는다. 움직이는 bone의 현재 위치를 rest-mesh 변형 기준으로 쓰지 않는다.
- 기본 리그/mesh는 기존 Quaternius SuperHero_Male 공유 모델이다. 여성 전용 mesh, 얼굴 morph target, 실제 머리카락 mesh를 새로 확보한 것은 아니다. 비율/색상 반영은 개선했으나 남녀 전용 모델 교체를 완료했다고 해석하지 않는다.

## 무료 자산 및 출처

외부 자산은 모두 기존 승인된 Quaternius CC0 1.0이다. `Tools/prepare_web_character_assets.py`는 고정 commit의 GLB만 받아 Git blob SHA-1, GLB 구조, 외부 buffer/image 미참조를 검사한다. receipt에 SHA-256/bytes/clip/node 목록과 source URL을 남긴다. `LICENSE.txt`도 함께 배포한다.

| 파일 | pinned mirror commit | Git blob |
|---|---|---|
| character.glb | programasweights/avatar @ ddd5fc34a445bcded3cf9836607aaeebc19a5c78 | b3fd79533fdb9fcedd077744f7e120920eb6cc97 |
| ual1.glb | Seyamalam/blood-league-kickoff @ aa02a4e6d8337a0604d2da131bcbbeb1f01badf0 | 4fccf561b9b2ef73f611efe21981ef8739080065 |
| ual2.glb | richardanaya/metaverse-avatar @ 84fd636910bf713099010efbab7f3c84550f4bcb | dc684c2a664927964307e8eb7b27b0000ebf6a18 |

원 라이선스 근거: 첫 mirror의 `public/assets/QUATERNIUS-LICENSE.txt`, UAL1 mirror의 `public/assets/vendor/quaternius/LICENSE-ANIMATIONS.txt`, UAL2 mirror의 `ASSET_LICENSES.md`. UAL2 mirror에 같이 있는 Ruth2 body/head는 AGPL이므로 가져오지 않는다.

다운로드 크기 총 11,517,676 bytes. 바이너리를 Git history에 넣지 않고 prebuild에서 검증해 Vite public 경로에 포함한다. runtime은 같은 origin `/vendor/characters/` 우선이며 누락된 개발 환경에서는 기존 pinned remote를 fallback으로 쓴다. 빌드에는 Python3와 공개 raw GitHub 접근이 필요하며 hash 불일치는 실패 처리한다.

도구·용기의 작은 mesh는 이 PR에서 작성한 procedural geometry이며 외부 다운로드 자산이라고 표시하지 않는다. game-dev CLI가 없는 환경이므로 해당 skill의 package admission 인증을 받은 것으로 주장하지 않는다.

## 검증 재현

```sh
python3 Tools/prepare_web_character_assets.py
cd web
npm install --ignore-scripts --no-audit --no-fund
npm run typecheck
node tests/characters/run.mjs
node tests/action-context/run.mjs
npm run build
node tests/characters/export-review.mjs
blender --background --python tests/characters/render-review.py
```

마지막 두 명령은 실제 pinned GLB의 리그/모션을 로드하여 16개 포즈를 bake하고 검토용 contact sheet를 만든다. 게임에 검증용 주민을 주입하지 않는다. `review/`는 Git에 포함하지 않으며 CI는 poses.json을 7일 artifact로 보존한다. Blender CPU 렌더는 WebGL 조명/기기 프레임 성능 검증을 대신하지 않는다.

## 남은 한계

- 도끼질/굴착 도구의 실제 active equipment/접촉 대상 계약이 없다. Wood라는 이유만으로 TreeChopping이나 검 공격을 연결하지 않았다.
- Tend는 전용 손 제초 모션이 아닌 기존 kneeling 근사 자세다. 수면의 enter/loop/wake 전체 authored sequence도 이번 범위에서 새로 만들지 않았다.
- inventory를 알 수 없으면 도구/자재를 보여 주지 않는다. 관측 범위를 확대하려면 Core/DTO 담당 작업과 별도 조율해야 한다.
- 소지품은 pelvis 기반 부착, 물주기/운반물은 손 위치 기반 부착이다. 정교한 손가락 IK/그립 리타깃과 여성 전용 모델은 후속 자산 작업이다.
- 이번 변경은 주민의 판단/생존 반복/장기 고착을 해결한 것이 아니다. 그 축은 #589 및 balance audit 소유 범위다.
