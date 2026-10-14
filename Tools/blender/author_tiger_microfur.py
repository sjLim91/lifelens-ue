"""Deterministic tiger pelage microgeometry for animated LifeLens GLBs.

Sub-centimeter guard hairs follow real surface normals and skinned vertices,
not static screen-space "fur" textures. Designed as a conservative prototype
with bounded triangle count for future mobile LOD evaluation.
"""
from __future__ import annotations

import math
import bpy
from mathutils import Vector


def is_tiger_dark(p):
    x,y,z=p.x,p.y,p.z
    torso=(-.9<y<.91 and z>.44 and
           math.sin(17.8*y+1.15*math.sin(z*7.4+abs(x)*2.9)
                    +.4*math.sin(14.1*z+9.3*x+y*4.2))>.75)
    legs=z<.51 and abs(x)>.19 and math.sin(z*26+6*y+abs(x)*3)>.80
    tail=y<-.9 and (y< -2.03 or math.sin(27*y+4*x+2*z)>.46)
    return torso or legs or tail


def create_tiger_fur(surface,rig,bone_specs,attach):
    """One GPU-skinned tiger fur mesh, 3 selectable materials, ~2000 fibers."""
    materials=[]
    for name,rgb in [('Guard hairs ochre',(.66,.32,.13)),
                     ('Tiger stripe guard hairs',(.062,.041,.032)),
                     ('Tiger pale guard hairs',(.77,.72,.62))]:
        mat=bpy.data.materials.new(name)
        mat.use_nodes=True
        shader=mat.node_tree.nodes.get('Principled BSDF')
        shader.inputs['Base Color'].default_value=(*rgb,1.0)
        shader.inputs['Roughness'].default_value=.96
        materials.append(mat)
    verts=[];faces=[];regions=[]
    count=surface.data.vertices.__len__()
    stride=max(1,count//1850)
    for index in range(0,count,stride):
        original=surface.data.vertices[index]
        base=Vector(original.co)
        normal=Vector(original.normal)
        if normal.length_squared<.5:
            continue
        normal.normalize()
        axis=Vector((0,0,1))
        tangent=normal.cross(axis)
        if tangent.length_squared<.1:
            tangent=normal.cross(Vector((0,1,0)))
        tangent.normalize()
        # The soft pelage angles backwards, with deterministic growth
        # variation but no random seed or frame-dependent pseudonoise.
        wiggle=math.sin(index*3.14159)*.12
        radius=.0011*(.8+.4*abs(math.sin(index*.331)))
        length=.0075*(.83+.36*abs(math.sin(index*.113)))
        root=base+normal*.0011
        tip=root+normal*length+Vector((wiggle*.003,-.0024,-.0015))
        start=len(verts)
        verts.extend([tuple(root-tangent*radius),tuple(root+tangent*radius),tuple(tip)])
        faces.append((start,start+1,start+2))
        regions.append(
            1 if is_tiger_dark(base)
            else 2 if base.z<.72 and abs(base.x)<.20 and abs(base.y)<.70
            else 0)
    assert len(faces)>=750,('Pelage cannot be represented',len(faces))
    mesh=bpy.data.meshes.new('TigerSkinnedMicroFurTopology')
    mesh.from_pydata(verts,[],faces)
    mesh.update()
    for mat in materials:
        mesh.materials.append(mat)
    for polygon,category in zip(mesh.polygons,regions):
        polygon.material_index=category
        polygon.use_smooth=False
    obj=bpy.data.objects.new('LifeLensSkinnedTigerGuardHairs',mesh)
    bpy.context.collection.objects.link(obj)
    attach(obj,rig,bone_specs)
    obj['lifelens_fur_kind']='fur geometry bound to 14-bone tiger skeleton'
    obj['lifelens_fur_clumps']=len(faces)
    obj['lifelens_production_ready']=False
    print('LIFELENS_TIGER_FUR_OK',len(faces),'clusters',len(verts),'verts')
    return obj
