# CLAUDE.md

Claude Code / Cowork follows the same repository rules as every other agent.

**Read `AGENTS.md` first.**

Then read:
1. `docs/RUNTIME_ARCHITECTURE_WEB_CORE_v1.md`
2. `docs/LIFELENS_SPEC_v1.1.md`
3. `docs/DEVELOPMENT_MILESTONES.md`
4. `docs/WEB_CLIENT_ARCHITECTURE_v1.md`
5. actual GitHub main / target PR / Actions
6. `tasks/WORK_STATE.md`

Current active runtime is **LifeLensCore -> WASM -> Web Observer**.

Do not recreate an Unreal project, Unreal bridge, Android Unreal pipeline, or UE asset dependency in active `main` unless the user explicitly decides to reintroduce a native engine client. The last native Unreal state is preserved at `archive/unreal-final-20260929`.

For Web presentation work, never duplicate simulation authority. Use Core DTO/read/action contracts and fail closed when authoritative context is missing.
