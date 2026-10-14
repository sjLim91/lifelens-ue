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
    eye=next((o for o in bpy.data.objects if o.name.startswith('CC0 anatomical textured eyes')),None)
    assert eye is not None
    lids=[obj for obj in bpy.data.objects if 'anatomical eyelid' in obj.name]
    assert len(lids)==4,('two upper/two lower eyelids required',len(lids))
    for obj in (body,scalp,eye,*lids):
        assert obj.parent==rig
        assert any(m.type=='ARMATURE' and m.object==rig for m in obj.modifiers)
    for eyelid in lids:
        xs=[p.co.x for p in eyelid.data.vertices]
        zs=[p.co.z for p in eyelid.data.vertices]
        assert max(xs)-min(xs)>.035,('eyelid too narrow',eyelid.name)
        assert 1.598<min(zs)<1.635 and 1.621<max(zs)<1.660,('eyelid out of orbit',eyelid.name)
    bpy.context.scene.frame_set(1)
    rest=sample(body)
    bpy.context.scene.frame_set(7)
    moved=sample(body)
    d=delta(rest,moved)
    assert d>.006,('CC0 body does not move with LifeLensWalkCycle',d)
    bpy.context.scene.frame_set(1)
    hair_rest=sample(scalp)
    eye_rest=sample(eye)
    lid_rest=[sample(o) for o in lids]
    head=rig.pose.bones.get('Head')
    assert head
    head.rotation_mode='XYZ'
    head.rotation_euler.y=.23
    bpy.context.view_layer.update()
    hair_move=delta(hair_rest,sample(scalp))
    eye_move=delta(eye_rest,sample(eye))
    lid_motions=[delta(rest,sample(o)) for rest,o in zip(lid_rest,lids)]
    assert hair_move>.004 and eye_move>.004 and min(lid_motions)>.004,(hair_move,eye_move,lid_motions)
    print('CC0_LIVE_SKINNING_PASS',sex,'body',round(d,4),
          'scalp',round(hair_move,4),'eyeball',round(eye_move,4),
          'eyelids',len(lids),'minHeadMotion',round(min(lid_motions),4))


if __name__=='__main__':
    main()
