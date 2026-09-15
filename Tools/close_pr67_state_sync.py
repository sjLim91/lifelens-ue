from pathlib import Path
import re

MERGE_SHA = "915906357d9752a5654b3dfeb85795419d885b59"
HEAD_SHA = "982d930a53f199b33ebf7ca3d4f5b72f72f8a91b"


def must_replace(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"missing replacement target: {label}")
    return text.replace(old, new, 1)


def must_sub(text: str, pattern: str, replacement: str, label: str) -> str:
    out, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise SystemExit(f"regex replacement count {count}: {label}")
    return out

# -----------------------------------------------------------------------------
# WORK_STATE
# -----------------------------------------------------------------------------
p = Path("tasks/WORK_STATE.md")
s = p.read_text()
s = must_replace(
    s,
    "Latest product merge:\n- PR #80 `[CORE] Add dug sanitation pit progression v1`\n- merge SHA: `291926cf78d12c1c61284eb9c59a50e7c70e54e7`\n\nValidated PR #80 head `fc918693bcd265f3c021f0bd826f6ba5a7113678`:",
    f"Latest product merge:\n- PR #67 `[UI] Character Appearance v1 — Quaternius CC0 human body, deterministic look (Track B)`\n- merge SHA: `{MERGE_SHA}`\n\nValidated PR #67 final head `{HEAD_SHA}`:\n- Structural Preflight run `34929703738`: PASS\n- Unreal Linux Compile run `34929703712`: PASS\n- UE 5.6 image verification / UHT / UBT / link: PASS\n- helper PR #81 integration tree Core Tests run `34929611584`: PASS, **45/45** + deterministic harness smoke PASS\n\nLatest Core/civilization baseline remains PR #80 `[CORE] Add dug sanitation pit progression v1`.\n\nValidated PR #80 head `fc918693bcd265f3c021f0bd826f6ba5a7113678`:",
    "work-state latest product",
)

s = must_sub(
    s,
    r"### Dagyeom lane — Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT.*?\n## Completed foundation / repair slices",
    f"""### Dagyeom lane — Motion Bootstrap — READY_NOW

Owner: 다겸 / 다겸 AI (Claude)
Dependency: PR #67 DONE.
Branch / PR: not yet observed on remote at this reconciliation checkpoint.

Character Appearance v1 closeout:
- PR #67 final head `{HEAD_SHA}`.
- helper PR #81 merged into the Dagyeom branch to sync latest main and close the final three review items.
- tracked `__pycache__/*.pyc` removed and ignore rules added.
- duplicate appearance/presentation includes removed.
- male Peasant exposed forearm/hand skin now selects the same light/dark skin BaseColor axis and tint as face/head.
- prior `CHANGES_REQUESTED` review dismissed after verification; final review approved.
- Preflight `34929703738`: PASS.
- Unreal Linux Compile `34929703712`: PASS including UE 5.6 image verify / UHT / UBT / link.
- helper integration Core Tests `34929611584`: PASS, 45/45 + deterministic harness.
- squash merge to main: `{MERGE_SHA}`.

Motion Bootstrap scope:
- Idle / Walk / Jog.
- velocity/movement-state-driven presentation switching.
- basic orientation smoothing.
- remove Idle-looking slide.
- Character animation remains presentation only; Core/World remains movement/action authority.

## Completed foundation / repair slices""",
    "work-state dagyeom section",
)

s = must_replace(s, "3. Character Appearance v1 — ACTIVE #67 closeout.\n4. Motion Bootstrap — READY_AFTER_#67.", "3. Character Appearance v1 — DONE via #67.\n4. Motion Bootstrap — **READY_NOW** (Dagyeom lane).", "work-state character sequence")
p.write_text(s)

# -----------------------------------------------------------------------------
# TEAM_BOARD
# -----------------------------------------------------------------------------
p = Path("tasks/TEAM_BOARD.md")
s = p.read_text()
s = must_replace(
    s,
    "| 다겸 + 다겸 AI | PR #67 `dagyeom/character-appearance-v1` | Character Appearance v1 closeout | Character appearance + `Content/Characters/**` | **ACTIVE / CLOSEOUT** |\n| 다겸 + 다겸 AI | after #67 | Character Motion Bootstrap | Character locomotion presentation | READY_AFTER_#67 |",
    "| 다겸 + 다겸 AI (Claude) | branch / PR pending | Character Motion Bootstrap | Character locomotion presentation | **READY_NOW** |",
    "team-board active table",
)

