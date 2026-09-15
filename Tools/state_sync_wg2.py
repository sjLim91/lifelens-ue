from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return (ROOT / path).read_text(encoding='utf-8')


def write(path, text):
    (ROOT / path).write_text(text, encoding='utf-8')


def sub_once(text, pattern, replacement, label):
    new, count = re.subn(pattern, replacement, text, count=1, flags=re.S)
    if count != 1:
        raise RuntimeError(f'{label}: expected exactly one match, got {count}')
    return new

# -----------------------------------------------------------------------------
# WORK_STATE
# -----------------------------------------------------------------------------
path = 'tasks/WORK_STATE.md'
t = read(path)

t = sub_once(
    t,
    r'Latest product merge:\n.*?Documentation-only commits may advance `main` beyond the product-code baseline above\.',
    '''Latest product merge:
- PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`

Validated PR #85 final head `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`:
- Structural Preflight run `34935360828`: PASS, including WG-2 validator
- Core Tests run `34935360834`: PASS, **47/47**
- deterministic harness smoke: PASS
- Unreal Linux Compile run `34935360862`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- PR comments/reviews/unresolved threads at merge checkpoint: 0

Delivered by #85:
- deterministic coherent macro elevation / moisture / temperature fields.
- broad water / fertility / wood / stone / food / traversal / hazard potentials.
- deterministic biome classification from WorldSeed + GenerationVersion, independent of PopulationSeed.
- deterministic viable initial start-region scoring over 625 candidates.
- selected start is survivable-but-unsolved; no house/toilet/farm/road/tool is created.
- current bootstrap presentation is not forcibly relocated before detailed chunk materialization exists.

Previous product checkpoints:
- PR #83 World Genesis WG-1 — DONE, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- PR #82 HumanWaste Environmental Visual Feedback — DONE, merge `831ba22ce17ca5fef8a92f2288e18a0495248a7a`.
- PR #80 Dug sanitation pit progression — DONE, merge `291926cf78d12c1c61284eb9c59a50e7c70e54e7`.

Documentation-only commits may advance `main` beyond the product-code baseline above.''',
    'WORK_STATE product baseline')

t = sub_once(
    t,
    r'### Jjun lane — World Genesis WG-2 — READY_NOW\n.*?(?=### World Genesis / Chunk / Migration)',
    '''### Jjun lane — World Generation Milestone A — READY_NOW

Owner: 쭌 / 쭌 AI
Dependency: WG-1 DONE #83 + WG-2 DONE #85; canonical `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`.
Handoff safety: SAFE.

Development-unit rule:
- from this checkpoint forward, same-purpose / same-layer / same-validation work is grouped into a milestone-sized PR instead of one PR per small contract.
- intermediate commits may be small, but canonical state sync and heavy UE validation happen at meaningful milestone checkpoints.

Milestone A goal:
- turn the WG-1/WG-2 logical world contracts into the first actually materializable natural-world slice without locking LifeLens to a fixed arena.

Milestone A scope:
- deterministic detailed natural chunk baseline (WG-3 core).
- biome/macro-fact-driven local natural resources/environment facts.
- generated-chunk registry and no-reroll identity.
- selected initial start-region materialization boundary.
- initial founder spawn integration into the materialized start region, with zero civilization infrastructure.
- minimum persistence boundary needed so generated/visited natural state cannot silently reroll across unload/load.
- minimal Core/Bridge read contracts needed by later World Visual presentation.

Acceptance:
- generation remains independent of exploration order.
- same WorldSeed + GenerationVersion reproduces untouched detail.
- PopulationSeed does not alter natural world detail.
- start region chosen by WG-2 is the region materialized for production NEW GAME.
- no prebuilt house/toilet/farm/storage/road/tool appears.
- unload/load or Save/Load cannot silently reroll generated/modified regions.
- relevant Core / Preflight / UE compile checks pass once at milestone merge gate.

''',
    'WORK_STATE Jjun dispatch')

t = t.replace('### World Genesis / Chunk / Migration — WG-1 DONE / WG-2 READY_NOW',
              '### World Genesis / Chunk / Migration — WG-1 DONE / WG-2 DONE / MILESTONE A READY_NOW')
