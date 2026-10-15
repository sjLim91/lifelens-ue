#!/usr/bin/env python3
"""Fail-closed release gate for the *actual* LifeLens creature model candidates.

This intentionally does not generate a creature, change Core, or approve
naturalism from machine-readable metrics. Codex model work remains independent.

Usage: python3 Tools/creature_asset_gate.py manifest.json --root candidate/
       python3 Tools/creature_asset_gate.py manifest.json --root candidate/ --allow-partial
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import struct
import sys
from typing import Any

REQUIRED = {"resident-female", "resident-male", "tiger"}
LICENSES = {"CC0-1.0", "CC-BY-4.0", "CC-BY-3.0"}
LIMITS = {"near": 85000, "mid": 30000, "far": 6000}
VIEWS = ("front", "side", "threequarter", "motion", "mobile")
HEX_SHA256 = re.compile(r"^[0-9a-f]{64}$")
PNG_HEADER = b"\x89PNG\r\n\x1a\n"


class GateError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise GateError(message)


def mapped_path(root: pathlib.Path, relative: str) -> pathlib.Path:
    require(isinstance(relative, str) and bool(relative) and
            not relative.startswith(("/", "\\")) and "\\" not in relative,
            "Paths must be nonempty relative POSIX paths")
    require(".." not in pathlib.PurePosixPath(relative).parts,
            f"Path traversal prohibited: {relative!r}")
    file = (root / relative).resolve()
    require(file.is_relative_to(root.resolve()), f"Path outside asset root: {relative}")
    require(file.is_file(), f"Required evidence missing: {relative}")
    return file


def sha256_file(path: pathlib.Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            sha.update(block)
    return sha.hexdigest()


def parse_glb(path: pathlib.Path) -> dict[str, Any]:
    data = path.read_bytes()
    require(len(data) >= 100, f"{path.name}: empty or truncated GLB")
    magic, version, nbytes = struct.unpack_from("<4sII", data, 0)
    require(magic == b"glTF" and version == 2 and nbytes == len(data),
            f"{path.name}: invalid GLB 2.0 header/length")
    offset = 12
    chunks = []
    while offset < len(data):
        require(offset + 8 <= len(data), f"{path.name}: malformed chunk header")
        length, kind = struct.unpack_from("<I4s", data, offset)
        offset += 8
        require(length % 4 == 0 and offset + length <= len(data),
                f"{path.name}: bad chunk length/alignment")
        chunks.append((kind, data[offset:offset + length]))
        offset += length
    require(chunks and chunks[0][0] == b"JSON", f"{path.name}: JSON chunk missing")
    require(len(chunks) in (1, 2), f"{path.name}: unexpected GLB chunks")
    if len(chunks) == 2:
        require(chunks[1][0] == b"BIN\x00", f"{path.name}: BIN chunk invalid")
    try:
        document = json.loads(chunks[0][1].decode("utf-8").rstrip(" \t\r\n\x00"))
    except (ValueError, UnicodeDecodeError) as exc:
        raise GateError(f"{path.name}: malformed glTF JSON: {exc}") from exc
    require(document.get("asset", {}).get("version") == "2.0",
            f"{path.name}: glTF asset version not 2.0")
    for kind in ("buffers", "images"):
        for item in document.get(kind, []):
            require(not item.get("uri"), f"{path.name}: external {kind} forbidden")
    buffers = document.get("buffers", [])
    if buffers:
        require(len(chunks) == 2 and len(buffers) == 1,
                f"{path.name}: GLB must contain exactly one embedded BIN buffer")
        require(buffers[0]["byteLength"] <= len(chunks[1][1]) <= buffers[0]["byteLength"] + 3,
                f"{path.name}: BIN buffer size mismatch")
    accessors = document.get("accessors", [])
    meshes = document.get("meshes", [])
    require(meshes and accessors, f"{path.name}: no geometry")
    nodes = document.get("nodes", [])
    for skin in document.get("skins", []):
        require(skin.get("joints") and
                all(isinstance(j, int) and 0 <= j < len(nodes) for j in skin["joints"]),
                f"{path.name}: invalid skeleton joints")
    for mesh in meshes:
        for prim in mesh.get("primitives", []):
            for accessor in list(prim.get("attributes", {}).values()) + (
                [prim["indices"]] if "indices" in prim else []):
                require(isinstance(accessor, int) and 0 <= accessor < len(accessors),
                        f"{path.name}: primitive references missing accessor")
    return document


def mesh_metrics(doc: dict[str, Any], *, skinned: bool, textured: bool) -> dict[str, int]:
    total = 0
    joints = max((len(skin.get("joints", [])) for skin in doc.get("skins", [])), default=0)
    prim_count = 0
    for mesh in doc["meshes"]:
        for prim in mesh.get("primitives", []):
            attrs = prim.get("attributes", {})
            require("POSITION" in attrs and "NORMAL" in attrs, "Geometry lacks positions/normals")
            if textured:
                require("TEXCOORD_0" in attrs or "COLOR_0" in attrs,
                        "Near/mid model lacks textured UV or vertex color")
            if skinned:
                require("JOINTS_0" in attrs and "WEIGHTS_0" in attrs,
                        "Skinned near/mid model has unweighted mesh primitive")
            mode = prim.get("mode", 4)
            require(mode == 4, "Non-triangle primitives in creature release asset")
            accessors = doc["accessors"]
            idx = prim.get("indices")
            indices = accessors[idx]["count"] if idx is not None else accessors[attrs["POSITION"]]["count"]
            require(indices >= 3 and indices % 3 == 0, "Invalid triangle index count")
            total += indices // 3
            prim_count += 1
    require(prim_count > 0, "Missing mesh primitives")
    if skinned:
        require(joints >= 12, f"Skeleton has only {joints} joints")
    return {"triangles": total, "joints": joints, "primitives": prim_count}


def check_picture(root: pathlib.Path, name: str, orientation: str) -> dict[str, int]:
    p = mapped_path(root, name)
    with p.open("rb") as f:
        hdr = f.read(24)
    require(len(hdr) == 24 and hdr[:8] == PNG_HEADER and hdr[12:16] == b"IHDR",
            f"{name}: review evidence must be a real PNG")
    width, height = struct.unpack(">II", hdr[16:24])
    require(p.stat().st_size > 12000, f"{name}: missing meaningful review image")
    if orientation == "mobile":
        require(width >= 360 and height >= 640 and height > width,
                f"{name}: expected portrait mobile screenshot")
    else:
        require(width >= 512 and height >= 512,
                f"{name}: insufficient desktop review resolution")
    return {"width": width, "height": height}


def check_license(source: dict[str, Any], name: str) -> dict[str, str]:
    require(isinstance(source, dict), f"{name}: missing source provenance")
    url = source.get("url", "")
    license_code = source.get("license", "")
    require(isinstance(url, str) and url.startswith("https://"),
            f"{name}: source URL must be HTTPS")
    require(license_code in LICENSES,
            f"{name}: restricted, unknown or unsupported license {license_code!r}")
    require(source.get("license_url", "").startswith("https://"),
            f"{name}: verifiable license URL required")
    author = source.get("author", "")
    require(isinstance(author, str) and author.strip(),
            f"{name}: source author/creator required even for CC0")
    if license_code.startswith("CC-BY"):
        require(isinstance(source.get("attribution"), str) and
                len(source["attribution"].strip()) >= 10,
                f"{name}: CC BY requires shipping attribution text")
    return {"license": license_code, "author": author.strip(), "url": url}


def inspect_candidate(root: pathlib.Path, model: dict[str, Any]) -> dict[str, Any]:
    name = model.get("id", "")
    require(name in REQUIRED, f"Invalid creature id: {name!r}")
    provenance = check_license(model.get("source", {}), name)
    lods = model.get("lods", {})
    require(isinstance(lods, dict) and set(lods) == set(LIMITS),
            f"{name}: near/mid/far LOD files mandatory")
    stats = {}
    for tier in ("near", "mid", "far"):
        record = lods[tier]
        path = mapped_path(root, record["file"])
        expected = record.get("sha256", "")
        require(isinstance(expected, str) and HEX_SHA256.fullmatch(expected) is not None,
                f"{name}:{tier}: unpinned SHA-256")
        require(sha256_file(path) == expected,
                f"{name}:{tier}: GLB changed since approval")
        require(path.stat().st_size <= (22 if tier == "near" else 10 if tier == "mid" else 4) * 1024 * 1024,
                f"{name}:{tier}: exceeds shipped asset budget")
        doc = parse_glb(path)
        metric = mesh_metrics(doc, skinned=(tier != "far"), textured=(tier != "far"))
        require(metric["triangles"] <= LIMITS[tier],
                f"{name}:{tier}: {metric['triangles']} triangles exceeds {LIMITS[tier]}")
        stats[tier] = {"file": record["file"], "bytes": path.stat().st_size, **metric}
    require(stats["near"]["triangles"] >= stats["mid"]["triangles"] * 1.5,
            f"{name}: no meaningful near-to-mid LOD reduction")
    require(stats["mid"]["triangles"] >= stats["far"]["triangles"] * 1.5,
            f"{name}: no meaningful mid-to-far LOD reduction")
    report_path = mapped_path(root, model["web_motion_report"])
    report = json.loads(report_path.read_text(encoding="utf-8"))
    required_clips = ("idle", "walk", "sleep", "work") if name.startswith("resident-") \
        else ("idle", "walk", "run")
    require(report.get("creature_id") == name and report.get("runtime") == "web-threejs",
            f"{name}: web motion report does not match actual runtime")
    require(report.get("tested") is True and report.get("fallback_works") is True,
            f"{name}: missing successful web motion/fallback test")
    clips = report.get("clips", {})
    require(all(clips.get(k) == "pass" for k in required_clips),
            f"{name}: required motion clip(s) unverified: {required_clips}")
    screenshots = model.get("review", {})
    require(isinstance(screenshots, dict) and all(k in screenshots for k in VIEWS),
            f"{name}: missing visual review views: {VIEWS}")
    views = {k: check_picture(root, screenshots[k], k) for k in VIEWS}
    return {"id": name, "provenance": provenance, "lods": stats,
            "web_motion": {k: clips[k] for k in required_clips}, "screenshots": views,
            "visual_naturalism_approved": False}


def validate_manifest(manifest: dict[str, Any], root: pathlib.Path, allow_partial: bool = False):
    require(manifest.get("schema_version") == 1, "Manifest schema_version must be 1")
    models = manifest.get("creatures", [])
    require(isinstance(models, list) and len(models) >= 1, "No models submitted")
    ids = [model.get("id") for model in models]
    require(len(ids) == len(set(ids)), "Duplicate creature ids")
    require(set(ids).issubset(REQUIRED), f"Unknown creature species/sex ids: {ids}")
    if not allow_partial:
        require(set(ids) == REQUIRED, f"Release requires all three models: {sorted(REQUIRED - set(ids))}")
    reports = [inspect_candidate(root, model) for model in models]
    return {"status": "structural-pass-visual-review-required",
            "release_approved": False, "review_required": True, "creatures": reports}


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=pathlib.Path)
    parser.add_argument("--root", type=pathlib.Path, default=pathlib.Path("."))
    parser.add_argument("--report", type=pathlib.Path)
    parser.add_argument("--allow-partial", action="store_true")
    args = parser.parse_args(argv)
    try:
        result = validate_manifest(json.loads(args.manifest.read_text(encoding="utf-8")),
                                   args.root.resolve(), args.allow_partial)
    except (GateError, KeyError, TypeError, OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"LIFELENS_CREATURE_ASSET_GATE_FAIL: {exc}", file=sys.stderr)
        return 1
    serialized = json.dumps(result, indent=2, ensure_ascii=False) + "\n"
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(serialized, encoding="utf-8")
    print(serialized)
    print("LIFELENS_CREATURE_STRUCTURAL_PASS_VISUAL_APPROVAL_PENDING")
    return 0


if __name__ == "__main__":
    sys.exit(main())
