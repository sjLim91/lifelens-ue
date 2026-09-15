# LifeLens Observer Camera Control v1

## 목적
LifeLens의 기본 관찰 카메라는 고정 프레이밍 검증용 상태를 넘어, PC와 Android에서 실제로 세계를 관찰할 수 있는 직접 조작 카메라를 제공해야 한다.

카메라 조작은 Presentation이며 Core 시뮬레이션의 위치/행동 authority를 변경하지 않는다.

## 제품 원칙
- 주민 선택만으로 카메라가 자동 점프하거나 강제 추적하지 않는다.
- 기본 회전 중심은 초기 정착지/생활권 중심이며 사용자의 pan으로 이동할 수 있다.
- 카메라 조작은 부드럽게 보간한다.
- 지면 아래로 파고들거나 수평선만 보게 되는 극단 각도를 허용하지 않는다.
- 지나친 근접/원거리 줌을 제한한다.
- UI 탭/주민 선택과 카메라 제스처를 명확히 구분한다.

## PC 입력
- 마우스 휠: 줌 인/아웃.
- 우클릭 드래그: orbit 회전.
  - 좌우: Yaw.
  - 상하: 제한된 Pitch/Elevation.
- 휠클릭(중클릭) 드래그: 지면 평면 기준 pan.
- 좌클릭: 기존 주민/UI 선택 의미를 유지한다.

## Android 입력
- 한 손가락 짧은 탭: 주민/UI 선택.
- 한 손가락 드래그: orbit 회전.
- 두 손가락 pinch: 줌.
- 두 손가락을 같은 방향으로 이동: pan.

### 탭과 드래그 판정
터치 시작 순간 주민을 선택하지 않는다.

`Pressed -> 이동량 추적 -> Released` 순서로 처리한다.
- 이동량이 threshold 미만이면 tap.
- threshold를 넘으면 camera gesture.
- 두 손가락이 사용되면 해당 gesture 동안 tap으로 판정하지 않는다.

이 규칙은 카메라 회전을 시도할 때 주민이 의도치 않게 선택되는 현상을 막는다.

## Orbit 상태
Presentation runtime은 최소 다음 값을 가진다.
- orbit target world position.
- yaw.
- elevation/pitch.
- distance.
- 각각의 desired/current 값 또는 동등한 smoothing 상태.

카메라 위치는 orbit target + spherical offset으로 계산하고, 카메라는 항상 현재 orbit target을 바라본다.

## 제한 / Config
다음 값은 제품 tuning 값이며 Config로 조정 가능하게 한다.
- minimum / maximum orbit distance.
- minimum / maximum elevation.
- mouse rotate sensitivity.
- mouse pan sensitivity.
- wheel zoom sensitivity.
- touch rotate sensitivity.
- touch pan sensitivity.
- pinch zoom sensitivity.
- tap-vs-drag threshold.
- smoothing speed.

## 초기화
- 기존 production observer framing을 첫 프레임의 초기 orbit 상태로 보존한다.
- 현재 production map/start region 중심을 초기 orbit target으로 사용한다.
- 카메라 컨트롤 추가가 Core start-region 위치, World generation, resident movement를 바꾸면 안 된다.

## UI / ownership interaction
`Source/LifeLens/UI/LLObserverPlayerController.*`는 Dagyeom UI/Observer presentation 소유영역이다.

Jjun이 camera integration을 구현할 경우:
- 별도 assist branch를 사용한다.
- TEAM_BOARD Assist Lock으로 정확한 파일 범위를 잡는다.
- HUD 기능(#30/#36/#38 등)과 같은 PR에 섞지 않는다.
- 기존 left-click/tap 주민 선택 동작을 보존한다.

## V1 acceptance
- PC mouse wheel로 zoom in/out이 실제로 동작한다.
- PC right-drag로 수평 회전 및 제한된 상하 orbit이 동작한다.
- PC middle-drag로 pan이 동작한다.
- Android one-finger drag가 주민 오선택 없이 rotate로 동작한다.
- Android pinch zoom이 동작한다.
- Android two-finger pan이 동작한다.
- 짧은 tap은 기존 주민/UI 선택을 유지한다.
- camera distance/elevation이 clamp 범위를 벗어나지 않는다.
- resident selection만으로 camera target/position이 강제로 변경되지 않는다.
- 입력 종료 후 카메라가 떨리거나 계속 움직이지 않는다.
- Core/World authority와 deterministic simulation 결과는 변하지 않는다.
