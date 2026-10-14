"""Independent anatomical facial / paw detail surfaces for LifeLens authored prototypes.

This module deliberately DOES NOT voxel-remesh faces, eyes, toes, lashes,
whiskers or claws into the body. Each surface keeps its topology and its
named skin group. All materials/models are generated locally (no downloads).
"""
from __future__ import annotations

import math
import bpy
from mathutils import Vector


def material(name, rgb, roughness=.78, metallic=0.0):
    # Reuse Blender IDs to keep output deterministic when repeated.
    m=bpy.data.materials.get(name)
    if m is not None:
        return m
    m=bpy.data.materials.new(name)
    m.use_nodes=True
    m.diffuse_color=(*rgb,1.0)
    p=m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value=(*rgb,1.0)
    p.inputs['Roughness'].default_value=roughness
    p.inputs['Metallic'].default_value=metallic
    return m


def build_palette():
    return {
        'sclera':material('Q5 subdued eye white',(.64,.61,.56),.40),
        'iris':material('Q4 amber iris',(.37,.25,.10),.22),
        'pupil':material('Q4 deep pupil',(.012,.011,.009),.13),
        'glint':material('Q4 highlight',(.94,.94,.87),.1),
        'lip':material('Q5 natural lip',(.64,.38,.32),.76),
        'skin':material('Q4 warm skin',(.71,.46,.33),.76),
        'nail':material('Q4 nail keratin',(.82,.64,.52),.42),
        'hair':material('Q4 warm dark hair',(.070,.044,.025),.88),
        'hair_female':material('Q4 auburn hair',(.14,.066,.034),.81),
        'ear':material('Q4 inner ear',(.57,.26,.21),.78),
        'tiger_nose':material('Q4 tiger nose',(.085,.052,.056),.43),
        'tiger_white':material('Q4 tiger cream',(.90,.79,.56),.91),
        'tiger_black':material('Q4 tiger stripes',(.055,.035,.025),.96),
        'tiger_eye':material('Q4 tiger gold eye',(.53,.37,.07),.24),
        'whisker':material('Q4 vibrissae',(.77,.73,.66),.7),
        'claw':material('Q4 ivory claw',(.72,.64,.52),.51),
    }


class DetailBuilder:
    def __init__(self, rig):
        self.rig=rig
        self.items=[]
        self.palette=build_palette()
        self.parts=set(rig.data.bones.keys())

    def attach(self,obj,bone):
        if bone not in self.parts:
            raise ValueError(f'no matching deform joint for {obj.name}: {bone}')
        # Geometry lies in world-rest/model space. The armature origin is
        # identity, so every vertex is directly compatible with boneInverses.
        bpy.context.view_layer.objects.active=obj
        bpy.ops.object.select_all(action='DESELECT')
        obj.select_set(True)
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        vg=obj.vertex_groups.new(name=bone)
        vg.add(list(range(len(obj.data.vertices))),1.0,'REPLACE')
        mod=obj.modifiers.new('Skin anatomical detail','ARMATURE')
        mod.object=self.rig
        obj.parent=self.rig
        obj['lifelens_bind_bone']=bone
        self.items.append(obj)
        return obj

    def ellipsoid(self,name,position,dimensions,bone,paint,segments=16,rings=10):
        bpy.ops.mesh.primitive_uv_sphere_add(segments=segments,ring_count=rings,
                                             location=position)
        ob=bpy.context.object
        ob.name=name
        ob.scale=dimensions
        ob.data.materials.append(self.palette[paint])
        for face in ob.data.polygons:
            face.use_smooth=True
        return self.attach(ob,bone)

    def fiber(self,name,points,radius,bone,paint):
        curve=bpy.data.curves.new(name,'CURVE')
        curve.dimensions='3D'
        curve.resolution_u=8
        curve.bevel_depth=radius
        curve.bevel_resolution=2
        curve.resolution_u=12
        points=list(points)
        spline=curve.splines.new('POLY')
        spline.points.add(len(points)-1)
        for i,xyz in enumerate(points):
            spline.points[i].co=(*xyz,1)
        ob=bpy.data.objects.new(name,curve)
        bpy.context.collection.objects.link(ob)
        bpy.context.view_layer.objects.active=ob
        bpy.ops.object.select_all(action='DESELECT')
        ob.select_set(True)
        bpy.ops.object.convert(target='MESH')
        ob=bpy.context.object
        ob.data.materials.append(self.palette[paint])
        return self.attach(ob,bone)

    def tapered(self,name,a,b,radius,bone,paint,vertices=10):
        mid=(Vector(a)+Vector(b))*.5
        direction=Vector(b)-Vector(a)
        bpy.ops.mesh.primitive_cone_add(vertices=vertices,
                                       radius1=radius,radius2=.001,
                                       depth=direction.length,
                                       location=mid)
        ob=bpy.context.object
        ob.name=name
        ob.rotation_mode='QUATERNION'
        ob.rotation_quaternion=Vector((0,0,1)).rotation_difference(direction)
        ob.data.materials.append(self.palette[paint])
        return self.attach(ob,bone)

    def compact(self):
        # All additional detail objects are converted once into a single
        # skinned geometry. Keep independent faces/material slots (no remesh).
        # Blender join retains per-vertex named bone groups and object materials.
        if not self.items:
            raise AssertionError('No anatomical detail geometry created')
        bpy.ops.object.select_all(action='DESELECT')
        for obj in self.items:
            obj.select_set(True)
        bpy.context.view_layer.objects.active=self.items[0]
        bpy.ops.object.join()
        obj=bpy.context.object
        obj.name='LifeLensIndependentAnatomicalDetails'
        obj.data.name='LifeLensIndependentAnatomicalDetailsMesh'
        obj['lifelens_q4_topology']='separate skinned facial/paw surfaces; never voxel welded'
        assert obj.parent==self.rig
        assert any(m.type=='ARMATURE' and m.object==self.rig for m in obj.modifiers)
        assert len(obj.vertex_groups)>3
        assert len(obj.data.materials)>=6
        return obj


