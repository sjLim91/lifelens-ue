# Tiger source/web integration: incomplete checkpoint

This checkpoint is not runtime acceptance. Main must not be merged.

## Confirmed repository state

Draft PR: https://github.com/sjLim91/lifelens-ue/pull/728

Last confirmed branch head: `09cd15252b8da56ed78a934801a740f3e938d9bb`.
GitHub reads succeed. Two tree-creation requests and one new-file request did
not return before cancellation. Rechecking the branch showed no movement.
Do not assume any local changes below were published. The shell proxy at
`proxy:8080` also refuses connections, preventing npm/dependency acquisition.

## Source and inspection distinction

Published source evidence on the Draft branch contains the original TopoForge
Tiger, source URL, CC BY 4.0 attribution, acquisition time and SHA-256.
The requested Sketchfab model required authentication; no bypass was attempted.

The original source hash is
`f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be`.

Local Blender experiments use rounded inspection positions (1e-6 precision),
the original inspection UVs and a reduced 1024 JPEG. These local GLBs and blend
files are **inspection derivatives**, not verified production originals.
The staging script rejects them because the source hash differs. Regenerate
the candidate in Actions from the pinned original before Web acceptance.

## Local work preserved

- Source-preserving display-base extraction, original corner UVs retained.
- Source-derived skin with 24 joints, four-weight limit, no unbound vertices.
- Seven in-place clips: idle, walk, run, hunt, forage, drink and rest.
- Near/mid inspection meshes: 16,913 / 5,919 triangles.
- Embedded source, orientation, clip-distance and LOD contract.
- WildlifeLayer loading/pooling candidate, maximum two near plus four mid
  skinned tigers; far and load failures retain the existing instanced fallback.
- Per-Core-ID actor ownership and measured presentation travel controlling
  animation playback; no Core/simulation/AI/save-state changes.
- Source-hash-gated staging and CC BY credits page generation.

## Actual validation and limitations

Local Python syntax checks passed. Node 24 TypeScript stripping accepted both
changed TS files; this is syntax checking only, **not TypeScript typecheck**.

Blender 4.3.2 flat-ground inspection walk evaluation: maximum sole-height
error 0.013402 m; maximum cumulative stance slip 0.019868 m; horizontal root
animation displacement zero. These results apply only to the local inspection
derivative and the tested walk cycle. They do not validate slopes, turns, all
clips or the production browser path.

Not run: exact-source final rig in Actions, latest Web typecheck/build,
runtime/regression suite, actual Observer/WASM screenshots, slope/turn foot
locking, Android device performance, resident final quality or retargeting.
The source only supplies a static mesh: all rig/clip changes are adaptations.
Rest/forage/drink anatomy and clip transitions still require visual review.
Do not imply source-author approval of the adaptation.

## Recovery

The companion patch is based on the confirmed Draft head above. Review and
apply it in a full checkout with `git apply --check` followed by `git apply`.
It does not include or modify LifeLensCore. Original CC BY source evidence
remains on the PR. The updated workflow must regenerate from that exact source,
then Web checks and actual Observer/device verification must pass before any
merge decision. The separate inspection archive is for diagnosis only and
must not be copied into `web/public/vendor/wildlife`.