s = must_sub(
    s,
    r"## Character Appearance v1 — ACTIVE / PR #67 CLOSEOUT.*?\n## Core ↔ World repair status",
    f"""## Latest Dagyeom product checkpoint — PR #67 DONE

PR #67 `[UI] Character Appearance v1 — Quaternius CC0 human body, deterministic look (Track B)`
- final head: `{HEAD_SHA}`
- main squash merge: `{MERGE_SHA}`
- helper closeout: PR #81 merged into `dagyeom/character-appearance-v1`

Final closeout delivered:
- committed Python cache artifact removed; `.gitignore` now covers `__pycache__/` and `*.pyc`.
- duplicate character includes removed.
- bright-skin male Peasant exposed arms/hands use the same deterministic light/dark skin BaseColor choice and tint as the resident body/head.
- latest main was integrated without moving simulation authority into Character presentation.

Validation:
- #67 final-head Preflight `34929703738`: PASS.
- #67 final-head Unreal Linux Compile `34929703712`: PASS including UE 5.6 image verify / UHT / UBT / link.
- helper integration Core Tests `34929611584`: PASS, **45/45** + deterministic harness smoke.
- previous blocking review dismissed after verification; final review approved.

Next Dagyeom lane:
- **Character Motion Bootstrap — READY_NOW.**
- scope: Idle / Walk / Jog + velocity-driven switching + orientation smoothing.
- Core/World continues to own movement/action authority; animation only reflects runtime movement state.

## Core ↔ World repair status""",
    "team-board appearance section",
)

s = must_replace(
    s,
    "17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — ACTIVE CLOSEOUT in parallel.\n19. Character Motion Bootstrap — after #67.",
    "17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — DONE.\n19. **Character Motion Bootstrap — READY_NOW** (Dagyeom lane).",
    "team-board queue",
)
p.write_text(s)

# -----------------------------------------------------------------------------
# PROJECT_PROGRESS
# -----------------------------------------------------------------------------
p = Path("docs/PROJECT_PROGRESS_2026-09-15.md")
s = p.read_text()
s = must_replace(
    s,
    "Latest merged product slice:\n- PR #80 `[CORE] Add dug sanitation pit progression v1`\n- merge SHA: `291926cf78d12c1c61284eb9c59a50e7c70e54e7`\n- validated head: `fc918693bcd265f3c021f0bd826f6ba5a7113678`\n\nValidation:\n- Structural Preflight `34926841905`: PASS\n- Core Tests `34926841895`: **45/45 PASS**\n- deterministic harness smoke: PASS\n- Unreal Linux Compile `34926841907`: PASS\n- UE 5.6 image verification / UHT / UBT / link: PASS",
    f"""Latest merged product slice:
- PR #67 `[UI] Character Appearance v1 — Quaternius CC0 human body, deterministic look (Track B)`
- merge SHA: `{MERGE_SHA}`
- validated final head: `{HEAD_SHA}`

Validation:
- Structural Preflight `34929703738`: PASS
- Unreal Linux Compile `34929703712`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- helper PR #81 integration Core Tests `34929611584`: **45/45 PASS** + deterministic harness smoke

Latest Core/civilization slice remains PR #80, merge `291926cf78d12c1c61284eb9c59a50e7c70e54e7`, with its previously recorded 45/45 + Preflight + UE compile validation.""",
    "progress latest slice",
)

s = must_sub(
    s,
    r"### Character Appearance v1 — PR #67 OPEN / CLOSEOUT\n.*?\n## 7\. Immediate next execution",
    f"""### Character Appearance v1 — PR #67 DONE

Owner: 다겸 / 다겸 AI

Final head / merge:
- final head: `{HEAD_SHA}`
- main squash merge: `{MERGE_SHA}`

Delivered:
- Quaternius CC0 humanoids.
- deterministic #65 appearance mapping.
- hair / skin / body variation.
- UAL animation assets.
- Peasant outfit + head-only derivative to avoid clothing penetration.
- appearance construction after resident identity binding.
- same-resident appearance continuity across restart/load.
- tracked `__pycache__/*.pyc` removed and ignore rules added.
- duplicate includes removed.
- male Peasant exposed skin now matches deterministic body/head light/dark BaseColor + tint.

Final validation:
- Preflight `34929703738`: PASS.
- Unreal Linux Compile `34929703712`: PASS including UHT / UBT / link.
- helper integration Core Tests `34929611584`: 45/45 PASS + deterministic harness.

Known presentation limitation now promoted to next work:
- locomotion is not wired yet; Idle-looking slide is Motion Bootstrap scope.

## 7. Immediate next execution""",
    "progress appearance section",
)