t = t.replace('- **WG-1 is implemented and merged via PR #83.**\n- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.\n- before `World Visual Environment v1` becomes a production-sized permanent map, **WG-2 macro world/start-site boundaries must be implemented or explicitly integrated into that work**.',
              '- **WG-1 is implemented and merged via PR #83.**\n- **WG-2 macro world/start-site selection is implemented and merged via PR #85.**\n- small bootstrap/test maps remain allowed for current Core/Bridge/animation validation.\n- production World Visual must now consume the merged WG-1/WG-2 contracts; the next Jjun unit is World Generation Milestone A.')

t = sub_once(
    t,
    r'### Dagyeom lane — Motion Bootstrap — READY_NOW\n.*?(?=## Completed foundation / repair slices)',
    '''### Dagyeom lane — Character Motion Bootstrap — ACTIVE / FINAL VISUAL CHECK

Owner: 다겸 / 다겸 AI (Claude)
Branch: `dagyeom/character-motion-v1`
PR: #84
Latest observed head: `70c6544abda45f547f900242753abe523a29e045`
Handoff safety: CONDITIONAL — CI green; PIE visual confirmation / final review / merge remain.

Delivered on branch:
- in-place Idle / Walk / Jog / Sprint locomotion BlendSpace.
- actual Actor movement measured over a 0.2s presentation window.
- body-mesh-only orientation smoothing; Actor/Core movement authority unchanged.
- teleport/load-like large steps are excluded from locomotion measurement.
- no Character-side action chooser and no root-motion authority.

Validation observed:
- Preflight `34935329473`: PASS.
- Unreal Linux Compile `34935329453`: PASS.

Exact next action:
- PIE visual confirmation → review/comments check → merge → final canonical state sync.

''',
    'WORK_STATE Dagyeom dispatch')

t = t.replace('4. Motion Bootstrap — **READY_NOW** (Dagyeom lane).', '4. Motion Bootstrap — **ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).')
t = t.replace('5. **World Genesis WG-1 — DONE #83 / WG-2 architectural gate — READY_NOW** before production-sized permanent World Visual implementation.',
              '5. **World Genesis WG-1 DONE #83 / WG-2 DONE #85; World Generation Milestone A — READY_NOW.**')
t = t.replace('### World Genesis runtime — WG-1 IMPLEMENTED / WG-2 READY_NOW',
              '### World Genesis runtime — WG-1 + WG-2 IMPLEMENTED / MILESTONE A READY_NOW')
t = t.replace('- WG-2 macro world/start-site selection — **READY_NOW**.\n- WG-3 lazy natural chunks.\n- WG-4 persistent chunk deltas.',
              '- WG-2 macro world/start-site selection — **DONE #85**.\n- **World Generation Milestone A — READY_NOW:** WG-3 detailed natural chunks + start-region materialization + initial spawn integration + minimum no-reroll persistence boundary.\n- deeper WG-4 persistent world deltas continue inside/after Milestone A as scope proves safe.')
t = t.replace('20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**',
              '20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**')
write(path, t)

# -----------------------------------------------------------------------------
# TEAM_BOARD
# -----------------------------------------------------------------------------
path = 'tasks/TEAM_BOARD.md'
t = read(path)
t = t.replace('| 쭌 + 쭌 AI | new branch after state sync | World Genesis WG-2 | Core macro world / viable start-site selector | **READY_NOW** |',
              '| 쭌 + 쭌 AI | new milestone branch after state sync | World Generation Milestone A | detailed natural chunks + start-region materialization + spawn/persistence boundary | **READY_NOW** |')
t = t.replace('| 다겸 + 다겸 AI (Claude) | branch / PR pending | Character Motion Bootstrap | Character locomotion presentation | **READY_NOW** |',
              '| 다겸 + 다겸 AI (Claude) | `dagyeom/character-motion-v1` / #84 | Character Motion Bootstrap | Character locomotion presentation | **ACTIVE — CI PASS / PIE+review pending** |')
t = t.replace('| 쭌 + 다겸 lanes | before production World Visual | World Genesis WG-2 integration gate | Macro world/start-site + environment architecture | WG-1 DONE / WG-2 REQUIRED GATE |',
              '| 쭌 + 다겸 lanes | before production World Visual | World generation integration gate | WG-1/WG-2 authority + materialized chunk boundary | WG-1/WG-2 DONE / MILESTONE A NEXT |')

