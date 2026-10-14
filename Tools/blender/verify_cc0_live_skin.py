"""Verify genuine CC0 anatomy surface deformation and facial attachment."""
from __future__ import annotations
import bpy
from mathutils import Vector


def sample(obj):
    graph=bpy.context.evaluated_depsgraph_get()
    eva=obj.evaluated_get(graph)
    mesh=eva.to_mesh()
    try:
        step=max(1,len(mesh.vertices)//280)
        return [eva.matrix_world @ mesh.vertices[i].co.copy()
                for i in range(0,len(mesh.vertices),step)]
    finally:
        eva.to_mesh_clear()


def delta(before,after):
    assert len(before)==len(after)
    return max((a-b).length for a,b in zip(before,after))


def main():
    name=bpy.data.filepath
    sex='female' if 'female' in name else 'male'
    body=bpy.data.objects.get('LifeLensMakeHumanCC0_'+sex)
    rig=bpy.data.objects.get('LifeLensCC0TrialRig')
    scalp=bpy.data.objects.get('LifeLensHairScalp_'+sex)
    assert body and rig and scalp
    eye=next((o for o in bpy.data.objects if o.name.startswith('CC0 inset eye sclera')),None)
    assert eye is not None
    for obj in (body,scalp,eye):
        assert obj.parent==rig
        assert any(m.type=='ARMATURE' and m.object==rig for m in obj.modifiers)
    bpy.context.scene.frame_set(1)
    rest=sample(body)
    bpy.context.scene.frame_set(7)
    moved=sample(body)
    d=delta(rest,moved)
    assert d>.006,('CC0 body does not move with LifeLensWalkCycle',d)
    bpy.context.scene.frame_set(1)
    hair_rest=sample(scalp)
    eye_rest=sample(eye)
    head=rig.pose.bones.get('Head')
    assert head
    head.rotation_mode='XYZ'
    head.rotation_euler.y=.23
    bpy.context.view_layer.update()
    hair_move=delta(hair_rest,sample(scalp))
    eye_move=delta(eye_rest,sample(eye))
    assert hair_move>.004 and eye_move>.004,(hair_move,eye_move)
    print('CC0_LIVE_SKINNING_PASS',sex,'body',round(d,4),
          'scalp',round(hair_move,4),'eyeball',round(eye_move,4))


if __name__=='__main__':
    main()
