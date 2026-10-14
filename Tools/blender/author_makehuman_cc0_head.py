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


def eyelid_sheets(eye_x, rig, gender):
    """Upper/lower lid tissue occludes the globe for a natural eye aperture.

    Doll-like exposed eyes are avoided by two bone-skinned anatomical sheets,
    tapered toward the inner and outer canthi.
    """
    tone=(.69,.46,.345) if gender=='female' else (.65,.415,.31)
    skin=simple_material('CC0 eyelid warm skin '+gender,tone,.77)
    wet=simple_material('CC0 eyelid margin '+gender,
                        (.42,.27,.225),.65)
    parts=[]
    count=16
    for upper in (True,False):
        vertices=[];faces=[]
        margin_points=[]
        for i in range(count+1):
            t=-1+2*i/count
            x=eye_x+t*.0205
            closing=max(0.0,1-t*t)**.72
            aperture=(.0052 if upper else -.0039)*closing
            z_inner=1.625+aperture+(.0015 if upper else -.0002)
            z_outer=z_inner+(.0065 if upper else -.0055)
            y_inner=-.052+.008*(abs(t)**1.5)
            y_outer=-.042+.002*(abs(t)**1.5)
            vertices.extend([(x,y_inner,z_inner),(x,y_outer,z_outer)])
            margin_points.append((x,y_inner-.0002,z_inner))
            if i:
                a=(i-1)*2
                faces.append((a,a+1,a+3,a+2) if upper else (a+2,a+3,a+1,a))
        mesh=bpy.data.meshes.new('CC0 '+gender+('Upper' if upper else 'Lower')+'LidTopology')
        mesh.from_pydata(vertices,[],faces)
        mesh.update()
        mesh.materials.append(skin)
        obj=bpy.data.objects.new(('upper' if upper else 'lower')+' anatomical eyelid '+str(eye_x),mesh)
        bpy.context.collection.objects.link(obj)
        for face in mesh.polygons:
            face.use_smooth=True
        parts.append(attach(obj,rig))
        curve=bpy.data.curves.new('Eyelid soft rim','CURVE')
        curve.dimensions='3D'
        curve.bevel_depth=.00048
        curve.resolution_u=16
        curve.bevel_resolution=1
        spline=curve.splines.new('POLY')
        spline.points.add(len(margin_points)-1)
        for j,p in enumerate(margin_points):
            spline.points[j].co=(*p,1.0)
        rim=bpy.data.objects.new('CC0 eyelid wet line',curve)
        bpy.context.collection.objects.link(rim)
        bpy.ops.object.select_all(action='DESELECT')
        rim.select_set(True)
        bpy.context.view_layer.objects.active=rim
        bpy.ops.object.convert(target='MESH')
        rim=bpy.context.object
        rim.data.materials.append(wet)
        parts.append(attach(rim,rig))
    assert len(parts)==4
    return parts


def decorate(body,rig,gender,source):
    # Replace circular hand-authored eyes with official anatomically positioned
    # and UV-textured high-poly eyeballs from the same CC0 MakeHuman topology.
    from author_makehuman_eyes import import_cc0_eyes
    hair=simple_material('CC0 natural hair '+gender,
                         (.095,.058,.033) if gender=='female' else (.044,.035,.029),.83)
    detail=[import_cc0_eyes(body,rig,source,attach)]
    for center in (-.033,.033):
        detail.extend(eyelid_sheets(center,rig,gender))
    detail.append(anatomical_scalp(body,rig,gender,hair))
    if gender=='female':
        # Four narrow back-hair sections. Broad rectangular curtains looked
        # artificial and obscured the natural neck/ear silhouette.
        for n in range(4):
            x=(n-1.5)*.029
            detail.append(hair_ribbon('Anatomical back hair '+str(n),[
                (x*.87,.090,1.728),
                (x,.115,1.66),
                (x*1.05,.124,1.57),
                (x*1.1,.130,1.53),
                (x*1.08,.131,1.515)],.0045,hair,rig))
    assert len(detail)>=2
    print('MAKEHUMAN_CC0_HEAD_DETAIL_OK',gender,'skinned pieces',len(detail))
    return detail
