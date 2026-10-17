"""Retarget pinned CC0 MakeHuman surfaces onto the existing UAL bind skeleton.

Run in Blender for texture resizing, or Python --geometry-only for offline
topology/bind-space diagnostics. Originals are never modified.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import struct
import sys
import numpy as np

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'docs/asset-reviews/resident-makehuman/source'
OFFICIAL = SOURCE / 'official/makehuman/data'


def read_glb(path):
    data = path.read_bytes()
    magic, version, length = struct.unpack_from('<4sII', data)
    assert magic == b'glTF' and version == 2 and length == len(data)
    size = struct.unpack_from('<I', data, 12)[0]
    return json.loads(data[20:20+size]), bytearray(data[28+size:])


def accessor(doc, binary, index):
    a = doc['accessors'][index]
    v = doc['bufferViews'][a['bufferView']]
    dtype = {5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']]
    width = {'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
    return np.frombuffer(binary, dtype=dtype, count=a['count']*width,
                         offset=v.get('byteOffset',0)+a.get('byteOffset',0)).reshape(-1,width).copy()


def obj(path, body_only=False):
    vertices, uv, faces, group = [], [], [], None
    for line in path.read_text().splitlines():
        parts = line.split()
        if not parts: continue
        if parts[0] == 'v': vertices.append([float(n) for n in parts[1:4]])
        elif parts[0] == 'vt': uv.append([float(n) for n in parts[1:3]])
        elif parts[0] == 'g': group = parts[1]
        elif parts[0] == 'f' and (not body_only or group == 'body'):
            polygon = [tuple(int(n)-1 for n in part.split('/')[:2]) for part in parts[1:]]
            faces.extend([[polygon[0],polygon[i],polygon[i+1]] for i in range(1,len(polygon)-1)])
    return np.array(vertices,dtype=float), np.array(uv,dtype=float), faces


def canonical(name):
    side = '_l' if name.endswith('.L') else '_r' if name.endswith('.R') else ''
    plain = name.split('.')[0]
    if plain.startswith('upperarm'): return 'upperarm'+side
    if plain.startswith('lowerarm'): return 'lowerarm'+side
    if plain.startswith('upperleg'): return 'thigh'+side
    if plain.startswith('lowerleg'): return 'calf'+side
    if plain == 'wrist' or plain.startswith('metacarpal'): return 'hand'+side
    if plain.startswith('clavicle') or plain.startswith('shoulder'): return 'clavicle'+side
    if plain.startswith('finger'):
        digit, segment = map(int,re.findall(r'\d+',plain))
        return ['thumb','index','middle','ring','pinky'][digit-1]+'_%02d'%segment+side
    if plain == 'foot': return 'foot'+side
    if plain.startswith('toe'): return 'ball'+side
    if plain in ['root','spine05'] or plain.startswith('pelvis'): return 'pelvis'
    if plain == 'spine04': return 'spine_01'
    if plain == 'spine03': return 'spine_02'
    if plain in ['spine01','spine02'] or plain.startswith('breast'): return 'spine_03'
    if plain.startswith('neck'): return 'neck_01'
    return 'Head'


def rotation(a,b):
    a=a/np.linalg.norm(a); b=b/np.linalg.norm(b)
    v=np.cross(a,b); c=np.dot(a,b)
    if c < -.99999:
        axis=np.cross(a,[0.,0.,1.])
        if np.linalg.norm(axis)<1e-6: axis=np.cross(a,[1.,0.,0.])
        axis/=np.linalg.norm(axis)
        return 2*np.outer(axis,axis)-np.eye(3)
    skew=np.array([[0,-v[2],v[1]],[v[2],0,-v[0]],[-v[1],v[0],0]])
    return np.eye(3)+skew+skew@skew/max(1e-8,1+c)


class Writer:
    def __init__(self, doc, binary):
        self.doc=copy.deepcopy(doc); self.binary=bytearray(binary)
        self.doc.pop('animations',None)
        for node in self.doc['nodes']:
            node.pop('mesh',None); node.pop('skin',None)
        for key in ['meshes','materials','textures','images','samplers']:
            self.doc[key]=[]
        self.doc['asset']['extras']={'source':'MakeHuman CC0; existing Quaternius UAL bind skeleton','softwareCodeImported':False}

    def view(self, data):
        while len(self.binary)%4: self.binary.append(0)
        offset=len(self.binary); self.binary.extend(data)
        views=self.doc['bufferViews']; views.append({'buffer':0,'byteOffset':offset,'byteLength':len(data)})
        return len(views)-1

    def attribute(self, values, kind, dtype='<f4'):
        values=np.asarray(values,dtype=dtype)
        a={'bufferView':self.view(values.tobytes()),'componentType':{'<f4':5126,'<u4':5125,'<u2':5123}[dtype],
           'count':len(values),'type':kind}
        if kind=='VEC3': a.update(min=values.min(axis=0).tolist(),max=values.max(axis=0).tolist())
        self.doc['accessors'].append(a)
        return len(self.doc['accessors'])-1

    def material(self, name, texture, color, geometry_only, alpha=False):
        material={'name':name,'pbrMetallicRoughness':{'baseColorFactor':color,'metallicFactor':0,'roughnessFactor':.78},'doubleSided':alpha}
        if not geometry_only:
            import bpy
            image=bpy.data.images.load(str(texture),check_existing=False)
            size=1024 if 'Skin' in name else 512
            if max(image.size)>size: image.scale(size,size)
            dest=ROOT/'docs/asset-reviews/resident-makehuman/generated'/('texture-'+name+'.png')
            dest.parent.mkdir(parents=True,exist_ok=True)
            image.filepath_raw=str(dest); image.file_format='PNG'; image.save()
            bpy.data.images.remove(image)
            self.doc['images'].append({'bufferView':self.view(dest.read_bytes()),'mimeType':'image/png'})
            self.doc['textures'].append({'source':len(self.doc['images'])-1})
            material['pbrMetallicRoughness']['baseColorTexture']={'index':len(self.doc['textures'])-1}
            material['pbrMetallicRoughness']['baseColorFactor']=[1,1,1,1]
        if alpha: material.update(alphaMode='MASK',alphaCutoff=.35)
        self.doc['materials'].append(material)
        return len(self.doc['materials'])-1

    def mesh(self,name,positions,uv,faces,joints,weights,material):
        # Smooth by original topology before splitting UV seams. Computing
        # normals after UV duplication creates false shoulder/neck cracks.
        original_triangles=np.array([[vertex for vertex,_ in face] for face in faces])
        original_normals=np.zeros_like(positions)
        face_normals=np.cross(positions[original_triangles[:,1]]-positions[original_triangles[:,0]],positions[original_triangles[:,2]]-positions[original_triangles[:,0]])
        for k in range(3): np.add.at(original_normals,original_triangles[:,k],face_normals)
        original_normals/=np.maximum(np.linalg.norm(original_normals,axis=1)[:,None],1e-12)
        lookup={};p=[];t=[];j=[];w=[];idx=[];smooth=[]
        for face in faces:
            for vertex,texcoord in face:
                key=(vertex,texcoord)
                if key not in lookup:
                    lookup[key]=len(p);p.append(positions[vertex]);t.append([uv[texcoord,0],1-uv[texcoord,1]]);j.append(joints[vertex]);w.append(weights[vertex]);smooth.append(original_normals[vertex])
                idx.append(lookup[key])
        p=np.array(p);idx=np.array(idx).reshape(-1,3)
        normals=np.array(smooth)
        primitive={'attributes':{'POSITION':self.attribute(p,'VEC3'),'NORMAL':self.attribute(normals,'VEC3'),
                   'TEXCOORD_0':self.attribute(t,'VEC2'),'JOINTS_0':self.attribute(j,'VEC4','<u2'),
                   'WEIGHTS_0':self.attribute(w,'VEC4')},'indices':self.attribute(idx.reshape(-1),'SCALAR','<u4'),'material':material}
        self.doc['meshes'].append({'name':name,'primitives':[primitive]})
        node={'name':name,'mesh':len(self.doc['meshes'])-1,'skin':0}
        self.doc['nodes'].append(node)
        self.doc['nodes'][68]['children'].append(len(self.doc['nodes'])-1)
        return {'name':name,'vertices':len(p),'triangles':len(idx)}

    def save(self,path):
        self.doc['buffers']=[{'byteLength':len(self.binary)}]
        document=json.dumps(self.doc,separators=(',',':')).encode()
        document+=b' '*((-len(document))%4)
        self.binary.extend(b'\0'*((-len(self.binary))%4))
        result=struct.pack('<4sII',b'glTF',2,28+len(document)+len(self.binary))
        result+=struct.pack('<I4s',len(document),b'JSON')+document
        result+=struct.pack('<I4s',len(self.binary),b'BIN\0')+self.binary
        path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(result)
        return hashlib.sha256(result).hexdigest()


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--geometry-only',action='store_true')
    parser.add_argument('--donor',type=Path,default=ROOT/'web/public/vendor/characters/character.glb')
    args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else ([] if 'blender' in Path(sys.argv[0]).name.lower() else None))
    donor,binary=read_glb(args.donor)
    skin=donor['skins'][0]
    names=[donor['nodes'][i]['name'] for i in skin['joints']]
    inv=accessor(donor,binary,skin['inverseBindMatrices']).reshape(-1,4,4).transpose(0,2,1)
    target={name:np.linalg.inv(matrix)[:3,3] for name,matrix in zip(names,inv)}
    rig=json.loads((OFFICIAL/'rigs/default.mhskel').read_text())
    official_weights=json.loads((OFFICIAL/'rigs/default_weights.mhw').read_text())['weights']
    base,uv,faces=obj(OFFICIAL/'3dobjs/base.obj',True)
    reports=[]
    for sex,style in [('female','ponytail01'),('male','short02')]:
        vertices=base.copy()
        for prefix in ['caucasian', 'universal']:
            suffix=f'{sex}-young'+('-averagemuscle-averageweight' if prefix=='universal' else '')
            for line in (OFFICIAL/f'targets/macrodetails/{prefix}-{suffix}.target').read_text().splitlines():
                if line and not line.startswith('#'):
                    v,*delta=line.split();vertices[int(v)]+=np.array(delta,dtype=float)
        def point(bone,end): return vertices[rig['joints'][rig['bones'][bone][end]]].mean(axis=0)
        groups={'pelvis':('spine05','spine05','spine_01'), 'spine_01':('spine04','spine04','spine_02'),
                'spine_02':('spine03','spine03','spine_03'),'spine_03':('spine02','spine01','neck_01'),
                'neck_01':('neck01','neck03','Head'),'Head':('head','head',None)}
        for side,suffix in [('L','_l'),('R','_r')]:
            for part,source,next_part in [('upperarm','upperarm','lowerarm'),('lowerarm','lowerarm','hand'),
                                          ('thigh','upperleg','calf'),('calf','lowerleg','foot')]:
                groups[part+suffix]=(source+'01.'+side,source+'02.'+side,next_part+suffix)
            groups['clavicle'+suffix]=('clavicle.'+side,'shoulder01.'+side,'upperarm'+suffix)
            groups['hand'+suffix]=('wrist.'+side,'wrist.'+side,'middle_01'+suffix)
            groups['foot'+suffix]=('foot.'+side,'foot.'+side,'ball'+suffix)
            groups['ball'+suffix]=('toe1-1.'+side,'toe1-2.'+side,'ball_leaf'+suffix)
            for digit,label in enumerate(['thumb','index','middle','ring','pinky'],1):
                for segment in range(1,4):
                    groups[f'{label}_{segment:02d}'+suffix]=(f'finger{digit}-{segment}.'+side,f'finger{digit}-{segment}.'+side,
                         f'{label}_{segment+1:02d}'+suffix if segment<3 else f'{label}_04_leaf'+suffix)
        scale=.103
        transforms={}; segment_ratios={}
        for name,(first,last,child) in groups.items():
            a=point(first,'head');b=point(last,'tail');ta=target[name]
            if name.startswith('hand_'):
                side='L' if name.endswith('_l') else 'R'
                b=point('finger3-1.'+side,'head')
            if name.startswith(('foot_','ball_')):
                side='L' if name.endswith('_l') else 'R'
                ball=np.mean([point(f'toe{digit}-1.'+side,'head') for digit in range(1,6)],axis=0)
                if name.startswith('foot_'): b=ball
                else:
                    a=ball
                    b=np.mean([point(f'toe{digit}-{2 if digit==1 else 3}.'+side,'tail') for digit in range(1,6)],axis=0)
            tb=target[child] if child else ta+np.array([0,.19,0])
            axis=b-a; length=np.linalg.norm(axis);axis/=length
            segment_ratios[name]=float(np.linalg.norm(tb-ta)/(length*scale))
            if name.startswith(('hand_','foot_')):
                assert .5 < segment_ratios[name] < 2, ('Palm/metatarsal landmark mismatch',name,segment_ratios[name])
            matrix=rotation(axis,tb-ta)@(scale*np.eye(3)+(np.linalg.norm(tb-ta)/length-scale)*np.outer(axis,axis))
            transforms[name]=(matrix,ta-matrix@a)
        by_vertex=[{} for _ in vertices]
        for bone,entries in official_weights.items():
            name=canonical(bone)
            assert name in transforms and name in names,name
            for vertex,weight in entries: by_vertex[vertex][name]=by_vertex[vertex].get(name,0)+weight
        posed=np.zeros_like(vertices);joints=np.zeros((len(vertices),4),dtype=np.uint16);weights=np.zeros((len(vertices),4))
        for v,entry in enumerate(by_vertex):
            selected=sorted(entry.items(),key=lambda x:-x[1])[:4] or [('Head',1.)]
            total=sum(weight for _,weight in selected)
            for k,(name,weight) in enumerate(selected):
                weight/=total;matrix,translation=transforms[name]
                posed[v]+=weight*(matrix@vertices[v]+translation)
                joints[v,k]=names.index(name);weights[v,k]=weight
        writer=Writer(donor,binary)
        skin_material=writer.material('NativeSkin-'+sex,SOURCE/f'assets/base/skins/textures/young_lightskinned_{sex}_diffuse.png',[.7,.48,.35,1],args.geometry_only)
        meshes=[writer.mesh('NativeResidentBody',posed,uv,faces,joints,weights,skin_material)]
        for label,path,texture in [('Eyes',OFFICIAL/'eyes/high-poly/high-poly.obj',OFFICIAL/'eyes/materials/brown_eye.png'),
                                   ('Hair',SOURCE/f'assets/base/hair/{style}/{style}.obj',SOURCE/f'assets/base/hair/{style}/{style}_diffuse.png')]:
            pv,puv,pfaces=obj(path)
            mapping=[];active=False;offset_scale=np.ones(3)
            for line in path.with_suffix('.mhclo').read_text().splitlines():
                row=line.split()
                if row and row[0] in ['x_scale','y_scale','z_scale']:
                    axis=['x_scale','y_scale','z_scale'].index(row[0])
                    offset_scale[axis]=abs(vertices[int(row[1]),axis]-vertices[int(row[2]),axis])/float(row[3])
                if row and row[0]=='verts': active=True;continue
                if active:
                    if not row or row[0].startswith('#'): continue
                    if not re.match(r'^\d+$',row[0]):
                        if mapping: break
                        continue
                    mapping.append(row)
            assert len(mapping)==len(pv),(label,len(mapping),len(pv))
            for v,row in enumerate(mapping):
                if len(row)==1: pv[v]=vertices[int(row[0])]
                else:
                    pv[v]=np.array(row[6:9],dtype=float)*offset_scale
                    for index,weight in zip(row[:3],row[3:6]): pv[v]+=float(weight)*vertices[int(index)]
            matrix,translation=transforms['Head'];pv=pv@matrix.T+translation
            pj=np.zeros((len(pv),4),dtype=np.uint16);pj[:,0]=names.index('Head')
            pw=np.zeros((len(pv),4));pw[:,0]=1
            material=writer.material('Native'+label+'-'+sex,texture,[.07,.04,.025,1] if label=='Hair' else [1,1,1,1],args.geometry_only,label=='Hair')
            meshes.append(writer.mesh('NativeResident'+label,pv,puv,pfaces,pj,pw,material))
        output=ROOT/f'web/public/vendor/characters/resident-{sex}.glb'
        digest=writer.save(output)
        lods=[]
        if not args.geometry_only:
            import bpy
            for tier,ratio in [('mid',.42),('far',.14)]:
                bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
                bpy.ops.import_scene.gltf(filepath=str(output))
                for object in list(bpy.context.scene.objects):
                    if object.type!='MESH': continue
                    bpy.context.view_layer.objects.active=object
                    armatures=[m for m in object.modifiers if m.type=='ARMATURE']
                    for modifier in armatures: modifier.show_viewport=False
                    modifier=object.modifiers.new('NativeResidentLOD','DECIMATE');modifier.ratio=ratio
                    bpy.ops.object.modifier_apply(modifier=modifier.name)
                    for modifier in armatures: modifier.show_viewport=True
                lod_path=output.with_name(f'resident-{sex}-{tier}.glb')
                bpy.ops.export_scene.gltf(filepath=str(lod_path),export_format='GLB',export_animations=False,export_extras=True)
                lods.append({'tier':tier,'file':lod_path.name,'bytes':lod_path.stat().st_size,'sha256':hashlib.sha256(lod_path.read_bytes()).hexdigest()})
        reports.append({'sex':sex,'file':output.name,'sha256':digest,'bytes':output.stat().st_size,'meshes':meshes,'bones':names,'segmentLengthRatios':segment_ratios,'lods':lods})
    result={'status':'GEOMETRY_ONLY_NOT_RUNTIME_ASSET' if args.geometry_only else 'GENERATED_REQUIRES_WEB_VERIFICATION','license':'CC0-1.0','sourceReceipt':hashlib.sha256((SOURCE/'receipts.json').read_bytes()).hexdigest(),'assets':reports,'mergeAllowed':False}
    path=ROOT/'docs/asset-reviews/resident-makehuman/generated/retarget.json';path.parent.mkdir(parents=True,exist_ok=True);path.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2))


if __name__=='__main__': main()
