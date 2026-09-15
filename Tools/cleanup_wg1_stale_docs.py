from pathlib import Path


def read(path: str) -> str:
    return Path(path).read_text(encoding='utf-8')


def write(path: str, text: str) -> None:
    Path(path).write_text(text, encoding='utf-8')


def replace_all_required(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count == 0:
        raise SystemExit(f'missing stale anchor: {label}')
    return text.replace(old, new)

# WORK_STATE stale queue cleanup
p = 'tasks/WORK_STATE.md'
t = read(p)
t = replace_all_required(t,
    '17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — ACTIVE in parallel.\n19. Motion Bootstrap — after #67.\n20. **World Genesis WG-1/WG-2 architecture implementation gate.**',
    '17. HumanWaste visual feedback #82 — DONE.\n18. Character Appearance #67 — DONE.\n19. **Motion Bootstrap — READY_NOW** (Dagyeom lane).\n20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**',
    'WORK_STATE execution queue')
t = t.replace(
    '5. **World Genesis WG-1/WG-2 architectural gate** — before production-sized permanent World Visual implementation.',
    '5. **World Genesis WG-1 — DONE #83 / WG-2 architectural gate — READY_NOW** before production-sized permanent World Visual implementation.')
write(p, t)

# TEAM_BOARD stale sanitation/queue/gate cleanup
p = 'tasks/TEAM_BOARD.md'
t = read(p)
t = replace_all_required(t,
    '- **HumanWaste visual feedback — READY_NOW.**',
    '- **HumanWaste visual feedback — DONE #82.**',
    'TEAM_BOARD sanitation visual')
t = replace_all_required(t,
    '17. **HumanWaste visual feedback — READY_NOW.**\n18. Character Appearance #67 — DONE.\n19. **Character Motion Bootstrap — READY_NOW** (Dagyeom lane).\n20. **World Genesis WG-1/WG-2 implementation gate.**',
    '17. HumanWaste visual feedback #82 — DONE.\n18. Character Appearance #67 — DONE.\n19. **Character Motion Bootstrap — READY_NOW** (Dagyeom lane).\n20. **World Genesis WG-1 — DONE #83; WG-2 — READY_NOW.**',
    'TEAM_BOARD merge queue')
t = t.replace(
    '- before `World Visual Environment v1` becomes a permanent production-sized map, WG-1/WG-2 must be implemented or explicitly integrated:\n  - WG-1 deterministic world coordinates/chunk keys.\n  - WG-2 macro world + viable start-site selector.',
    '- WG-1 deterministic world coordinates/chunk keys are **DONE #83**.\n- before `World Visual Environment v1` becomes a permanent production-sized map, WG-2 macro world + viable start-site boundaries must be implemented or explicitly integrated.')
write(p, t)

# PROJECT_PROGRESS gate wording now that WG-1 is real runtime
p = 'docs/PROJECT_PROGRESS_2026-09-15.md'
t = read(p)
t = t.replace(
    'Before `World Visual Environment v1` becomes a permanent production-sized map:\n- WG-1 deterministic world coordinates/chunk keys must be established.\n- WG-2 macro world/start-site selection boundaries must be established or explicitly integrated.',
    'Before `World Visual Environment v1` becomes a permanent production-sized map:\n- WG-1 deterministic world coordinates/chunk keys are **established via #83**.\n- WG-2 macro world/start-site selection boundaries must be established or explicitly integrated.')
write(p, t)

# Strong stale-state assertions across canonical state docs.
combined = '\n'.join([
    read('tasks/WORK_STATE.md'),
    read('tasks/TEAM_BOARD.md'),
    read('docs/PROJECT_PROGRESS_2026-09-15.md'),
])
for stale in [
    'HumanWaste visual feedback — READY_NOW',
    'Character Appearance #67 — ACTIVE in parallel',
    'Motion Bootstrap — after #67',
    'World Genesis WG-1 — READY_NOW',
    'World Genesis runtime — DESIGN FIXED / NOT IMPLEMENTED',
]:
    if stale in combined:
        raise SystemExit(f'stale state remains: {stale}')

for required in [
    'World Genesis WG-2 — READY_NOW',
    'WG-1 — DONE #83',
    'HumanWaste visual feedback #82 — DONE',
    'Character Appearance #67 — DONE',
]:
    if required not in combined:
        raise SystemExit(f'required synchronized state missing: {required}')

print('WG-1 stale document cleanup: PASS')
