# 맵 표면 시각 개선 인계 — 2026-10-01

## 경계 / 복구 지점
- 기준 main: `7aedd14aa6c2c0f3145bc04ee526d86119d0b4ed`.
- 작업 브랜치: `visual/map-surface-polish-20261001`.
- 목적: 고도별 지형색의 불연속 완화, 밤/안개에서 수면이 주변과 따로 밝게 보이는 문제 완화.
- 변경 소유 범위: `web/src/render/terrain-presentation.ts`, `water-surface-material.ts`, 전용 맵 표면 회귀 검사와 기존 presentation runner 연결, 이 문서.
- 병행 #561/#562의 물 운반/용기/생존 밸런스, #540 감사와 분리. Core/DTO/store/App/공용 CSS/WORK_STATE/다른 렌더 레이어는 변경하지 않는다.
- 시각 색상·조명만 변경. 지형 높이, 수계 모양/수위/흐름, 통행·자원·시설·실행 결과는 기존 Core 계약 그대로 유지.
- main에 병합·배포하지 않는다. 신규 Draft PR로 검토한다.

## 상태
- 구현 및 자동 검증 완료. 지형의 기존 5개 색을 고도 중간값 사이 smoothstep으로 연결한다. Ocean/Coast/Wetland 특수색과 식생/암석 혼합은 보존한다.
- 수면 ShaderMaterial에 독립 fog uniforms, Three.js fog/tonemapping/colorspace chunks 연결. 기존 Core daylight 입력으로 물 본체·반사 밝기도 낮춘다. 야간 가독성 하한은 표현값 0.12이며 simulation threshold가 아니다.
- 신규 `web/tests/presentation/map-surface-checks.mjs`는 지형색 연속성/관측 불변/물 종류/안개 uniforms 독립성/셰이더 연결/수계 값 보존을 검사하며 기존 runner가 실행한다.
- 최초 CI `36825441042`는 추가 검사의 `instanceof THREE.Color`에서 실패했다. 테스트가 ESM Three와 CJS Three를 함께 로드해 생성자 identity가 다르기 때문이다. Three의 `isColor` 표식 및 실제 색 변경의 독립성 검사로 수정했다. 제품 코드는 이 수정에서 변경하지 않았다.
- 추가 로컬 EGL/llvmpipe 검증: 실제 수면 GLSL + Three r180 shader chunks로 안개 없음/linear/exp × 톤 매핑 off/on 6종 compile/link/render PASS. 야간 픽셀이 낮보다 어두워지고 짙은 안개가 fogColor에 수렴함을 확인했다. 실제 월드/브라우저 통합 검증은 아니다.

## 검증 결과 / 미완료 항목

- `2407b049` 기준 [Web Typecheck 36825815415](https://github.com/sjLim91/lifelens-ue/actions/runs/36825815415): PASS.
- [Web Runtime Resilience 36825815294](https://github.com/sjLim91/lifelens-ue/actions/runs/36825815294): PASS. 기존 Web 회귀, 추가 맵 표면 검사, TypeScript, Vite production build 포함.
- [Preflight 36825815297](https://github.com/sjLim91/lifelens-ue/actions/runs/36825815297): PASS.
- 로컬 지형색 9개 경계 연속성 및 선택적 EGL 6종 실렌더 검사: PASS.
- 대표 EGL 픽셀(출력 RGBA): 톤 매핑 없음 낮 `[80,130,150,208]` → 밤 `[22,43,52,208]`; 짙은 안개 `[51,76,102,208]`. 낮/밤/안개 변화가 alpha를 바꾸지 않았다.
- **실제 모바일 맵 화면 검수는 미완료.** 로컬 브라우저 제한 때문에 스크린샷/실기기 합격을 주장하지 않는다.
- 새 메시/지형 재생성/그리기 호출/외부 자산 없음. fragment shader의 안개·색공간 연산은 추가됐으며 실제 GPU 성능 측정은 하지 않았다.
- main `7aedd14a` 및 Core 작업 브랜치는 변경하지 않았다. 최종 인계 보강 커밋은 문서만 변경한다.

## 셰이더 검증 재현

- 신규 `web/tests/presentation/water-shader-egl.py`는 선택적 로컬 도구. Linux libEGL + GLES + surfaceless Mesa 필요. npm 의존성 설치 후 `MESA_SHADER_CACHE_DISABLE=true python web/tests/presentation/water-shader-egl.py` 실행.
- Three shader chunks는 기본 `web/node_modules/three/src/renderers/shaders/ShaderChunk`에서 읽는다. `--three-chunks PATH`로 위치 지정, `--output /tmp/water-results.json`으로 근거 보존 가능.
- CI/제품에 Python/EGL 의존성을 추가하지 않는다. 운영 앱/시뮬레이션을 실행하거나 수정하는 도구가 아니다.
- 중단 후 actual GitHub main/branch/PR/Actions부터 확인. 이 문서의 상태를 성공 추정에 사용하지 않는다.
- 로컬 환경 `/workspace/lifelens-map`은 부분 소스이며 full checkout이 아니다.
- 이전 작업에서 shell npm 네트워크 및 Chromium 소켓 제한이 확인됐다. 같은 장기 권한 요청을 반복하지 않고 GitHub CI로 타입/빌드/회귀를 검증한다.
- CI 성공과 실제 WebGL 시각 검증은 구분해 기록한다.

## 챗에서 이어갈 순서

1. [Draft PR #563](https://github.com/sjLim91/lifelens-ue/pull/563)과 최신 Actions를 조회한다. 제품 변경 `60d1c0d1`, 테스트 보완/재현 도구 `2407b049` 기준이다.
2. CI 합격 후 실제 WebGL 환경에서 같은 seed·Core 시간·카메라로 이전 main과 비교한다. 낮/새벽/밤, 맑음/안개/비를 포함하고 환경은 실제 Core 관측 상태를 사용한다.
3. 모바일에서 밤의 강/호수 경계가 읽히는지, 낮 반사가 과도하게 밝아지지 않는지, 안개에서 물만 떠 보이지 않는지 확인한다. 해안선 위치/이동 목표는 이전과 같아야 한다.
4. 지형색 변경은 고도 임계값의 색 튐 완화다. 모든 청크 이음이나 biome 경계를 없앤 변경으로 확대 해석하지 않는다.
5. 수면에는 Three 표준 표시 색공간 변환이 새로 적용돼 낮 색감도 달라질 수 있다. CI/EGL PASS를 실제 기기 시각 합격으로 대신하지 않는다.
6. 화면 비교/기기 확인 결과를 이 문서와 PR에 추가한 뒤 병합을 별도로 결정한다. 병행 Core 브랜치나 감사 브랜치를 합치지 않는다.