def human_details(d,female):
    # Forward on humans = -Y. The head donor bone remains Head; the
    # anatomical features are fused by skinned transformation, not by remesh.
    for s in (-1,1):
        prefix='left' if s>0 else 'right'
        eyeX=s*.053
        d.ellipsoid(prefix+' orbital sclera',(eyeX,-.130,1.664),
                    (.018,.008,.010),'Head','sclera')
        d.ellipsoid(prefix+' iris',(eyeX,-.138,1.664),
                    (.007,.003,.007),'Head','iris')
        d.ellipsoid(prefix+' pupil',(eyeX,-.142,1.664),
                    (.0035,.0018,.005),'Head','pupil')
        d.ellipsoid(prefix+' corneal highlight',(eyeX-s*.003,-.146,1.668),
                    (.002,.0015,.002),'Head','glint',10,6)
        # Eyelid contours and natural eyebrows, not huge exposed spheres.
        d.fiber(prefix+' upper eyelid',[
            (eyeX-s*.024,-.139,1.664),(eyeX-s*.014,-.142,1.673),
            (eyeX,-.144,1.676),(eyeX+s*.014,-.139,1.670),
            (eyeX+s*.023,-.135,1.665)],.0014,'Head','lip')
        d.fiber(prefix+' eyebrow',[
            (eyeX-s*.032,-.129,1.711),(eyeX-s*.014,-.141,1.724),
            (eyeX+s*.015,-.139,1.719),(eyeX+s*.028,-.128,1.711)],
            .0025 if not female else .0021,'Head','hair_female' if female else 'hair')
        # Ear helix plus darker concha follows the head bone.
        d.ellipsoid(prefix+' inner ear',(s*.117,-.032,1.650),
                    (.004,.014,.019),'Head','ear',12,8)

    # Separate philtrum / nostrils / Cupid bow and mandibular lip topology.
    for s in (-1,1):
        d.ellipsoid('nostril',(s*.017,-.176,1.601),
                    (.004,.0015,.002),'Head','ear',12,6)
    d.fiber('cupid bow',[
        (-.040,-.128,1.566),(-.018,-.138,1.575),(0,-.140,1.570),
        (.018,-.138,1.575),(.040,-.128,1.566)],.0024,'Head','lip')
    d.fiber('lower vermillion',[
        (-.034,-.127,1.558),(-.015,-.136,1.549),
        (0,-.138,1.546),(.015,-.136,1.549),(.034,-.127,1.558)],.0024,'Head','lip')

    # Crown/temple hair has distinct head-bound topology and anatomy-driven
    # sex/identity silhouette; never hard-code scalp colour as the face.
    crown=d.ellipsoid('sculpted crown',(0,.015,1.782),
                     (.114,.101,.047) if female else (.112,.100,.038),
                     'Head','hair_female' if female else 'hair',24,14)
    crown['lifelens_hair_style']='shoulder-length' if female else 'short'
    for s in (-1,1):
        if female:
            d.ellipsoid('temple hair',(s*.112,.014,1.668),
                        (.021,.047,.089),'Head','hair_female')
            d.fiber('layered hair strands',[
                (s*.118,.008,1.77),(s*.13,.007,1.68),
                (s*.13,.016,1.57),(s*.122,.033,1.49)],
                .0065,'Head','hair_female')
            d.fiber('front swept fringe',[
                (s*.025,-.085,1.815),(s*.06,-.115,1.77),
                (s*.084,-.130,1.73)],.0038,'Head','hair_female')
        else:
            d.ellipsoid('short side hair',(s*.104,.01,1.733),
                        (.018,.070,.039),'Head','hair')
            d.fiber('short hairline',[
                (s*.11,-.05,1.76),(s*.075,-.105,1.782),
                (s*.027,-.110,1.794)],.0050,'Head','hair')

    # The Q3 remesh erased nearly all nail/finger separation. Q4 overlays
    # independent keratin plates and creases; use real hand/foot deformation.
    for s,side in ((-1,'r'),(1,'l')):
        for digit in range(4):
            x=(.37+digit*.025)*s*(.84 if female else 1.05)
            d.ellipsoid('fingernail',(x,-.050,.846),
                        (.012,.004,.015),'hand_'+side,'nail',12,8)
        for digit in range(4):
            x=(.080+digit*.035)*s*(1.16 if female else .96)
            d.ellipsoid('toenail',(x,-.284,.053),
                        (.017,.006,.013),'foot_'+side,'nail',12,8)


