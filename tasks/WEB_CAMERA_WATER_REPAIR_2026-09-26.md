# Web camera / water repair — 2026-09-26

Product PR: #458
Product merge: `369f813047a333289a4425d103b92f76304ab9aa`
Base: `f634b180dcfc634e5b2bb031ddb0c1e7abf6ffb4`
Branch: `work/web-current-repair-20260926`

## Findings and changes

- The camera looked at height zero, although terrain/residents use Core-derived elevated ground. Focus now samples the same interpolated terrain and camera height keeps clearance over the terrain below its eye.
- Crossing a streamed chunk boundary moved terrain and resident local coordinates without moving the smoothed camera into the same origin. Rebase the current pan before interpolating toward its new goal. New WorldSeed resets framing.
- All open-water masks, river ribbons and spring caps used downward triangle winding with upward vertex normals. From above, the double-sided shader flipped those normals downward. Correct winding and front-side materials now agree.
- River mouths used coastal bed elevation instead of the connected open-water surface level. The reproduced fixture ended at Y=9.694 while its ocean was Y=4.880; the mouth now meets that surface.
- Water geometry cache omitted WorldSeed even though flow curvature uses it. A new world with otherwise matching chunks now rebuilds/disposes its water geometry; identical refreshes keep it.

## Validation

- Before repair, 9 of the initial 10 presentation cases failed against main. Added an uphill-camera clearance case and all four flow directions.
- After repair, 11 presentation cases pass: real Three.js camera projection, geometry winding, raycasting, mouth levels and cache lifecycle. Only unrelated asset/dressing layers are stubbed.
- Existing 49 checks pass: runtime 17, resident continuity 13, weather 7, appearance 5, input 7.
- Typecheck, production build, water/weather/environment structural validators pass locally.
- Exact PR HEAD `f6f06ed432c538965d4be884c3f0aaca2f491de0`: all four GitHub checks pass.
- Preflight run 36247542321; runtime/build run 36247542279; typecheck runs 36247542278 and 36247540279.
- Main Web Preview 36247613583 and Pages deployment 36247646656 succeed; published gh-pages SHA `eeac7553c45019a1b1b6545d09164576616a1753` references this product merge. URL: https://sjlim91.github.io/lifelens-ue/?v=369f813

## Scope and remaining acceptance

- Preserves #452–457 environment, outfit, water and snow work.
- No Core authority, resident action-context (#423), runtime-release (#425), character asset or user gesture mapping changes.
- Cloud browser cannot create WebGL. These geometry/projection checks are not a real-device screenshot or subjective visual acceptance.
- Device QA still needs low/high angle framing, continuous two-finger pan across several boundaries, river/coast lighting and the current character appearance.
- Broader connected-water height/terrain intersection and authoritative downstream projection are outside this bounded repair. Do not treat this as hydrology or overall visual completion.
