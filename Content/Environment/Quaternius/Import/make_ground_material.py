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
# displacement, no tessellation, four texture samplers.
#
# UVs are world-aligned rather than mesh UVs. The ground plane is one 100 uu
# engine plane scaled by 240, so its UV range stays 0..1 across 240 m; scaling
# that by a plain tiling factor stretches a single texture over the whole map.
# Dividing world XY by a tile size gives an exact "one tile per N cm" that is
# independent of how the ground mesh is scaled.

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

# Centimetres of world space per texture tile. The observer camera sits well
# above the residents, so a 4 m tile keeps detail without visible repetition.
DEFAULT_TILE_SIZE_CM = 400.0

# Ground variation across areas is not implemented yet: a scripted two-surface
# blend did not produce a verifiable result in a headless build, so the ground
# ships as one surface per instance and the variation work is tracked
# separately rather than shipped unverified.


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

    # World-aligned UVs: tiling no longer depends on the ground mesh scale.
    tile_size = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -1100, 420)
    tile_size.set_editor_property("parameter_name", "TileSizeCm")
    tile_size.set_editor_property("default_value", DEFAULT_TILE_SIZE_CM)

    world_position = MEL.create_material_expression(
        material, unreal.MaterialExpressionWorldPosition, -1100, 260)

    world_xy = MEL.create_material_expression(
        material, unreal.MaterialExpressionComponentMask, -880, 280)
    world_xy.set_editor_property("r", True)
    world_xy.set_editor_property("g", True)
    world_xy.set_editor_property("b", False)
    world_xy.set_editor_property("a", False)
    MEL.connect_material_expressions(world_position, "", world_xy, "")

    scaled = MEL.create_material_expression(material, unreal.MaterialExpressionDivide, -700, 300)
    MEL.connect_material_expressions(world_xy, "", scaled, "A")
    MEL.connect_material_expressions(tile_size, "", scaled, "B")

    def sampler(name, x, y, sampler_type, default_texture):
        node = MEL.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
        node.set_editor_property("parameter_name", name)
        # A parameter without a default texture falls back to the engine colour
        # DefaultTexture, which does not match a Normal or Linear Grayscale
        # sampler; the material then fails to compile and every ground surface
        # silently renders with the grey default material.
        texture = EAL.load_asset(default_texture)
        if texture is None:
            raise RuntimeError("missing default texture %s" % default_texture)
        node.set_editor_property("texture", texture)
        node.set_editor_property("sampler_type", sampler_type)
        MEL.connect_material_expressions(scaled, "", node, "UVs")
        return node

    default_set = INSTANCES["MI_Ground_Grass"]
    base = sampler("BaseColor", -400, 0, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
                   "%s/%s_Color" % (TEX, default_set))
    normal = sampler("Normal", -400, 300, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                     "%s/%s_NormalGL" % (TEX, default_set))
    rough = sampler("Roughness", -400, 600, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE,
                    "%s/%s_Roughness" % (TEX, default_set))
    occlusion = sampler("AmbientOcclusion", -400, 900, unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE,
                        "%s/%s_AmbientOcclusion" % (TEX, default_set))

    # A tint parameter lets one surface cover several biome variations without
    # another texture set.
    tint = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, -250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    tinted = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, 220, 50)
    MEL.connect_material_expressions(base, "RGB", tinted, "A")
    MEL.connect_material_expressions(tint, "", tinted, "B")

    # Runtime weather parameters are driven by LLDynamicEnvironmentPresentationActor.
    wetness = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 80, 520)
    wetness.set_editor_property("parameter_name", "Wetness")
    wetness.set_editor_property("default_value", 0.0)

    snow = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 80, 680)
    snow.set_editor_property("parameter_name", "Snow")
    snow.set_editor_property("default_value", 0.0)

    precipitation = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 80, 840)
    precipitation.set_editor_property("parameter_name", "Precipitation")
    precipitation.set_editor_property("default_value", 0.0)

    air_temperature = MEL.create_material_expression(material, unreal.MaterialExpressionScalarParameter, 80, 1000)
    air_temperature.set_editor_property("parameter_name", "AirTemperatureC")
    air_temperature.set_editor_property("default_value", 15.0)

    snow_color = MEL.create_material_expression(material, unreal.MaterialExpressionVectorParameter, 80, -180)
    snow_color.set_editor_property("parameter_name", "SnowColor")
    snow_color.set_editor_property("default_value", unreal.LinearColor(0.82, 0.87, 0.91, 1.0))

    wet_dark = MEL.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, 480, 160)
    wet_dark.const_a = 1.0
    wet_dark.const_b = 0.58
    MEL.connect_material_expressions(wetness, "", wet_dark, "Alpha")

    wet_base = MEL.create_material_expression(material, unreal.MaterialExpressionMultiply, 700, 80)
    MEL.connect_material_expressions(tinted, "", wet_base, "A")
    MEL.connect_material_expressions(wet_dark, "", wet_base, "B")

    final_base = MEL.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, 920, 80)
    MEL.connect_material_expressions(wet_base, "", final_base, "A")
    MEL.connect_material_expressions(snow_color, "", final_base, "B")
    MEL.connect_material_expressions(snow, "", final_base, "Alpha")

    wet_rough = MEL.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, 520, 560)
    MEL.connect_material_expressions(rough, "R", wet_rough, "A")
    wet_rough.const_b = 0.22
    MEL.connect_material_expressions(wetness, "", wet_rough, "Alpha")

    final_rough = MEL.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, 760, 560)
    MEL.connect_material_expressions(wet_rough, "", final_rough, "A")
    final_rough.const_b = 0.82
    MEL.connect_material_expressions(snow, "", final_rough, "Alpha")

    MEL.connect_material_property(final_base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(normal, "RGB", unreal.MaterialProperty.MP_NORMAL)
    MEL.connect_material_property(final_rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
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

    MEL.set_material_instance_scalar_parameter_value(instance, "TileSizeCm", DEFAULT_TILE_SIZE_CM)
    MEL.set_material_instance_scalar_parameter_value(instance, "Wetness", 0.0)
    MEL.set_material_instance_scalar_parameter_value(instance, "Snow", 0.0)
    MEL.set_material_instance_scalar_parameter_value(instance, "Precipitation", 0.0)
    MEL.set_material_instance_scalar_parameter_value(instance, "AirTemperatureC", 15.0)
    MEL.set_material_instance_vector_parameter_value(instance, "Tint", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    MEL.set_material_instance_vector_parameter_value(instance, "SnowColor", unreal.LinearColor(0.82, 0.87, 0.91, 1.0))
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