t = sub_once(
    t,
    r'## Latest Jjun product checkpoint — PR #83 DONE\n.*?(?=## Jjun next lane)',
    '''## Latest Jjun product checkpoint — PR #85 DONE

PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`
- validated head: `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`

Validation:
- Structural Preflight `34935360828`: PASS, including WG-2 validator.
- Core Tests `34935360834`: PASS, **47/47**.
- deterministic harness smoke: PASS.
- Unreal Linux Compile `34935360862`: PASS.
- UE 5.6 image verification / UHT / UBT / link: PASS.
- merge checkpoint PR comments/reviews/unresolved threads: 0.

Delivered:
- coherent deterministic macro terrain/climate fields and biome classification.
- broad water/fertility/natural-resource/traversal/hazard potential.
- PopulationSeed-independent geography and start-region ranking.
- deterministic viable start-region selection over 625 candidates.
- no starting civilization infrastructure and no forced relocation outside the current bootstrap surface before chunk materialization exists.

''',
    'TEAM_BOARD latest Jjun checkpoint')

t = sub_once(
    t,
    r'## Jjun next lane — World Genesis WG-2 — READY_NOW\n.*?(?=## World Genesis / Chunk / Migration)',
    '''## Jjun next lane — World Generation Milestone A — READY_NOW

Purpose:
- reduce PR/CI/document churn by grouping the next tightly coupled world-generation work into one milestone-sized delivery.

Scope:
- WG-3 deterministic detailed natural chunk baseline.
- macro/biome-driven local nature/resource facts.
- generated-chunk registry / no-reroll identity.
- WG-2 selected start-region materialization boundary.
- initial founder spawn integration into that region.
- minimum persistence boundary required for unload/load and Save/Load continuity.
- minimal Bridge read path required by later World Visual presentation.

Boundary:
- still no house/toilet/farm/storage/road/tool auto-spawn.
- Unreal streaming/PCG remains presentation/implementation, not simulation authority.
- one heavy UE compile gate at milestone close unless an earlier interface change specifically requires it.

''',
    'TEAM_BOARD next Jjun lane')

t = t.replace('## World Genesis / Chunk / Migration — WG-1 DONE / WG-2 READY_NOW',
              '## World Genesis / Chunk / Migration — WG-1 DONE / WG-2 DONE / MILESTONE A READY_NOW')
t = t.replace('- WG-1 deterministic world coordinates/chunk keys are **DONE #83**.\n- before `World Visual Environment v1` becomes a permanent production-sized map, WG-2 macro world + viable start-site boundaries must be implemented or explicitly integrated.',
              '- WG-1 deterministic world coordinates/chunk keys are **DONE #83**.\n- WG-2 macro world + viable start-site selection is **DONE #85**.\n- production World Visual must consume these contracts; World Generation Milestone A materializes the first detailed natural/start-region slice.')
t = t.replace('19. **Character Motion Bootstrap — READY_NOW** (Dagyeom lane).',
              '19. **Character Motion Bootstrap — ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).')
t = t.replace('20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**',
              '20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**')
write(path, t)

# -----------------------------------------------------------------------------
# PROJECT_PROGRESS
# -----------------------------------------------------------------------------
path = 'docs/PROJECT_PROGRESS_2026-09-15.md'
t = read(path)
t = sub_once(
    t,
    r'Latest merged product slice:\n.*?Documentation commits may advance `main` beyond the product-code merge SHA without changing runtime behavior\.',
    '''Latest merged product slice:
- PR #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- merge SHA: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`
- validated final head: `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`

Validation:
- Structural Preflight `34935360828`: PASS, including WG-2 validator
- Core Tests `34935360834`: **47/47 PASS**
- deterministic harness smoke: PASS
- Unreal Linux Compile `34935360862`: PASS
- UE 5.6 image verification / UHT / UBT / link: PASS
- PR comments/reviews/unresolved threads at merge checkpoint: 0

Previous product slices:
- #83 World Genesis WG-1 — DONE.
- #82 HumanWaste Environmental Visual Feedback — DONE.
- #80 Dug sanitation pit progression — DONE.

Documentation commits may advance `main` beyond the product-code merge SHA without changing runtime behavior.''',
    'PROJECT_PROGRESS baseline')

