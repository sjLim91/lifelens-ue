# Quaternius Track B import (Character Appearance v1).
#
# Run headless from the repository root, with the editor closed:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Characters/Quaternius/Import/import_quaternius.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# Sources: docs/CHARACTER_ASSET_TRACK.md, Content/Characters/Quaternius/PROVENANCE.md.
# Staging folder (outside the repo) holds the original zips and extracted packs.
#
# Pipeline choices follow the pack's Unreal_Setup.png: body glTF first with
# Skeleton = None, animations imported onto that skeleton with
# "Use 30Hz to bake bone animation" and "Snap to closest frame boundary".

import os
import unreal

STAGING = os.environ.get(
    "LL_QUATERNIUS_STAGING",
    "/Users/mac/Desktop/다겸이취미/Lifelens/assets_staging/Quaternius")
UBC = os.path.join(STAGING, "UniversalBaseCharacters_Standard", "Universal Base Characters[Standard]")
UAL = os.path.join(STAGING, "UniversalAnimationLibrary_Standard", "Universal Animation Library[Standard]")

DEST = "/Game/Characters/Quaternius"
DEST_IMPORT = DEST + "/Import"
DEST_MALE = DEST + "/UBC/Male"
DEST_FEMALE = DEST + "/UBC/Female"
DEST_HAIR = DEST + "/UBC/Hair"
DEST_TEX = DEST + "/UBC/Textures"
DEST_UAL = DEST + "/UAL"

GLTF_ASSETS_PIPELINE = "/Interchange/Pipelines/DefaultGLTFAssetsPipeline.DefaultGLTFAssetsPipeline"
GLTF_PIPELINE = "/Interchange/Pipelines/DefaultGLTFPipeline.DefaultGLTFPipeline"
ASSETS_PIPELINE = "/Interchange/Pipelines/DefaultAssetsPipeline.DefaultAssetsPipeline"
TEXTURE_PIPELINE = "/Interchange/Pipelines/DefaultTexturePipeline.DefaultTexturePipeline"

HAIR_FILES = [
    "Hair_Buns", "Hair_SimpleParted", "Hair_Long", "Hair_BuzzedFemale",
    "Hair_Buzzed", "Hair_Beard", "Eyebrows_Regular", "Eyebrows_Female",
]
LIGHT_SKIN_TEXTURES = [
    "T_Superhero_Male_Ligh.png",
    "T_Superhero_Female_Light_BaseColor.png",
]

EAL = unreal.EditorAssetLibrary


def log(msg):
    unreal.log("[LLImport] " + str(msg))


def set_prop(obj, names, value):
    """Set the first editor property that exists among candidate names."""
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return name
        except Exception:
            continue
    raise RuntimeError("none of the properties exist: %s" % (names,))


def make_pipeline(name, source_path, configure):
    """Duplicate an engine default pipeline into the project and configure it."""
    # The commandlet's asset registry has not scanned /Interchange yet, so load
    # the source object directly and duplicate through AssetTools.
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.scan_paths_synchronous(["/Interchange/Pipelines", DEST_IMPORT], True)
    source = unreal.load_object(None, source_path)
    if source is None:
        raise RuntimeError("could not load pipeline %s" % source_path)
    dest = "%s/%s" % (DEST_IMPORT, name)
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)
    pipeline = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset(name, DEST_IMPORT, source)
    if pipeline is None:
        raise RuntimeError("could not duplicate pipeline %s -> %s" % (source_path, dest))
    configure(pipeline)
    EAL.save_asset(dest, only_if_is_dirty=False)
    return unreal.SoftObjectPath("%s.%s" % (dest, name))


def configure_common(pipeline, import_only_animations, skeleton, import_skeletal, import_static, import_materials, import_animations):
    common = pipeline.get_editor_property("common_skeletal_meshes_and_animations_properties")
    set_prop(common, ["import_only_animations"], import_only_animations)
    if skeleton is not None:
        set_prop(common, ["skeleton"], skeleton)
    set_prop(common, ["import_meshes_in_bone_hierarchy"], True)

    mesh = pipeline.get_editor_property("mesh_pipeline")
    set_prop(mesh, ["import_skeletal_meshes"], import_skeletal)
    set_prop(mesh, ["import_static_meshes"], import_static)

    anim = pipeline.get_editor_property("animation_pipeline")
    set_prop(anim, ["import_animations"], import_animations)
    if import_animations:
        set_prop(anim, ["import_bone_tracks"], True)
        set_prop(anim, ["use30_hz_to_bake_bone_animation", "use_30hz_to_bake_bone_animation", "use30hz_to_bake_bone_animation"], True)
        set_prop(anim, ["snap_to_closest_frame_boundary"], True)

    material = pipeline.get_editor_property("material_pipeline")
    set_prop(material, ["import_materials"], import_materials)


