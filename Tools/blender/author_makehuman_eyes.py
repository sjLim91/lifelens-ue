"""Load official MakeHuman CC0 anatomical eyes + brown eye texture.

The 1064-vertex eye mesh and PNG belong to MakeHuman's bundled CC0 assets.
This is LifeLens-authored parsing/rigging code, not a copied addon.
"""
from __future__ import annotations
import pathlib
import bpy


def import_cc0_eyes(body,rig,source,attach):
    objfile=pathlib.Path(source)/'high-poly.obj'
    imagefile=pathlib.Path(source)/'brown_eye.png'
    assert objfile.is_file() and imagefile.is_file()
    positions=[];texcoords=[];polygons=[];face_uv=[]
    with objfile.open('rt',encoding='utf-8',errors='replace') as f:
        for line in f:
            if line.startswith('v '):
                positions.append(tuple(float(x) for x in line.split()[1:4]))
            elif line.startswith('vt '):
                texcoords.append(tuple(float(x) for x in line.split()[1:3]))
            elif line.startswith('f '):
                indices=[];uvindex=[]
                for token in line.split()[1:]:
                    refs=token.split('/')
                    index=int(refs[0])-1
                    uv=int(refs[1])-1 if len(refs)>1 and refs[1] else 0
                    indices.append(index)
                    uvindex.append(uv)
                polygons.append(indices)
                face_uv.append(uvindex)
    assert 500<len(positions)<10000 and len(polygons)>500
    assert len(texcoords)>100
    up=int(body['source_up_axis'])
    side=int(body['source_side_axis'])
    forward=int(body['source_forward_axis'])
    scale=float(body['source_scale'])
    origin_x=float(body['source_side_center'])
    origin_y=float(body['source_forward_center'])
    origin_z=float(body['source_min_height'])
    vertices=[]
    for raw in positions:
        vertices.append((-(raw[side]-origin_x)*scale,
                         -(raw[forward]-origin_y)*scale,
                         (raw[up]-origin_z)*scale))
    # Same winding conversion as visible body extraction.
    if up in (0,1):
        polygons=[list(reversed(p)) for p in polygons]
        face_uv=[list(reversed(p)) for p in face_uv]
    mesh=bpy.data.meshes.new('CC0 MakeHuman High Poly Eyes')
    mesh.from_pydata(vertices,[],polygons)
    mesh.update()
    layer=mesh.uv_layers.new(name='UVMap')
    for polygon,uv_refs in zip(mesh.polygons,face_uv):
        for loop_id,uvindex in zip(polygon.loop_indices,uv_refs):
            if uvindex>=len(texcoords):
                raise AssertionError(('out of range UV index',uvindex,len(texcoords)))
            layer.data[loop_id].uv=texcoords[uvindex]
    material=bpy.data.materials.new('CC0 MakeHuman brown iris texture')
    material.use_nodes=True
    nodes=material.node_tree.nodes
    bsdf=nodes.get('Principled BSDF')
    image=nodes.new('ShaderNodeTexImage')
    image.image=bpy.data.images.load(str(imagefile),check_existing=True)
    image.image.pack()
    material.node_tree.links.new(image.outputs['Color'],bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value=.27
    mesh.materials.append(material)
    eye=bpy.data.objects.new('CC0 anatomical textured eyes',mesh)
    bpy.context.collection.objects.link(eye)
    eye['lifelens_cc0_eyes']='makehuman/data/eyes/high-poly/high-poly.obj + materials/brown_eye.png'
    for face in mesh.polygons:
        face.use_smooth=True
    attach(eye,rig)
    print('CC0_ANATOMICAL_EYE_ASSET_OK',len(vertices),'vertices',
          len(polygons),'faces',len(texcoords),'UVs')
    return eye
