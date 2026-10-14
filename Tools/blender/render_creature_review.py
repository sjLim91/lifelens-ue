"""Render a neutral offline review photograph of a Blender creature model.

blender --background FILE.blend --threads 2 --python Tools/blender/render_creature_review.py -- OUTPUT.png

Cycles CPU only: reproducible on GitHub runner without WebGL, GPU,
paid services or asset downloads. PNG is review evidence, not approval.
"""
from __future__ import annotations

import pathlib
import sys

import bpy
from mathutils import Vector


def main():
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if len(args) != 1:
        raise SystemExit('Pass OUTPUT.png after --')
    out = pathlib.Path(args[0]).resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    tiger = 'tiger' in pathlib.Path(bpy.data.filepath).stem.lower()
    portrait = 'portrait' in out.stem
    scene = bpy.context.scene
    bpy.ops.object.select_all(action='DESELECT')
    target = (Vector((0,1.24,1.18)) if tiger else Vector((0,-.02,1.67))) if portrait else (Vector((0,.22,.83)) if tiger else Vector((0,.13,.88)))
    origin = (Vector((.47,4.3,1.72)) if tiger else Vector((.12,-3.15,1.75))) if portrait else (Vector((3.2,4.0,2.4)) if tiger else Vector((2.8,-4.3,2.55)))
    camera_data = bpy.data.cameras.new('LifeLensOfflineReviewCamera')
    camera = bpy.data.objects.new('LifeLensOfflineReviewCamera', camera_data)
    scene.collection.objects.link(camera)
    camera.location = origin
    direction = target - camera.location
    camera.rotation_euler = direction.to_track_quat('-Z','Y').to_euler()
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = (.95 if tiger else .65) if portrait else (3.6 if tiger else 2.38)
    scene.camera = camera

    light_data = bpy.data.lights.new('LifeLensReviewSoftbox', 'AREA')
    light = bpy.data.objects.new('LifeLensReviewSoftbox', light_data)
    scene.collection.objects.link(light)
    light.location = (1.8,2.2,4.7)
    light.rotation_euler = (target - light.location).to_track_quat('-Z','Y').to_euler()
    light_data.energy = 650
    light_data.shape = 'DISK'
    light_data.size = 4.0

    world = bpy.data.worlds.new('LifeLensNeutralReviewWorld')
    world.use_nodes = True
    bg = world.node_tree.nodes.get('Background')
    bg.inputs['Color'].default_value = (.62,.67,.72,1.)
    bg.inputs['Strength'].default_value = .8
    scene.world = world
    scene.render.engine = 'CYCLES'
    scene.cycles.device = 'CPU'
    # Ubuntu's free Blender build lacks OpenImageDenoiser; disable it on
    # both scene and view layer before invoking the Cycles CPU backend.
    if hasattr(scene.cycles, 'use_denoising'):
        scene.cycles.use_denoising = False
    for layer in scene.view_layers:
        if hasattr(layer, 'cycles') and hasattr(layer.cycles, 'use_denoising'):
            layer.cycles.use_denoising = False
    scene.cycles.samples = 12
    scene.render.resolution_x = 768 if portrait else 640
    scene.render.resolution_y = 768 if portrait else 640
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = 'PNG'
    scene.render.film_transparent = False
    scene.view_settings.view_transform = 'Standard'
    scene.render.filepath = str(out)
    scene.frame_set(1)
    bpy.ops.render.render(write_still=True)
    if not out.exists() or out.stat().st_size < 10000:
        raise RuntimeError(f'No actual rendered review PNG: {out}')
    print('LIFELENS_CREATURE_REVIEW_RENDER_OK',out.name,out.stat().st_size)


if __name__ == '__main__':
    main()
