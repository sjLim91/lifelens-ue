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
        'sclera':material('Q4 warm sclera',(.83,.79,.71),.27),
        'iris':material('Q4 amber iris',(.37,.25,.10),.22),
        'pupil':material('Q4 deep pupil',(.012,.011,.009),.13),
        'glint':material('Q4 highlight',(.94,.94,.87),.1),
        'lip':material('Q4 soft lips',(.48,.24,.22),.61),
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
        d.ellipsoid(prefix+' orbital sclera',(eyeX,-.133,1.666),
                    (.025,.010,.013),'Head','sclera')
        d.ellipsoid(prefix+' iris',(eyeX,-.145,1.666),
                    (.010,.004,.010),'Head','iris')
        d.ellipsoid(prefix+' pupil',(eyeX,-.149,1.666),
                    (.005,.002,.007),'Head','pupil')
        d.ellipsoid(prefix+' corneal highlight',(eyeX-s*.003,-.152,1.670),
                    (.002,.0015,.002),'Head','glint',10,6)
        # Eyelid contours and natural eyebrows, not huge exposed spheres.
        d.fiber(prefix+' upper eyelid',[
            (eyeX-s*.031,-.143,1.667),(eyeX-s*.018,-.147,1.681),
            (eyeX,-.149,1.683),(eyeX+s*.020,-.143,1.676),
            (eyeX+s*.030,-.136,1.667)],.002,'Head','lip')
        d.fiber(prefix+' eyebrow',[
            (eyeX-s*.032,-.129,1.711),(eyeX-s*.014,-.141,1.724),
            (eyeX+s*.015,-.139,1.719),(eyeX+s*.028,-.128,1.711)],
            .0036 if not female else .003,'Head','hair_female' if female else 'hair')
        # Ear helix plus darker concha follows the head bone.
        d.ellipsoid(prefix+' inner ear',(s*.117,-.032,1.650),
                    (.004,.014,.019),'Head','ear',12,8)

    # Separate philtrum / nostrils / Cupid bow and mandibular lip topology.
    for s in (-1,1):
        d.ellipsoid('nostril',(s*.017,-.176,1.601),
                    (.006,.002,.003),'Head','ear',12,6)
    d.fiber('cupid bow',[
        (-.040,-.128,1.566),(-.018,-.138,1.575),(0,-.140,1.570),
        (.018,-.138,1.575),(.040,-.128,1.566)],.0038,'Head','lip')
    d.fiber('lower vermillion',[
        (-.034,-.127,1.558),(-.015,-.136,1.549),
        (0,-.138,1.546),(.015,-.136,1.549),(.034,-.127,1.558)],.0035,'Head','lip')

    # Crown/temple hair has distinct head-bound topology and anatomy-driven
    # sex/identity silhouette; never hard-code scalp colour as the face.
    crown=d.ellipsoid('sculpted crown',(0,.015,1.782),
                     (.116,.105,.064) if female else (.112,.099,.049),
                     'Head','hair_female' if female else 'hair',24,14)
    crown['lifelens_hair_style']='shoulder-length' if female else 'short'
    for s in (-1,1):
        if female:
            d.ellipsoid('temple hair',(s*.112,.014,1.668),
                        (.028,.064,.105),'Head','hair_female')
            d.fiber('layered hair strands',[
                (s*.118,.008,1.77),(s*.13,.007,1.68),
                (s*.13,.016,1.57),(s*.122,.033,1.49)],
                .014,'Head','hair_female')
            d.fiber('front swept fringe',[
                (s*.025,-.085,1.815),(s*.06,-.115,1.77),
                (s*.084,-.130,1.73)],.010,'Head','hair_female')
        else:
            d.ellipsoid('short side hair',(s*.104,.01,1.733),
                        (.018,.070,.039),'Head','hair')
            d.fiber('short hairline',[
                (s*.11,-.05,1.76),(s*.075,-.105,1.782),
                (s*.027,-.110,1.794)],.010,'Head','hair')

    # The Q3 remesh erased nearly all nail/finger separation. Q4 overlays
    # independent keratin plates and creases; use real hand/foot deformation.
    for s,side in ((-1,'r'),(1,'l')):
        for digit in range(4):
            x=(.37+digit*.025)*s*(.90 if female else 1.06)
            d.ellipsoid('fingernail',(x,-.050,.846),
                        (.012,.004,.015),'hand_'+side,'nail',12,8)
        for digit in range(4):
            x=(.080+digit*.035)*s*(1.11 if female else .96)
            d.ellipsoid('toenail',(x,-.284,.053),
                        (.017,.006,.013),'foot_'+side,'nail',12,8)


