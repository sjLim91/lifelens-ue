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
    # MakeHuman eye proxy OBJ uses a different local eye-group origin than
    # the HM08 body. Fit its two eyeballs to the original body's anatomical
    # orbital plane (the CC0 body is normalized to 1.75 metres). Without
    # this calibration the source eyes float nearly a metre above the head.
    eye_center=(min(v[2] for v in vertices)+max(v[2] for v in vertices))*.5
    eye_z=1.626
    dz=eye_z-eye_center
    vertices=[(x,y,z+dz) for x,y,z in vertices]
    # Q6 visual audit: 1:1 imported eyeballs looked like enormous black
    # marbles. Fit each independently to the anatomical orbit without
    # destroying the CC0 high-poly topology or its source UV coordinates.
    centers={}
    for sign in (-1,1):
        group=[p for p in vertices if (-1 if p[0]<0 else 1)==sign]
        assert len(group)>200,('CC0 eye group missing',sign,len(group))
        centers[sign]=tuple(sum(p[a] for p in group)/len(group) for a in range(3))
    ratio=.61
    fitted=[]
    for x,y,z in vertices:
        sign=-1 if x<0 else 1
        cx,cy,cz=centers[sign]
        fitted.append((sign*.033+(x-cx)*ratio,
                       -.030+(y-cy)*ratio,
                       1.626+(z-cz)*ratio))
    vertices=fitted
    print('CC0 anatomical eye proxy calibrated, source dz',round(dz,5),
          'eye scale',ratio,'source centers',centers)
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
    # Some upstream eye UVs make the entire eye look nearly black under
    # neutral studio illumination. Preserve a muted CC0 image influence
    # while giving the scleral shell a warm, physically plausible tone.
    mix=nodes.new('ShaderNodeMixRGB')
    mix.blend_type='MIX'
    mix.inputs[0].default_value=.16
    mix.inputs[1].default_value=(.79,.76,.70,1.0)
    material.node_tree.links.new(image.outputs['Color'],mix.inputs[2])
    material.node_tree.links.new(mix.outputs['Color'],bsdf.inputs['Base Color'])
    bsdf.inputs['Roughness'].default_value=.33
    mesh.materials.append(material)
    eye=bpy.data.objects.new('CC0 anatomical textured eyes',mesh)
    bpy.context.collection.objects.link(eye)
    eye['lifelens_cc0_eyes']='makehuman/data/eyes/high-poly/high-poly.obj + materials/brown_eye.png'
    for face in mesh.polygons:
        face.use_smooth=True
    attach(eye,rig)
    # Small iris/pupil disks lie on the spherical cornea; separate surface
    # elements preserve legibility without replacing anatomical CC0 eyeballs.
    from author_makehuman_cc0_head import simple_material,add_ellipsoid
    iris=simple_material('CC0 natural warm brown iris',(.28,.17,.075),.25)
    pupil=simple_material('CC0 black pupil',(.018,.013,.011),.19)
    for sign in (-1,1):
        add_ellipsoid('CC0 subtle iris '+str(sign),
                      (sign*.033,-.040,1.626),(.0046,.0012,.0046),
                      iris,rig)
        add_ellipsoid('CC0 subtle pupil '+str(sign),
                      (sign*.033,-.042,1.626),(.0023,.0010,.0034),
                      pupil,rig)
    print('CC0_ANATOMICAL_EYE_ASSET_OK',len(vertices),'vertices',
          len(polygons),'faces',len(texcoords),'UVs')
    return eye
