"""LifeLens free Blender build smoke test.

Run:
 blender --background --factory-startup --python Tools/blender/verify_toolchain.py -- OUTDIR

The output is deliberately a *toolchain verification* model, not a finished
human/animal production mesh. Authoring scripts may be added alongside it.
"""
from __future__ import annotations

import pathlib
import sys

import bpy


def main() -> None:
    args = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else []
    if len(args) != 1:
        raise SystemExit('Pass exactly one output directory after --')
    out = pathlib.Path(args[0]).resolve()
    out.mkdir(parents=True, exist_ok=True)

    print(f'LifeLens Blender version: {bpy.app.version_string}')
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

    # A diagnostic shape to prove scene authoring, materials and GLB export
    # are available in the *actual headless runner*.
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=16)
    specimen = bpy.context.object
    specimen.name = 'LifeLensToolchainSmokeOnly'
    specimen.scale = (0.8, 1.05, 0.68)

    material = bpy.data.materials.new('BlenderToolchainCheck')
    material.use_nodes = True
    material.node_tree.nodes.get('Principled BSDF').inputs[
        'Base Color'
    ].default_value = (0.32, 0.58, 0.83, 1.0)
    specimen.data.materials.append(material)

    bpy.ops.wm.save_as_mainfile(filepath=str(out / 'toolchain-smoke.blend'))
    bpy.ops.export_scene.gltf(
        filepath=str(out / 'toolchain-smoke.glb'), export_format='GLB'
    )
    print('LIFELENS_BLENDER_TOOLCHAIN_OK')


if __name__ == '__main__':
    main()
