# Headless Unreal audit for LifeLens imported photoreal static meshes.
#
# Read-only: this script never changes or saves the audited assets.
# It reports geometry/material/LOD/Nanite state and whether a source folder
# contains multiple StaticMeshes while runtime consumes only one canonical mesh.

import json
import os
from pathlib import Path
import unreal

ROOT = "/Game/Environment/Photoreal/PolyHaven"
OUT = Path(
    os.environ.get(
        "LL_PHOTOREAL_AUDIT_OUT",
        "/workspace/Diagnostics/PhotorealAssetAudit/photoreal-asset-audit.json"
    )
)
EAL = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[LLPhotorealAudit] " + str(message))


def static_mesh_paths(folder):
    paths = []
    for asset_path in EAL.list_assets(
        folder, recursive=True, include_folder=False
    ):
        data = EAL.find_asset_data(asset_path)
        if str(data.asset_class_path.asset_name) == "StaticMesh":
            paths.append(asset_path)
    return sorted(paths)


def vector_dict(value):
    return {
        "x": float(value.x),
        "y": float(value.y),
        "z": float(value.z),
    }


def audit_mesh(asset_id, asset_path, sibling_meshes):
    mesh = EAL.load_asset(asset_path)
    if mesh is None or not isinstance(mesh, unreal.StaticMesh):
        raise RuntimeError(f"failed to load StaticMesh {asset_path}")

    box = mesh.get_bounding_box()
    box_min = box.get_editor_property("min")
    box_max = box.get_editor_property("max")
    center = unreal.Vector(
        (box_min.x + box_max.x) * 0.5,
        (box_min.y + box_max.y) * 0.5,
        (box_min.z + box_max.z) * 0.5,
    )
    dimensions = unreal.Vector(
        box_max.x - box_min.x,
        box_max.y - box_min.y,
        box_max.z - box_min.z,
    )
    nanite = mesh.get_editor_property("nanite_settings")

    lods = int(mesh.get_num_lods())
    lod_rows = []
    for lod in range(lods):
        lod_rows.append({
            "lod": lod,
            "vertices": int(mesh.get_num_vertices(lod)),
            "triangles": int(mesh.get_num_triangles(lod)),
            "sections": int(mesh.get_num_sections(lod)),
        })

    return {
        "asset_id": asset_id,
        "canonical_path": asset_path,
        "all_static_meshes_in_folder": sibling_meshes,
        "static_mesh_count_in_folder": len(sibling_meshes),
        "canonical_is_only_static_mesh": len(sibling_meshes) == 1,
        "lod_count": lods,
        "lods": lod_rows,
        "material_slots": len(mesh.get_editor_property("static_materials")),
        "bounds_center_cm": vector_dict(center),
        "bounds_size_cm": vector_dict(dimensions),
        "nanite_enabled": bool(nanite.get_editor_property("enabled")),
        "nanite_fallback_percent_triangles": float(
            nanite.get_editor_property("fallback_percent_triangles")
        ),
        "nanite_keep_percent_triangles": float(
            nanite.get_editor_property("keep_percent_triangles")
        ),
        "has_navigation_data": bool(
            mesh.get_editor_property("has_navigation_data")
        ),
    }


def main():
    if not EAL.does_directory_exist(ROOT):
        raise RuntimeError(f"missing photoreal root {ROOT}")

    rows = []
    # EditorAssetLibrary folder listing differs between engine minor versions,
    # so discover asset ids from all recursive asset paths instead.
    asset_ids = set()
    for path in EAL.list_assets(ROOT, recursive=True, include_folder=False):
        relative = path[len(ROOT):].lstrip("/")
        if "/" in relative:
            asset_ids.add(relative.split("/", 1)[0])

    for asset_id in sorted(asset_ids):
        folder = f"{ROOT}/{asset_id}"
        meshes = static_mesh_paths(folder)
        canonical = f"{folder}/SM_LL_{asset_id}"
        if not EAL.does_asset_exist(canonical):
            rows.append({
                "asset_id": asset_id,
                "canonical_path": canonical,
                "all_static_meshes_in_folder": meshes,
                "static_mesh_count_in_folder": len(meshes),
                "canonical_missing": True,
            })
            continue

        rows.append(audit_mesh(asset_id, canonical, meshes))

    if not rows:
        raise RuntimeError("no photoreal asset groups discovered")

    missing = [r["asset_id"] for r in rows if r.get("canonical_missing")]
    multi = [
        r["asset_id"] for r in rows
        if r.get("static_mesh_count_in_folder", 0) > 1
    ]

    payload = {
        "schema": "lifelens.photoreal.asset-audit.v1",
        "root": ROOT,
        "asset_count": len(rows),
        "canonical_missing": missing,
        "multi_static_mesh_groups": multi,
        "assets": rows,
    }

    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(
        json.dumps(payload, indent=2, sort_keys=True),
        encoding="utf-8"
    )

    log(
        f"wrote {OUT}: assets={len(rows)} "
        f"missing={len(missing)} multi_mesh_groups={len(multi)}"
    )
    for row in rows:
        lod0 = (row.get("lods") or [{}])[0]
        log(
            f"{row['asset_id']}: "
            f"tris0={lod0.get('triangles', 'n/a')} "
            f"verts0={lod0.get('vertices', 'n/a')} "
            f"lods={row.get('lod_count', 'n/a')} "
            f"materials={row.get('material_slots', 'n/a')} "
            f"nanite={row.get('nanite_enabled', 'n/a')} "
            f"meshesInFolder={row.get('static_mesh_count_in_folder', 0)}"
        )

    if missing:
        raise RuntimeError(
            "canonical StaticMesh missing for: " + ", ".join(missing)
        )


main()
