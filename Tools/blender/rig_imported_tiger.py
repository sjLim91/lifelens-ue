"""Rig the extracted source surface and bake in-place quadruped clips for review.

No body primitives or voxel remesh. Source corner UVs and materials survive.
All output remains a candidate until skin deformation and Observer review pass.
"""
import hashlib
import json
import math
from pathlib import Path
import sys
import struct

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Quaternion, Vector

sys.path.insert(0,str(Path(__file__).resolve().parent))
from extract_tiger_body import FOOT_ANCHORS


def embed_contract(path,contract):
    binary=path.read_bytes()
    size,kind=struct.unpack_from('<II',binary,12)
    data=json.loads(binary[20:20+size])
    data['asset']['copyright']='Tiger by TopoForge, generated with Sloyd, CC BY 4.0; LifeLens rig/animation/LOD adaptations.'
    data['asset'].setdefault('extras',{})['lifelensTiger']=contract
    encoded=json.dumps(data,separators=(',',':')).encode()
    encoded+=b' '*((-len(encoded))%4)
    remainder=binary[20+size:]
    result=struct.pack('<4sII',b'glTF',2,20+len(encoded)+len(remainder))
    result+=struct.pack('<II',len(encoded),0x4e4f534a)+encoded+remainder
    path.write_bytes(result)


def lod_materials(body,source_materials,limit):
    body.data.materials.clear()
    for source in source_materials:
        material=source.copy()
        material.name=source.name+'_LifeLens_'+str(limit)
        if material.use_nodes:
            for node in material.node_tree.nodes:
                if node.type!='TEX_IMAGE' or not node.image:continue
                original=node.image
                width,height=original.size
                pixels=np.empty(width*height*4,dtype=np.float32)
                original.pixels.foreach_get(pixels)
                image=bpy.data.images.new(original.name+'_LOD'+str(limit),width=width,height=height,alpha=True)
                image.colorspace_settings.name=original.colorspace_settings.name
                image.pixels.foreach_set(pixels)
                factor=min(1,limit/max(width,height))
                if factor<1:image.scale(max(1,round(width*factor)),max(1,round(height*factor)))
                image.pack()
                node.image=image
        body.data.materials.append(material)


