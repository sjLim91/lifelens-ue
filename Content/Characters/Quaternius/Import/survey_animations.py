# Character Context Motion v2 — animation clip survey.
#
# Read-only. Loads every AnimSequence under the imported Quaternius UAL pack and
# prints the facts the runtime clip mapping depends on: play length, frame
# count, sample rate, root motion flag and skeleton. Nothing is written, so this
# can be re-run at any time to re-confirm the mapping evidence.
#
# Run headless from the repository root, with the editor closed:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Characters/Quaternius/Import/survey_animations.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput

import unreal

EAL = unreal.EditorAssetLibrary
SEARCH_ROOTS = [
    "/Game/Characters/Quaternius/UAL",
]


def log(msg):
    unreal.log("[LLAnimSurvey] " + str(msg))


def prop(obj, name, default=None):
    """Editor property names differ between engine versions; never assume."""
    try:
        return obj.get_editor_property(name)
    except Exception:
        return default


def describe(sequence):
    length = prop(sequence, "sequence_length")
    if length is None:
        try:
            length = sequence.get_play_length()
        except Exception:
            length = -1.0

    frames = prop(sequence, "number_of_sampled_keys")
    if frames is None:
        frames = prop(sequence, "number_of_frames", -1)

    rate = prop(sequence, "target_frame_rate")
    rate_text = "?"
    if rate is not None:
        try:
            rate_text = "%g" % (float(rate.numerator) / float(rate.denominator))
        except Exception:
            rate_text = str(rate)

    root_motion = prop(sequence, "enable_root_motion", None)
    if root_motion is None:
        root_motion = prop(sequence, "b_enable_root_motion", "?")

    skeleton = prop(sequence, "skeleton")
    skeleton_name = skeleton.get_name() if skeleton else "?"

    return {
        "length": length if length is not None else -1.0,
        "frames": frames if frames is not None else -1,
        "rate": rate_text,
        "root_motion": root_motion,
        "skeleton": skeleton_name,
    }


def main():
    paths = []
    for root in SEARCH_ROOTS:
        if not EAL.does_directory_exist(root):
            log("missing directory %s" % root)
            continue
        paths.extend(EAL.list_assets(root, recursive=True, include_folder=False))

    rows = []
    for path in sorted(set(paths)):
        asset = EAL.load_asset(path)
        if not isinstance(asset, unreal.AnimSequence):
            continue
        rows.append((asset.get_name(), path.split(".")[0], describe(asset)))

    log("found %d AnimSequence assets" % len(rows))
    log("%-26s %8s %7s %8s %11s  %s" % ("NAME", "SECONDS", "FRAMES", "RATE", "ROOTMOTION", "PATH"))
    for name, path, info in rows:
        log("%-26s %8.3f %7s %8s %11s  %s" % (
            name, float(info["length"]), str(info["frames"]), info["rate"],
            str(info["root_motion"]), path))

    # Blend spaces are listed separately: the locomotion blend space is already
    # wired and must not be confused with a context clip candidate.
    for path in sorted(set(paths)):
        asset = EAL.load_asset(path)
        if isinstance(asset, unreal.BlendSpace) or isinstance(asset, unreal.BlendSpace1D):
            log("blendspace: %s" % path.split(".")[0])

    log("done")


main()
