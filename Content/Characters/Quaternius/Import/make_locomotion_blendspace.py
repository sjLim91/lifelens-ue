# Build the resident locomotion BlendSpace1D (Character Motion Bootstrap).
#
# Run headless from the repository root, with the editor closed, after
# import_quaternius.py has imported the UAL animations:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Characters/Quaternius/Import/make_locomotion_blendspace.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# One axis: ground speed in cm/s. The presentation layer feeds the measured
# speed of the resident actor; Core/World keep all movement authority.

import unreal

EAL = unreal.EditorAssetLibrary

UAL = "/Game/Characters/Quaternius/UAL/UAL1_Standard/SkeletalMeshes"
SKELETON = "/Game/Characters/Quaternius/UBC/Male/Superhero_Male_FullBody/SkeletalMeshes/Superhero_Male_FullBody_Skeleton"
DEST_FOLDER = "/Game/Characters/Quaternius/UAL"
ASSET_NAME = "BS_ResidentLocomotion"

# Sample speeds follow the UE humanoid convention (walk ~150, jog ~375,
# sprint ~600 cm/s); the Quaternius rig is a standard-height humanoid.
SAMPLES = [
    ("Idle_Loop", 0.0),
    ("Walk_Loop", 150.0),
    ("Jog_Fwd_Loop", 375.0),
    ("Sprint_Loop", 600.0),
]
MAX_SPEED = 600.0


def log(msg):
    unreal.log("[LLMotion] " + str(msg))


def main():
    skeleton = EAL.load_asset(SKELETON)
    if skeleton is None:
        raise RuntimeError("missing skeleton: %s" % SKELETON)

    dest = "%s/%s" % (DEST_FOLDER, ASSET_NAME)
    if EAL.does_asset_exist(dest):
        EAL.delete_asset(dest)

    factory = unreal.BlendSpaceFactory1D()
    factory.set_editor_property("target_skeleton", skeleton)
    blend_space = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_NAME, DEST_FOLDER, unreal.BlendSpace1D, factory)
    if blend_space is None:
        raise RuntimeError("could not create %s" % dest)

    axis = unreal.BlendParameter()
    axis.set_editor_property("display_name", "Speed")
    axis.set_editor_property("min", 0.0)
    axis.set_editor_property("max", MAX_SPEED)
    axis.set_editor_property("grid_num", 4)
    parameters = blend_space.get_editor_property("blend_parameters")
    parameters[0] = axis
    blend_space.set_editor_property("blend_parameters", parameters)

    samples = []
    for name, speed in SAMPLES:
        animation = EAL.load_asset("%s/%s" % (UAL, name))
        if animation is None:
            raise RuntimeError("missing animation: %s/%s" % (UAL, name))
        sample = unreal.BlendSample()
        sample.set_editor_property("animation", animation)
        sample.set_editor_property("sample_value", unreal.Vector(speed, 0.0, 0.0))
        sample.set_editor_property("rate_scale", 1.0)
        samples.append(sample)
        log("sample %-14s at %.0f cm/s" % (name, speed))
    blend_space.set_editor_property("sample_data", samples)

    # Scale the playback rate along the speed axis so the stride roughly
    # follows the actual ground speed between samples.
    blend_space.set_editor_property("axis_to_scale_animation", unreal.BlendSpaceAxis.BSA_X)

    EAL.save_asset(dest, only_if_is_dirty=False)
    reloaded = EAL.load_asset(dest)
    stored = reloaded.get_editor_property("sample_data")
    log("saved %s with %d samples" % (dest, len(stored)))
    for sample in stored:
        log("  stored: %s @ %s" % (sample.get_editor_property("animation").get_name(),
                                   sample.get_editor_property("sample_value").x))
    if len(stored) != len(SAMPLES):
        raise RuntimeError("sample data not persisted")
    log("done")


main()
