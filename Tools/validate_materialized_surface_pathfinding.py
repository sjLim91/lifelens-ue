from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / "Source/LifeLens/World/LLWorldDirector.cpp").read_text(encoding="utf-8")

for token in (
    "TSet<FIntPoint> Walkable;",
    'Chunk.Surface != FName(TEXT("Ocean"))',
    "Walkable.Add(FIntPoint(X, Y));",
    "const bool bGoalEnvironmentBlocked = Blocked.Contains(Goal);",
    "|| !Walkable.Contains(Next)",
    "!Walkable.Contains(SideX)",
    "!Walkable.Contains(SideY)",
    "Character.ClearMovementTarget();",
    "Do not bypass the authoritative route mask with a direct sweep.",
    "bHasFailedRouteTarget",
    "LastFailedRouteTarget",
    "NextRouteRetryWorldSeconds",
    "FailedRouteRetrySeconds",
    "SameFailedTargetToleranceUU",
):
    assert token in cpp or token in (root / "Source/LifeLens/World/LLWorldDirector.h").read_text(encoding="utf-8"), \
        f"missing materialized-surface pathfinding guard: {token}"

assert "Character.SetMovementTarget(DesiredLocation);" not in cpp[
    cpp.index("void ALLWorldDirector::MoveResidentToward"):
    cpp.index("void ALLWorldDirector::CollectActivityAnchors")
], "A* failure must not fall back to an unsafe direct sweep"

endpoint_block = cpp[
    cpp.index("const bool bGoalEnvironmentBlocked"):
    cpp.index("const int32 Margin")
]
assert "Blocked.Remove(Goal);" in endpoint_block
assert endpoint_block.index("bGoalEnvironmentBlocked") < endpoint_block.index("Blocked.Remove(Goal);")
assert "!Walkable.Contains(Start)" in endpoint_block
assert "!Walkable.Contains(Goal)" in endpoint_block

print("LifeLens materialized surface pathfinding: PASS")
