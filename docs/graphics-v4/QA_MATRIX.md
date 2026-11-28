# G00 QA matrix — baseline (all execution pending)
Baseline `4b2039780aa39c6bc0580f6519875751bcc74093`. No lockfile at baseline; existing CI install command is authoritative.
| Check | Actual command or requirement | Status |
|---|---|---|
| Install | cd web && npm install --ignore-scripts --no-audit --no-fund | NOT_RUN |
| Types | cd web && npm run typecheck | NOT_RUN |
| Regression | python3 Tools/ci_web.py regressions | NOT_RUN |
| Build | cd web && npm run build | NOT_RUN |
| Preflight | .github/workflows/preflight.yml full job | NOT_RUN |
| Runtime | download web-runtime-latest release lifelens_core.js + .wasm; record release ID, actual SHA256 hashes and ABI/version | NOT_RUN |
| Smoke | existing Tools/probe_lifelens_field_browser.mjs on preview /?view=2.5d with Chromium; existing 390×844 thresholds | NOT_RUN |
| Fixed seed | use real seed replay UI; initial seed 12345 from existing web/tests/tests.json; confirm UI reports seed; record exact minute and camera | NOT_RUN |
| Baseline scene capture | real Core snapshots for land/forest/lake or coast/river/relief/settlement/day/night/rain/snow; report absent states as unverified | NOT_RUN |
| Viewport/zoom | 360×800, 412×915, 915×412, 1440×900; wide/mid/close, pan/zoom; record state, mode and actual CSS/DPR | NOT_RUN |
| Paint trace | instrument field drawImage destination and projected geometry without changing rendering; compare land/water and sampled shore; record causal screenshot | NOT_RUN |
| Frame and lifecycle | ≥10 min; average FPS, p95 frame interval, cache/memory/resource counts, exceptions, mode/resize/zoom; distinguish CI vs real device | NOT_RUN |
| Core immutability | compare complete changed paths and protected Core/WASM/simulation boundaries; snapshot identity replay/mode switch | NOT_RUN |
| Visual review | inspect actual captures independently of automated pass | NOT_RUN |

Existing `web/tests/tests.json` and workflows remain intact. No npm test script exists at this baseline. No fixture can satisfy actual Core acceptance. No default-mode switch. G00 remains incomplete until evidence is stored and reviewed. For missing scene states, report unverified; do not fabricate.
