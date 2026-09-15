# World Visual Milestone A — production environment map.
#
# Run headless from the repository root, with the editor closed:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Environment/Quaternius/Import/make_world_map.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# The map authors only the sky/lighting baseline and one WorldPresentation
# actor. Ground and natural dressing are built at runtime from the
# authoritative world generation observation (`ALLWorldPresentationActor`), so
# no world content is baked into the level and nothing here becomes a second
# authority over what exists in the world.

import unreal

EAL = unreal.EditorAssetLibrary
MAP_PATH = "/Game/Maps/LifeLensWorld"
PRESENTATION_CLASS = "/Script/LifeLens.LLWorldPresentationActor"


def log(msg):
    unreal.log("[LLEnv] " + str(msg))


def spawn(actor_subsystem, cls, location, rotation=None):
    return actor_subsystem.spawn_actor_from_class(
        cls, location, rotation if rotation else unreal.Rotator(0.0, 0.0, 0.0))


def build_sky(actors):
    # Daylight baseline, fully dynamic. Movable lights need no baked lightmaps,
    # which removes the "lighting needs to be rebuilt" state for every static
    # mesh in the level, and they are also the prerequisite for driving the sun
    # from the authoritative simulation minute later.
    sun = spawn(actors, unreal.DirectionalLight, unreal.Vector(0, 0, 1500),
                unreal.Rotator(-48.0, -35.0, 0.0))
    sun.set_actor_label("Sun")
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    light.set_editor_property("intensity", 4.0)
    light.set_editor_property("light_color", unreal.Color(255, 246, 228))
    light.set_editor_property("atmosphere_sun_light", True)
    # Android budget: cascaded shadows only around the observed area.
    light.set_editor_property("dynamic_shadow_distance_movable_light", 9000.0)
    light.set_editor_property("dynamic_shadow_cascades", 3)

    sky_light = spawn(actors, unreal.SkyLight, unreal.Vector(0, 0, 1600))
    sky_light.set_actor_label("SkyLight")
    sky_component = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky_component.set_editor_property("real_time_capture", True)
    sky_component.set_editor_property("intensity", 1.0)

    atmosphere = spawn(actors, unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    atmosphere.set_actor_label("SkyAtmosphere")

    fog = spawn(actors, unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    fog.set_actor_label("HeightFog")
    fog_component = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fog_component.set_editor_property("fog_density", 0.012)
    fog_component.set_editor_property("fog_height_falloff", 0.15)

    clouds = spawn(actors, unreal.VolumetricCloud, unreal.Vector(0, 0, 0))
    clouds.set_actor_label("Clouds")
    log("sky/lighting actors placed")


def build_world_presentation(actors):
    """Place the runtime presentation actor. It reads the authoritative world
    generation observation and builds ground and natural dressing from Core
    facts, so the map itself authors no world content."""
    presentation_class = unreal.load_class(None, PRESENTATION_CLASS)
    if presentation_class is None:
        raise RuntimeError("LLWorldPresentationActor not found; compile the project first")
    actor = spawn(actors, presentation_class, unreal.Vector(0.0, 0.0, 0.0))
    actor.set_actor_label("WorldPresentation")
    log("placed WorldPresentation actor")
    return actor


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    # Rebuilding is deterministic, so an existing map is replaced rather than
    # merged into.
    if EAL.does_asset_exist(MAP_PATH):
        EAL.delete_asset(MAP_PATH)
        log("removed previous %s" % MAP_PATH)
    if not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError("could not create level %s" % MAP_PATH)
    log("created level %s" % MAP_PATH)

    build_sky(actor_subsystem)
    build_world_presentation(actor_subsystem)

    if not level_subsystem.save_current_level():
        raise RuntimeError("could not save level")
    log("saved level")
    log("done")


main()
