#!/usr/bin/env python3
from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8")


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise SystemExit(f"{label}: expected exactly one match, found {text.count(old)}")
    return text.replace(old, new, 1)


# WORK_STATE: collapse the duplicated Motion lane into the existing canonical lane.
path = "tasks/WORK_STATE.md"
s = read(path)
start = "### Dagyeom lane — Character Motion Bootstrap — ACTIVE / FINAL VISUAL CHECK\n"
dup = "### Dagyeom lane — Character Motion Bootstrap — ACTIVE\n"
completed = "## Completed foundation / repair slices\n"
if s.count(start) != 1 or s.count(dup) != 1 or s.count(completed) != 1:
    raise SystemExit("WORK_STATE motion section anchors are not unique")

start_i = s.index(start)
dup_i = s.index(dup, start_i + len(start))
completed_i = s.index(completed, dup_i + len(dup))

canonical_motion = """### Dagyeom lane — Character Motion Bootstrap — ACTIVE / REVIEW CLOSEOUT

Owner: 다겸 / 다겸 AI (Claude)
Branch: `dagyeom/character-motion-v1`
PR: #84
Latest observed head: `fa781b0829a29f8b29b99fe19fe699cc53792966`
Handoff safety: CONDITIONAL — CI + PIE verified; final review / merge / state sync remain.

Delivered on branch:
- in-place Idle / Walk / Jog / Sprint locomotion BlendSpace.
- actual Actor movement measured over a 0.2s presentation window.
- body-mesh-only orientation smoothing; Actor/Core movement authority unchanged.
- stationary body orientation follows the owner's authoritative yaw so activity/social facing is preserved.
- teleport/load-like large steps are excluded entirely from locomotion speed accumulation.
- no Character-side action chooser and no root-motion authority.

Validation observed:
- Preflight `34940159290`: PASS.
- Unreal Linux Compile `34940159300`: PASS.
- PIE visual verification: PASS — Idle/Walk locomotion visible, T-pose resolved, no sliding, smooth turning.

Exact next action:
- close remaining review/state-doc reconciliation → merge #84 → final canonical state sync.

"""

# Replace the first canonical section and remove the appended duplicate section.
s = s[:start_i] + canonical_motion + s[completed_i:]

s = replace_once(
    s,
    "4. Motion Bootstrap — **ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).",
    "4. Motion Bootstrap — **ACTIVE #84; CI + PIE PASS, review closeout/merge pending** (Dagyeom lane).",
    "WORK_STATE character sequence",
)
s = replace_once(
    s,
    "19. **Motion Bootstrap — READY_NOW** (Dagyeom lane).",
    "19. **Motion Bootstrap — ACTIVE #84; CI + PIE PASS, review closeout/merge pending** (Dagyeom lane).",
    "WORK_STATE execution order",
)
write(path, s)


# TEAM_BOARD: remove the stale READY_NOW dispatch for Motion and keep one ACTIVE truth.
path = "tasks/TEAM_BOARD.md"
s = read(path)
s = replace_once(
    s,
    "| 다겸 + 다겸 AI (Claude) | `dagyeom/character-motion-v1` / #84 | Character Motion Bootstrap | Character locomotion presentation | **ACTIVE — CI PASS / PIE+review pending** |",
    "| 다겸 + 다겸 AI (Claude) | `dagyeom/character-motion-v1` / #84 | Character Motion Bootstrap | Character locomotion presentation | **ACTIVE — CI+PIE PASS / review closeout pending** |",
    "TEAM_BOARD active row",
)
old_next = """Next Dagyeom lane:
- **Character Motion Bootstrap — READY_NOW.**
- scope: Idle / Walk / Jog + velocity-driven switching + orientation smoothing.
- Core/World continues to own movement/action authority; animation only reflects runtime movement state.
"""
new_next = """Current Dagyeom lane:
- **Character Motion Bootstrap — ACTIVE #84.**
- head `fa781b0829a29f8b29b99fe19fe699cc53792966`; Preflight `34940159290` PASS; Unreal Linux Compile `34940159300` PASS; PIE visual verification PASS.
- remaining: review closeout → merge → final canonical state sync.
- Core/World continues to own movement/action authority; animation only reflects runtime movement state.
"""
s = replace_once(s, old_next, new_next, "TEAM_BOARD current Dagyeom lane")
s = replace_once(
    s,
    "19. **Character Motion Bootstrap — ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).",
    "19. **Character Motion Bootstrap — ACTIVE #84; CI + PIE PASS, review closeout/merge pending** (Dagyeom lane).",
    "TEAM_BOARD execution queue",
)
write(path, s)

print("#84 canonical motion-lane reconciliation: PASS")
