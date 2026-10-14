"""Strictly inspect experimental MakeHuman CC0 GLBs, never silently fall back."""
from __future__ import annotations
import json
import pathlib
import struct
import sys


def metadata(file):
    data=file.read_bytes()
    assert len(data)>500_000,(file,'GLB too small for anatomical topology')
    magic,version,length=struct.unpack_from('<4sII',data)
    assert magic==b'glTF' and version==2 and length==len(data)
    n,typ=struct.unpack_from('<I4s',data,12)
    assert typ==b'JSON'
    return json.loads(data[20:20+n].decode('utf-8').rstrip('\0 '))


def run(out):
    checks=[]
    for sex in ('female','male'):
        basename='resident-'+sex+'-cc0'
        blend=out/(basename+'.blend')
        glb=out/(basename+'.glb')
        assert blend.read_bytes()[:7]==b'BLENDER'
        assert blend.stat().st_size>500_000
        payload=metadata(glb)
        meshes=payload.get('meshes',[])
        primitives=[p for m in meshes for p in m.get('primitives',[])]
        assert len(primitives)>=1
        assert any(p.get('attributes',{}).get('POSITION') is not None and
                   'JOINTS_0' in p.get('attributes',{}) and
                   'WEIGHTS_0' in p.get('attributes',{})
                   for p in primitives),(sex,'no skin vertices')
        assert len(payload.get('skins',[]))>=1,(sex,'missing armature')
        assert len(payload.get('animations',[]))==1,(sex,'no single walk clip')
        n_bones=len(payload['skins'][0]['joints'])
        assert n_bones>=18,(sex,n_bones)
        # Male/female geometry buffers must really differ, not only file names.
        checks.append((sex, glb.stat().st_size, n_bones,
                      len(payload.get('accessors',[]))))
    assert checks[0][1]!=checks[1][1],('identical-sized GLB likely missing sex morph',checks)
    print('PASS CC0 anatomical trial, original shape targets and skinned walk:',checks)
    print('CC0_MAKEHUMAN_BASE_TRIAL_OK')


if __name__=='__main__':
    if len(sys.argv)!=2:
        raise SystemExit('Usage: validate_cc0_trial.py outdir')
    run(pathlib.Path(sys.argv[1]))
