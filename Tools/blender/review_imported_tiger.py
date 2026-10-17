"""Render an imported candidate with its own topology and materials, without remeshing."""
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Matrix, Vector


def main():
    bpy.context.preferences.filepaths.save_version=0
    args = sys.argv[sys.argv.index('--') + 1:]
    out = Path(args[0]).resolve()
    source_file = args[1] if len(args)>1 else 'source.glb'
    extracted = source_file != 'source.glb'
    prefix = ('rig-' if source_file == 'tiger-rigged.glb' else 'body-') if extracted else ''
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(out / source_file))
    bone_shapes = {pose.custom_shape for armature in bpy.context.scene.objects if armature.type=='ARMATURE'
                   for pose in armature.pose.bones if pose.custom_shape}
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH' and obj not in bone_shapes]
    if not meshes:
        raise ValueError('Imported candidate has no meshes')
    if source_file=='tiger-rigged.glb':
        for armature in bpy.context.scene.objects:
            if armature.type!='ARMATURE':continue
            idle=next((action for action in bpy.data.actions
                       if action.name in ('tiger_idle','tiger_idle_'+armature.name)),None)
            armature.animation_data_create()
            for track in armature.animation_data.nla_tracks:track.mute=True
            if idle:armature.animation_data.action=idle
        bpy.context.scene.frame_set(1)
    # This particular source faces -X, whereas the observer's animals face +Z
    # in glTF (= -Y in Blender). Correct the review labels using a root transform.
    rotate = Matrix.Identity(4) if extracted else Matrix.Rotation(math.pi/2, 4, 'Z')
    for obj in meshes:
        obj.matrix_world = rotate @ obj.matrix_world
    if not extracted and out.name == 'tiger-topoforge' and len(meshes)==1:
        source = meshes[0]
        positions = [[round(value,6) for value in source.matrix_world @ v.co] for v in source.data.vertices]
        triangles = [[p.vertices[0],p.vertices[i],p.vertices[i+1]]
                     for p in source.data.polygons for i in range(1,len(p.vertices)-1)]
        texcoords = [None]*len(positions)
        uv_layer = source.data.uv_layers.active
        if uv_layer:
            for loop in source.data.loops:
                uv = [round(value,7) for value in uv_layer.data[loop.index].uv]
                texcoords[loop.vertex_index] = uv
        for name,data in [('positions',positions),('triangles',triangles),('uv',texcoords)]:
            (out/('inspection-'+name+'.json')).write_text(json.dumps(
                {'purpose':'INSPECTION_ONLY_COORDINATES_ROUNDED_NOT_RUNTIME','data':data},separators=(',',':'))+'\n')
        images = [node.image for material in source.data.materials if material and material.use_nodes
                  for node in material.node_tree.nodes if node.type=='TEX_IMAGE' and node.image]
        if images:
            preview = images[0].copy()
            width,height = preview.size
            factor = min(1,1024/max(width,height))
            preview.scale(max(1,round(width*factor)),max(1,round(height*factor)))
            preview.filepath_raw = str(out/'inspection-albedo-1024.jpg')
            preview.file_format = 'JPEG'
            preview.save()
            bpy.data.images.remove(preview)
    components = []
    geometric_components = []
    for obj in meshes:
        adjacency = [[] for _ in obj.data.vertices]
        for edge in obj.data.edges:
            a, b = edge.vertices
            adjacency[a].append(b)
            adjacency[b].append(a)
        position_groups = {}
        for vertex in obj.data.vertices:
            key = tuple(round(value, 5) for value in vertex.co)
            position_groups.setdefault(key, []).append(vertex.index)
        welded_adjacency = [list(neighbors) for neighbors in adjacency]
        for indices in position_groups.values():
            for index in indices:
                welded_adjacency[index].extend(i for i in indices if i != index)
        # Inspect geometry connectivity across UV/normal seams without welding
        # or otherwise changing the original render mesh or its UV coordinates.
        for neighbors, destination in [(welded_adjacency, geometric_components)]:
            remaining = set(range(len(neighbors)))
            while remaining:
                seed = remaining.pop()
                pending, indices = [seed], [seed]
                while pending:
                    for neighbor in neighbors[pending.pop()]:
                        if neighbor in remaining:
                            remaining.remove(neighbor)
                            pending.append(neighbor)
                            indices.append(neighbor)
                points = [obj.matrix_world @ obj.data.vertices[index].co for index in indices]
                destination.append({'vertices': len(indices),
                                    'min': [min(p[i] for p in points) for i in range(3)],
                                    'max': [max(p[i] for p in points) for i in range(3)]})
        remaining = set(range(len(adjacency)))
        while remaining:
            seed = remaining.pop()
            pending = [seed]
            indices = [seed]
            while pending:
                for neighbor in adjacency[pending.pop()]:
                    if neighbor in remaining:
                        remaining.remove(neighbor)
                        pending.append(neighbor)
                        indices.append(neighbor)
            points = [obj.matrix_world @ obj.data.vertices[index].co for index in indices]
            components.append({'vertices': len(indices),
                               'min': [min(p[i] for p in points) for i in range(3)],
                               'max': [max(p[i] for p in points) for i in range(3)]})
    components.sort(key=lambda item: -item['vertices'])
    geometric_components.sort(key=lambda item: -item['vertices'])
    bpy.context.view_layer.update()
    depsgraph=bpy.context.evaluated_depsgraph_get()
    evaluated=[obj.evaluated_get(depsgraph) for obj in meshes]
    corners = [obj.matrix_world @ Vector(corner) for obj in evaluated for corner in obj.bound_box]
    low = Vector([min(p[i] for p in corners) for i in range(3)])
    high = Vector([max(p[i] for p in corners) for i in range(3)])
    center = (low + high) * .5
    size = high - low
    radius = max(size) * .75
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 16
    scene.cycles.use_denoising = False
    scene.render.resolution_x = 640
    scene.render.resolution_y = 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.world.color = (.12, .12, .12)
    scene.view_settings.view_transform = 'Standard'
    scene.view_settings.look = 'Medium High Contrast'
    bpy.ops.object.camera_add()
    camera = bpy.context.object
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = max(size) * 1.2
    scene.camera = camera
    camera.location = center + Vector((0, -radius, radius*.22))
    camera.rotation_euler = (center-camera.location).to_track_quat('-Z', 'Y').to_euler()
    for position, energy, light_size in [((2, -3, 5), 1100, 5), ((-3, 1, 3), 800, 4)]:
        bpy.ops.object.light_add(type='AREA', location=center + Vector(position))
        light = bpy.context.object
        light.data.energy = energy
        light.data.size = light_size
        light.rotation_euler = (center - light.location).to_track_quat('-Z', 'Y').to_euler()
    floor=0 if source_file=='tiger-rigged.glb' else low.z
    bpy.ops.mesh.primitive_plane_add(size=max(size)*4, location=(center.x, center.y, floor - .003))
    plane = bpy.context.object
    material = bpy.data.materials.new('ReviewNeutralGround')
    material.diffuse_color = (.18, .18, .18, 1)
    plane.data.materials.append(material)
    # Source geometry/materials are untouched. Only review stage objects are added.
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(out / (prefix + 'source-review.blend')))
    for name, angle in [('front', 0), ('side', math.pi/2), ('three-quarter', math.pi/4), ('back', math.pi)]:
        camera.location = center + Vector((math.sin(angle)*radius, -math.cos(angle)*radius, radius*.22))
        camera.rotation_euler = (center-camera.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(out / (prefix + name + '.png'))
        bpy.ops.render.render(write_still=True)
    if not extracted and out.name == 'tiger-topoforge':
        camera.location = center + Vector((radius*.707,-radius*.707,radius*.22))
        camera.rotation_euler = (center-camera.location).to_track_quat('-Z','Y').to_euler()
        sys.path.insert(0,str(Path(__file__).resolve().parent))
        from extract_tiger_body import FOOT_ANCHORS
        anchors = FOOT_ANCHORS
        colors = [(1,.02,.02,1),(.02,1,.02,1),(.02,.15,1,1),(1,.1,1,1)]
        diagnostic_labels = []
        for number, (anchor, color) in enumerate(zip(anchors,colors),1):
            material = bpy.data.materials.new('DIAGNOSTIC_MARKER_'+str(number))
            material.use_nodes = True
            tree = material.node_tree
            tree.nodes.clear()
            emission = tree.nodes.new('ShaderNodeEmission')
            emission.inputs['Color'].default_value = color
            output = tree.nodes.new('ShaderNodeOutputMaterial')
            tree.links.new(emission.outputs[0],output.inputs['Surface'])
            bpy.ops.mesh.primitive_uv_sphere_add(segments=12,ring_count=8,radius=.055,location=anchor)
            bpy.context.object.data.materials.append(material)
            point = Vector(anchor)
            bpy.ops.object.text_add(location=point+Vector((0,0,.20)))
            label = bpy.context.object
            label.data.body = str(number)
            label.data.size = .16
            label.rotation_euler = camera.rotation_euler
            label.data.materials.append(material)
            diagnostic_labels.append(label)
        scene.render.filepath = str(out/'source-foot-anchor-diagnostic.png')
        bpy.ops.render.render(write_still=True)
        for name, angle in [('front',0),('side',math.pi/2)]:
            camera.location = center + Vector((math.sin(angle)*radius,-math.cos(angle)*radius,radius*.22))
            camera.rotation_euler = (center-camera.location).to_track_quat('-Z','Y').to_euler()
            for label in diagnostic_labels:
                label.rotation_euler = camera.rotation_euler
            scene.render.filepath = str(out/('source-foot-anchor-'+name+'.png'))
            bpy.ops.render.render(write_still=True)
    report = {
        'mesh_objects': len(meshes),
        'vertices': sum(len(obj.data.vertices) for obj in meshes),
        'triangles': sum(sum(len(p.vertices)-2 for p in obj.data.polygons) for obj in meshes),
        'bounds_blender': {'min': list(low), 'max': list(high)},
        'armatures': len([obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']),
        'connected_component_count': len(components),
        'connected_components': components[:40],
        'geometric_component_count': len(geometric_components),
        'geometric_components': geometric_components[:40],
        'review_orientation_change': 'canonical -Y forward' if extracted else 'source -X forward rotated +90 degrees about Blender Z',
        'visual_acceptance': 'PENDING_HUMAN_OR_AGENT_IMAGE_REVIEW',
        'observer_validation': 'NOT_RUN', 'performance': 'NOT_MEASURED',
    }
    (out / (prefix + 'blender-inspection.json')).write_text(json.dumps(report, indent=2) + '\n')
    print('BLENDER_INSPECTION', json.dumps(report))


if __name__ == '__main__':
    main()
