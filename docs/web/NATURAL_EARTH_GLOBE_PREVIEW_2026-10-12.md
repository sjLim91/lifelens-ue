# EARTH-2 첫 가시 구현 — 실제 자연 대륙 구체 지구 미리보기 (2026-10-12)

## 데이터 계보
- Source: [Natural Earth vector land GeoJSON](https://github.com/nvkelso/natural-earth-vector/blob/693f11422f4e08d2da4566b854dda53eb7c39fb3/geojson/ne_110m_land.geojson).
- Pinned upstream source repository commit: `693f11422f4e08d2da4566b854dda53eb7c39fb3`.
- `ne_110m_land.geojson`: Natural Earth physical land polygons (1:110 million), 127 polygon features, ~135KB.
- License: Natural Earth **Public Domain**, [license](https://github.com/nvkelso/natural-earth-vector/blob/master/LICENSE.md); attribution optional but displayed.
- Local bundled file: `web/public/earth/ne_110m_land.geojson`, no runtime third-party network.
- **Not an elevation map, climate model, wind/current map or city map**. Physical land-only preview. Natural Earth generalized coastline is not survey-grade.

## 구현 범위
- Web Topbar의 "지구본" 버튼 -> read-only Earth preview overlay. 기존 local Core observer continues to run; no changes to any World truth, seed, simulation clock, snapshots, residents, resources or weather.
- Static Three.js sphere and HTML canvas-based equirectangular land texture from the pinned Natural Earth polygons. Globe drag/swipe to rotate, mouse wheel zoom, close/ESC, mobile antialias quality budget, WebGL resources disposed on close.
- Seam-crossing land polygons are unwrapped by minimal longitude delta, then drawn with +/- texture-width duplicates to avoid cut-off. Geometry tests for antimeridian and pinned data count.
- The Earth globe **does not yet mark the current local world on its surface**. That connection is forbidden until EARTH-0 geo anchor/Scenario positions become Core-authoritative. Any wind/current overlay also requires Core/regional physical vector fields.
- Globe is explicitly labeled **natural-geography preview**. Earth weather and ocean currents are not yet implemented or represented as real-time facts.
- Asset is self-hosted with the build; no required paid API/cloud/map tiles.

## 검증
`cd web && node tests/earth-globe/run.mjs`, Web typecheck/build, existing UI regressions & mobile smoke.
Particularly: camera turn/zoom never affects Core, close disposes WebGL, static data load failure is honestly surfaced, geographic seam has no false wrap stroke.

## 후속 과제
EARTH-0 region anchors -> EarthDataVersion and real geography region model -> authoritative region link on globe -> proper globe↔regional↔local scale transition (#329) -> two 4-founder groups (#672) -> regional climate and real wind/current simulation overlays.
