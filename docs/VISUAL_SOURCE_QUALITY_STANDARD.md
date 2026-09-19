# LifeLens Visual Source Quality Standard v1.0

## 1. 목적

이 문서는 LifeLens의 화면/그래픽 관련 소스 품질 기준을 정의한다.

핵심 원칙:

> **화면을 직접 보기 전에도 소스 구조만으로 잡을 수 있는 문제는 모두 선제적으로 제거한다.**
>
> 실제 실행 화면 확인은 소스 품질을 대신하는 절차가 아니라 정적 검증 이후의 최종 런타임 QA 단계다.

기본 순서:

```text
소스 구조 검증
→ 렌더링 경로 검증
→ 플랫폼별 Presentation ownership 검증
→ 에셋 참조 검증
→ Preflight / Compile
→ 실제 실행 화면 검증
→ 플랫폼별 실기기 QA
```

## 2. 이번 문제에서 확인된 구조적 원인

### 2.1 특정 정상 코드 존재 여부만 검사

기존 일부 validator는 특정 토큰이나 방어 코드가 존재하는지 확인하는 방식에 치우쳐 있었다.

이 방식은 특정 회귀 방지에는 유효하지만, 프로젝트 다른 경로에 동일한 유형의 결함이 남아 있는지는 보장하지 못한다.

앞으로는 다음을 함께 검사한다.

- 정상 코드 존재
- 금지된 visual execution path 부재
- 중복 renderer 부재
- placeholder/proxy 노출 가능성 부재
- asset hard reference 유효성

### 2.2 화면 ownership 중복

동일 공간을 여러 Presentation 시스템이 동시에 그리면 개별 구현이 정상이어도 최종 화면에서 결함이 발생한다.

실제 사례:

- `LLWorldPresentationActor` planar chunk ground
- `LLDesktopTerrainPresentationActor` procedural terrain

가능한 결과:

- z-fighting
- 회색/사각 chunk 노출
- 지면 겹침
- terrain edge 노출
- 나무/바위/시설물의 높이 불일치
- 오브젝트가 뜨거나 지면에 박힘

따라서 플랫폼별로 **한 공간의 최종 visual owner를 하나로 정의**한다.

### 2.3 Debug / Placeholder geometry 노출

Smoke 단계에서 사용한 primitive는 production 화면에 자동 노출되어서는 안 된다.

확인된 사례:

- Resident `DebugBody` Engine Cube
- Environmental Residue Engine Cube
- Facility 구조용 primitive
- Collision proxy

원칙:

- Debug geometry 기본값은 항상 Hidden
- Collision authority는 항상 Render Disabled
- Placeholder는 production path에서 자동 활성화 금지
- 정상 에셋 로딩 실패 시 저품질 primitive를 보여주기보다 숨김을 우선
- fallback이 필요하면 품질 기준을 통과한 visual asset만 사용

## 3. 소스 품질과 실제 화면 QA 구분

```text
소스 품질 ≠ 실제 화면 품질
```

소스 단계에서 반드시 잡아야 하는 항목:

- 중복 렌더러
- primitive fallback 노출
- 잘못된 visibility 초기값
- collision proxy 렌더링
- 잘못된 camera ownership
- 중복 terrain ownership
- 잘못된 asset hard reference
- 플랫폼 분기 누락
- material fallback 경로
- animation-context 불일치

실제 실행에서 최종 확인해야 하는 항목:

- 셰이더 결과
- 실제 material appearance
- LOD 전환
- temporal artifact
- GPU/driver별 렌더링 차이
- 실제 조명/노출 균형
- 최종 구도
- 실제 scale perception
- 플랫폼별 성능/프레임 안정성

## 4. Primitive 사용 정책

Production presentation에서 Engine primitive를 임의 사용하지 않는다.

대상:

- Cube
- Sphere
- Cylinder
- Plane
- Capsule substitute

허용 가능한 경우:

1. collision-only
2. editor/debug-only
3. 테스트 전용
4. 시각적으로 검증된 의도적 표현

collision/debug primitive는 최소 다음을 강제한다.

```cpp
SetVisibility(false, true);
SetHiddenInGame(true, true);
SetCastShadow(false);
```

필요한 경우 collision은 유지할 수 있으나 rendering authority는 가지지 않는다.

## 5. Visibility 정책

금지 패턴:

- 초기 생성 시 visible 후 나중에 숨김
- asset bind 성공을 전제로 debug mesh 제거
- transient frame에서 placeholder 노출 가능
- visibility state drift에 따라 proxy 노출 가능

허용 패턴:

```text
기본 Hidden
→ 정상 에셋/상태 검증
→ 명시적 Visible
```

Presentation 객체는 안전한 비가시 상태에서 시작한다.

## 6. Terrain / Ground Ownership

### Desktop: Windows / macOS

```text
LLDesktopTerrainPresentationActor
→ smooth procedural terrain
→ local surface 단독 소유
```

planar chunk cube는 desktop local surface를 소유하지 않는다.

Broad/far continuity ground는 사용할 수 있으나 smooth terrain 아래에 위치하며 local detail과 경쟁하지 않는다.

### Android

모바일 예산에 맞춘 lightweight ground path를 사용할 수 있다.

Android와 Desktop 경로가 동시에 활성화되지 않도록 compile-time/platform gate를 유지한다.

## 7. Terrain 높이 계약

다음 요소는 동일한 terrain height contract를 사용한다.

- terrain surface
- trees
- rocks
- shrubs
- grass
- facilities
- resource patches
- resident interaction targets

각 subsystem이 서로 다른 surface interpolation 공식을 사용하면 안 된다.

