# LifeLens photoreal environment import.
#
# Source acquisition:
#   python Tools/acquire_photoreal_environment_assets.py
#
# Headless Unreal import:
#   UnrealEditor-Cmd LifeLens.uproject -run=pythonscript \
#     -script=Content/Environment/Photoreal/Import/import_photoreal_nature.py \
#     -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout
#
# Original source files remain under LL_ASSET_STAGING/PolyHaven and are not
# committed. Imported Unreal assets are stored under /Game/Environment/Photoreal.

import json
import os
from pathlib import Path
import unreal

STAGING_ROOT = Path(os.environ.get("LL_ASSET_STAGING", "assets_staging")).expanduser().resolve()
SOURCE_ROOT = STAGING_ROOT / "PolyHaven"
DEST_ROOT = "/Game/Environment/Photoreal/PolyHaven"
DEST_IMPORT = "/Game/Environment/Photoreal/Import"

DEFAULT_MODEL_IDS = [
    "fir_sapling",
    "pine_sapling_small",
    "boulder_01",
    "tree_stump_01",
    "shrub_02",
    "shrub_03",
    "weed_plant_02",
    "dead_tree_trunk",
]

GLTF_ASSETS_PIPELINE = "/Interchange/Pipelines/DefaultGLTFAssetsPipeline.DefaultGLTFAssetsPipeline"
GLTF_PIPELINE = "/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline"

EAL = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[LLPhotoreal] " + str(message))


def set_prop(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            pass
    return False


def make_static_pipeline():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Interchange/Pipelines", DEST_IMPORT], True)
    source = unreal.load_object(None, GLTF_ASSETS_PIPELINE)
    if source is None:
        raise RuntimeError("missing engine glTF asset pipeline")

    name = "IP_LL_PhotorealStatic"
    dest = f"{DEST_IMPORT}/{name}"
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)
    pipeline = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(
        name, DEST_IMPORT, source)
    if pipeline is None:
        raise RuntimeError("failed to duplicate glTF pipeline")

    common = pipeline.get_editor_property(
        "common_skeletal_meshes_and_animations_properties")
    set_prop(common, ["import_only_animations"], False)
    mesh = pipeline.get_editor_property("mesh_pipeline")
    set_prop(mesh, ["import_static_meshes"], True)
    set_prop(mesh, ["import_skeletal_meshes"], False)
    set_prop(mesh, ["combine_static_meshes"], False)
    animation = pipeline.get_editor_property("animation_pipeline")
    set_prop(animation, ["import_animations"], False)
    material = pipeline.get_editor_property("material_pipeline")
    set_prop(material, ["import_materials"], True)
    EAL.save_asset(dest, only_if_is_dirty=False)
    return unreal.SoftObjectPath(f"{dest}.{name}")


def import_file(path: Path, destination: str, pipeline):
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    source = unreal.InterchangeManager.create_source_data(str(path))
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property(
        "override_pipelines",
        [pipeline, unreal.SoftObjectPath(GLTF_PIPELINE)])
    result = manager.import_asset(destination, source, params)
    ok, objects = (result if isinstance(result, tuple) else (bool(result), []))
    if not ok:
        raise RuntimeError(f"import failed: {path}")
    return objects


def canonicalize_primary_mesh(folder: str, asset_id: str):
    """Give the primary imported mesh a stable project-owned asset path.

    Poly Haven source filenames/Interchange folder layout may change without
    changing the semantic asset. Runtime C++ should depend on LifeLens-owned
    names, not third-party importer internals.
    """
    mesh_paths = []
    for asset_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        data = EAL.find_asset_data(asset_path)
        if str(data.asset_class_path.asset_name) == "StaticMesh":
            mesh_paths.append(asset_path)
    mesh_paths.sort()
    if not mesh_paths:
        raise RuntimeError(f"no StaticMesh imported for {asset_id}")

    canonical_name = "SM_LL_" + asset_id
    canonical_path = f"{folder}/{canonical_name}"
    if mesh_paths[0] != canonical_path:
        if EAL.does_asset_exist(canonical_path):
            EAL.delete_asset(canonical_path)
        if not EAL.rename_asset(mesh_paths[0], canonical_path):
            raise RuntimeError(
                f"failed to canonicalize {mesh_paths[0]} -> {canonical_path}")
    return canonical_path


def configure_imported_assets(folder: str):
    meshes = 0
    textures = 0
    for asset_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        data = EAL.find_asset_data(asset_path)
        class_name = str(data.asset_class_path.asset_name)
        asset = EAL.load_asset(asset_path)
        if asset is None:
            continue

        if class_name == "Texture2D":
            # Keep 2K desktop detail. Android streaming/device profiles reduce
            # residency rather than permanently destroying the source quality.
            set_prop(asset, ["max_texture_size"], 2048)
            textures += 1
        elif class_name == "StaticMesh":
            # Generate standard LODs where supported. Do not make Nanite a
            # universal requirement because Android remains a primary target.
            set_prop(asset, ["auto_compute_lod_screen_size"], True)
            meshes += 1
        EAL.save_asset(asset_path, only_if_is_dirty=False)
    return meshes, textures


def selected_model_ids():
    raw = os.environ.get("LL_PHOTOREAL_MODEL_IDS", "").strip()
    if not raw:
        return list(DEFAULT_MODEL_IDS)
    ids = [item.strip() for item in raw.split(",") if item.strip()]
    if not ids:
        raise RuntimeError("LL_PHOTOREAL_MODEL_IDS was set but contained no asset ids")
    return ids


def main():
    manifest_path = SOURCE_ROOT / "manifest.json"
    if not manifest_path.is_file():
        raise RuntimeError(
            f"missing {manifest_path}; run Tools/acquire_photoreal_environment_assets.py first")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    if manifest.get("license") != "CC0 1.0":
        raise RuntimeError("unexpected photoreal asset license manifest")

    for folder in (DEST_ROOT, DEST_IMPORT):
        if not EAL.does_directory_exist(folder):
            EAL.make_directory(folder)

    pipeline = make_static_pipeline()
    imported = 0
    model_ids = selected_model_ids()
    log("requested model ids: " + ", ".join(model_ids))

    for asset_id in model_ids:
        source_dir = SOURCE_ROOT / asset_id
        if not source_dir.is_dir():
            log(f"skip missing optional model {asset_id}")
            continue

        candidates = sorted(
            list(source_dir.glob("*.gltf"))
            + list(source_dir.glob("*.glb"))
            + list(source_dir.glob("*.fbx")))
        if not candidates:
            raise RuntimeError(f"no model file found for {asset_id}")

        # Prefer glTF/GLB because dependencies and PBR material semantics map
        # predictably through UE 5.6 Interchange.
        source_model = sorted(
            candidates,
            key=lambda p: ({".gltf": 0, ".glb": 1, ".fbx": 2}.get(p.suffix.lower(), 9), p.name)
        )[0]
        destination = f"{DEST_ROOT}/{asset_id}"
        if not EAL.does_directory_exist(destination):
            EAL.make_directory(destination)
        log(f"import {asset_id}: {source_model}")
        import_file(source_model, destination, pipeline)
        canonical_mesh = canonicalize_primary_mesh(destination, asset_id)
        meshes, textures = configure_imported_assets(destination)
        log(
            f"{asset_id}: canonical={canonical_mesh} "
            f"meshes={meshes} textures={textures}")
        imported += 1

    EAL.save_directory(DEST_ROOT, only_if_is_dirty=False, recursive=True)
    if imported == 0:
        raise RuntimeError("no photoreal models imported")
    log(f"done: {imported} model groups")


main()