t = t.replace('## 5. World Genesis / map scalability — WG-1 IMPLEMENTED / WG-2 NEXT',
              '## 5. World Genesis / map scalability — WG-1 + WG-2 IMPLEMENTED / MILESTONE A NEXT')
t = t.replace('- **WG-1 DONE via PR #83**, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.\n- runtime now has separate WorldSeed/PopulationSeed/GenerationVersion identity, stable ChunkCoord conversion and order-independent untouched chunk baseline seeds.\n- **WG-2 Macro World + viable initial start-site selector is READY_NOW.**\n- detailed natural chunk materialization, persistence, migration and multi-settlement runtime remain later phases.',
              '- **WG-1 DONE via PR #83**, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.\n- runtime has separate WorldSeed/PopulationSeed/GenerationVersion identity, stable ChunkCoord conversion and order-independent untouched chunk baseline seeds.\n- **WG-2 DONE via PR #85**, merge `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`.\n- runtime now derives coherent macro elevation/moisture/temperature/biome/resource potential and deterministically selects a viable-but-unsolved start region.\n- next is **World Generation Milestone A**: detailed natural chunks + selected start-region materialization + initial spawn integration + minimum no-reroll persistence boundary.')
t = t.replace('- WG-2 macro world/start-site selection boundaries must be established or explicitly integrated.',
              '- WG-2 macro world/start-site selection boundaries are **established via #85**.\n- production World Visual must consume WG-1/WG-2 and the materialization boundary from World Generation Milestone A.')

t = sub_once(
    t,
    r'### Jjun lane — READY_NOW\n\n\*\*World Genesis WG-2\*\*\n.*?(?=### Dagyeom lane — READY_NOW)',
    '''### Jjun lane — READY_NOW

**World Generation Milestone A**

Goal:
- deliver the first detailed, materializable natural-world slice as one meaningful milestone instead of separate micro-PRs.
- include deterministic detailed chunks, selected start-region materialization, founder spawn integration and the minimum persistence/no-reroll boundary.
- preserve zero starting civilization infrastructure and Core world authority.

''',
    'PROJECT_PROGRESS Jjun next')

t = t.replace('### Dagyeom lane — READY_NOW\n\n1. **Motion Bootstrap — Idle / Walk / Jog + velocity-driven transition + orientation smoothing.**',
              '### Dagyeom lane — ACTIVE\n\n1. **Motion Bootstrap PR #84 — CI PASS; PIE visual confirmation / final review / merge pending.**')
t = t.replace('### B. World Genesis runtime — PARTIALLY IMPLEMENTED\nWG-1 is DONE via #83. WG-2~WG-7 remain, beginning with Macro World + viable start-site selection.',
              '### B. World Genesis runtime — PARTIALLY IMPLEMENTED\nWG-1 is DONE via #83 and WG-2 is DONE via #85. The next grouped delivery is World Generation Milestone A, combining detailed natural chunks, start-region materialization, initial spawn integration and a minimum persistence/no-reroll boundary.')
t = t.replace('19. **Motion Bootstrap — READY_NOW** (Dagyeom lane).',
              '19. **Motion Bootstrap — ACTIVE #84; CI PASS, PIE/review/merge pending** (Dagyeom lane).')
t = t.replace('20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**',
              '20. **World Genesis WG-1 — DONE #83; WG-2 — DONE #85; World Generation Milestone A — READY_NOW.**')
write(path, t)

