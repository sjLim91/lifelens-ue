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

# Natural cover is clumped, not evenly sprinkled: groves of trees with open
# ground between them. Each group places its meshes around a set of cluster
# centres, so the same asset count reads as woodland instead of noise.
GROVE_COUNT = 14
GROVE_RADIUS_UU = 2100.0
THICKET_COUNT = 26
THICKET_RADIUS_UU = 900.0

# group: (mesh names, count, scale range, cluster set, tilt degrees)
SCATTER_GROUPS = [
    (["CommonTree_1", "CommonTree_2", "CommonTree_3", "CommonTree_4", "CommonTree_5"],
     150, (0.85, 1.6), "grove", 3.0),
    (["Pine_1", "Pine_2", "Pine_3", "Pine_4", "Pine_5"],
     110, (0.9, 1.7), "grove", 3.0),
    (["TwistedTree_1", "TwistedTree_2", "TwistedTree_3"],
     45, (0.8, 1.45), "grove", 5.0),
    (["DeadTree_1", "DeadTree_2", "DeadTree_3"],
     28, (0.8, 1.35), "grove", 6.0),
    (["Bush_Common", "Bush_Common_Flowers"],
     160, (0.7, 1.6), "thicket", 5.0),
    (["Grass_Common_Tall", "Grass_Common_Short", "Grass_Wispy_Tall", "Grass_Wispy_Short"],
     420, (0.7, 1.8), "thicket", 4.0),
    (["Fern_1", "Plant_1", "Plant_1_Big", "Plant_7"],
     150, (0.7, 1.5), "thicket", 5.0),
    (["Clover_1", "Clover_2", "Flower_3_Group", "Flower_4_Group", "Mushroom_Common"],
     120, (0.8, 1.5), "thicket", 4.0),
    (["Rock_Medium_1", "Rock_Medium_2", "Rock_Medium_3"],
     46, (0.7, 1.9), "scatter", 8.0),
    (["Pebble_Round_1", "Pebble_Round_3", "Pebble_Square_2", "Pebble_Square_4", "Pebble_Square_6"],
     120, (0.6, 1.6), "scatter", 10.0),
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


def make_centres(rng, count, inner_clear):
    centres = []
    while len(centres) < count:
        angle = rng.range(0.0, math.pi * 2.0)
        radius = math.sqrt(rng.next()) * SCATTER_RADIUS_UU
        if radius < inner_clear:
            continue
        centres.append((math.cos(angle) * radius, math.sin(angle) * radius))
    return centres


def build_scatter(actors):
    rng = Random(SCATTER_SEED)
    # Groves stay away from the founder area; thickets may come closer but
    # still leave the immediate spawn ground readable.
    groves = make_centres(rng, GROVE_COUNT, CLEAR_RADIUS_UU * 3.0)
    thickets = make_centres(rng, THICKET_COUNT, CLEAR_RADIUS_UU * 1.2)

    placed = 0
    missing = []
    for names, count, scale_range, cluster_set, tilt in SCATTER_GROUPS:
        meshes = []
        for name in names:
            mesh = find_mesh(name)
            if mesh is None:
                missing.append(name)
            else:
                meshes.append((name, mesh))
        if not meshes:
            continue

        for index in range(count):
            if cluster_set == "grove":
                centre = groves[int(rng.next() * len(groves)) % len(groves)]
                spread = GROVE_RADIUS_UU
            elif cluster_set == "thicket":
                centre = thickets[int(rng.next() * len(thickets)) % len(thickets)]
                spread = THICKET_RADIUS_UU
            else:
                centre = (0.0, 0.0)
                spread = SCATTER_RADIUS_UU

            # Gaussian-ish falloff from the cluster centre keeps edges soft.
            offset_angle = rng.range(0.0, math.pi * 2.0)
            offset_radius = (rng.next() * rng.next()) * spread
            x = centre[0] + math.cos(offset_angle) * offset_radius
            y = centre[1] + math.sin(offset_angle) * offset_radius

            if math.hypot(x, y) < CLEAR_RADIUS_UU or math.hypot(x, y) > SCATTER_RADIUS_UU:
                continue

            name, mesh = meshes[int(rng.next() * len(meshes)) % len(meshes)]
            actor = spawn(actors, unreal.StaticMeshActor, unreal.Vector(x, y, 0.0),
                          unreal.Rotator(rng.range(-tilt, tilt), rng.range(0.0, 360.0),
                                         rng.range(-tilt, tilt)))
            actor.set_actor_label("Nature_%s_%d" % (name, placed))
            component = actor.static_mesh_component
            component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
            component.set_static_mesh(mesh)
            component.set_collision_profile_name("NoCollision")
            scale = rng.range(scale_range[0], scale_range[1])
            actor.set_actor_scale3d(unreal.Vector(scale * rng.range(0.92, 1.08),
                                                  scale * rng.range(0.92, 1.08),
                                                  scale * rng.range(0.9, 1.15)))
            placed += 1

    if missing:
        log("WARNING: meshes not found: %s" % ", ".join(sorted(set(missing))))
    log("scattered %d natural meshes around %d groves and %d thickets"
        % (placed, len(groves), len(thickets)))
    return placed


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
    build_ground(actor_subsystem)
    placed = build_scatter(actor_subsystem)

    if not level_subsystem.save_current_level():
        raise RuntimeError("could not save level")
    log("saved level with %d scattered meshes" % placed)
    log("done")


main()
