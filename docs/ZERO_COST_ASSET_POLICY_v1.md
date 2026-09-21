# LifeLens Zero-Cost Asset Policy v1

## 목적

LifeLens의 그래픽 품질을 높이기 위해 비용 0원의 외부 에셋을 사용할 수 있다. 이 문서는 무료 에셋의 선택, 라이선스 검증, 저장소 반입, 플랫폼 분리, QA 원칙을 고정한다.

## 1. 기본 원칙

- **비용 0원**이어야 한다.
- 다운로드 버튼이 무료라는 사실만으로 사용 허가를 추정하지 않는다.
- 상업적 패키징/배포가 가능한 라이선스를 실제로 확인한다.
- Core / World simulation truth를 외부 asset이 결정하지 않는다. 외부 asset은 기본적으로 Presentation 표현 자산이다.
- 기존의 Poly Haven 계열 CC0 photoreal path와 Quaternius zero-cost nature path는 현재 프로젝트에서 사용 중인 승인된 방향으로 유지한다.
- production local-view에서 저품질 Engine primitive를 최종 미술 fallback으로 남기는 방식은 피한다.

## 2. 허용 라이선스

우선순위:
1. CC0 / Public Domain.
2. 상업적 사용과 게임 패키징을 명시적으로 허용하는 permissive license.
3. CC-BY 등 attribution 조건이 있는 무료 라이선스. 단, attribution 의무를 실제 배포물에서 지킬 수 있어야 한다.

자동 제외:
- NC / Non-Commercial.
- Personal Use Only.
- Editorial Only.
- 출처 또는 권리자가 불명확한 파일.
- 명시적 라이선스가 없는 파일.
- 타 게임/앱/마켓플레이스 제품에서 추출된 ripped content.
- 모델/텍스처 재배포 범위가 LifeLens packaging과 충돌하는 라이선스.

## 3. Provenance 기록

새 asset을 import하기 전에 다음 정보를 기록한다.

| 필드 | 필수 |
|---|---|
| Asset / Pack name | 예 |
| Creator / Provider | 예 |
| Source URL | 예 |
| License | 예 |
| License checked date | 예 |
| Attribution required | 예 |
| Imported LifeLens path | 예 |
| Purpose | 예 |
| Platform tier | 예 |
| Modified / optimized | 예 |

asset이 늘어나면 별도 manifest를 유지하고, 최소한 PR 본문과 canonical asset 문서에서 출처를 역추적할 수 있어야 한다.

## 4. 플랫폼 원칙

### Shared
- 모바일/데스크톱 양쪽에서 비용이 합리적인 소형 공용 asset.
- 공용이라고 해서 자동으로 Android에 고해상도 원본을 cook하지 않는다.

### Desktop — Windows + macOS
- photoreal/high-detail mesh와 material 사용 가능.
- Nanite 등 기능은 각 플랫폼 capability에 맞춰 사용.
- macOS에서 Windows 전용 renderer 가정을 강제하지 않는다.

### Mobile — Android
- lightweight mesh / LOD / texture / material.
- desktop 전용 고해상도 payload를 APK에 포함하지 않는다.
- runtime PCG / water / vegetation 비용이 높으면 bake/cache/instancing/간소화 path를 사용한다.

## 5. 도입 절차

1. 전수검사에서 실제 시각 결손을 확인한다.
2. built-in/현재 asset으로 해결 가능한지 먼저 확인한다.
3. 외부 무료 asset이 더 적합하면 라이선스와 출처를 확인한다.
4. 용도에 맞는 플랫폼 tier를 결정한다.
5. import / material / scale / collision / LOD / pivot / naming을 정리한다.
6. simulation authority와 충돌하지 않는지 검증한다.
7. Preflight / Unreal compile / 필요한 cook/package 검증을 수행한다.
8. 실제 runtime screenshot/device에서 품질을 확인한다.
9. 출처와 라이선스를 문서에 남기고 merge한다.

## 6. 품질 기준

- 단순히 asset 수를 늘리는 것이 목표가 아니다.
- 카메라에서 실제로 보이는 장면의 밀도, 스케일, material 일관성, lighting 반응, silhouette를 우선한다.
- 지형과 vegetation은 deterministic placement와 settlement readability를 해치지 않아야 한다.
- 물 asset은 Hydrology truth와 위치/종류가 일치해야 한다.
- 시설/생활 소품은 해당 행동의 실제 context와 일치해야 한다.
- 같은 mesh를 과도하게 반복해 패턴이 드러나는 경우 variation/rotation/scale/cluster를 사용한다.

## 7. 병합 기준

외부 asset PR은 최소 다음을 만족한다:
- license/provenance 확인.
- 잘못된 hard reference 또는 platform cook 누수 없음.
- Core gameplay truth 변경 없음 또는 별도 명시적 contract 변경.
- relevant CI green.
- runtime 시각 확인이 필요한 asset은 screenshot/device QA 항목을 남긴다.

무료 에셋 사용은 허용하지만 **권리 불명확한 에셋을 품질 때문에 예외 처리하지 않는다.**

## Animation / motion asset policy

무료 외부 animation/mocap도 이 정책의 적용 대상이다.

허용 조건:
- 상업적 게임/앱 배포 및 packaged redistribution이 명확히 허용될 것.
- source/provider/license URL과 확인일을 기록할 것.
- attribution이 필요하면 실제 배포에서 준수 가능할 것.
- Unreal 5.6 retarget 가능성을 확인할 것.

금지:
- ripped game animation.
- NC / personal-use-only / editorial-only.
- license가 불명확한 animation dump.
- 다른 IP 캐릭터 고유 motion을 추출한 자료.

Animation별 추가 provenance:
- source skeleton / rig.
- root motion 또는 in-place.
- loop 여부.
- retarget target.
- semantic action mapping.
- contact-sensitive 여부(IK/Motion Warp 필요 여부).
- 수정/trim/blend 처리.
- Android/Desktop 포함 여부.

선정 원칙:
- LifeLens의 실제 행동 gap을 채우기 위해 사용한다.
- asset이 있다는 이유로 행동을 설계하지 않는다.
- 일상생활 모션은 자연스러움과 접촉 정확도를 우선한다.
- 필요 시 여러 무료 source를 조합할 수 있으나 동일 license/provenance 기준을 적용한다.
