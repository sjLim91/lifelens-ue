"""Load legitimate CC0 MakeHuman hair proxy via its MHClO barycentric mapping.

The underlying mesh and source texture belong to pinned MakeHuman graphical
assets (CC0). Proxy attachment and Blender skinning below are LifeLens-owned
Python, not GPL/AGPL MakeHuman application code.
"""
from __future__ import annotations

import pathlib
import bpy


def _parse_hair_obj(path):
    count=0
    uv=[];faces=[];face_uv=[]
    with pathlib.Path(path).open('rt',encoding='utf-8',errors='replace') as f:
        for line in f:
            if line.startswith('v '):
                count+=1
            elif line.startswith('vt '):
                t=line.split()
                uv.append((float(t[1]),float(t[2])))
            elif line.startswith('f '):
                indices=[];texture=[]
                for item in line.split()[1:]:
                    members=item.split('/')
                    indices.append(int(members[0])-1)
                    texture.append(int(members[1])-1)
                faces.append(indices)
                face_uv.append(texture)
    assert 1000<count<30000 and len(faces)>1000 and len(uv)>1000,(count,len(faces),len(uv))
    return count,faces,uv,face_uv


def _parse_mhclo(path,body):
    scale=[1.,1.,1.]
    coords=[]
    in_verts=False
    with pathlib.Path(path).open('rt',encoding='utf-8',errors='replace') as f:
        for line in f:
            parts=line.split()
            if not parts or parts[0].startswith('#'):
                continue
            if len(parts)==4 and parts[0] in ('x_scale','y_scale','z_scale'):
                scale[{'x_scale':0,'y_scale':1,'z_scale':2}[parts[0]]]=float(parts[-1])
            elif parts[0]=='verts' and len(parts)==2:
                in_verts=True
            elif in_verts and len(parts)==9:
                ids=[int(v) for v in parts[:3]]
                w=[float(v) for v in parts[3:6]]
                offset=[float(v) for v in parts[6:9]]
                assert all(0<=i<len(body) for i in ids),(path,ids)
                assert abs(sum(w)-1.)<.03,(path,w)
                coords.append(tuple(
                    sum(w[k]*body[ids[k]][axis] for k in range(3))
                    +offset[axis]*scale[axis] for axis in range(3)))
            elif in_verts and parts[0].lstrip('-').isdigit():
                raise RuntimeError('Malformed hair mapping vertex '+str(parts))
            # Material and metadata declarations may be interleaved between
            # 'verts 0' and the first barycentric mapping row.
    assert len(coords)>1000,(path,len(coords))
    return coords


def import_hair(body,rig,source,full_morph,gender,attach):
    identity='ponytail01' if gender=='female' else 'short02'
    path=pathlib.Path(source)
    count,faces,texcoords,uvrefs=_parse_hair_obj(path/(identity+'.obj'))
    coords=_parse_mhclo(path/(identity+'.mhclo'),full_morph)
    assert len(coords)==count,(count,len(coords))
    up=int(body['source_up_axis'])
    side=int(body['source_side_axis'])
    forward=int(body['source_forward_axis'])
    scalar=float(body['source_scale'])
    center_x=float(body['source_side_center'])
    center_y=float(body['source_forward_center'])
    low=float(body['source_min_height'])
    positions=[(
        -(co[side]-center_x)*scalar,
        -(co[forward]-center_y)*scalar,
        (co[up]-low)*scalar)
        for co in coords]
    # Hard spatial guard to reject malformed mapping, rather than approving a
    # large hair object hovering over the body like the Q3 helmet.
    min_z=min(p[2] for p in positions)
    max_z=max(p[2] for p in positions)
    max_x=max(abs(p[0]) for p in positions)
    print('CC0_PROXY_HAIR_LIMITS',gender,
          tuple(round(v,4) for v in (min_z,max_z,max_x)))
    assert 1.35<min_z<1.74 and 1.68<max_z<2.00,(gender,min_z,max_z)
    assert max_x<.35,(gender,max_x)
    if up in (0,1):
        faces=[list(reversed(face)) for face in faces]
        uvrefs=[list(reversed(face)) for face in uvrefs]
    mesh=bpy.data.meshes.new('CC0 Original Hair Proxy '+identity)
    mesh.from_pydata(positions,[],faces)
    mesh.update()
    layer=mesh.uv_layers.new(name='OriginalCC0HairUV')
    for poly,uvface in zip(mesh.polygons,uvrefs):
        for lid,idx in zip(poly.loop_indices,uvface):
            layer.data[lid].uv=texcoords[idx]
    mat=bpy.data.materials.new('CC0 Authentic '+gender+' hair PBR')
    mat.use_nodes=True
    shader=mat.node_tree.nodes.get('Principled BSDF')
    texture=mat.node_tree.nodes.new('ShaderNodeTexImage')
    texture.image=bpy.data.images.load(str(path/(identity+'_diffuse.png')),check_existing=True)
    assert texture.image.size[0]>=512 and texture.image.size[1]>=512
    texture.image.pack()
    mat.node_tree.links.new(texture.outputs['Color'],shader.inputs['Base Color'])
    shader.inputs['Roughness'].default_value=.81
    mesh.materials.append(mat)
    obj=bpy.data.objects.new('CC0 Styled '+gender+' Hair',mesh)
    bpy.context.collection.objects.link(obj)
    for polygon in mesh.polygons:
        polygon.use_smooth=True
    obj['lifelens_cc0_hair_proxy']=identity
    obj['lifelens_original_proxy_mapping']=True
    obj['lifelens_source_license']='CC0'
    attach(obj,rig)
    print('CC0_PROXY_HAIR_OK',gender,count,len(faces))
    return obj