def main():
    out=Path(sys.argv[sys.argv.index('--')+1]).resolve()
    bpy.context.preferences.filepaths.save_version=0
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    bpy.ops.import_scene.gltf(filepath=str(out/'body-extraction.glb'))
    body=next(obj for obj in bpy.context.scene.objects if obj.type=='MESH')
    bpy.context.view_layer.objects.active=body
    body.select_set(True)
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bm=bmesh.new();bm.from_mesh(body.data)
    bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-5)
    # Two tiny, disconnected source fragments float beside the muzzle. They
    # cannot receive heat weights through the body and are not the eyes.
    remaining=set(bm.verts);fragments=[]
    while remaining:
        seed=remaining.pop();part=[seed];pending=[seed]
        while pending:
            for edge in pending.pop().link_edges:
                for other in edge.verts:
                    if other in remaining:
                        remaining.remove(other);pending.append(other);part.append(other)
        if len(part)<100 and min(v.co.z for v in part)>1.4:fragments.extend(part)
    removed_fragments=len(fragments)
    if fragments:bmesh.ops.delete(bm,geom=fragments,context='VERTS')
    boundaries=[e for e in bm.edges if e.is_boundary]
    bmesh.ops.holes_fill(bm,edges=boundaries,sides=256)
    bm.to_mesh(body.data);bm.free();body.data.update()
    # Undo the raised-front display pose with a rigid tilt, preserving anatomy.
    pivot=Vector((0,.8,.3))
    tilt=Matrix.Translation(pivot)@Matrix.Rotation(.15,4,'X')@Matrix.Translation(-pivot)
    original=[v.co.copy() for v in body.data.vertices]
    tilted=[tilt@point for point in original]
    lowest=min(point.z for point in tilted)
    highest=max(point.z for point in tilted)
    scale=1.45/(highest-lowest)
    def convert(point):
        point=tilt@Vector(point)
        point+=Vector((.08,.18,-lowest))
        return point*scale
    for vertex,point in zip(body.data.vertices,original):vertex.co=convert(point)
    body.data.update()
    for polygon in body.data.polygons:polygon.use_smooth=True
    body.name='LifeLensImportedTigerBody'
    body['source_topology']='TopoForge Tiger CC-BY-4.0; extracted base, original UV'
    armature=bpy.data.armatures.new('LifeLensTigerSkeleton')
    rig=bpy.data.objects.new('LifeLensTigerRig',armature)
    bpy.context.collection.objects.link(rig)
    bpy.ops.object.select_all(action='DESELECT')
    rig.select_set(True);bpy.context.view_layer.objects.active=rig
    bpy.ops.object.mode_set(mode='EDIT')
    def bone(name,a,b,parent=None,raw=True):
        item=armature.edit_bones.new(name)
        item.use_deform=name!='Root'
        item.head=convert(a) if raw else Vector(a)
        item.tail=convert(b) if raw else Vector(b)
        if parent:item.parent=armature.edit_bones[parent]
        return item
    bone('Root',(0,0,0),(0,0,.15),raw=False)
    bone('Pelvis',(0,.6,1.48),(0,.6,1.68),'Root')
    bone('SpineRear',(0,.6,1.68),(0,.05,1.77),'Pelvis')
    bone('SpineFront',(0,.05,1.77),(0,-.60,1.85),'SpineRear')
    bone('Neck',(0,-.60,1.85),(.05,-1.0,2.05),'SpineFront')
    bone('Head',(.05,-1.0,2.05),(.25,-1.30,2.20),'Neck')
    bone('Jaw',(.20,-1.23,1.95),(.35,-1.40,1.92),'Head')
    legs=[]
    # Paw centers come from the source's real four contacts, not a symmetric guess.
    definitions=[
        ('Fore.L',FOOT_ANCHORS[0],(-.25,-.70,1.77),(-.28,-.82,1.10),None,.25),
        ('Fore.R',FOOT_ANCHORS[1],(.23,-.68,1.77),(.19,-.54,1.05),None,.75),
        ('Hind.R',FOOT_ANCHORS[2],(.22,.60,1.50),(.20,.58,.96),(.18,.95,.62),.5),
        ('Hind.L',FOOT_ANCHORS[3],(-.22,.60,1.50),(-.25,.25,1.00),(-.25,.50,.65),0),
    ]
    for name,paw,hip,knee,hock,phase in definitions:
        ankle=(paw[0],paw[1]+.08,paw[2]+.17)
        toe=(paw[0],paw[1]-.12,paw[2]-.02)
        upper=bone(name+'.Upper',hip,knee,'SpineFront' if hock is None else 'Pelvis')
        lower=bone(name+'.Lower',knee,hock or ankle,upper.name)
        driver=lower
        if hock:driver=bone(name+'.Metatarsal',hock,ankle,lower.name)
        foot=bone(name+'.Paw',ankle,toe,driver.name)
        candidates=[convert(point).z for point in original
                    if abs(point.x-paw[0])<.18 and abs(point.y-paw[1])<.23
                    and abs(point.z-paw[2])<.20]
        if not candidates:raise ValueError('Source paw region missing: '+name)
        sole=min(candidates)
        legs.append({'name':name,'ankle':convert(ankle),'pole':convert(knee)+Vector((0,.6 if hock is None else -.6,0)),
                     'driver':driver.name,'upper':upper.name,'paw':foot.name,'phase':phase,
                     'chain':2 if hock is None else 3,'sole':sole})
    tail=[(0,1.02,1.42),(.02,1.37,1.04),(.07,1.58,.78),(.18,1.62,.95)]
    for i in range(3):bone('Tail'+str(i),tail[i],tail[i+1],'Pelvis' if i==0 else 'Tail'+str(i-1))
    bpy.ops.object.mode_set(mode='OBJECT')
    # Heat weights operate on the source surface. Abort if any vertex is unbound.
    bpy.ops.object.select_all(action='DESELECT')
    body.select_set(True);rig.select_set(True);bpy.context.view_layer.objects.active=rig
    bpy.ops.object.parent_set(type='ARMATURE_AUTO')
    bpy.context.view_layer.objects.active=body
    bpy.ops.object.vertex_group_limit_total(limit=4)
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    valid_names={item.name for item in armature.bones}
    deform={group.index for group in body.vertex_groups if group.name in valid_names}
    unbound=[v.index for v in body.data.vertices if sum(g.weight for g in v.groups if g.group in deform)<.99]
    if unbound:raise ValueError(f'{len(unbound)} source vertices lack normalized skin weights')
    controls=[]
    for leg in legs:
        target=bpy.data.objects.new('IK_'+leg['name'],None)
        pole=bpy.data.objects.new('Pole_'+leg['name'],None)
        bpy.context.collection.objects.link(target);bpy.context.collection.objects.link(pole)
        target.location=leg['ankle'];pole.location=leg['pole']
        target.rotation_mode='QUATERNION'
        target.rotation_quaternion=armature.bones[leg['paw']].matrix_local.to_quaternion()
        constraint=rig.pose.bones[leg['driver']].constraints.new('IK')
        constraint.target=target;constraint.chain_count=leg['chain']
        # Keep the source rest bend plane. A forced pole introduced a
        # near-180-degree upper-limb twist in the first deformation review.
        copy=rig.pose.bones[leg['paw']].constraints.new('COPY_ROTATION')
        copy.target=target;copy.owner_space='WORLD';copy.target_space='WORLD'
        leg['target']=target;leg['rest_rotation']=target.rotation_quaternion.copy()
        controls.extend([target,pole])
    scene=bpy.context.scene;scene.render.fps=30
    clips={}
    for name,duration,amplitude,stance in [
        ('idle',2,0,.62),('walk',1.2,.44,.62),('run',.7,.65,.38),
        ('hunt',1.6,.32,.70),('forage',2,0,.62),('drink',2,0,.62),('rest',2,0,.62),
    ]:
        frames=round(duration*30)
        action=bpy.data.actions.new('tiger_'+name)
        action.use_fake_user=True
        rig.animation_data_create();rig.animation_data.action=action
        for leg in legs:leg['target'].animation_data_clear()
        for frame in range(1,frames+2):
            t=(frame-1)/frames
            scene.frame_set(frame)
            for pose in rig.pose.bones:
                pose.rotation_mode='XYZ';pose.rotation_euler=(0,0,0);pose.location=(0,0,0)
            if name in ('forage','drink'):
                rig.pose.bones['Neck'].rotation_euler.x=.30
                rig.pose.bones['Head'].rotation_euler.x=.12+.025*math.sin(t*math.tau)
            vertical=-.38 if name=='rest' else -.08
            rig.pose.bones['Root'].location=armature.bones['Root'].matrix_local.to_3x3().inverted()@Vector((0,0,vertical))
            if name=='hunt':rig.pose.bones['Neck'].rotation_euler.x=.10
            rig.pose.bones['Tail1'].rotation_euler.z=.08*math.sin(t*math.tau)
            for leg in legs:
                phase=(t+leg['phase'])%1
                if phase<stance:
                    advance=amplitude*(phase/stance-.5);lift=0
                else:
                    swing=(phase-stance)/(1-stance)
                    advance=amplitude*(.5-swing)
                    lift=(.11 if name!='run' else .17)*math.sin(math.pi*swing) if amplitude else 0
                target=leg['target']
                target.location=leg['ankle']+Vector((0,advance,-leg['sole']+lift))
                target.keyframe_insert(data_path='location',frame=frame)
            for pose in rig.pose.bones:
                pose.keyframe_insert(data_path='rotation_euler',frame=frame)
                pose.keyframe_insert(data_path='location',frame=frame)
            bpy.context.view_layer.update()
        bpy.ops.object.select_all(action='DESELECT')
        rig.select_set(True);bpy.context.view_layer.objects.active=rig
        bpy.ops.object.mode_set(mode='POSE')
        bpy.ops.nla.bake(frame_start=1,frame_end=frames+1,step=1,only_selected=False,
                        visual_keying=True,clear_constraints=False,use_current_action=True,bake_types={'POSE'})
        bpy.ops.object.mode_set(mode='OBJECT')
        clips[name]={'name':action.name,'duration':duration,'cycle_distance':amplitude/stance if amplitude else 0}
    for pose in rig.pose.bones:
        for constraint in list(pose.constraints):pose.constraints.remove(constraint)
    for obj in controls:bpy.data.objects.remove(obj,do_unlink=True)
    rig.animation_data.action=bpy.data.actions['tiger_idle'];scene.frame_set(1)
    bpy.ops.object.select_all(action='DESELECT');body.select_set(True);rig.select_set(True)
    bpy.context.view_layer.objects.active=rig
    bpy.ops.file.pack_all()
    bpy.ops.wm.save_as_mainfile(filepath=str(out/'tiger-rigged.blend'))
    source_materials=list(body.data.materials)
    lod_materials(body,source_materials,2048)
    bpy.ops.export_scene.gltf(filepath=str(out/'tiger-rigged.glb'),export_format='GLB',
                              use_selection=True,export_skins=True,export_animations=True,export_force_sampling=True)
    contract={'version':1,'units':'metres','forward':'+Z','up':'+Y','tier':'near',
              'license':'CC-BY-4.0','creator':'TopoForge',
              'source':'https://www.sloyd.ai/free-3d-models/model/tiger-7m4ap2k1',
              'source_sha256':'f7e3b63e83efd92ba379f750f02e9971f52679173c6a268a29f352ad0c2092be',
              'status':'DRAFT_REQUIRES_OBSERVER_AND_DEVICE_VALIDATION','clips':clips,
              'paws':[{'name':leg['paw'],'upper':leg['upper'],'lower':leg['name']+'.Lower',
                       'tip':leg['paw'] if leg['chain']==2 else leg['name']+'.Metatarsal',
                       'phase':leg['phase'],'ankle_height':leg['ankle'].z-leg['sole']}
                      for leg in legs]}
    embed_contract(out/'tiger-rigged.glb',contract)
    mid=body.copy();mid.data=body.data.copy();mid.name='LifeLensImportedTigerBodyMid'
    bpy.context.collection.objects.link(mid)
    bpy.ops.object.select_all(action='DESELECT');mid.select_set(True)
    bpy.context.view_layer.objects.active=mid
    armature_modifiers=[modifier for modifier in mid.modifiers if modifier.type=='ARMATURE']
    for modifier in armature_modifiers:modifier.show_viewport=False
    decimate=mid.modifiers.new('MobileSurfaceLOD','DECIMATE')
    decimate.ratio=.35;decimate.use_collapse_triangulate=True
    bpy.ops.object.modifier_apply(modifier=decimate.name)
    for modifier in armature_modifiers:modifier.show_viewport=True
    bpy.ops.object.vertex_group_limit_total(limit=4)
    bpy.ops.object.vertex_group_normalize_all(lock_active=False)
    lod_materials(mid,source_materials,1024)
    rig.select_set(True)
    bpy.ops.export_scene.gltf(filepath=str(out/'tiger-mid.glb'),export_format='GLB',
                              use_selection=True,export_skins=True,export_animations=True,export_force_sampling=True)
    embed_contract(out/'tiger-mid.glb',{**contract,'tier':'mid'})
    report={'status':'RIG_REVIEW_REQUIRED_NOT_RUNTIME','merge_allowed':False,
            'bones':len(armature.bones),'vertices':len(body.data.vertices),'unbound_vertices':len(unbound),
            'source_scale':scale,'clips':clips,
            'removed_disconnected_muzzle_vertices':removed_fragments,
            'near_triangles':sum(len(p.vertices)-2 for p in body.data.polygons),
            'mid_triangles':sum(len(p.vertices)-2 for p in mid.data.polygons),
            'paw_joints':[leg['paw'] for leg in legs],
            'sha256':hashlib.sha256((out/'tiger-rigged.glb').read_bytes()).hexdigest()}
    (out/'tiger-rigged.json').write_text(json.dumps(report,indent=2)+'\n')
    print('TIGER_RIG_REVIEW',json.dumps(report))


if __name__=='__main__':main()