# -----------------------------------------------------------------------------
# WORLD GENESIS CANONICAL DOC
# -----------------------------------------------------------------------------
path = 'docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md'
t = read(path)
t = sub_once(
    t,
    r'## Implementation status — 2026-09-15\n\n.*?\n---',
    '''## Implementation status — 2026-09-15

- **WG-1 DONE** via PR #83, merge `f5c8cbab3aa41c6a37c3bae06838eeb583749771`.
- WG-1 runtime contract includes `WorldSeed`, separate `PopulationSeed`, `WorldGenerationVersion`, stable `ChunkCoord`, negative-safe grid/chunk mapping, and order-independent untouched chunk baseline/substream seeds.
- WG-1 validation: Preflight `34933128958` PASS, Core `34933128953` **46/46 PASS** + deterministic harness, Unreal Linux Compile `34933128950` PASS.
- **WG-2 DONE** via PR #85, merge `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`.
- WG-2 runtime contract adds coherent macro elevation/moisture/temperature fields, biome classification, broad water/fertility/resource/traversal/hazard potential and deterministic viable initial start-region selection independent of `PopulationSeed`.
- WG-2 validation: Preflight `34935360828` PASS, Core `34935360834` **47/47 PASS** + deterministic harness, Unreal Linux Compile `34935360862` PASS including UE 5.6 UHT/UBT/link.
- **World Generation Milestone A READY_NOW:** group detailed natural chunk baseline, selected start-region materialization, initial founder spawn integration and minimum no-reroll persistence boundary into one milestone-sized delivery.
- deeper persistence/streaming/migration remain future implementation and must not be described as already running.

---''',
    'WORLD_GENESIS implementation status')

t = t.replace('This document remains the architecture gate. WG-1 is now implemented; WG-2 and later runtime phases remain incomplete until separately merged and validated.',
              'This document remains the architecture gate. WG-1 and WG-2 are now implemented. From this checkpoint, tightly coupled world-generation work is grouped into milestone-sized deliveries to reduce PR/CI/state-sync churn while preserving the same authority boundaries.')
t = t.replace('### WG-2 — macro world + initial start-site selector — **READY_NOW**\n- lightweight terrain/biome/water/resource potential\n- start viability scoring\n- no infrastructure spawn',
              '### WG-2 — macro world + initial start-site selector — **DONE #85**\n- coherent macro terrain/climate/biome/resource potential.\n- deterministic start viability scoring over 625 candidates.\n- geography/start ranking independent of PopulationSeed.\n- no infrastructure spawn.\n- merge `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`; Core 47/47 + Preflight + UE compile PASS.\n\n### World Generation Milestone A — **READY_NOW**\n- WG-3 deterministic detailed natural chunk baseline.\n- selected WG-2 start-region materialization boundary.\n- initial founder spawn integration into the materialized natural region.\n- generated-chunk registry and minimum no-reroll persistence boundary.\n- minimum Core/Bridge read contract needed by later World Visual presentation.\n- no civilization infrastructure auto-spawn.')
write(path, t)

# -----------------------------------------------------------------------------
# HANDOFF append-only
# -----------------------------------------------------------------------------
path = 'tasks/HANDOFF_LOG.md'
t = read(path)
entry = '''

### 쭌 측 AI — World Genesis WG-2 완료 / milestone-sized development 전환

- 날짜: 2026-09-15 KST
- PR: #85 `[CORE] Add World Genesis WG-2 macro world and viable start region`
- final head: `eac5a50b83416e9f55e160d1e6ad0d022b3f926a`
- squash merge: `ed4bf34d4b5bd0eb917a8bfb7fc5da16f52a4907`
- 검증:
  - Preflight `34935360828` PASS
  - Core Tests `34935360834` **47/47 PASS** + deterministic harness
  - Unreal Linux Compile `34935360862` PASS including UE 5.6 UHT/UBT/link
  - merge checkpoint comments/reviews/unresolved threads: 0
- 구현:
  - coherent macro elevation/moisture/temperature + biome classification
  - broad water/fertility/wood/stone/food/traversal/hazard potential
  - PopulationSeed-independent natural geography/start ranking
  - deterministic viable initial start-region selection over 625 candidates
  - no house/toilet/farm/storage/road/tool spawn; current bootstrap surface is not falsely treated as the production world
- 운영 결정:
  - micro-PR 반복 비용이 커졌으므로 이후 동일 목적/동일 레이어/동일 검증 범위 작업은 milestone-sized PR로 묶는다.
  - 쭌 다음 lane은 **World Generation Milestone A — READY_NOW**: detailed natural chunks + selected start-region materialization + founder spawn integration + minimum no-reroll persistence boundary.
  - 다겸 Motion PR #84는 별도 presentation lane으로 유지하며 Core/World movement authority를 침범하지 않는다.
'''
if 'World Genesis WG-2 완료 / milestone-sized development 전환' not in t:
    t += entry
write(path, t)

print('WG-2 canonical state sync: PASS')