def tiger_details(d):
    # A realistic tiger's eye is an inset, oblique almond, not a round
    # white cartoon eyeball. Muzzle fur and cheek stripes belong to the
    # skin pigment shader; rigid contrasting white balls are removed.
    for s in (-1,1):
        tag='right' if s<0 else 'left'
        d.ellipsoid(tag+' recessed cream orbital skin',(s*.139,1.295,1.214),
                    (.044,.010,.022),'head','tiger_white')
        d.ellipsoid(tag+' deep amber iris',(s*.139,1.304,1.215),
                    (.022,.006,.014),'head','tiger_eye')
        d.ellipsoid(tag+' vertical pupil',(s*.139,1.311,1.215),
                    (.006,.0025,.012),'head','tiger_black')
        d.ellipsoid(tag+' tiny ocular specular',(s*.132,1.314,1.220),
                    (.0025,.0015,.0025),'head','glint',12,8)
        d.fiber(tag+' subtle upper lid',[
            (s*.093,1.306,1.225),(s*.129,1.309,1.232),
            (s*.164,1.302,1.228),(s*.181,1.291,1.215)],
            .0027,'head','tiger_black')
        # Inner ear recedes behind the orange pinna; no spherical black nubs.
        d.ellipsoid(tag+' inner ear',(s*.166,1.145,1.359),
                    (.027,.009,.026),'head','tiger_black',12,8)
        # Small whisker pads are defined by coat pigmentation rather than
        # large separate, conspicuously white snout spheres.
        for k in range(4):
            x=s*(.072+(k//2)*.032)
            z=1.019+(k%2)*.020
            y=1.548-(k//2)*.028
            d.ellipsoid('whisker pore',(x,y,z),(.0035,.0035,.0030),
                        'head','tiger_black',10,6)
        for k in range(3):
            start=(s*(.11+k*.011),1.536,1.014+k*.011)
            end=(s*(.26+k*.015),1.569,1.042+k*.019)
            mid=((start[0]+end[0])*.5,1.563,1.039+k*.016)
            d.fiber('fine facial vibrissa '+tag,[start,mid,end],.00075,'head','whisker')

    d.ellipsoid('soft triangular black nose',(0,1.567,1.059),
                (.047,.013,.023),'head','tiger_nose',16,10)
    d.fiber('philtrum',[(0,1.570,1.043),(0,1.555,1.018),
                        (0,1.542,.999)],.0025,'head','tiger_black')
    for s in (-1,1):
        d.fiber('curved muzzle crease',[(0,1.552,1.000),
                                      (s*.038,1.545,.995),
                                      (s*.084,1.524,.998)],
                .0026,'head','tiger_black')

    # Retractable cat claws project only a few centimeters from the paw,
    # rather than fantasy monster talons.
    for s,side in ((-1,'r'),(1,'l')):
        for front,y in ((True,.46),(False,-.57)):
            bone=('front_' if front else 'back_')+side+'_lower'
            for digit in (-1,0,1):
                x=s*.29+digit*.065
                d.tapered('small retractable claw',
                          (x,y+.340,.086),(x,y+.391,.060),.009,
                          bone,'claw',vertices=9)
                d.fiber('paw crease',[
                    (x-.022,y+.265,.129),(x,y+.287,.135),
                    (x+.022,y+.265,.129)],.002,bone,'tiger_black')



def create_creature_details(which,rig):
    """New skinned mesh(s), absent from existing Q3 coarse voxel mesh."""
    d=DetailBuilder(rig)
    if which=='resident-female':
        human_details(d,True)
    elif which=='resident-male':
        human_details(d,False)
    elif which=='tiger':
        tiger_details(d)
    else:
        raise ValueError(which)
    detailed=d.compact()
    print('LIFELENS_Q4_ANATOMICAL_DETAIL',which,
          len(d.items),'individual authored surfaces',
          len(detailed.data.vertices),'detail vertices',
          len(detailed.vertex_groups),'deform bone groups')
    return detailed