def import_file(path, dest_folder, pipelines):
    if not os.path.isfile(path):
        raise RuntimeError("missing source file: %s" % path)
    manager = unreal.InterchangeManager.get_interchange_manager_scripted()
    source = unreal.InterchangeManager.create_source_data(path)
    params = unreal.ImportAssetParameters()
    params.set_editor_property("is_automated", True)
    params.set_editor_property("override_pipelines", pipelines)
    result = manager.import_asset(dest_folder, source, params)
    # Python returns (bool, imported_objects) for the out-parameter overload.
    ok, objects = (result if isinstance(result, tuple) else (bool(result), []))
    log("import %s -> %s : %s, %d objects" % (os.path.basename(path), dest_folder, ok, len(objects)))
    for obj in objects:
        log("  + %s (%s)" % (obj.get_path_name(), obj.get_class().get_name()))
    if not ok:
        raise RuntimeError("import failed: %s" % path)
    return objects


def find_skeleton(folder):
    for asset_path in EAL.list_assets(folder, recursive=True, include_folder=False):
        data = EAL.find_asset_data(asset_path)
        if data.asset_class_path.asset_name == "Skeleton":
            return EAL.load_asset(asset_path)
    return None


def main():
    log("staging: %s" % STAGING)
    for folder in (DEST_IMPORT, DEST_MALE, DEST_FEMALE, DEST_HAIR, DEST_TEX, DEST_UAL):
        EAL.make_directory(folder)

    # 1. Male body: creates the shared skeleton (Skeleton = None on first import).
    body_pipeline = make_pipeline(
        "IP_UBC_Body", GLTF_ASSETS_PIPELINE,
        lambda p: configure_common(p, False, None, True, False, True, False))
    import_file(os.path.join(UBC, "Base Characters", "Godot - UE", "Superhero_Male_FullBody.gltf"),
                DEST_MALE, [body_pipeline, unreal.SoftObjectPath(GLTF_PIPELINE)])

    skeleton = find_skeleton(DEST_MALE)
    if skeleton is None:
        raise RuntimeError("no Skeleton asset produced by the male body import")
    log("shared skeleton: %s" % skeleton.get_path_name())

    # 2. Female body on the same skeleton.
    body_shared_pipeline = make_pipeline(
        "IP_UBC_Body_SharedSkeleton", GLTF_ASSETS_PIPELINE,
        lambda p: configure_common(p, False, skeleton, True, False, True, False))
    import_file(os.path.join(UBC, "Base Characters", "Godot - UE", "Superhero_Female_FullBody.gltf"),
                DEST_FEMALE, [body_shared_pipeline, unreal.SoftObjectPath(GLTF_PIPELINE)])

    # 3. Animations only, onto the shared skeleton (root motion disabled variant).
    anim_pipeline = make_pipeline(
        "IP_UAL_Animations", GLTF_ASSETS_PIPELINE,
        lambda p: configure_common(p, True, skeleton, False, False, False, True))
    import_file(os.path.join(UAL, "Unreal-Godot", "UAL1_Standard.glb"),
                DEST_UAL, [anim_pipeline, unreal.SoftObjectPath(GLTF_PIPELINE)])

    # 4. Hair / eyebrow static meshes (origin at 0; attached to the head socket at runtime).
    hair_pipeline = make_pipeline(
        "IP_UBC_Hair", ASSETS_PIPELINE,
        lambda p: configure_common(p, False, None, False, True, True, False))
    hair_dir = os.path.join(UBC, "Hairstyles", "Origin at 0", "FBX (Unreal Engine)")
    for name in HAIR_FILES:
        import_file(os.path.join(hair_dir, name + ".fbx"), DEST_HAIR, [hair_pipeline])

    # 5. Light skin variants (not referenced by the glTF files).
    tex_dir = os.path.join(UBC, "Base Characters", "Textures")
    for name in LIGHT_SKIN_TEXTURES:
        import_file(os.path.join(tex_dir, name), DEST_TEX, [unreal.SoftObjectPath(TEXTURE_PIPELINE)])

    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    log("done")


main()
