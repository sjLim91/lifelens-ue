from pathlib import Path

MERGE_SHA = "f5c8cbab3aa41c6a37c3bae06838eeb583749771"
HEAD_SHA = "c4d724b9650647ad986cd1ab235f4d8052840014"
PREFLIGHT = "34933128958"
CORE = "34933128953"
UE = "34933128950"


def read(path: str) -> str:
    return Path(path).read_text(encoding="utf-8")


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding="utf-8")


def replace_between(text: str, start: str, end: str, replacement: str) -> str:
    i = text.index(start)
    j = text.index(end, i)
    return text[:i] + replacement + text[j:]


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if old not in text:
        raise SystemExit(f"missing anchor: {label}")
    if text.count(old) != 1:
        raise SystemExit(f"non-unique anchor: {label} ({text.count(old)})")
    return text.replace(old, new, 1)


# -----------------------------------------------------------------------------
# WORK_STATE
# -----------------------------------------------------------------------------
path = "tasks/WORK_STATE.md"
t = read(path)

baseline = f'''Latest product merge:\n- PR #83 `[CORE] Add deterministic World Genesis WG-1 contracts`\n- merge SHA: `{MERGE_SHA}`\n\nValidated PR #83 final head `{HEAD_SHA}`:\n- Structural Preflight run `{PREFLIGHT}`: PASS, including World Genesis WG-1 validator\n- Core Tests run `{CORE}`: PASS, **46/46**\n- deterministic harness smoke: PASS\n- Unreal Linux Compile run `{UE}`: PASS\n- UE 5.6 image verification / UHT / UBT / link: PASS\n- PR comments/reviews/unresolved threads at merge checkpoint: 0\n\nDelivered by #83:\n- explicit `WorldSeed`, `PopulationSeed`, and `WorldGenerationVersion` runtime identity.\n- stable logical `ChunkCoord` with negative-coordinate floor semantics.\n- order-independent untouched chunk baseline derived from `(WorldSeed, GenerationVersion, ChunkCoord)`.\n- fixed deterministic mixer and separated terrain/climate/resource/detail chunk substreams.\n- founder generation and initial relationship familiarity now consume `PopulationSeed`, not the World RNG.\n- same WorldSeed with a different PopulationSeed preserves natural chunk identity while allowing different initial residents.\n- WG-1 is now a runtime contract, not design-only.\n\nPrevious product checkpoints:\n- PR #82 HumanWaste Environmental Visual Feedback — DONE, merge `831ba22ce17ca5fef8a92f2288e18a0495248a7a`.\n- PR #80 Dug sanitation pit progression — DONE, merge `291926cf78d12c1c61284eb9c59a50e7c70e54e7`.\n\n'''
t = replace_between(t, "Latest product merge:\n", "Documentation-only commits may advance", baseline)

