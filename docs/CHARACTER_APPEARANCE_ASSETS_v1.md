# LifeLens Character Appearance v1 — Asset / License / Structure Prep

상태: `PREP_ONLY` (ASSIST_LOCK-29-R1 활성 중 병렬 준비 문서). 코드 변경 없음.

기준 문서: `docs/CHARACTER_APPEARANCE_ROADMAP.md`(canonical), `docs/LIFELENS_SPEC_v1.1.md`, `docs/CIVILIZATION_PROGRESSION_v1.md`.

로드맵 5절 원칙 적용: 런타임 비용 0원, 유료 API/클라우드 생성 서비스 미의존, 저장소에 넣기 전 재배포/프로젝트 사용 license 확인, license/provenance 기록, 불명확하면 main에 binary asset 미반입, Core data model을 vendor 포맷에 종속시키지 않음.

## 1. 후보표

조사일: 2026-09-14. "재배포"는 저장소(`Content/Characters/**`)에 원본/변환 에셋을 커밋해 공개 저장소로 배포하는 경우를 뜻한다.

| 후보 | 구성 | 라이선스 (확인 출처) | 상업 이용 | 저장소 재배포 | Android 리스크 | 비고 |
|---|---|---|---|---|---|---|
| MetaHuman (Epic) | 사실형 얼굴/피부/눈/헤어(groom+카드)/기본 의상, MetaHuman 스켈레톤(UE5 Mannequin 호환), LOD 0–7 | Epic 콘텐츠: UE 기반 제품 내 사용 허용, 타 엔진 사용 불가([UE-Only Content 설명](https://forums.unrealengine.com/t/ue-only-content-licensed-for-use-only-with-unreal-engine-based-products/1270173), [Epic Content EULA](https://www.unrealengine.com/eula/content), [MetaHuman](https://www.unrealengine.com/en-US/metahuman)) | UE 제품 내 가능 | 불필요 (Epic 배포 채널에서 프로젝트로 가져옴; 원본을 저장소에 재배포하지 않음) | 높음: 고폴리·머티리얼 슬롯 다수·groom 헤어. 모바일은 LOD 5–7 + 카드 헤어 fallback 필수, 실기기 검증 전까지 미확정 | 로드맵 35행 "전체 런타임을 기본 전제로 삼지 않음"에 따라 조건부 후보. 직접 EULA 본문은 403으로 인용 불가, 2차 출처 기록 |
| Quaternius Universal Base Characters | 남/녀 × Superhero/Regular/Teen 6종, Humanoid rig, 헤어 20종, 눈·피부색 커스텀(Source tier 셰이더), 평균 13k tri, FBX/glTF | CC0 ([팩 페이지](https://quaternius.com/packs/universalbasecharacters.html), [itch.io](https://quaternius.itch.io/universal-base-characters)) | 가능 | 가능 (CC0) | 낮음: 저폴리, 단일 머티리얼 계열, 모바일 적합 | 무료 Standard tier가 모델의 60–70%. 사실형이 아닌 stylized-clean 계열 |
| Quaternius Ultimate Modular Men / Women | 각 10종, 4파트 모듈 교체, humanoid rig, 24 애니, FBX/OBJ/glTF/Blend | CC0 ([Women](https://quaternius.com/packs/ultimatemodularwomen.html), [Fantasy outfits](https://quaternius.com/packs/modularcharacteroutfitsfantasy.html)) | 가능 | 가능 | 낮음 | 의상 모듈·원시/판타지 의상 확장에 유용 |
| Kenney Character Assets | 4 모델, 75 스킨, 40 액세서리, 17 애니, FBX/Blend/PNG | CC0 ([itch.io](https://kenney.itch.io/kenney-character-assets), [kenney.nl/support](https://kenney.nl/support)) | 가능 | 가능 | 낮음 | 블록형 스타일. 사실형 목표와 거리가 있어 fallback/디버그 용도 |
| Blender Studio Human Base Meshes | 사실형·스타일형 남/녀 전신 + 손·발·머리·턱·안구 등 17 메시, 비리깅, .blend | CC0 ([Blender docs](https://developer.blender.org/docs/features/asset_system/asset_bundles/human_base_meshes/), [CG Channel](https://www.cgchannel.com/2023/06/download-blender-studios-free-human-base-meshes/)) | 가능 | 가능 | 중간: 리깅·리토폴로지·텍스처 자체 제작 필요 | 사실형 자체 파이프라인용 base |
| MakeHuman (공식 미수정판) 익스포트 | 파라메트릭 사실형 인체, 커뮤니티 피부/헤어/의상, FBX 익스포트 | 앱 AGPL, 공식 익스포트 산출물 CC0 ([license explanation](http://www.makehumancommunity.org/content/license_explanation.html), [GitHub](https://github.com/makehumancommunity/makehuman)) | 가능 | 가능 (익스포트 산출물) | 중간: 익스포트 폴리 수 조정·LOD 생성 필요 | 커뮤니티 에셋은 건별 라이선스 확인 필요. 수정판/자동화 대량 익스포트는 CC0 예외 미적용 |

제외:
- Mixamo: 프로젝트 내 사용은 가능하나 원본 재배포 불가 ([Mixamo FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html), 직접 인용 403).
- Epic Manny/Quinn·Starter Content: UE 제품 한정. 스켈레톤 규격 참조로만 사용.
- Reallusion CC Base: 재배포 조건 미확인.
- Sketchfab 개별 모델: 건별 라이선스 확인 전까지 보류.

## 2. 두 트랙

### 트랙 A — 실사: MetaHuman + 모바일 LOD 검증 필요
- 얼굴/피부/눈/헤어 품질이 목표(로드맵 2절 "실제 인간 캐릭터로 인식")에 가장 가깝다.
- 전제: Android Development build에서 주민 4명 + 세대 증가분이 LOD 5–7, 카드 헤어, 축소 머티리얼로 안정 동작하는지 실기기 검증. 통과 전에는 채택하지 않는다.
- 스켈레톤: MetaHuman 스켈레톤(UE5 Mannequin 호환 본 체계).
- 리스크: 용량, 머티리얼 슬롯 수, groom 비용, 애니 리타깃 비용, vendor 종속(외형 데이터는 AppearanceProfile로 분리해 종속 차단).

### 트랙 B — 경량: Quaternius Universal Base Characters
- CC0, 저폴리, humanoid rig, 헤어 20종, 눈·피부색 파라미터. Android 리스크 낮음.
- 사실형은 아니므로 로드맵 "상업 게임급 현실형"과의 간극은 피부/눈 머티리얼·헤어·의상 확장으로 보완한다.
- 스켈레톤: 팩의 humanoid rig를 UE5 Mannequin 규격으로 리타깃.

최종 트랙 선택: **다겸 검토 후 결정**.

## 3. 공통 humanoid 스켈레톤 기준 (하나)

- 기준 규격: UE5 Mannequin 본 계층(`root/pelvis/spine_01..05/neck_01..02/head`, 좌우 `clavicle/upperarm/lowerarm/hand`, `thigh/calf/foot/ball`, 손가락 3마디, IK 본). MetaHuman 스켈레톤은 이 규격의 상위 호환이므로 두 트랙 모두 같은 기준으로 리타깃한다.
- 저장소에는 Epic 스켈레톤 에셋을 재배포하지 않고, 프로젝트 내 생성(Content/Characters/Skeleton)으로 둔다.
- 애니메이션은 이 스켈레톤 하나에만 제작/리타깃한다. Motion & Context v1(로드맵 Phase D)이 이 스켈레톤을 사용한다.

## 4. 모듈 슬롯

| 슬롯 | 내용 | 교체 단위 | 머티리얼 |
|---|---|---|---|
| body | 몸통·팔·다리 base mesh(남/녀, 체형 archetype) | Skeletal Mesh | 1 (피부) |
| head | 얼굴 base + variant/morph, 눈 | Skeletal Mesh(+morph) | 피부 1 + 눈 1 |
| hair | 스타일 × 색, 얼굴 수염 포함 | Skeletal/Static Mesh (head 소켓) | 1 (카드 헤어) |
| outfit | 기본 의상 세트(원시 wrap → 후대 의상 확장) | Skeletal Mesh 세트 | 1–2 |

원칙: 슬롯당 머티리얼 슬롯 최소, dynamic material은 슬롯별 1개(색 파라미터)로 제한, `body` 하나에 `head/hair/outfit`를 Leader Pose 또는 소켓 부착.

## 5. AppearanceProfile 필드 초안

로드맵 3.1 + VisualSeed. Core data model 종속 없음(모두 presentation 계층 값).

| 필드 | 형 | 설명 |
|---|---|---|
| VisualSeed | uint64 | `hash(WorldSeed, CharacterId)`. 모든 변형 축의 결정론 시드 |
| BodyArchetype | enum | 남/녀 × 체형(예: Regular/Slim/Heavy) |
| HeightScale | float | 생애단계 기본 키에 곱하는 개인 편차 |
| ProportionParams | float[] | 어깨/허리/다리 비율 등 소수 |
| FaceVariant / FaceMorphSet | int / float[] | 얼굴 variant 인덱스 또는 morph 파라미터 |
| SkinToneVariant | int | 피부톤 팔레트 인덱스(연속값 아님) |
| EyeColor | int | 눈 색 팔레트 인덱스 |
| HairStyle, HairColor | int, int | 스타일·색 팔레트 인덱스 |
| FacialHair | int | 해당 시 스타일 인덱스(0=없음) |
| DefaultOutfitSet | int | 기본 의상 세트 인덱스 |
| AgePresentation | struct | LifeStage별 표현 파라미터(키 계수, 얼굴 성숙도, 머리/피부 노화 단계) |
| Provenance | struct | 생성 시점 seed·버전, 부모 참조(Phase F용, CharacterId만) |

생성 규칙: NEW GAME 최초 1회 `VisualSeed`로 결정론 생성, Save/Load 후 재생성 금지(로드맵 3.1). 권위 데이터로 Save에 넣어야 하면 코드 선행 구현 대신 `tasks/TEAM_BOARD.md` Integration Request로 등록한다.

## 6. Android LOD 3단계·fallback

| 단계 | 거리 기준(기본 관찰 카메라 ≈ 1900유닛) | body/head | hair | 애니메이션 |
|---|---|---|---|---|
| LOD0 | 근접(< 1200) | 전체 폴리, morph 활성 | 카드 헤어 전체 | 전체 업데이트 |
| LOD1 | 기본 관찰 거리(1200–3000) | 50% 폴리, morph 고정 | 카드 헤어 축소 | 업데이트 주기 절반 |
| LOD2 | 원거리(> 3000) / 화면 밖 | 25% 폴리, head 병합 | 헤어 캡(단색 메시) | 화면 밖 업데이트 중지(`VisibilityBasedAnimTickOption`) |

fallback:
- groom/strand 헤어는 사용하지 않거나 PC 전용. Android는 항상 카드 헤어 또는 헤어 캡.
- 머티리얼 슬롯 합계 주민당 5 이하 목표. 실패 시 body/head 병합 머티리얼.
- 주민 수는 세대에 따라 증가하므로 4명 전용 최적화 금지(로드맵 6절). 인스턴스 수 상한 도달 시 LOD2 강제.
- 트랙 A는 위 표를 MetaHuman LOD 5–7에 매핑해 실기기 검증 후 확정.

## 7. 다음 단계

1. 다겸 검토 → 트랙 선택.
2. 선택 트랙 에셋을 `Content/Characters/**`에 준비(CC0는 원본 커밋 가능, Epic 콘텐츠는 프로젝트 내 생성만).
3. ASSIST_LOCK-29-R1 해제 후 Character Appearance v1 코드 통합(로드맵 Phase C).
