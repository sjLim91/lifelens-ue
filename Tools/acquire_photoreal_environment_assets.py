#!/usr/bin/env python3
"""Acquire curated zero-cost photoreal environment assets for LifeLens.

Primary source: Poly Haven public API.
- Assets are CC0.
- The live API is free to use and requires a unique User-Agent.
- Original source files are downloaded into LL_ASSET_STAGING and are NOT
  committed to the repository.

The script is intentionally deterministic: curated asset ids, preferred
resolution and file-format ordering are versioned here instead of doing a
"latest popular assets" search during builds.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
import urllib.request
from typing import Any, Iterable

USER_AGENT = "LifeLensAssetPipeline/1.0 (+https://github.com/sjLim91/lifelens-ue)"
API = "https://api.polyhaven.com"
SOURCE = "Poly Haven"
LICENSE = "CC0 1.0"

# Keep the first wave compact enough for repo/import iteration while replacing
# the most visibly stylized classes in the current scene. Payload budgets are
# validated before any file for that asset is downloaded, so an accidentally
# huge scan fails fast instead of consuming the Unreal CI runner.
CURATED = {
    # Keep production-local-view assets inside a practical mobile/repository
    # geometry budget. Very high-poly Poly Haven trees (for example the
    # 17M-triangle pine_tree_01) are intentionally excluded from the baseline.
    "fir_sapling": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 90},
    "pine_sapling_small": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 90},
    # Mature broadleaf replacement for the temporary stylized desktop canopy.
    # Poly Haven reports ~312K source triangles and authored LODs; the pipeline
    # still selects only the 1K glTF payload and enforces a hard byte budget.
    "jacaranda_tree": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "pachira_aquatica_01": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "quiver_tree_01": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "quiver_tree_02": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "tree_small_02": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "island_tree_02": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 90},
    "boulder_01": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 45},
    "tree_stump_01": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 45},
    "shrub_02": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 45},
    "shrub_03": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 45},
    "weed_plant_02": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 45},
    "dead_tree_trunk": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 60},
    # Primitive-civilization props: real CC0 meshes that replace completed
    # facility hero visuals without reintroducing Engine Cube/Cone fallback.
    "stone_fire_pit": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 70},
    "wicker_basket_01": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 65},
    "wooden_axe": {"kind": "model", "resolution": "2k", "format": "gltf", "max_mib": 50},
    # Primitive-compatible daily-life vessel; neutral enough for generic food/
    # drink presentation without inventing a specific crop/species.
    "wooden_bowl_01": {"kind": "model", "resolution": "1k", "format": "gltf", "max_mib": 25},
    "forest_floor": {"kind": "texture", "resolution": "2k"},
    "forrest_ground_01": {"kind": "texture", "resolution": "2k"},
    "mossy_rock": {"kind": "texture", "resolution": "2k"},
    "nature_reserve_forest": {"kind": "hdri", "resolution": "2k", "format": "hdr"},
}

MODEL_EXTENSIONS = {".gltf", ".glb", ".fbx", ".bin", ".png", ".jpg", ".jpeg", ".webp"}
TEXTURE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp"}
HDRI_EXTENSIONS = {".hdr", ".exr"}


def api_json(path: str) -> Any:
    req = urllib.request.Request(API + path, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=60) as response:
        return json.load(response)


def url_leaves(value: Any, path: tuple[str, ...] = ()) -> Iterable[tuple[tuple[str, ...], dict[str, Any]]]:
    if isinstance(value, dict):
        if isinstance(value.get("url"), str):
            yield path, value
        for key, child in value.items():
            if key in {"url", "size", "md5"}:
                continue
            yield from url_leaves(child, path + (str(key),))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            yield from url_leaves(child, path + (str(index),))


def find_named_branch(value: Any, wanted: str) -> Any | None:
    wanted = wanted.lower()
    if isinstance(value, dict):
        for key, child in value.items():
            if str(key).lower() == wanted:
                return child
        for child in value.values():
            found = find_named_branch(child, wanted)
            if found is not None:
                return found
    elif isinstance(value, list):
        for child in value:
            found = find_named_branch(child, wanted)
            if found is not None:
                return found
    return None


def choose_resolution_branch(branch: Any, requested: str) -> Any:
    if not isinstance(branch, dict):
        return branch
    for candidate in (requested, "2k", "1k", "4k"):
        for key, child in branch.items():
            if str(key).lower() == candidate:
                return child
    return branch


def extension(url: str) -> str:
    clean = url.split("?", 1)[0]
    return Path(clean).suffix.lower()


def select_files(files: Any, spec: dict[str, str]) -> list[dict[str, Any]]:
    kind = spec["kind"]
    resolution = spec.get("resolution", "2k")
    fmt = spec.get("format")

    scope = files
    if kind == "model" and fmt:
        branch = find_named_branch(files, fmt)
        if branch is not None:
            scope = choose_resolution_branch(branch, resolution)
    elif kind == "hdri":
        branch = find_named_branch(files, "hdri")
        if branch is not None:
            scope = choose_resolution_branch(branch, resolution)
    else:
        # Textures expose map categories at the top level; keep only the
        # requested-resolution leaves and common runtime maps below.
        scope = files

    selected: list[dict[str, Any]] = []
    for key_path, leaf in url_leaves(scope):
        url = leaf["url"]
        ext = extension(url)
        lowered = "/".join(key_path).lower()

        if kind == "model":
            if ext not in MODEL_EXTENSIONS:
                continue
            # When the API branch includes multiple resolutions in dependencies,
            # prefer the requested one and keep model companion .bin/images.
            if any(token in lowered for token in ("1k", "2k", "4k", "8k")) and resolution not in lowered:
                continue
        elif kind == "texture":
            if ext not in TEXTURE_EXTENSIONS:
                continue
            if resolution not in lowered and f"_{resolution}" not in url.lower():
                continue
            # Keep only maps used by the LifeLens PBR material path.
            useful = ("diff", "albedo", "basecolor", "nor_gl", "normal", "rough", "ao", "arm", "disp")
            if not any(token in lowered or token in url.lower() for token in useful):
                continue
        elif kind == "hdri":
            if ext not in HDRI_EXTENSIONS:
                continue
            if fmt and ext != "." + fmt.lower():
                continue
        else:
            continue

        selected.append({
            "path": list(key_path),
            "url": url,
            "size": int(leaf.get("size") or 0),
            "md5": str(leaf.get("md5") or ""),
        })

    # Stable order and duplicate URL removal.
    dedup: dict[str, dict[str, Any]] = {}
    for item in selected:
        dedup.setdefault(item["url"], item)
    return [dedup[url] for url in sorted(dedup)]



def inspect_gltf_metadata(chosen: list[dict[str, Any]]) -> None:
    """Fetch only the tiny .gltf JSON and report its scene/LOD structure."""
    gltf_items = [item for item in chosen if extension(item["url"]) == ".gltf"]
    if not gltf_items:
        print("[LifeLens assets] glTF metadata probe: no .gltf leaf selected")
        return
    for item in gltf_items:
        reported_size = int(item.get("size") or 0)
        if reported_size > 2 * 1024 * 1024:
            raise RuntimeError(
                f"Refusing metadata probe for unexpectedly large glTF JSON: "
                f"{reported_size / 1024 / 1024:.1f} MiB")
        req = urllib.request.Request(item["url"], headers={"User-Agent": USER_AGENT})
        with urllib.request.urlopen(req, timeout=60) as response:
            doc = json.load(response)

        print(
            f"[LifeLens assets] glTF metadata: "
            f"scenes={len(doc.get('scenes', []))} "
            f"nodes={len(doc.get('nodes', []))} "
            f"meshes={len(doc.get('meshes', []))} "
            f"buffers={len(doc.get('buffers', []))} "
            f"extensionsUsed={doc.get('extensionsUsed', [])}")

        for index, buffer in enumerate(doc.get("buffers", [])):
            print(
                f"[LifeLens assets] glTF buffer[{index}]: "
                f"uri={buffer.get('uri', '')} "
                f"byteLength={int(buffer.get('byteLength') or 0)}")

        for index, mesh in enumerate(doc.get("meshes", [])):
            print(
                f"[LifeLens assets] glTF mesh[{index}]: "
                f"name={mesh.get('name', '')!r} "
                f"primitives={len(mesh.get('primitives', []))}")

        for index, node in enumerate(doc.get("nodes", [])):
            name = str(node.get("name") or "")
            lowered = name.lower()
            if "lod" in lowered or "static" in lowered or "geometry" in lowered:
                print(
                    f"[LifeLens assets] glTF node[{index}]: "
                    f"name={name!r} mesh={node.get('mesh')} "
                    f"children={node.get('children', [])}")

def md5_file(path: Path) -> str:
    digest = hashlib.md5()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def download(item: dict[str, Any], dest: Path, dry_run: bool) -> dict[str, Any]:
    url = item["url"]
    name = Path(url.split("?", 1)[0]).name or "asset.bin"
    target = dest / name

    result = dict(item)
    result["local"] = target.name

    if dry_run:
        return result

    target.parent.mkdir(parents=True, exist_ok=True)
    if target.exists() and item.get("md5") and md5_file(target) == item["md5"]:
        return result

    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    with urllib.request.urlopen(req, timeout=120) as response, target.open("wb") as output:
        while True:
            block = response.read(1024 * 1024)
            if not block:
                break
            output.write(block)

    expected = item.get("md5")
    if expected:
        actual = md5_file(target)
        if actual != expected:
            target.unlink(missing_ok=True)
            raise RuntimeError(f"MD5 mismatch for {url}: {actual} != {expected}")
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--asset", action="append", choices=sorted(CURATED))
    parser.add_argument("--staging", default=os.environ.get("LL_ASSET_STAGING", "assets_staging"))
    parser.add_argument("--resolution", choices=["1k", "2k", "4k"],
                        help="override curated resolution for this acquisition run")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument(
        "--inspect-gltf",
        action="store_true",
        help="fetch only selected .gltf JSON and print mesh/node/LOD structure")
    args = parser.parse_args()

    ids = args.asset or list(CURATED)
    root = Path(args.staging).expanduser().resolve() / "PolyHaven"
    manifest = {
        "source": SOURCE,
        "license": LICENSE,
        "api": API,
        "user_agent": USER_AGENT,
        "assets": {},
    }

    for asset_id in ids:
        spec = dict(CURATED[asset_id])
        if args.resolution:
            spec["resolution"] = args.resolution
        print(f"[LifeLens assets] query {asset_id} ({spec})")
        files = api_json(f"/files/{asset_id}")
        chosen = select_files(files, spec)
        if not chosen:
            raise RuntimeError(
                f"No matching files returned for {asset_id}; "
                f"API structure may have changed.")

        if args.inspect_gltf:
            inspect_gltf_metadata(chosen)

        selected_bytes = sum(int(item.get("size") or 0) for item in chosen)
        max_mib = spec.get("max_mib")
        over_budget = bool(max_mib and selected_bytes > int(max_mib) * 1024 * 1024)
        if args.dry_run or over_budget:
            print(
                f"[LifeLens assets] selected payload detail for {asset_id}: "
                f"{len(chosen)} files, {selected_bytes / 1024 / 1024:.1f} MiB")
            for item in sorted(
                chosen,
                key=lambda candidate: int(candidate.get("size") or 0),
                reverse=True,
            ):
                leaf_path = "/".join(item.get("path") or [])
                leaf_size = int(item.get("size") or 0) / 1024 / 1024
                leaf_name = Path(item["url"].split("?", 1)[0]).name
                print(
                    f"[LifeLens assets] selected leaf: "
                    f"{leaf_path} | {leaf_name} | {leaf_size:.1f} MiB")

        if over_budget:
            largest = sorted(
                chosen,
                key=lambda item: int(item.get("size") or 0),
                reverse=True,
            )[:12]
            detail = "; ".join(
                f"{'/'.join(item.get('path') or [])}="
                f"{int(item.get('size') or 0) / 1024 / 1024:.1f}MiB"
                for item in largest
            )
            raise RuntimeError(
                f"{asset_id} selected payload is {selected_bytes / 1024 / 1024:.1f} MiB, "
                f"over the {max_mib} MiB LifeLens baseline budget. "
                f"Largest selected leaves: {detail}. "
                "Choose a lower-geometry asset or narrow the selected LOD/variant "
                "instead of silently importing a hero scan.")

        asset_dir = root / asset_id
        downloaded = [download(item, asset_dir, args.dry_run) for item in chosen]
        manifest["assets"][asset_id] = {
            "source_page": f"https://polyhaven.com/a/{asset_id}",
            "spec": spec,
            "files": downloaded,
        }
        total = sum(item.get("size", 0) for item in downloaded)
        print(f"[LifeLens assets] {asset_id}: {len(downloaded)} files, {total/1024/1024:.1f} MiB")

    if not args.dry_run:
        root.mkdir(parents=True, exist_ok=True)
        (root / "manifest.json").write_text(
            json.dumps(manifest, indent=2, ensure_ascii=False) + "\n",
            encoding="utf-8")
        print(f"[LifeLens assets] manifest: {root / 'manifest.json'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
