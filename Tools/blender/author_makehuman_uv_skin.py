"""Real CC0 MakeHuman anatomical UV/body PBR skin.

Do not synthesize UV by planar projection: preserve the original CC0 OBJ
seam layout, and use the matching CC0 texture map. Only bundled graphical
assets are consumed, with no MakeHuman GPL/AGPL application source.
"""
from __future__ import annotations

import pathlib
import bpy


def populate_original_skin_uv(mesh,source,visible_limit=13380,reverse=True):
    uv_coords=[]
    faces=[]
    with pathlib.Path(source).open('rt',encoding='utf-8',errors='replace') as f:
        for line in f:
            if line.startswith('vt '):
                tokens=line.split()
                uv_coords.append((float(tokens[1]),float(tokens[2])))
            elif line.startswith('f '):
                items=line.split()[1:]
                vertex=[int(k.split('/')[0])-1 for k in items]
                if all(0<=i<visible_limit for i in vertex):
                    uv=[int(k.split('/')[1])-1 for k in items]
                    faces.append(list(reversed(uv)) if reverse else uv)
    assert len(uv_coords)>1000,('OBJ no UV coordinates',len(uv_coords))
    assert len(faces)==len(mesh.polygons),('face order changed',len(faces),len(mesh.polygons))
    assert max(i for face in faces for i in face)<len(uv_coords),'UV index out of range'
    layer=mesh.uv_layers.new(name='CC0BodyUV')
    for poly,uvface in zip(mesh.polygons,faces):
        assert len(poly.loop_indices)==len(uvface)
        for lid,idx in zip(poly.loop_indices,uvface):
            layer.data[lid].uv=uv_coords[idx]
    print('CC0_ORIGINAL_SKIN_UV_OK',len(uv_coords),'source UVs',
          len(mesh.polygons),'UV mapped faces')
    return layer


def apply_original_cc0_pbr_skin(obj,source_dir,sex):
    imagepath=pathlib.Path(source_dir)/('young_lightskinned_'+sex+'_diffuse.png')
    assert imagepath.is_file(),('Missing pinned CC0 skin image',imagepath)
    if imagepath.stat().st_size<100000:
        raise RuntimeError('Git LFS pointer instead of real skin image: '+str(imagepath))
    image=bpy.data.images.load(str(imagepath),check_existing=True)
    assert image.size[0]>=1024 and image.size[1]>=1024,('Unexpected skin texture',image.size[:])
    image.pack()
    skin=bpy.data.materials.new('CC0 Genuine MakeHuman '+sex+' skin PBR')
    skin.use_nodes=True
    nodes=skin.node_tree.nodes
    links=skin.node_tree.links
    bsdf=nodes.get('Principled BSDF')
    texture=nodes.new('ShaderNodeTexImage')
    texture.name='CC0 anatomical skin UV texture'
    texture.image=image
    texture.interpolation='Linear'
    links.new(texture.outputs['Color'],bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value=.74
    if 'Subsurface Weight' in bsdf.inputs:
        bsdf.inputs['Subsurface Weight'].default_value=.07
    elif 'Subsurface' in bsdf.inputs:
        bsdf.inputs['Subsurface'].default_value=.07
    obj.data.materials.clear()
    obj.data.materials.append(skin)
    obj['lifelens_skin_license']='CC0 1.0 Universal'
    obj['lifelens_skin_source']='s20220526/makehuman-assets graphical assets'
    obj['lifelens_skin_source_commit']='8cf9645b975a98eea056b140df11a1d278da0d10'
    print('CC0_TEXTURED_SKIN_OK',sex,tuple(image.size),
          imagepath.stat().st_size,'bytes')
    return skin