def tiger_details(d):
    # Tiger forward direction = +Y. Face/paws have their own surfaces and
    # separate materials, not flattened into Q3's smooth orange bulk.
    for s in (-1,1):
        tag='right' if s<0 else 'left'
        d.ellipsoid(tag+' white eye surround',(s*.137,1.305,1.225),
                    (.064,.017,.044),'head','tiger_white')
        d.ellipsoid(tag+' gold eye',(s*.135,1.322,1.232),
                    (.030,.009,.022),'head','tiger_eye')
        d.ellipsoid(tag+' vertical pupil',(s*.137,1.333,1.232),
                    (.009,.005,.021),'head','tiger_black')
        d.ellipsoid(tag+' eye reflection',(s*.128,1.341,1.244),
                    (.004,.002,.005),'head','glint',12,8)
        d.fiber(tag+' black eyeliner',[
            (s*.077,1.331,1.260),(s*.122,1.345,1.286),
            (s*.17,1.32,1.282),(s*.204,1.283,1.256)],
            .009,'head','tiger_black')
        d.ellipsoid(tag+' inner triangular ear',(s*.169,1.190,1.402),
                    (.050,.017,.054),'head','tiger_black')
        d.ellipsoid(tag+' muzzle cheek',(s*.103,1.495,1.022),
                    (.085,.042,.040),'head','tiger_white')
        for j in range(4):
            # Facial tiger pattern = anatomical cheek/forehead markings.
            d.fiber(tag+' cheek stripe '+str(j),[
                (s*(.20+j*.006),1.16+j*.04,1.195-j*.018),
                (s*(.241+j*.010),1.23+j*.035,1.154-j*.023),
                (s*(.242+j*.013),1.32+j*.027,1.12-j*.023)],
                .009 if j<2 else .007,'head','tiger_black')
        for k in range(7):
            z=1.002+(k%3)*.024
            x=s*(.08+(k//3)*.028)
            y=1.548-(k//3)*.021
            d.ellipsoid('whisker follicle',(x,y,z),(.005,.008,.005),
                        'head','tiger_black',10,6)
        for k in range(5):
            start=(s*(.13+k*.01),1.50,1.00+k*.014)
            end=(s*(.32+k*.020),1.58,1.07+k*.013)
            mid=((start[0]+end[0])*.5,1.56,1.045+k*.014)
            d.fiber('tiger whisker '+tag,[start,mid,end],.0012,'head','whisker')

    # Detailed nasal leather and philtrum / lower jaw line.
    d.ellipsoid('triangular tiger nose',(0,1.580,1.062),
                (.057,.020,.027),'head','tiger_nose',18,12)
    d.fiber('tiger nasal seam',[(0,1.596,1.04),(0,1.58,1.002),
                               (0,1.557,.978)],.006,'head','tiger_black')
    for s in (-1,1):
        d.fiber('upper jowl seam',[(0,1.564,.977),(s*.046,1.564,.968),
                                  (s*.099,1.529,.971)],
                .006,'head','tiger_black')

    # Actual paw claws retain their own hard surface, rotate/deform with
    # front and hind lower-leg joints independently.
    for s,side in ((-1,'r'),(1,'l')):
        for front,y in ((True,.46),(False,-.57)):
            bone=('front_' if front else 'back_')+side+'_lower'
            for digit in (-1,0,1):
                x=s*.29+digit*.075
                d.tapered('curved keratin claw',
                          (x,y+.355,.085),(x,y+.424,.047),.020,
                          bone,'claw',vertices=12)
                d.fiber('paw digit crease',[
                    (x-.028,y+.257,.157),(x,y+.292,.161),
                    (x+.028,y+.260,.157)],.004,bone,'tiger_black')


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
