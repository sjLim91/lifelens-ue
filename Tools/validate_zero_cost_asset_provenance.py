#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
provenance = (root / "Content/Environment/PROVENANCE.md").read_text(encoding="utf-8")
polyhaven_root = root / "Content/Environment/Photoreal/PolyHaven"
policy = (root / "docs/ZERO_COST_ASSET_POLICY_v1.md").read_text(encoding="utf-8")

assert "CC0" in policy, "zero-cost asset policy lost the CC0/permissive baseline"
assert polyhaven_root.is_dir(), "Poly Haven runtime asset root is missing"

asset_dirs = sorted(p.name for p in polyhaven_root.iterdir() if p.is_dir())
missing = [asset_id for asset_id in asset_dirs if asset_id not in provenance]
assert not missing, f"external runtime assets missing provenance entries: {missing}"

for asset_id in ("pachira_aquatica_01", "island_tree_02"):
    assert f"https://polyhaven.com/a/{asset_id}" in provenance, (
        f"mature canopy source URL missing from provenance: {asset_id}"
    )
    assert f"/Game/Environment/Photoreal/PolyHaven/{asset_id}" in provenance, (
        f"mature canopy LifeLens path missing from provenance: {asset_id}"
    )

for token in (
    "license checked: 2026-09-21",
    "license: CC0 1.0",
    "attribution required: no (CC0 1.0)",
    "Windows + macOS Desktop",
    "Android package exclusion",
):
    assert token in provenance, f"mature-canopy provenance contract missing: {token}"

print(
    "LifeLens zero-cost asset provenance: PASS "
    f"({len(asset_dirs)} Poly Haven runtime asset groups documented)"
)
