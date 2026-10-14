"""Deterministic scalp-grown hair strands for MakeHuman CC0 LifeLens assets.

Hair roots sample the actual anatomical skull mesh and flow back over the
cranium, instead of a single opaque hemisphere or rigid wigs. Free workflow.
"""
from __future__ import annotations
import math
import bpy
from mathutils import Vector


def author_scalp_strands(scalp, rig, gender, material, attach):
    hair=bpy.data.curves.new('LifeLens Skinned Scalp Fibers','CURVE')
    hair.dimensions='3D'
    hair.bevel_depth=.00034 if gender=='female' else .00030
    hair.bevel_resolution=1
    hair.resolution_u=4
    candidates=[v for v in scalp.data.vertices
                if v.co.z>1.688 and abs(v.co.x)<.126 and
                -.135<v.co.y<.145]
    candidates.sort(key=lambda v:(round(v.co.z,5),round(v.co.x,5),round(v.co.y,5)))
    if len(candidates)<55:
        raise AssertionError(('not enough scalp source vertices',len(candidates)))
    stride=max(1,len(candidates)//175)
    count=0
    for idx,root in enumerate(candidates[::stride]):
        p=Vector(root.co)
        normal=Vector(root.normal)
        start=p+normal*.00025
        jitter=.003*math.sin(idx*4.33)
        flow=(.030 if gender=='female' else .022)*(0.84+.20*math.cos(idx*2.71))
        path=[
            start,
            start+Vector((jitter*.3,flow*.35,-.005)),
            start+Vector((jitter*.65,flow*.73,-.017)),
            start+Vector((jitter,flow,-.036)),
        ]
        spline=hair.splines.new('POLY')
        spline.points.add(len(path)-1)
        for i,pos in enumerate(path):
            spline.points[i].co=(*pos,1.0)
            spline.points[i].radius=1.0-(i/(len(path)-1))*.70
        count+=1
    assert count>=60,count
    obj=bpy.data.objects.new('CC0 individually rooted hair strands',hair)
    bpy.context.collection.objects.link(obj)
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.convert(target='MESH')
    obj=bpy.context.object
    obj.data.materials.append(material)
    obj['lifelens_hair_source']='CC0 anatomical scalp'
    obj['lifelens_hair_strand_count']=count
    attach(obj,rig)
    print('CC0_SCALP_STRANDS_OK',gender,count,len(obj.data.vertices))
    return obj
