# World Visual Milestone A — natural dressing and ground material import.
#
# Run headless from the repository root, with the editor closed:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Environment/Quaternius/Import/import_nature.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# Sources and licences: Content/Environment/PROVENANCE.md.
# Android budget: every imported texture is clamped, and the meshes are static
# so they can be drawn through instanced components.

import os
import unreal

STAGING = os.environ.get(
    "LL_ASSET_STAGING",
    "/Users/mac/Desktop/다겸이취미/Lifelens/assets_staging")
NATURE = os.path.join(STAGING, "Quaternius", "StylizedNatureMegaKit")
NATURE_GLTF = os.path.join(NATURE, "glTF")
GROUND = os.path.join(STAGING, "ambientCG")

DEST = "/Game/Environment"
DEST_IMPORT = DEST + "/Import"
DEST_NATURE = DEST + "/Quaternius/StylizedNature"
DEST_GROUND = DEST + "/ambientCG"

GLTF_ASSETS_PIPELINE = "/Interchange/Pipelines/DefaultGLTFAssetsPipeline.DefaultGLTFAssetsPipeline"
GLTF_PIPELINE = "/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline"
TEXTURE_PIPELINE = "/Interchange/Pipelines/DefaultTexturePipeline.DefaultTexturePipeline"

# Vegetation and rock meshes keep a small texture budget; ground materials are
# tiling surfaces seen up close, so they keep the full 1K source.
MAX_NATURE_TEXTURE = 1024
MAX_GROUND_TEXTURE = 1024

# ambientCG map suffixes actually consumed by the ground material.
GROUND_SETS = ["Grass004_1K-PNG", "Ground037_1K-PNG", "Ground054_1K-PNG"]
GROUND_MAPS = ["_Color", "_NormalGL", "_Roughness", "_AmbientOcclusion"]

EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log("[LLEnv] " + str(msg))


def set_prop(obj, names, value):
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    log("WARNING: none of %s settable on %s" % (names, obj))
    return None


def make_pipeline(name, source_path, configure):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Interchange/Pipelines", DEST_IMPORT], True)
    source = unreal.load_object(None, source_path)
    if source is None:
        raise RuntimeError("missing engine pipeline: %s" % source_path)
    dest = "%s/%s" % (DEST_IMPORT, name)
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)
    pipeline = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, DEST_IMPORT, source)
    if pipeline is None:
        raise RuntimeError("could not duplicate pipeline %s" % source_path)
    configure(pipeline)
    EAL.save_asset(dest, only_if_is_dirty=False)
    return unreal.SoftObjectPath("%s.%s" % (dest, name))


def configure_static(pipeline):
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    set_prop(common, ["import_only_animations"], False)
    mesh = pipeline.get_editor_property("mesh_pipeline")
    set_prop(mesh, ["import_static_meshes"], True)
    set_prop(mesh, ["import_skeletal_meshes"], False)
    set_prop(mesh, ["combine_static_meshes"], False)
    anim = pipeline.get_editor_property("animation_pipeline")
    set_prop(anim, ["import_animations"], False)
    material = pipeline.get_editor_property("material_pipeline")
    set_prop(material, ["import_materials"], True)


def import_file(path, dest_folder, pipelines):
    if not os.path.isfile(path):
        raise RuntimeError("missing source file: %s" % path)
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    source = unreal.InterchangeManager.create_source_data(path)
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("override_pipelines", pipelines)
    result = manager.import_asset(dest_folder, source, params)
    ok, objects = (result if isinstance(result, tuple) else (bool(result), []))
    if not ok:
        raise RuntimeError("import failed: %s" % path)
    return objects


def dedupe_by_name(folder, class_name):
    """The pack shares one texture set across many models, and Interchange
    imports a private copy per model. Keep one canonical copy per asset name
    and redirect the rest, otherwise the same bark/leaf textures are stored
    dozens of times."""
    by_name = {}
    for asset_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        data = EAL.find_asset_data(asset_path)
        if data.asset_class_path.asset_name != class_name:
            continue
        name = asset_path.rsplit("/", 1)[-1].split(".")[0]
        by_name.setdefault(name, []).append(asset_path)

    merged = 0
    for name, paths in sorted(by_name.items()):
        if len(paths) < 2:
            continue
        paths.sort()
        keep = EAL.load_asset(paths[0])
        drops = [EAL.load_asset(other) for other in paths[1:]]
        drops = [d for d in drops if d is not None]
        if keep is None or not drops:
            continue
        EAL.consolidate_assets(keep, drops)
        merged += len(drops)
    log("consolidated %d duplicate %s assets under %s" % (merged, class_name, folder))


def clamp_textures(folder, max_size):
    count = 0
    for asset_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        data = EAL.find_asset_data(asset_path)
        if data.asset_class_path.asset_name != "Texture2D":
            continue
        texture = EAL.load_asset(asset_path)
        set_prop(texture, ["max_texture_size"], max_size)
        EAL.save_asset(asset_path, only_if_is_dirty=False)
        count += 1
    log("clamped %d textures under %s to %d" % (count, folder, max_size))


def main():
    log("staging: %s" % STAGING)
    for folder in (DEST_IMPORT, DEST_NATURE, DEST_GROUND):
        if not EAL.does_directory_exist(folder):
            EAL.make_directory(folder)

    models = sorted(f for f in os.listdir(NATURE_GLTF) if f.endswith(".gltf"))
    if not models:
        raise RuntimeError("no glTF models under %s" % NATURE_GLTF)
    log("nature models found: %d" % len(models))

    nature_pipeline = make_pipeline("IP_Env_Nature", GLTF_ASSETS_PIPELINE, configure_static)
    for name in models:
        import_file(os.path.join(NATURE_GLTF, name), DEST_NATURE,
                    [nature_pipeline, unreal.SoftObjectPath(GLTF_PIPELINE)])
    log("imported %d nature models" % len(models))

    imported_maps = 0
    for set_name in GROUND_SETS:
        folder = os.path.join(GROUND, set_name)
        for suffix in GROUND_MAPS:
            path = os.path.join(folder, set_name + suffix + ".png")
            if not os.path.isfile(path):
                log("WARNING: missing ground map %s" % path)
                continue
            import_file(path, DEST_GROUND, [unreal.SoftObjectPath(TEXTURE_PIPELINE)])
            imported_maps += 1
    log("imported %d ground maps" % imported_maps)

    dedupe_by_name(DEST_NATURE, "Texture2D")
    dedupe_by_name(DEST_NATURE, "MaterialInstanceConstant")
    clamp_textures(DEST_NATURE, MAX_NATURE_TEXTURE)
    clamp_textures(DEST_GROUND, MAX_GROUND_TEXTURE)
    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)

    meshes = [p for p in EAL.list_assets(DEST_NATURE, recursive=True, include_folder=False)
              if EAL.find_asset_data(p).asset_class_path.asset_name == "StaticMesh"]
    log("static meshes in %s: %d" % (DEST_NATURE, len(meshes)))
    log("done")


main()