wg2_dispatch = '''### Jjun lane — World Genesis WG-2 — READY_NOW\n\nOwner: 쭌 / 쭌 AI\nDependency: WG-1 DONE via PR #83; canonical `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.\nHandoff safety: SAFE; deterministic coordinate/seed contract is merged and validated.\n\nGoal:\n- add the lightweight macro natural-world layer and a deterministic viable initial start-site selector without creating civilization infrastructure.\n\nWG-2 scope:\n- macro elevation / moisture / temperature / biome-potential facts.\n- broad water / resource / fertility / traversal / hazard potential.\n- deterministic candidate-region scoring from World Genesis identity.\n- select a survivable-but-unsolved initial region.\n- initial state remains nature + 2 male + 2 female residents + zero civilization infrastructure.\n- no full detailed world materialization and no Unreal streaming authority yet.\n\nAcceptance:\n- same WorldSeed + GenerationVersion yields the same macro facts and start-region result.\n- PopulationSeed does not change geography/start-region ranking.\n- start-site selection uses environmental viability, not arbitrary `{0,0}` or a prebuilt settlement.\n- no house/toilet/farm/storage/road/tool is silently created.\n- relevant Core / Preflight / UE compile checks pass.\n\n'''
t = replace_between(t, "### Jjun lane — World Genesis WG-1 — READY_NOW\n", "### World Genesis / Chunk / Migration", wg2_dispatch)
t = t.replace("### World Genesis / Chunk / Migration — DESIGN FIXED / IMPLEMENTATION GATE", "### World Genesis / Chunk / Migration — WG-1 DONE / WG-2 READY_NOW", 1)
t = replace_once(t,
    "Implementation timing:\n- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.\n- before `World Visual Environment v1` becomes a production-sized permanent map, **WG-1 deterministic world/chunk coordinate contract and WG-2 macro world/start-site boundaries must be implemented or explicitly integrated into that work**.\n- do not lock the project into a hand-authored small arena that later requires a world rewrite.",
    "Implementation timing:\n- **WG-1 is implemented and merged via PR #83.**\n- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.\n- before `World Visual Environment v1` becomes a production-sized permanent map, **WG-2 macro world/start-site boundaries must be implemented or explicitly integrated into that work**.\n- do not lock the project into a hand-authored small arena that later requires a world rewrite.",
    "WORK_STATE world timing")
t = replace_once(t, "6. **HumanWaste Environmental Visual Feedback — READY_NOW.**", "6. **HumanWaste Environmental Visual Feedback — DONE #82.**", "WORK_STATE sanitation visual")
t = replace_once(t,
    "Actual visual implementation still pending:\n- **HumanWaste decal/material/VFX — READY_NOW.**",
    "Actual visual implementation:\n- **HumanWaste baseline ground feedback — DONE #82.**\n- material/mesh polish may continue later without changing simulation authority.",
    "WORK_STATE visual feedback")
t = t.replace("### World Genesis runtime — DESIGN FIXED / NOT IMPLEMENTED", "### World Genesis runtime — WG-1 IMPLEMENTED / WG-2 READY_NOW", 1)
t = replace_once(t,
    "- WG-1 deterministic world/chunk coordinates.\n- WG-2 macro world/start-site selection.",
    "- WG-1 deterministic world/chunk coordinates — **DONE #83**.\n- WG-2 macro world/start-site selection — **READY_NOW**.",
    "WORK_STATE risk phase list")
write(path, t)


# -----------------------------------------------------------------------------
# TEAM_BOARD
# -----------------------------------------------------------------------------
path = "tasks/TEAM_BOARD.md"
t = read(path)
t = replace_once(t,
    "| 쭌 + 쭌 AI | new branch after preflight | World Genesis WG-1 | Core world seed / deterministic chunk-coordinate contract | **READY_NOW** |",
    "| 쭌 + 쭌 AI | new branch after state sync | World Genesis WG-2 | Core macro world / viable start-site selector | **READY_NOW** |",
    "TEAM_BOARD active Jjun row")
t = replace_once(t,
    "| 쭌 + 다겸 lanes | before production World Visual | World Genesis WG-1/WG-2 integration gate | Core world coordinates + environment architecture | DESIGN FIXED / REQUIRED GATE |",
    "| 쭌 + 다겸 lanes | before production World Visual | World Genesis WG-2 integration gate | Macro world/start-site + environment architecture | WG-1 DONE / WG-2 REQUIRED GATE |",
    "TEAM_BOARD shared gate row")

latest = f'''## Latest Jjun product checkpoint — PR #83 DONE\n\nPR #83 `[CORE] Add deterministic World Genesis WG-1 contracts`\n- merge SHA: `{MERGE_SHA}`\n- validated head: `{HEAD_SHA}`\n\nValidation:\n- Structural Preflight `{PREFLIGHT}`: PASS, including WG-1 validator.\n- Core Tests `{CORE}`: PASS, **46/46**.\n- deterministic harness smoke: PASS.\n- Unreal Linux Compile `{UE}`: PASS.\n- UE 5.6 image verification / UHT / UBT / link: PASS.\n- merge checkpoint PR comments/reviews/unresolved threads: 0.\n\nDelivered:\n- separate WorldSeed / PopulationSeed / GenerationVersion runtime identity.\n- stable negative-safe ChunkCoord mapping.\n- order-independent untouched chunk baseline and deterministic per-domain substreams.\n- initial residents no longer consume/mutate the World RNG stream.\n- same natural world can be replayed with a different initial population.\n\n'''
t = replace_between(t, "## Latest Jjun product checkpoint — PR #82 DONE\n", "## Jjun next lane", latest)

