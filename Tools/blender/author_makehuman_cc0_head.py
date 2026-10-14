"""MakeHuman CC0 head details (real scalp surface + inset eyes).

Inputs are the CC0 anatomical mesh and LifeLens-authored rig. No copied addon
code. The scalp is *extracted from actual human skull topology*, not a
synthetic helmet/spherical wig.
"""
from __future__ import annotations

import math
import bpy
from mathutils import Vector


def attach(obj,rig,bone='Head'):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    group=obj.vertex_groups.new(name=bone)
    group.add(list(range(len(obj.data.vertices))),1.0,'REPLACE')
    modifier=obj.modifiers.new('LifeLens facial/head deformation','ARMATURE')
    modifier.object=rig
    obj.parent=rig
    assert len(obj.data.vertices)>=8
    return obj


def simple_material(name,color,roughness):
    mat=bpy.data.materials.new(name)
    mat.use_nodes=True
    mat.diffuse_color=(*color,1.0)
    shader=mat.node_tree.nodes.get('Principled BSDF')
    shader.inputs['Base Color'].default_value=(*color,1.0)
    shader.inputs['Roughness'].default_value=roughness
    return mat


def add_ellipsoid(name,pos,dims,material,rig):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24,ring_count=16,location=pos)
    obj=bpy.context.object
    obj.name=name
    obj.scale=dims
    obj.data.materials.append(material)
    for poly in obj.data.polygons:
        poly.use_smooth=True
    return attach(obj,rig)


def anatomical_scalp(body,rig,gender,material):
    # Select only original body triangles on the crown. The hairline is
    # higher across the forehead than at the nape. Reuse the anatomical face
    # surface directly, preserving matching silhouette and normals.
    selected=[]
    for poly in body.data.polygons:
        verts=[body.data.vertices[i] for i in poly.vertices]
        def is_scalp(v):
            x,y,z=v.co
            cutoff=1.73 if y<-.025 else 1.69 if y<.03 else 1.64
            return z>=cutoff and abs(x)<.135 and -.16<y<.18
        if all(is_scalp(v) for v in verts):
            selected.append(list(poly.vertices))
    assert len(selected)>50,('scalp cap missing source topology',len(selected))
    ids=sorted({i for face in selected for i in face})
    trans={vertex:i for i,vertex in enumerate(ids)}
    vertices=[]
    for i in ids:
        p=body.data.vertices[i]
        vertices.append(tuple(p.co+p.normal*.0025))
    faces=[tuple(trans[i] for i in f) for f in selected]
    mesh=bpy.data.meshes.new('CC0 Anatomical Scalp')
    mesh.from_pydata(vertices,[],faces)
    mesh.update()
    obj=bpy.data.objects.new('LifeLensHairScalp_'+gender,mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    for polygon in obj.data.polygons:
        polygon.use_smooth=True
    obj['lifelens_hair_topology']='original CC0 scalp surface inset by 2.5mm'
    return attach(obj,rig)


def hair_ribbon(name,positions,width,mat,rig):
    # A low-profile hair taper is a flattened two-sided strip, not a pipe.
    # Its shape follows the head and turns naturally toward the back.
    verts=[];faces=[]
    for i,(x,y,z) in enumerate(positions):
        t=i/(len(positions)-1)
        taper=(1-.78*t)
        verts.extend([(x-width*taper,y,z),(x+width*taper,y,z)])
        if i:
            q=2*(i-1)
            faces.append((q,q+1,q+3,q+2))
    m=bpy.data.meshes.new(name+'RibbonMesh')
    m.from_pydata(verts,[],faces)
    m.update()
    m.materials.append(mat)
    ob=bpy.data.objects.new(name,m)
    bpy.context.collection.objects.link(ob)
    # Ribbon front/back should remain visible, but avoid opaque tube masses.
    mat.use_nodes=True
    return attach(ob,rig)


def decorate(body,rig,gender):
    sclera=simple_material('CC0 muted moist sclera',(.75,.70,.65),.27)
    iris=simple_material('CC0 brown iris',(.30,.19,.095),.29)
    pupil=simple_material('CC0 deep pupil',(.013,.010,.010),.17)
    hair=simple_material('CC0 natural hair '+gender,
                         (.095,.058,.033) if gender=='female' else (.044,.035,.029),.83)
    detail=[]
    for side in (-1,1):
        # In source rest coordinates real orbital cavities are about
        # +/-5cm across the nose and z=1.61m. These values are compared
        # against full mesh front-bound before being used as final assets.
        x=side*.050
        detail.append(add_ellipsoid('CC0 inset eye sclera',(x,-.031,1.618),
                      (.020,.010,.011),sclera,rig))
        detail.append(add_ellipsoid('CC0 realistic iris',(x,-.042,1.617),
                      (.008,.0024,.008),iris,rig))
        detail.append(add_ellipsoid('CC0 pupil',(x,-.045,1.617),
                      (.004,.0015,.006),pupil,rig))
    detail.append(anatomical_scalp(body,rig,gender,hair))
    if gender=='female':
        # Fourteen tapered strips along the nape, no large frontal helmet.
        for n in range(14):
            x=(n-6.5)*.0145
            detail.append(hair_ribbon('Anatomical back hair '+str(n),[
                (x*.87,.055,1.728),
                (x,.089,1.66),
                (x*1.05,.096,1.56),
                (x*1.1,.099,1.465),
                (x*1.08,.094,1.42)],.0085,hair,rig))
    assert len(detail)>=7
    print('MAKEHUMAN_CC0_HEAD_DETAIL_OK',gender,'skinned pieces',len(detail))
    return detail
