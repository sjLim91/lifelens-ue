"""Verify Blender exported actual armature-driven, vertex-colored creature GLBs.

Does not declare visual quality or runtime integration; it establishes a
repeatable, machine-checkable prerequisite for rig retargeting review.
"""
from __future__ import annotations

import json
import pathlib
import struct
import sys


def inspect_glb(path):
    data = path.read_bytes()
    if len(data) < 200:
        raise AssertionError(f'{path}: missing meaningful GLB payload')
    magic, version, length = struct.unpack_from('<4sII',data)
    assert magic == b'glTF' and version == 2 and length == len(data),path
    json_len, json_type = struct.unpack_from('<I4s',data,12)
    assert json_type == b'JSON',path
    contents = json.loads(data[20:20+json_len].decode('utf-8').rstrip('\0 '))
    return contents


def main():
    if len(sys.argv)!=2:
        raise SystemExit('Usage: python3 validate_creatures.py OUTDIR')
    out=pathlib.Path(sys.argv[1])
    for species in ('resident-female','resident-male','tiger'):
        project=out/f'{species}.blend'
        glb=out/f'{species}.glb'
        assert project.exists() and project.stat().st_size>100_000,project
        assert project.read_bytes()[:7]==b'BLENDER',project
        obj=inspect_glb(glb)
        assert obj.get('asset',{}).get('version')=='2.0'
        assert len(obj.get('skins',[]))>=1,(species,'no actual skin')
        joint_count=max(len(skin['joints']) for skin in obj['skins'])
        assert joint_count >= (16 if species.startswith('resident-') else 10)
        assert len(obj.get('animations',[]))==1,(species,'stale animation clip contamination')
        channels=sum(len(anim.get('channels',[])) for anim in obj['animations'])
        assert channels>=6,(species,'motion not bound to bones')
        assert len(obj.get('materials',[]))>=1,(species,'missing surface material')
        primitives=[p for mesh in obj.get('meshes',[]) for p in mesh.get('primitives',[])]
        assert primitives,(species,'missing mesh')
        assert any('JOINTS_0' in p.get('attributes',{}) and
                   'WEIGHTS_0' in p.get('attributes',{}) for p in primitives),(species,'no GPU skin')
        assert any('COLOR_0' in p.get('attributes',{}) for p in primitives),(species,'missing authored pigmentation')
        detail=[m for m in obj.get('meshes',[]) if 'IndependentAnatomicalDetails' in m.get('name','')]
        assert detail,(species,'missing separately authored anatomical topology')
        detail_prims=[p for mesh in detail for p in mesh.get('primitives',[])]
        assert len(detail_prims)>=5,(species,'insufficient independent facial/paw materials')
        assert all('JOINTS_0' in p.get('attributes',{}) and 'WEIGHTS_0' in p.get('attributes',{}) for p in detail_prims),(species,'unspecified rigid facial/paw attachments')
        assert len(obj['materials'])>=7,(species,'missing independent face/eye/coat materials')
        if species=='tiger':
            fur=[m for m in obj.get('meshes',[]) if 'TigerSkinnedMicroFur' in m.get('name','')]
            assert fur,('tiger','no skinned microfur model exported')
            fur_prims=[p for mesh in fur for p in mesh.get('primitives',[])]
            assert len(fur_prims)>=3,('tiger','pelage pigmentation regions missing')
            assert all('JOINTS_0' in p.get('attributes',{}) and
                       'WEIGHTS_0' in p.get('attributes',{}) for p in fur_prims),('tiger','fur detached from armature')
            print('PASS Q10 tiger microfur rigged GLB:',len(fur_prims),'fur primitives')
        print('PASS Q4 anatomical face/paws:',species,len(detail_prims),'skin-weighted material primitives')
        print('PASS',species,glb.stat().st_size,'bytes,',
              joint_count,'joints,',len(obj['animations']),'animations,',
              channels,'animated channels,',len(primitives),'mesh primitives')
    print('LIFELENS_CREATURE_EXPORT_OK')


if __name__=='__main__':
    main()
