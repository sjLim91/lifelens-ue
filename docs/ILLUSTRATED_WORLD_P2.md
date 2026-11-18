# LifeLens P2 — 수계·경사면·주민 시야 가독성 개선

기준: main 349fedcd (#1041, P1 그림풍 시각 모드). 상태: Core 변경 없는 Web presentation 개선.

## P2 실제 변경

- **강/호수/해안 캐시 정확성**: 기존 water layer는 수계 geometry cache key가 waterKind/elevation에만 의존했다. 하류 방향, 이용 가능 물량, 유량, 염분 속성이 변해도 낡은 메시가 재사용될 수 있었다. 이제 Core TerrainWindow의 실제 waterKind/elevation/salinity/availability/flow/downstream 필드를 키로 사용한다. 관측 청크 순서는 같은 지형으로 취급하고 불필요한 rebuild를 피한다.
- **수면 대비**: P1 `?world-style=illustrated` 모드에서만 실제 강/해안/호수 표면의 청록색 재질 대비를 높인다. 수면 폭·흐름·지형 충돌·수자원 공급 위치는 바꾸지 않는다. 기존 기본 3D 표현은 유지.
- **경사·능선 가독성**: P1 그림풍 모드에서는 상세 17×17 고도 샘플이 없는 coarse Core 지형에도 기존 Core 높이·법선에 근거한 가벼운 암석 색채와 방향성 음영을 적용한다. 정점, 법선, UV, 삼각형, 높이는 모두 그대로.
- **주민 시야 가림**: 기존 foliage dither shader의 최대 네 대상 구조를 유지하되, 카메라에서 가까운 네 명보다 우선하여 *실제로 선택된* 주민의 위치가 첫 번째 시야 확보 대상으로 전달되도록 한다. 이중 위치/NaN은 제거. 나무 제거, 채집량 변조, 경로 변경, 별도 GPU pass 없음.
- **모바일 안전성**: GPU 없는 Canvas 호환 경로는 그대로이고 WebGL 그림풍 효과는 P1 실험 모드에서만 활성화된다.

## 인수 검증

- `npm run typecheck`, `Tools/ci_web.py regressions`, `npm run build`, Preflight.
- 전체 Web presentation 테스트에 순서 불변 cache, downstream/availability/salinity 변경 시 캐시 재생성, old geometry dispose, 선택 주민 우선/중복 제거, 실제 경사 정상 보존 검사 추가.
- 병합 후 GitHub Pages 빌드 및 외부 브라우저 Core 부팅 확인.

## 남겨둔 과제

이 단계만으로 실제 사용 기기에서 강·호수의 장면 전반 미적 완성도, 모든 절벽에서 캐릭터가 잘 보이는 정도, 30분 이상 모바일 사용 중 FPS/메모리 개선이 확인되는 것은 아니다. Core 원천 수계나 지형에서 발생하는 형상 결함은 이번 변경의 해결 범위가 아니다.

다음 P3: 주민 및 시설 모델의 근접 관찰 품질과 선택·상호작용 인과를 시각적으로 정합시킨다.
