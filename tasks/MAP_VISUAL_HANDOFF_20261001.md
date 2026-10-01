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
- 구현 완료, CI 검증 전 checkpoint. 지형의 기존 5개 색을 고도 중간값 사이 smoothstep으로 연결한다. Ocean/Coast/Wetland 특수색과 식생/암석 혼합은 보존한다.
- 수면 ShaderMaterial에 독립 fog uniforms, Three.js fog/tonemapping/colorspace chunks 연결. 기존 Core daylight 입력으로 물 본체·반사 밝기도 낮춘다. 야간 가독성 하한은 표현값 0.12이며 simulation threshold가 아니다.
- 신규 `web/tests/presentation/map-surface-checks.mjs`는 지형색 연속성/관측 불변/물 종류/안개 uniforms 독립성/셰이더 연결/수계 값 보존을 검사하며 기존 runner가 실행한다.
- 최초 CI `36825441042`는 추가 검사의 `instanceof THREE.Color`에서 실패했다. 테스트가 ESM Three와 CJS Three를 함께 로드해 생성자 identity가 다르기 때문이다. Three의 `isColor` 표식 및 실제 색 변경의 독립성 검사로 수정했다. 제품 코드는 이 수정에서 변경하지 않았다.
- 추가 로컬 EGL/llvmpipe 검증: 실제 수면 GLSL + Three r180 shader chunks로 안개 없음/linear/exp × 톤 매핑 off/on 6종 compile/link/render PASS. 야간 픽셀이 낮보다 어두워지고 짙은 안개가 fogColor에 수렴함을 확인했다. 실제 월드/브라우저 통합 검증은 아니다.

## 셰이더 검증 재현

- 신규 `web/tests/presentation/water-shader-egl.py`는 선택적 로컬 도구. Linux libEGL + GLES + surfaceless Mesa 필요. npm 의존성 설치 후 `MESA_SHADER_CACHE_DISABLE=true python web/tests/presentation/water-shader-egl.py` 실행.
- Three shader chunks는 기본 `web/node_modules/three/src/renderers/shaders/ShaderChunk`에서 읽는다. `--three-chunks PATH`로 위치 지정, `--output /tmp/water-results.json`으로 근거 보존 가능.
- CI/제품에 Python/EGL 의존성을 추가하지 않는다. 운영 앱/시뮬레이션을 실행하거나 수정하는 도구가 아니다.
- 중단 후 actual GitHub main/branch/PR/Actions부터 확인. 이 문서의 상태를 성공 추정에 사용하지 않는다.
- 로컬 환경 `/workspace/lifelens-map`은 부분 소스이며 full checkout이 아니다.
- 이전 작업에서 shell npm 네트워크 및 Chromium 소켓 제한이 확인됐다. 같은 장기 권한 요청을 반복하지 않고 GitHub CI로 타입/빌드/회귀를 검증한다.
- CI 성공과 실제 WebGL 시각 검증은 구분해 기록한다.
