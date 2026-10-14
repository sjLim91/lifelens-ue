"""Render an imported candidate with its own topology and materials, without remeshing."""
import json
import math
from pathlib import Path
import sys

import bpy
from mathutils import Vector


def main():
    out = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(out / 'source.glb'))
    meshes = [obj for obj in bpy.context.scene.objects if obj.type == 'MESH']
    if not meshes:
        raise ValueError('Imported candidate has no meshes')
    corners = [obj.matrix_world @ Vector(corner) for obj in meshes for corner in obj.bound_box]
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
    for position, energy, light_size in [((2, -3, 5), 1100, 5), ((-3, 1, 3), 800, 4)]:
        bpy.ops.object.light_add(type='AREA', location=center + Vector(position))
        light = bpy.context.object
        light.data.energy = energy
        light.data.size = light_size
        light.rotation_euler = (center - light.location).to_track_quat('-Z', 'Y').to_euler()
    bpy.ops.mesh.primitive_plane_add(size=max(size)*4, location=(center.x, center.y, low.z - .003))
    plane = bpy.context.object
    material = bpy.data.materials.new('ReviewNeutralGround')
    material.diffuse_color = (.18, .18, .18, 1)
    plane.data.materials.append(material)
    # Source geometry/materials are untouched. Only review stage objects are added.
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(out / 'source-review.blend'))
    for name, angle in [('front', 0), ('side', math.pi/2), ('three-quarter', math.pi/4), ('back', math.pi)]:
        camera.location = center + Vector((math.sin(angle)*radius, -math.cos(angle)*radius, radius*.22))
        camera.rotation_euler = (center-camera.location).to_track_quat('-Z', 'Y').to_euler()
        scene.render.filepath = str(out / (name + '.png'))
        bpy.ops.render.render(write_still=True)
    report = {
        'mesh_objects': len(meshes),
        'vertices': sum(len(obj.data.vertices) for obj in meshes),
        'triangles': sum(sum(len(p.vertices)-2 for p in obj.data.polygons) for obj in meshes),
        'bounds_blender': {'min': list(low), 'max': list(high)},
        'armatures': len([obj for obj in bpy.context.scene.objects if obj.type == 'ARMATURE']),
        'visual_acceptance': 'PENDING_HUMAN_OR_AGENT_IMAGE_REVIEW',
        'observer_validation': 'NOT_RUN', 'performance': 'NOT_MEASURED',
    }
    (out / 'blender-inspection.json').write_text(json.dumps(report, indent=2) + '\n')
    print('BLENDER_INSPECTION', json.dumps(report))


if __name__ == '__main__':
    main()