next_lane = '''## Jjun next lane — World Genesis WG-2 — READY_NOW\n\nGoal:\n- build the deterministic macro natural-world layer and viable initial start-region selector on top of the merged WG-1 identity/coordinate contract.\n\nWG-2 boundary:\n- macro elevation / moisture / temperature / biome potential.\n- broad water / fertility / natural-resource / traversal / hazard potential.\n- deterministic viable-region scoring and initial-region selection.\n- nature + four founders only; zero civilization infrastructure.\n- detailed lazy chunks, persistent deltas, migration and Unreal streaming remain later phases.\n\nAcceptance:\n- geography/start-site is reproducible for the same WorldSeed + GenerationVersion.\n- changing PopulationSeed does not change natural macro facts.\n- no arbitrary fixed arena or `{0,0}`-only start assumption.\n- no hidden house/toilet/farm/storage/road/tool spawn.\n\n'''
t = replace_between(t, "## Jjun next lane — World Genesis WG-1 — READY_NOW\n", "## World Genesis / Chunk / Migration", next_lane)
t = t.replace("## World Genesis / Chunk / Migration — WG-1 READY_NOW / WG-2 AFTER WG-1", "## World Genesis / Chunk / Migration — WG-1 DONE / WG-2 READY_NOW", 1)
t = replace_once(t,
    "- #82 authoritative HumanWaste residue → Android-safe Unreal visual projection.",
    "- #82 authoritative HumanWaste residue → Android-safe Unreal visual projection.\n- #83 deterministic World Genesis WG-1 coordinate/seed/runtime contract.",
    "TEAM_BOARD repair status")
write(path, t)


# -----------------------------------------------------------------------------
# PROJECT_PROGRESS
# -----------------------------------------------------------------------------
path = "docs/PROJECT_PROGRESS_2026-09-15.md"
t = read(path)
progress_baseline = f'''Latest merged product slice:\n- PR #83 `[CORE] Add deterministic World Genesis WG-1 contracts`\n- merge SHA: `{MERGE_SHA}`\n- validated final head: `{HEAD_SHA}`\n\nValidation:\n- Structural Preflight `{PREFLIGHT}`: PASS, including WG-1 validator\n- Core Tests `{CORE}`: **46/46 PASS**\n- deterministic harness smoke: PASS\n- Unreal Linux Compile `{UE}`: PASS\n- UE 5.6 image verification / UHT / UBT / link: PASS\n- PR comments/reviews/unresolved threads at merge checkpoint: 0\n\nPrevious product slices:\n- #82 HumanWaste Environmental Visual Feedback — DONE.\n- #80 Dug sanitation pit progression — DONE.\n\n'''
t = replace_between(t, "Latest merged product slice:\n", "Documentation commits may advance", progress_baseline)
t = t.replace("## 5. World Genesis / map scalability — DESIGN FIXED", "## 5. World Genesis / map scalability — WG-1 IMPLEMENTED / WG-2 NEXT", 1)
status_note = f'''Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.\n\nImplementation checkpoint:\n- **WG-1 DONE via PR #83**, merge `{MERGE_SHA}`.\n- runtime now has separate WorldSeed/PopulationSeed/GenerationVersion identity, stable ChunkCoord conversion and order-independent untouched chunk baseline seeds.\n- **WG-2 Macro World + viable initial start-site selector is READY_NOW.**\n- detailed natural chunk materialization, persistence, migration and multi-settlement runtime remain later phases.\n'''
t = replace_once(t, "Canonical: `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.\n", status_note, "PROJECT_PROGRESS genesis canonical")

