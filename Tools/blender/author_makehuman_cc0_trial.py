"""Experimental CC0 MakeHuman basemesh morph + LifeLens-authored rig workflow.

The *input mesh and targets* are MakeHuman CC0 assets, not AGPL/GPL program
code. We implement the minimal OBJ/target parser ourselves; no addon/vendor
scripts are copied, bundled or executed. Pin data by immutable upstream commit.

This is only an offline comparison path. Existing Web Observer Q1 and Q4
models remain completely unchanged.
"""
from __future__ import annotations

import hashlib
import pathlib
import sys

import bpy
from mathutils import Vector

sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
from author_creatures import make_armature, attach_weights, idle_and_walk, shader

UPSTREAM_COMMIT='a8bc2d54ff0ac92e78ff71431b1023eda42bf482'


def parse_obj(source):
    vertices=[]
    faces=[]
    with source.open('rt',encoding='utf-8',errors='replace') as file:
        for line in file:
            if line.startswith('v '):
                xyz=line.split()
                vertices.append([float(a) for a in xyz[1:4]])
            elif line.startswith('f '):
                # Faces in MakeHuman are quads; preserve exact topology.
                corners=[int(p.split('/')[0]) for p in line.split()[1:]]
                face=[i-1 if i>0 else len(vertices)+i for i in corners]
                if len(face)>=3:
                    faces.append(face)
    assert 12000<len(vertices)<50000 and len(faces)>10000
    assert all(0<=i<len(vertices) for face in faces for i in face)
    return vertices,faces


def apply_morph(vertices,target,weight):
    affected=0
    with target.open('rt',encoding='utf-8',errors='replace') as file:
        for line in file:
            entry=line.split()
            if not entry or entry[0].startswith('#'):
                continue
            if len(entry)<4:
                continue
            index=int(entry[0])
            if not 0<=index<len(vertices):
                raise AssertionError((target.name,index,len(vertices)))
            for k in range(3):
                vertices[index][k]+=float(entry[1+k])*weight
            affected+=1
    assert affected>300,(target,affected)
    return affected


def import_basemesh(input_dir,gender):
    source=input_dir/'base.obj'
    vertices,faces=parse_obj(source)
    target=input_dir/('universal-'+gender+'-young-averagemuscle-averageweight.target')
    racial=input_dir/('caucasian-'+gender+'-young.target')
    affected=apply_morph(vertices,target,1.0)
    affected2=apply_morph(vertices,racial,.35)
    extents=[max(v[i] for v in vertices)-min(v[i] for v in vertices)
             for i in range(3)]
    up=max(range(3),key=extents.__getitem__)
    assert extents[up]>max(extents[(up+1)%3],extents[(up+2)%3])*1.4,(extents,up)
    # The canonical MakeHuman OBJ is authored with Y up; for any upstream
    # coordinate revision, detect the up axis from the actual bounds.
    forward=2 if up==1 else 1
    side=0 if up!=0 else 2
    scale=1.75/extents[up]
    center_side=(max(v[side] for v in vertices)+min(v[side] for v in vertices))*.5
    center_depth=(max(v[forward] for v in vertices)+min(v[forward] for v in vertices))*.5
    lowest=min(v[up] for v in vertices)
    coords=[]
    for v in vertices:
        coords.append(((v[side]-center_side)*scale,
                       (v[forward]-center_depth)*scale,
                       (v[up]-lowest)*scale))
    mesh=bpy.data.meshes.new('MakeHumanCC0Topology_'+gender)
    # Odd axis permutations invert polygon handedness; keep exterior-facing
    # normals after converting MakeHuman Y-up to Blender Z-up.
    if up in (0,1):
        faces=[list(reversed(face)) for face in faces]
    mesh.from_pydata(coords,[],faces)
    mesh.validate(clean_customdata=False)
    mesh.update()
    obj=bpy.data.objects.new('LifeLensMakeHumanCC0_'+gender,mesh)
    bpy.context.collection.objects.link(obj)
    for p in mesh.polygons:
        p.use_smooth=True
    skin=shader('CC0 MakeHuman source skin '+gender,
                (.68,.445,.345) if gender=='female' else (.63,.397,.307),
                roughness=.76)
    obj.data.materials.append(skin)
    obj['CC0_SOURCE']='makehumancommunity/makehuman'
    obj['CC0_SOURCE_COMMIT']=UPSTREAM_COMMIT
    obj['CC0_MORPHED_VERTICES']=affected+affected2
    obj['lifelens_asset_status']='evaluation-only-not-runtime'
    print('MAKEHUMAN_IMPORTED',gender,'verts',len(vertices),'faces',len(faces),
          'bounds',tuple(round(v,4) for v in extents),'source_up',up,
          'morphs',affected,affected2)
    return obj


def rig_human(mesh):
    # Skeleton is authored by LifeLens; rest pivots remain deterministic.
    bones=[
        ('pelvis',(0,0,.92),(0,0,1.03),None),
        ('spine_02',(0,0,1.03),(0,0,1.47),'pelvis'),
        ('neck_01',(0,0,1.47),(0,0,1.59),'spine_02'),
        ('Head',(0,0,1.59),(0,0,1.75),'neck_01'),
    ]
    for s,suffix in [(-1,'r'),(1,'l')]:
        bones+= [
          ('clavicle_'+suffix,(0,0,1.44),(s*.22,0,1.45),'spine_02'),
          ('upperarm_'+suffix,(s*.22,0,1.45),(s*.36,0,1.21),'clavicle_'+suffix),
          ('lowerarm_'+suffix,(s*.36,0,1.21),(s*.40,0,1.02),'upperarm_'+suffix),
          ('hand_'+suffix,(s*.40,0,1.02),(s*.40,0,.84),'lowerarm_'+suffix),
          ('thigh_'+suffix,(s*.11,0,.91),(s*.13,0,.50),'pelvis'),
          ('calf_'+suffix,(s*.13,0,.50),(s*.13,-.025,.14),'thigh_'+suffix),
          ('foot_'+suffix,(s*.13,-.025,.14),(s*.13,-.18,.06),'calf_'+suffix)
        ]
    rig=make_armature('LifeLensCC0TrialRig',bones)
    attach_weights(mesh,rig,bones)
    idle_and_walk(rig,'human')
    return rig


def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    if len(args)!=2:
        raise SystemExit('Pass MakeHuman downloaded INPUT_DIR and OUTPUT_DIR')
    source=pathlib.Path(args[0]).resolve()
    out=pathlib.Path(args[1]).resolve()
    out.mkdir(exist_ok=True,parents=True)
    for gender in ('female','male'):
        bpy.ops.object.select_all(action='SELECT')
        bpy.ops.object.delete(use_global=False)
        for action in list(bpy.data.actions):
            bpy.data.actions.remove(action,do_unlink=True)
        mesh=import_basemesh(source,gender)
        rig=rig_human(mesh)
        assert len(mesh.data.vertices)>10000
        assert len(rig.data.bones)>=18
        filepath=out/('resident-'+gender+'-cc0')
        bpy.ops.wm.save_as_mainfile(filepath=str(filepath)+'.blend')
        bpy.ops.export_scene.gltf(filepath=str(filepath)+'.glb',
                                  export_format='GLB',export_skins=True,
                                  export_animations=True)
        print('MAKEHUMAN_CC0_EVALUATION_OK',gender,filepath)


if __name__=='__main__':
    main()
