# LifeLens project-authored PCG ground-cover graph.
#
# This creates a small, deterministic CPU PCG graph that scatters only
# decorative ground cover. Gameplay trees/rocks/resources remain Core authority.
#
# Headless:
#   UnrealEditor-Cmd LifeLens.uproject -run=pythonscript \
#     -script=Content/Environment/PCG/Import/create_lifelens_groundcover_pcg.py \
#     -EnablePlugins=PythonScriptPlugin -nullrhi -unattended -nopause -nosplash -stdout

import unreal

DEST_FOLDER = "/Game/Environment/PCG"
GRAPH_NAME = "PCG_LL_GroundCover"
GRAPH_PATH = f"{DEST_FOLDER}/{GRAPH_NAME}"
GROUND_COVER_MESH_PATH = (
    "/Game/Environment/Photoreal/PolyHaven/weed_plant_02/"
    "SM_LL_weed_plant_02.SM_LL_weed_plant_02"
)

EAL = unreal.EditorAssetLibrary


def log(message):
    unreal.log("[LLPCG] " + str(message))


def set_prop(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as exc:
        log(f"property skip {obj.__class__.__name__}.{name}: {exc}")
        return False


def created_node(result):
    # UE Python exposes AddNodeOfType as (node, default_settings). Keep this
    # tolerant of wrapper changes where only the node is returned.
    if isinstance(result, tuple):
        return result[0], result[1]
    if isinstance(result, list):
        return result[0], result[1] if len(result) > 1 else None
    return result, None


def first_pin_label(node, direction):
    pins = node.get_editor_property(
        "output_pins" if direction == "out" else "input_pins")
    if not pins:
        raise RuntimeError(
            f"{node.get_name()} has no {direction} pins")
    for pin in pins:
        props = pin.get_editor_property("properties")
        label = props.get_editor_property("label")
        if str(label):
            return label
    raise RuntimeError(
        f"{node.get_name()} has no labelled {direction} pin")


def connect(graph, source, target):
    source_label = first_pin_label(source, "out")
    target_label = first_pin_label(target, "in")
    log(
        f"connect {source.get_name()}:{source_label} -> "
        f"{target.get_name()}:{target_label}")
    graph.add_edge(source, source_label, target, target_label)


def make_graph():
    mesh = unreal.load_object(None, GROUND_COVER_MESH_PATH)
    if mesh is None:
        raise RuntimeError(
            f"missing approved ground-cover mesh: {GROUND_COVER_MESH_PATH}")

    if not EAL.does_directory_exist(DEST_FOLDER):
        EAL.make_directory(DEST_FOLDER)
    if EAL.does_asset_exist(GRAPH_PATH):
        if not EAL.delete_asset(GRAPH_PATH):
            raise RuntimeError(f"failed to replace {GRAPH_PATH}")

    factory = unreal.PCGGraphFactory()
    graph = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        GRAPH_NAME,
        DEST_FOLDER,
        unreal.PCGGraph,
        factory)
    if graph is None:
        raise RuntimeError("failed to create PCG graph")

    set_prop(graph, "title", "LifeLens Ground Cover")
    set_prop(
        graph,
        "description",
        "Decorative deterministic ground cover; never gameplay resource authority.")
    set_prop(graph, "expose_to_library", True)
    set_prop(graph, "use2d_grid", True)
    set_prop(graph, "use_hierarchical_generation", False)

    grid_node, grid = created_node(
        graph.add_node_of_type(unreal.PCGCreatePointsGridSettings))
    if grid is None:
        grid = grid_node.get_editor_property(
            "settings_interface").get_settings()
    set_prop(grid_node, "node_title", "LL Ground Cover Grid")
    set_prop(grid, "cell_size", unreal.Vector(625.0, 625.0, 100.0))
    set_prop(grid, "grid_extents", unreal.Vector(2500.0, 2500.0, 50.0))
    set_prop(grid, "cull_points_outside_volume", False)
    set_prop(grid, "set_points_bounds", True)
    set_prop(grid, "point_steepness", 1.0)
    set_prop(grid, "seed", 9419)
    set_prop(grid, "execute_on_gpu", False)
    try:
        set_prop(grid, "point_position", unreal.PCGPointPosition.CELL_CENTER)
    except Exception as exc:
        log(f"point_position default retained: {exc}")

    transform_node, transform = created_node(
        graph.add_node_of_type(unreal.PCGTransformPointsSettings))
    if transform is None:
        transform = transform_node.get_editor_property(
            "settings_interface").get_settings()
    set_prop(transform_node, "node_title", "LL Ground Cover Jitter")
    set_prop(transform, "offset_min", unreal.Vector(-235.0, -235.0, 0.0))
    set_prop(transform, "offset_max", unreal.Vector(235.0, 235.0, 0.0))
    set_prop(transform, "rotation_min", unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
    set_prop(transform, "rotation_max", unreal.Rotator(roll=0.0, pitch=0.0, yaw=359.0))
    set_prop(transform, "scale_min", unreal.Vector(0.65, 0.65, 0.65))
    set_prop(transform, "scale_max", unreal.Vector(1.18, 1.18, 1.18))
    set_prop(transform, "uniform_scale", True)
    set_prop(transform, "recompute_seed", True)
    set_prop(transform, "seed", 17749)
    set_prop(transform, "execute_on_gpu", False)

    spawner_node, spawner = created_node(
        graph.add_node_of_type(unreal.PCGStaticMeshSpawnerSettings))
    if spawner is None:
        spawner = spawner_node.get_editor_property(
            "settings_interface").get_settings()
    set_prop(spawner_node, "node_title", "LL Approved Ground Cover")
    spawner.set_mesh_selector_type(unreal.PCGMeshSelectorWeighted)
    set_prop(spawner, "synchronous_load", True)
    set_prop(spawner, "allow_descriptor_changes", True)
    set_prop(spawner, "execute_on_gpu", False)

    selector = spawner.get_editor_property("mesh_selector_parameters")
    if selector is None:
        raise RuntimeError("weighted PCG mesh selector was not created")

    descriptor = unreal.PCGSoftISMComponentDescriptor()
    set_prop(descriptor, "static_mesh", mesh)
    set_prop(descriptor, "can_ever_affect_navigation", False)
    set_prop(descriptor, "generate_overlap_events", False)
    set_prop(descriptor, "use_default_collision", False)
    set_prop(descriptor, "enable_density_scaling", True)
    set_prop(descriptor, "cast_shadow", False)
    set_prop(descriptor, "affect_distance_field_lighting", False)
    set_prop(descriptor, "instance_start_cull_distance", 2800)
    set_prop(descriptor, "instance_end_cull_distance", 11500)

    entry = unreal.PCGMeshSelectorWeightedEntry()
    set_prop(entry, "descriptor", descriptor)
    set_prop(entry, "weight", 100)
    set_prop(selector, "mesh_entries", [entry])

    connect(graph, grid_node, transform_node)
    connect(graph, transform_node, spawner_node)
    connect(graph, spawner_node, graph.get_output_node())

    try:
        graph.force_notification_for_editor(unreal.PCGChangeType.STRUCTURAL)
    except Exception:
        graph.force_notification_for_editor()

    if not EAL.save_asset(GRAPH_PATH, only_if_is_dirty=False):
        raise RuntimeError(f"failed to save {GRAPH_PATH}")

    loaded = EAL.load_asset(GRAPH_PATH)
    if loaded is None or not isinstance(loaded, unreal.PCGGraph):
        raise RuntimeError(f"saved asset is not a PCGGraph: {GRAPH_PATH}")

    node_count = len(loaded.get_editor_property("nodes"))
    if node_count < 3:
        raise RuntimeError(
            f"unexpected PCG graph node count: {node_count}")
    log(
        f"created {GRAPH_PATH}: nodes={node_count} "
        f"mesh={GROUND_COVER_MESH_PATH}")


make_graph()
