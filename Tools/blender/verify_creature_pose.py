"""Evaluate real Blender skin deformation, not just GLB channel presence.

Runs with a generated .blend loaded into the scene:
blender --background resident-female.blend --python Tools/blender/verify_creature_pose.py
"""
from __future__ import annotations

import math
import bpy
from mathutils import Vector


def sampled_world_vertices(obj):
    depsgraph=bpy.context.evaluated_depsgraph_get()
    evaluated=obj.evaluated_get(depsgraph)
    mesh=evaluated.to_mesh()
    try:
        stride=max(1,len(mesh.vertices)//400)
        return [evaluated.matrix_world @ mesh.vertices[i].co.copy()
                for i in range(0,len(mesh.vertices),stride)]
    finally:
        evaluated.to_mesh_clear()


def max_delta(left,right):
    assert len(left)==len(right)
    return max((a-b).length for a,b in zip(left,right))


def main():
    scene=bpy.context.scene
    kind='tiger' if 'tiger' in bpy.data.filepath else 'human'
    rig=bpy.data.objects.get('LifeLensTigerRig' if kind=='tiger' else 'LifeLensHumanoidRig')
    body=bpy.data.objects.get('LifeLensSeamlessTiger' if kind=='tiger' else 'LifeLensSeamlessHuman')
    detail=bpy.data.objects.get('LifeLensIndependentAnatomicalDetails')
    assert rig and body and detail,'Must export both skinned body and anatomy details'
    for obj in (body,detail):
        assert obj.type=='MESH' and obj.parent==rig
        assert any(m.type=='ARMATURE' and m.object==rig for m in obj.modifiers)
    assert rig.animation_data and rig.animation_data.action,'Missing actual rig walk clip'
    scene.frame_set(1)
    before=sampled_world_vertices(body)
    scene.frame_set(7)
    moving=sampled_world_vertices(body)
    displacement=max_delta(before,moving)
    assert displacement>.004, f'Walk clip fails to deform actual body surface: {displacement}'

    # Independent face/paw detail should move with its assigned bone, not
    # float at its initial position when animation/pose changes.
    scene.frame_set(1)
    bind=sampled_world_vertices(detail)
    joint=rig.pose.bones.get('Head' if kind=='human' else 'head')
    assert joint
    joint.rotation_mode='XYZ'
    joint.rotation_euler=(.0,.0,.24)
    bpy.context.view_layer.update()
    rotated=sampled_world_vertices(detail)
    attached=max_delta(bind,rotated)
    assert attached>.004, f'Detail surface is detached from its head bone: {attached}'
    print('PASS Q4 evaluated skin motion:',kind,
          'walk displacement',round(displacement,6),
          'head attachment displacement',round(attached,6))


if __name__=='__main__':
    main()