jnext = '''### Jjun lane — READY_NOW\n\n**World Genesis WG-2**\n\nGoal:\n- build deterministic macro terrain/climate/water/resource-potential facts and select a viable initial natural start region.\n- preserve the WG-1 WorldSeed/PopulationSeed split.\n- do not create civilization infrastructure to make the start easier.\n- keep detailed lazy chunks and Unreal streaming out of authority scope for this phase.\n\n'''
t = replace_between(t, "### Jjun lane — READY_NOW\n", "### Dagyeom lane — READY_NOW", jnext)
t = replace_once(t,
    "### B. World Genesis runtime — NOT IMPLEMENTED\nDesign is canonical, runtime phases WG-1~WG-7 remain.",
    "### B. World Genesis runtime — PARTIALLY IMPLEMENTED\nWG-1 is DONE via #83. WG-2~WG-7 remain, beginning with Macro World + viable start-site selection.",
    "PROJECT_PROGRESS risk B")
t = replace_once(t,
    "20. **World Genesis WG-1 — READY_NOW; WG-2 follows WG-1.**",
    "20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**",
    "PROJECT_PROGRESS sequence")
t = replace_once(t,
    "- World Genesis design fixed ≠ chunked world already running.",
    "- World Genesis WG-1 is implemented, but Macro World / detailed lazy chunks / persistent chunk history are not yet running.",
    "PROJECT_PROGRESS interpretation")
write(path, t)


# -----------------------------------------------------------------------------
# CANONICAL WORLD GENESIS DESIGN STATUS
# -----------------------------------------------------------------------------
path = "docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md"
t = read(path)
if "## Implementation status — 2026-09-15" not in t:
    anchor = "This document extends `docs/LIFELENS_SPEC_v1.1.md`, `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`, `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`, `docs/CIVILIZATION_PROGRESSION_v1.md`, and `docs/STATE_MANAGEMENT.md`.\n\n"
    note = f'''This document extends `docs/LIFELENS_SPEC_v1.1.md`, `docs/WORLD_AFFORDANCE_ENVIRONMENT_v1.md`, `docs/WORLD_VISUAL_ENVIRONMENT_v1.md`, `docs/CIVILIZATION_PROGRESSION_v1.md`, and `docs/STATE_MANAGEMENT.md`.\n\n## Implementation status — 2026-09-15\n\n- **WG-1 DONE** via PR #83, merge `{MERGE_SHA}`.\n- merged runtime contract includes `WorldSeed`, separate `PopulationSeed`, `WorldGenerationVersion`, stable `ChunkCoord`, negative-safe grid/chunk mapping, and order-independent untouched chunk baseline/substream seeds.\n- validation: Preflight `{PREFLIGHT}` PASS, Core `{CORE}` **46/46 PASS** + deterministic harness, Unreal Linux Compile `{UE}` PASS including UE 5.6 UHT/UBT/link.\n- **WG-2 READY_NOW:** Macro World facts + viable initial start-site selector.\n- WG-3+ detailed chunks/persistence/streaming/migration remain future implementation and must not be described as already running.\n\n'''
    t = replace_once(t, anchor, note, "WORLD_GENESIS status insertion")
t = replace_once(t,
    "This document is a design gate, not a claim that the following runtime exists today.",
    "This document remains the architecture gate. WG-1 is now implemented; WG-2 and later runtime phases remain incomplete until separately merged and validated.",
    "WORLD_GENESIS implementation preface")
t = replace_once(t,
    "### WG-1 — deterministic world coordinates and chunk keys\n- chunk coordinate contract\n- stable `Hash(WorldSeed, ChunkCoord, GenerationVersion)`\n- order-independence test\n\n### WG-2 — macro world + initial start-site selector",
    f"### WG-1 — deterministic world coordinates and chunk keys — **DONE #83**\n- merged as `{MERGE_SHA}`.\n- chunk coordinate contract.\n- stable deterministic derivation from `WorldSeed + ChunkCoord + GenerationVersion`.\n- negative-coordinate floor semantics.\n- PopulationSeed separated from natural-world identity.\n- order-independence test and Core 46/46 validation.\n\n### WG-2 — macro world + initial start-site selector — **READY_NOW**",
    "WORLD_GENESIS phase status")