s = must_replace(
    s,
    "### Dagyeom lane\n\n1. #67 closeout / merge.\n2. Motion Bootstrap — Idle / Walk / Jog + orientation.\n3. World Genesis WG-1/WG-2 integration must be respected before production-sized World Visual map commitment.\n4. World Visual Environment v1.\n5. remaining Motion & Context.\n6. Observer UX polish / mobile touch.",
    "### Dagyeom lane — READY_NOW\n\n1. **Motion Bootstrap — Idle / Walk / Jog + velocity-driven transition + orientation smoothing.**\n2. World Genesis WG-1/WG-2 integration must be respected before production-sized World Visual map commitment.\n3. World Visual Environment v1.\n4. remaining Motion & Context.\n5. Observer UX polish / mobile touch.\n\nMotion is presentation-only: Core/World remains movement/action authority.",
    "progress dagyeom lane",
)

s = must_replace(
    s,
    "17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — ACTIVE in parallel.\n19. Motion Bootstrap — after #67.",
    "17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — DONE.\n19. **Motion Bootstrap — READY_NOW** (Dagyeom lane).",
    "progress sequence",
)

s = must_replace(s, "- #67 CI PASS ≠ #67 review closeout/merge done.\n", "- #67 is DONE; Motion Bootstrap needs its own branch/PR/validation before it is DONE.\n", "progress interpretation")
p.write_text(s)

# -----------------------------------------------------------------------------
# HANDOFF_LOG — append only
# -----------------------------------------------------------------------------
p = Path("tasks/HANDOFF_LOG.md")
s = p.read_text()
marker = "## 2026-09-15 — Character Appearance v1 PR #67 closeout 완료"
if marker not in s:
    entry = f"""

{marker}

- 작성자: 쭌 측 AI / 다겸 lane assist
- 상태: `DONE / main squash merge 완료`
- PR #67: `dagyeom/character-appearance-v1`
- final head: `{HEAD_SHA}`
- main merge: `{MERGE_SHA}`
- helper PR #81: `jjun/assist-67-closeout` → `dagyeom/character-appearance-v1`, merge `982d930a53f199b33ebf7ca3d4f5b72f72f8a91b`
- closeout 변경:
  - tracked `Content/Characters/Quaternius/Import/__pycache__/make_headonly_gltf.cpython-314.pyc` 제거.
  - `.gitignore`에 `__pycache__/`, `*.pyc` 추가.
  - `LLResidentCharacter.cpp` appearance/presentation 중복 include 제거.
  - male Peasant `Regular` 노출 피부 슬롯이 `SkinToneAxis`에 따라 body/head와 동일한 light/dark BaseColorTexture를 선택하고 동일 tint를 적용하도록 수정.
  - latest main 동기화 후 #67 mergeability 복구.
- 검증:
  - #67 final-head Structural Preflight `34929703738` PASS.
  - #67 final-head Unreal Linux Compile `34929703712` PASS; UE 5.6 image verification / UHT / UBT / link 모두 SUCCESS.
  - helper PR #81 integration Core Tests `34929611584` PASS, **45/45** + deterministic harness smoke PASS.
  - 기존 `CHANGES_REQUESTED` review는 closeout 검증 후 dismissed, final review APPROVED.
- 제품 결과:
  - Quaternius CC0 deterministic humanoid appearance baseline이 main에 병합됨.
  - Peasant 기본 의상, head-only 파생 body, hair/skin/body variation, identity-bound deterministic appearance, Save/Load appearance continuity baseline 포함.
  - Character presentation은 simulation/action authority를 소유하지 않음.
- 다음 다겸 레인:
  - **Character Motion Bootstrap — READY_NOW**.
  - 범위: Idle / Walk / Jog, velocity/movement-state 기반 전환, orientation smoothing, Idle-slide 제거.
  - Core/World 이동 및 action authority는 유지하고 animation은 결과를 표현만 한다.
- 병렬 쭌 레인:
  - HumanWaste Environmental Visual Feedback — READY_NOW.
"""
    p.write_text(s.rstrip() + entry + "\n")

# Remove one-shot files before the commit created by the workflow.
Path("Tools/close_pr67_state_sync.py").unlink(missing_ok=True)
Path(".github/workflows/close-pr67-state-sync.yml").unlink(missing_ok=True)
