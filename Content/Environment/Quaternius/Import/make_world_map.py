# World Visual Milestone A — production environment map.
#
# Run headless from the repository root, with the editor closed:
#   "/Users/Shared/Epic Games/UE_5.6/Engine/Binaries/Mac/UnrealEditor-Cmd" \
#       "<repo>/LifeLens.uproject" -run=pythonscript \
#       -script="<repo>/Content/Environment/Quaternius/Import/make_world_map.py" \
#       -EnablePlugins=PythonScriptPlugin -unattended -nopause -nosplash -stdout -FullStdOutLogOutput
#
# Content-only: sky/lighting baseline, a natural ground plane and a
# deterministic natural dressing pass. The scatter is authored into the level
# because the runtime chunk-driven presentation needs a Dagyeom-owned source
# path (Integration Request IR-A) that is not granted yet.
#
# Nothing here creates simulation authority: no beds, toilets, roads, farms or
# tools, and no gameplay state.

import math
import unreal

EAL = unreal.EditorAssetLibrary
MAP_PATH = "/Game/Maps/LifeLensWorld"

NATURE = "/Game/Environment/Quaternius/StylizedNature"
GROUND_MATERIAL = "/Game/Environment/Materials/MI_Ground_Grass"
PLANE_MESH = "/Engine/BasicShapes/Plane"

# The game mode spawns a 1400 x 1400 runtime floor; residents roam several
# thousand units, so the visual ground has to be much larger than that.
GROUND_SIZE_UU = 24000.0
SCATTER_RADIUS_UU = 11000.0
CLEAR_RADIUS_UU = 900.0        # keep the founder area open and readable

SCATTER_SEED = 20260915

# mesh name, count, uniform scale range
SCATTER = [
    ("CommonTree_1", 26, (0.9, 1.35)),
    ("CommonTree_3", 22, (0.9, 1.35)),
    ("CommonTree_5", 18, (0.9, 1.3)),
    ("Pine_1", 16, (1.0, 1.4)),
    ("Pine_3", 14, (1.0, 1.4)),
    ("TwistedTree_2", 12, (0.9, 1.25)),
    ("DeadTree_2", 8, (0.9, 1.2)),
    ("Bush_Common", 34, (0.8, 1.3)),
    ("Grass_Common_Tall", 60, (0.8, 1.4)),
    ("Grass_Wispy_Tall", 50, (0.8, 1.4)),
    ("Fern_1", 24, (0.8, 1.2)),
    ("Rock_Medium_1", 14, (0.8, 1.5)),
    ("Rock_Medium_3", 12, (0.8, 1.5)),
    ("Pebble_Round_2", 20, (0.7, 1.3)),
    ("Pebble_Square_4", 18, (0.7, 1.3)),
]


def log(msg):
    unreal.log("[LLEnv] " + str(msg))


class Random:
    """Deterministic LCG so the same map is produced on every run."""

    def __init__(self, seed):
        self.state = seed & 0xFFFFFFFF

    def next(self):
        self.state = (1103515245 * self.state + 12345) & 0x7FFFFFFF
        return self.state / float(0x7FFFFFFF)

    def range(self, low, high):
        return low + (high - low) * self.next()


def find_mesh(name):
    for asset_path in EAL.list_assets(NATURE, recursive=True, include_folder=False):
        if asset_path.rsplit("/", 1)[-1].split(".")[0] != name:
            continue
        if EAL.find_asset_data(asset_path).asset_class_path.asset_name != "StaticMesh":
            continue
        return EAL.load_asset(asset_path)
    return None


def spawn(actor_subsystem, cls, location, rotation=None):
    return actor_subsystem.spawn_actor_from_class(
        cls, location, rotation if rotation else unreal.Rotator(0.0, 0.0, 0.0))


def build_sky(actors):
    # Daylight baseline. Kept deliberately plain so Observer readability and
    # the Android budget come first; the sun can later be driven by the
    # authoritative simulation minute.
    sun = spawn(actors, unreal.DirectionalLight, unreal.Vector(0, 0, 1500),
                unreal.Rotator(-48.0, -35.0, 0.0))
    sun.set_actor_label("Sun")
    light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    light.set_editor_property("intensity", 4.0)
    light.set_editor_property("light_color", unreal.Color(255, 246, 228))
    light.set_editor_property("atmosphere_sun_light", True)

    sky_light = spawn(actors, unreal.SkyLight, unreal.Vector(0, 0, 1600))
    sky_light.set_actor_label("SkyLight")
    sky_component = sky_light.get_component_by_class(unreal.SkyLightComponent)
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


def build_ground(actors):
    mesh = EAL.load_asset(PLANE_MESH)
    material = EAL.load_asset(GROUND_MATERIAL)
    if mesh is None or material is None:
        raise RuntimeError("missing ground mesh or material")

    ground = spawn(actors, unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    ground.set_actor_label("NaturalGround")
    component = ground.static_mesh_component
    component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    component.set_static_mesh(mesh)
    component.set_material(0, material)
    scale = GROUND_SIZE_UU / 100.0     # engine plane is 100 x 100 uu
    ground.set_actor_scale3d(unreal.Vector(scale, scale, 1.0))
    log("ground plane %.0f x %.0f uu" % (GROUND_SIZE_UU, GROUND_SIZE_UU))
    return ground


def build_scatter(actors):
    rng = Random(SCATTER_SEED)
    placed = 0
    missing = []
    for name, count, scale_range in SCATTER:
        mesh = find_mesh(name)
        if mesh is None:
            missing.append(name)
            continue
        for _ in range(count):
            # Rejection sampling keeps the founder area clear.
            for _attempt in range(12):
                angle = rng.range(0.0, math.pi * 2.0)
                radius = math.sqrt(rng.next()) * SCATTER_RADIUS_UU
                if radius >= CLEAR_RADIUS_UU:
                    break
            x = math.cos(angle) * radius
            y = math.sin(angle) * radius
            actor = spawn(actors, unreal.StaticMeshActor, unreal.Vector(x, y, 0.0),
                          unreal.Rotator(0.0, rng.range(0.0, 360.0), 0.0))
            actor.set_actor_label("Nature_%s_%d" % (name, placed))
            component = actor.static_mesh_component
            component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
            component.set_static_mesh(mesh)
            component.set_collision_profile_name("NoCollision")
            scale = rng.range(scale_range[0], scale_range[1])
            actor.set_actor_scale3d(unreal.Vector(scale, scale, rng.range(scale * 0.92, scale * 1.08)))
            placed += 1
    if missing:
        log("WARNING: meshes not found: %s" % ", ".join(missing))
    log("scattered %d natural meshes" % placed)
    return placed


def main():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not level_subsystem.new_level(MAP_PATH):
        raise RuntimeError("could not create level %s" % MAP_PATH)
    log("created level %s" % MAP_PATH)

    build_sky(actor_subsystem)
    build_ground(actor_subsystem)
    placed = build_scatter(actor_subsystem)

    if not level_subsystem.save_current_level():
        raise RuntimeError("could not save level")
    log("saved level with %d scattered meshes" % placed)
    log("done")


main()