write(path, t)


# -----------------------------------------------------------------------------
# APPEND-ONLY HANDOFF
# -----------------------------------------------------------------------------
path = "tasks/HANDOFF_LOG.md"
t = read(path)
marker = "### World Genesis WG-1 완료 — PR #83"
if marker not in t:
    entry = f'''\n\n## 2026-09-15 — 쭌 측 AI\n\n{marker}\n- 작성자: 쭌 측 AI\n- 브랜치/PR: `jjun/world-genesis-wg1`, PR #83\n- 상태: `DONE / main 병합 완료`\n- validated head: `{HEAD_SHA}`\n- merge SHA: `{MERGE_SHA}`\n- 변경 범위:\n  - `Source/LifeLensCore/include/lifelens/WorldGenesis.h`\n  - Core `World` / `Simulation` world-population seed separation\n  - `test_world_genesis_wg1`\n  - `Tools/validate_world_genesis_wg1.py` + Preflight gate\n- 구현:\n  - `WorldSeed`, `PopulationSeed`, `WorldGenerationVersion` 분리\n  - stable `ChunkCoord`, 음수 좌표 floor mapping\n  - `(WorldSeed, GenerationVersion, ChunkCoord)` 기반 order-independent untouched chunk baseline\n  - terrain/climate/resource/detail deterministic substream seeds\n  - founder/name/trait/genetics/initial familiarity randomization은 PopulationSeed 전용 RNG 사용\n  - PopulationSeed 변경이 자연 chunk identity를 바꾸지 않도록 계약 고정\n- 검증:\n  - Preflight `{PREFLIGHT}`: PASS\n  - Core Tests `{CORE}`: **46/46 PASS**\n  - deterministic harness smoke: PASS\n  - Unreal Linux Compile `{UE}`: PASS, UE 5.6 image verify / UHT / UBT / link PASS\n  - merge checkpoint comments/reviews/unresolved threads: 0\n- 상대가 알아야 할 점:\n  - WG-1은 이제 design-only가 아니라 실제 Core runtime 계약이다.\n  - 다음 쭌 레인은 **WG-2 Macro World + viable initial start-site selector — READY_NOW**.\n  - 아직 실제 biome/river/terrain chunk가 생성되는 단계는 아니다; WG-2/WG-3 이후다.\n  - 다겸 Motion/World Visual 작업은 WG-1 좌표/seed authority를 침범하지 말고, production map 고정 전 WG-2 boundary를 반영해야 한다.\n'''
    t = t.rstrip() + entry + "\n"
write(path, t)


# Final sync assertions.
checks = {
    "tasks/WORK_STATE.md": ["PR #83", "World Genesis WG-2 — READY_NOW", "WG-1 IMPLEMENTED / WG-2 READY_NOW"],
    "tasks/TEAM_BOARD.md": ["PR #83 DONE", "World Genesis WG-2 — READY_NOW", "WG-1 DONE / WG-2 READY_NOW"],
    "docs/PROJECT_PROGRESS_2026-09-15.md": ["PR #83", "WG-1 IMPLEMENTED / WG-2 NEXT", "WG-2 — READY_NOW"],
    "docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md": ["WG-1 DONE", "WG-2 READY_NOW", MERGE_SHA],
    "tasks/HANDOFF_LOG.md": [marker, MERGE_SHA, "46/46 PASS"],
}
for file, needles in checks.items():
    body = read(file)
    for needle in needles:
        if needle not in body:
            raise SystemExit(f"sync validation failed: {file} missing {needle}")

print("WG-1 state/document synchronization: PASS")
