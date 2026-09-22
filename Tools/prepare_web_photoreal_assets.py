#!/usr/bin/env python3
"""Prepare a compact, same-origin Web nature asset pack from LifeLens staging.

Source acquisition stays centralized in:
    Tools/acquire_photoreal_environment_assets.py

This tool does not download arbitrary assets. It consumes that script's CC0
manifest, enforces Web-specific payload budgets, copies only browser-compatible
source files and writes a deterministic manifest for later Three.js loading.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
from typing import Any

WEB_ASSET_BUDGETS = {
    "boulder_01": 10,
    "tree_stump_01": 12,
    "shrub_02": 8,
    "shrub_03": 8,
    "weed_plant_02": 8,
    "dead_tree_trunk": 14,
    "tree_small_02": 18,
    "fir_sapling": 18,
    "pine_sapling_small": 24,
}
WEB_BASELINE_ASSETS = (
    "boulder_01",
    "tree_stump_01",
    "shrub_02",
    "shrub_03",
    "weed_plant_02",
    "dead_tree_trunk",
)
ALLOWED_EXTENSIONS = {
    ".gltf",
    ".glb",
    ".bin",
    ".png",
    ".jpg",
    ".jpeg",
    ".webp",
}
TOTAL_BUDGET_MIB = 48


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def mib(value: int) -> float:
    return value / 1024 / 1024


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--staging",
        default="assets_staging/PolyHaven",
        help="Poly Haven staging directory produced by the canonical acquisition tool",
    )
    parser.add_argument(
        "--output",
        default="web/public/vendor/lifelens-nature",
        help="Web same-origin asset directory",
    )
    parser.add_argument(
        "--asset",
        action="append",
        choices=sorted(WEB_ASSET_BUDGETS),
        help="optional subset; defaults to the compact Web baseline",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="remove the output directory before copying",
    )
    args = parser.parse_args()

    staging = Path(args.staging).expanduser().resolve()
    output = Path(args.output).expanduser().resolve()
    manifest_path = staging / "manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError(
            f"missing {manifest_path}; run acquire_photoreal_environment_assets.py first"
        )

    source_manifest: dict[str, Any] = json.loads(
        manifest_path.read_text(encoding="utf-8")
    )
    if source_manifest.get("license") != "CC0 1.0":
        raise RuntimeError("Web pack requires the canonical CC0 1.0 staging manifest")

    requested = args.asset or list(WEB_BASELINE_ASSETS)
    source_assets = source_manifest.get("assets") or {}

    if args.clean and output.exists():
        shutil.rmtree(output)
    output.mkdir(parents=True, exist_ok=True)

    web_manifest: dict[str, Any] = {
        "version": 1,
        "source": source_manifest.get("source", "Poly Haven"),
        "license": "CC0 1.0",
        "assets": {},
    }
    total_bytes = 0

    for asset_id in requested:
        source_entry = source_assets.get(asset_id)
        if not isinstance(source_entry, dict):
            raise RuntimeError(
                f"{asset_id}: missing from staging manifest; acquire it at 1k first"
            )

        files = source_entry.get("files") or []
        selected: list[dict[str, Any]] = []
        asset_bytes = 0
        has_model = False
        source_dir = staging / asset_id
        target_dir = output / asset_id
        target_dir.mkdir(parents=True, exist_ok=True)

        for item in files:
            local_name = str(item.get("local") or "")
            if not local_name:
                continue
            ext = Path(local_name).suffix.lower()
            if ext not in ALLOWED_EXTENSIONS:
                continue

            source = source_dir / local_name
            if not source.is_file():
                raise RuntimeError(f"{asset_id}: staged file missing: {source}")

            size = source.stat().st_size
            asset_bytes += size
            total_bytes += size
            if ext in {".gltf", ".glb"}:
                has_model = True

            target = target_dir / local_name
            shutil.copy2(source, target)
            selected.append({
                "file": local_name,
                "bytes": size,
                "sha256": sha256_file(target),
            })

        if not has_model:
            raise RuntimeError(f"{asset_id}: no glTF/GLB model selected for Web")

        budget_mib = WEB_ASSET_BUDGETS[asset_id]
        if asset_bytes > budget_mib * 1024 * 1024:
            raise RuntimeError(
                f"{asset_id}: Web payload {mib(asset_bytes):.1f} MiB exceeds "
                f"{budget_mib} MiB budget; choose a lighter LOD/source instead"
            )

        models = sorted(
            item["file"]
            for item in selected
            if Path(item["file"]).suffix.lower() in {".gltf", ".glb"}
        )
        web_manifest["assets"][asset_id] = {
            "source_page": source_entry.get("source_page"),
            "payload_bytes": asset_bytes,
            "models": models,
            "files": sorted(selected, key=lambda item: item["file"]),
        }
        print(
            f"[LifeLens Web assets] {asset_id}: "
            f"{len(selected)} files, {mib(asset_bytes):.1f} MiB"
        )

    if total_bytes > TOTAL_BUDGET_MIB * 1024 * 1024:
        raise RuntimeError(
            f"Web nature pack is {mib(total_bytes):.1f} MiB, "
            f"over total baseline budget {TOTAL_BUDGET_MIB} MiB"
        )

    web_manifest["payload_bytes"] = total_bytes
    web_manifest["base_url"] = "/vendor/lifelens-nature"
    (output / "manifest.json").write_text(
        json.dumps(web_manifest, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(
        f"[LifeLens Web assets] manifest={output / 'manifest.json'} "
        f"total={mib(total_bytes):.1f} MiB"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
