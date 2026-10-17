"""Bounded texture LODs; preserve every non-image GLB buffer view verbatim."""
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import bpy

out=Path(sys.argv[sys.argv.index('--')+1]).resolve()
report=json.loads((out/'native-authoring.json').read_text())
for receipt in report['receipts']:
 path=out/receipt['file'];raw=path.read_bytes();n=struct.unpack_from('<I',raw,12)[0]
 doc=json.loads(raw[20:20+n]);binary=raw[28+n:]
 limit={'near':2048,'mid':1024,'far':512}[receipt['contract']['tier']]
 image_views={image['bufferView']:image for image in doc.get('images',[])}
 packed=bytearray();maximum=0
 for index,view in enumerate(doc['bufferViews']):
  offset=view.get('byteOffset',0);piece=binary[offset:offset+view['byteLength']]
  if index in image_views:
   with tempfile.TemporaryDirectory() as directory:
    temp=Path(directory);source=temp/'source';source.write_bytes(piece)
    image=bpy.data.images.load(str(source),check_existing=False);w,h=image.size
    if max(w,h)>limit:
     ratio=limit/max(w,h);image.scale(max(1,round(w*ratio)),max(1,round(h*ratio)))
     image.file_format='PNG';image.filepath_raw=str(temp/'lod.png');image.save()
     piece=Path(image.filepath_raw).read_bytes();image_views[index]['mimeType']='image/png'
    maximum=max(maximum,*image.size);bpy.data.images.remove(image)
  packed.extend(b'\0'*((-len(packed))%4));view['byteOffset']=len(packed);view['byteLength']=len(piece);packed.extend(piece)
 doc['buffers'][0]['byteLength']=len(packed);doc['asset']['extras']['lifelensTiger']['texture_limit']=limit
 encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4)
 packed.extend(b'\0'*((-len(packed))%4))
 result=struct.pack('<4sII',b'glTF',2,28+len(encoded)+len(packed))+struct.pack('<II',len(encoded),0x4e4f534a)+encoded+struct.pack('<II',len(packed),0x004e4942)+packed
 path.write_bytes(result);receipt['sha256']=hashlib.sha256(result).hexdigest();receipt['bytes']=len(result);receipt['textureLimit']=limit;receipt['maximumTextureSize']=maximum
 print('NATIVE_TEXTURE_LOD',path.name,len(result),maximum)
(out/'native-authoring.json').write_text(json.dumps(report,indent=2)+'\n')