목표 구조:

```text
Authoritative Terrain Observation
→ Shared Presentation Surface Function
→ Terrain Mesh
→ Dressing Z
→ Facility Z
→ Interaction Visual Z
```

## 8. Character Presentation 정책

### Debug Body

Resident bootstrap/debug geometry는 production 화면에 노출하지 않는다.

```text
Resident Spawn
→ Debug geometry Hidden
→ Identity Bind
→ Appearance Resolve
→ Human Body 또는 승인된 fallback 생성
```

### Animation Context Truth

애니메이션은 실제 world affordance와 일치해야 한다.

```text
Seat 없음
→ Sitting animation 금지

Chair/Bench/Bed 등 실제 affordance 존재
→ Context resolve
→ Sitting animation 허용
```

Social/Parenting 같은 의미론적 행동만으로 임의로 앉은 자세를 만들지 않는다.

## 9. Camera 정책

Observer Camera는 명확한 production identity를 가져야 한다.

필수 조건:

- GameMode가 production observer camera를 보관
- stable tag 또는 명시적 reference 보유
- PlayerController가 ViewTarget 상실 시 self-heal
- 임의 camera actor로 남지 않음
- sky-only 상태 발생 시 정상 observer camera 복구 가능

Camera initialization은 일회성 성공을 전제로 하지 않는다.

## 10. Material / Asset 정책

C++에서 직접 참조하는 모든 `/Game/...` 자산은 실제 repository content에 존재하는지 검증한다.

금지:

- material load 실패 → Engine default grey 표시
- imported material failure를 silent fallback으로 방치
- 흰색/회색 broken asset을 production nature mesh로 사용

원칙:

```text
approved material
→ verified asset
→ visible

missing/broken material
→ hidden 또는 approved fallback
```

## 11. Environment Presentation 정책

날씨/환경 효과도 동일한 visual quality 기준을 적용한다.

검토 대상 예:

- primitive rain cube
- primitive snow sphere
- debug fog volume
- unmaterialed environmental residue

Niagara 또는 승인된 fallback asset이 없는 경우 저품질 geometry를 억지로 보여주지 않는 방향을 우선한다.

## 12. 플랫폼별 Rendering 계약

### Windows

기본 renderer:

```text
DX12 + SM6
```

D3D11은 production 기본 경로가 아니며 diagnostic/legacy fallback으로만 사용한다.

### macOS

Windows와 동일한 PC product family로 취급하되 Metal/Lumen/Nanite/VSM 지원 차이를 별도 profile로 관리한다.

### Android

별도 Mobile Renderer / Device Profile을 유지한다.

PC용 고비용 renderer 또는 desktop-only asset을 APK에 불필요하게 포함하지 않는다.

## 13. 전역 정적 검사 항목

Visual 관련 변경은 최소 다음 항목을 전역 검사한다.

### Primitive scan

```text
/Engine/BasicShapes/
```

모든 사용처를 확인한다.

### Visibility scan

```text
SetVisibility(true)
SetHiddenInGame(false)
```

placeholder/proxy 활성화 가능성을 확인한다.

### Rendering proxy scan

- collision proxy
- debug mesh
- temporary mesh
- fallback mesh
- prototype mesh

### Terrain ownership scan

동일 플랫폼에서 복수 terrain renderer가 같은 영역을 소유하지 않는지 확인한다.

### Asset reference scan

hard reference 대상이 실제 repository content에 존재하는지 확인한다.

### Material safety scan

default grey/white material 경로가 production에서 노출될 수 있는지 확인한다.

### Platform split scan

```cpp
#if PLATFORM_ANDROID
#if PLATFORM_WINDOWS
#if PLATFORM_MAC
```

플랫폼 경계가 의도대로 분리되어 있는지 확인한다.

## 14. CI / Preflight 기준

Visual source quality gate는 단순 문자열 존재 검사에서 다음 구조로 강화한다.

```text
정상 코드가 존재하는가?
+
금지된 visual execution path가 존재하지 않는가?
+
동일 presentation ownership이 중복되지 않는가?
+
참조 에셋이 실제 존재하는가?
```

필수 gate:

- runtime visual sanity
- desktop smooth terrain
- graphics foundation
- character animation truth
- observer camera resilience
- platform cook boundary
- asset reference validation

## 15. 완료 기준

Visual 작업 완료 조건:

```text
1. Source Architecture PASS
2. Static Visual Audit PASS
3. Asset Reference PASS
4. Platform Boundary PASS
5. Preflight PASS
6. UHT / UBT Compile PASS
7. Runtime Launch PASS
8. Actual Screen QA PASS
9. Windows / macOS / Android platform QA
```

어느 한 단계라도 실패하면 화면 작업을 완료로 처리하지 않는다.

## 16. 작업 운영 규칙

```text
분석
→ 수정
→ 검증
→ 컴파일
→ 결과 보고
→ 사용자 지시 대기
```

자동으로 다음 개발 단계, merge, 추가 기능 구현으로 넘어가지 않는다.

현재 사용자가 허용한 자동 범위:

```text
지시된 소스 수정
→ Preflight
→ Compile
```

Merge 및 다음 기능 작업은 별도 지시에 따른다.

## 17. 최상위 원칙

> **화면을 봐야 알 수 있는 문제와 소스만 봐도 잡아야 하는 문제를 구분한다.**
>
> 소스만 봐도 잡을 수 있는 visual defect는 실행 화면 확인 전에 모두 제거한다.
>
> 실제 화면 검증은 구조적 결함을 찾는 첫 단계가 아니라 최종 품질 확인 단계다.
