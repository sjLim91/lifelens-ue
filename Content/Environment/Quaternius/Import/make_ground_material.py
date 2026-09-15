# World Visual Milestone A — tiling ground material from the ambientCG sets.
#
# Run headless from the repository root, with the editor closed, after
# import_nature.py:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Environment/Quaternius/Import/make_ground_material.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# One parent material, one instance per surface. Mobile-oriented: no
# displacement, no tessellation, single UV set, four texture samplers.

import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary

DEST = "/Game/Environment/Materials"
TEX = "/Game/Environment/ambientCG"
PARENT_NAME = "M_LL_Ground"

# instance name -> ambientCG set prefix
INSTANCES = {
    "MI_Ground_Grass": "Grass004_1K-PNG",
    "MI_Ground_DryEarth": "Ground054_1K-PNG",
    "MI_Ground_Transition": "Ground037_1K-PNG",
}

# One tile per 2 m of world space reads well at the observer camera distance.
DEFAULT_TILING = 0.5


def log(msg):
    unreal.log("[LLEnv] " + str(msg))


def build_parent():
    dest = "%s/%s" % (DEST, PARENT_NAME)
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        PARENT_NAME, DEST, unreal.Material, unreal.MaterialFactoryNew())
    if material is None:
        raise RuntimeError("could not create %s" % dest)

    # World-space tiling so the ground plane can be scaled without stretching.
    tiling = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -900, 400)
    tiling.set_editor_property("parameter_name", "Tiling")
    tiling.set_editor_property("default_value", DEFAULT_TILING)

    coords = MEL.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -900, 300)

    scaled = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -700, 340)
    MEL.connect_material_expressions(coords, "", scaled, "A")
    MEL.connect_material_expressions(tiling, "", scaled, "B")

    def sampler(name, x, y, sampler_type):
        node = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        node.set_editor_property("parameter_name", name)
        node.set_editor_property("sampler_type", sampler_type)
        MEL.connect_material_expressions(scaled, "", node, "UVs")
        return node

    base = sampler("BaseColor", -400, 0, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    normal = sampler("Normal", -400, 300, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    rough = sampler("Roughness", -400, 600, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
    occlusion = sampler("AmbientOcclusion", -400, 900, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)

    # A tint parameter lets one surface cover several biome variations without
    # another texture set.
    tint = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, -250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    tinted = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, -150, -50)
    MEL.connect_material_expressions(base, "RGB", tinted, "A")
    MEL.connect_material_expressions(tint, "", tinted, "B")

    MEL.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(rough, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(occlusion, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    material.set_editor_property("two_sided", False)
    MEL.recompile_material(material)
    EAL.save_asset(dest, only_if_is_dirty=False)
    log("built parent material %s" % dest)
    return material


def build_instance(parent, name, prefix):
    dest = "%s/%s" % (DEST, name)
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)
    instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, DEST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if instance is None:
        raise RuntimeError("could not create %s" % dest)
    MEL.set_material_instance_parent(instance, parent)

    for param, suffix in (("BaseColor", "_Color"),
                          ("Normal", "_NormalGL"),
                          ("Roughness", "_Roughness"),
                          ("AmbientOcclusion", "_AmbientOcclusion")):
        texture_path = "%s/%s%s" % (TEX, prefix, suffix)
        texture = EAL.load_asset(texture_path)
        if texture is None:
            raise RuntimeError("missing texture %s" % texture_path)
        MEL.set_material_instance_texture_parameter_value(instance, param, texture)

    MEL.set_material_instance_scalar_parameter_value(instance, "Tiling", DEFAULT_TILING)
    EAL.save_asset(dest, only_if_is_dirty=False)
    log("built instance %s from %s" % (dest, prefix))


def main():
    if not EAL.does_directory_exist(DEST):
        EAL.make_directory(DEST)
    parent = build_parent()
    for name, prefix in sorted(INSTANCES.items()):
        build_instance(parent, name, prefix)
    EAL.save_directory(DEST, only_if_is_dirty=False, recursive=True)
    log("done")


main()
