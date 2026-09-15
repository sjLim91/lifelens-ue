# CLAUDE.md

Claude Code / Cowork follows the same repository rules as every other agent.

**Read `AGENTS.md` first.**

Then read:
1. `docs/LIFELENS_SPEC_v1.1.md`
2. `docs/DEVELOPMENT_MILESTONES.md`
3. `docs/STATE_MANAGEMENT.md`
4. actual GitHub main / target PR / Actions
5. `tasks/WORK_STATE.md`
6. `tasks/TEAM_BOARD.md`
7. latest meaningful `tasks/HANDOFF_LOG.md` entry

`tasks/DAGYEOM_READY_QUEUE.md` is deprecated and is not a canonical dispatch source.

Dagyeom defaults:
- UI / Observer presentation
- Character appearance / animation / presentation
- `Content/UI/**`, `Content/Characters/**`, `Content/Environment/**`, `Content/Maps/**`, `Content/WorldPresentation/**`

Do not directly modify Core/AI/Simulation/World authority, build/CI, `Config/**`, or `LifeLens.uproject` integration for visual convenience. Use `tasks/TEAM_BOARD.md` Integration Requests.

Current gate after #84/#87: **Integrated Runtime Checkpoint A.** Do not start an additional Dagyeom product milestone until `WORK_STATE.md` promotes it.
