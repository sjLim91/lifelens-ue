/** Index-only LOD reduction: retain original native joints, UVs and weights. */
import {readFileSync,writeFileSync} from 'node:fs';
import {resolve} from 'node:path';
import {createHash} from 'node:crypto';
import {MeshoptSimplifier} from 'meshoptimizer';
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
const root=resolve(import.meta.dirname,'../../docs/asset-reviews/native-wildlife/derived');
const slugs=['hare','deer','wolf','freshwater-fish'];
await MeshoptSimplifier.ready;
const reports=[];
for(const slug of slugs){
 const raw=readFileSync(resolve(root,slug+'-rigged.glb')),size=raw.readUInt32LE(12),doc=JSON.parse(raw.subarray(20,20+size)),binary=raw.subarray(28+size);
 const gltf=await new GLTFLoader().register(()=>({name:'LIFELENS_OFFLINE_REDUCTION',loadTexture:()=>Promise.resolve(new T.Texture())}))
  .parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
 const total=doc.meshes.flatMap(m=>m.primitives).reduce((sum,p)=>sum+doc.accessors[p.indices].count/3,0);
 const target=slug==='deer'?8000:slug==='freshwater-fish'?1500:3500,ratio=Math.min(1,target/total);
 const chunks=[binary];let length=binary.length,triangles=0,maxError=0;
 for(const mesh of doc.meshes)for(const primitive of mesh.primitives){
  const indices=await gltf.parser.getDependency('accessor',primitive.indices),position=await gltf.parser.getDependency('accessor',primitive.attributes.POSITION);
  const positions=Float32Array.from({length:position.count*3},(_,i)=>position.getComponent(Math.floor(i/3),i%3));
  const old=Uint32Array.from({length:indices.count},(_,i)=>indices.getX(i));
  // This trout stores a separate vertex for every face corner. Weld only
  // corners with identical positions, UVs and skin data; keep the source
  // position/UV/weight streams and smooth the reduced surface's normals.
  let welded=false;
  if(slug==='freshwater-fish'){
   const attributes=await Promise.all(Object.entries(primitive.attributes).filter(([key])=>!['NORMAL','TANGENT'].includes(key)).map(async([key,id])=>[key,await gltf.parser.getDependency('accessor',id)]));
   const first=new Map(),mapping=new Uint32Array(position.count);
   for(let i=0;i<position.count;i++){
    const key=attributes.map(([name,a])=>name+':'+Array.from({length:a.itemSize},(_,k)=>a.getComponent(i,k).toFixed(6)).join(',')).join('|');
    if(!first.has(key))first.set(key,i);mapping[i]=first.get(key);
   }
   for(let i=0;i<old.length;i++)old[i]=mapping[old[i]];welded=true;
  }
  const wanted=Math.min(old.length,Math.max(60,Math.floor(old.length*ratio/3)*3));
  let workingIndices=old,workingPositions=positions,restoreIndices=null;
  if(welded){
   restoreIndices=[...new Set(old)];const compact=new Map(restoreIndices.map((v,i)=>[v,i]));
   workingIndices=Uint32Array.from(old,i=>compact.get(i));
   workingPositions=Float32Array.from(restoreIndices.flatMap(i=>[positions[i*3],positions[i*3+1],positions[i*3+2]]));
  }
  const [simplified,error]=MeshoptSimplifier.simplify(workingIndices,workingPositions,3,wanted,.04,[]);
  const reduced=restoreIndices?Uint32Array.from(simplified,i=>restoreIndices[i]):simplified;
  triangles+=reduced.length/3;maxError=Math.max(maxError,error);
  const padding=(-length)&3;if(padding){chunks.push(Buffer.alloc(padding));length+=padding;}
  const data=Buffer.from(new Uint32Array(reduced).buffer),view=doc.bufferViews.length;doc.bufferViews.push({buffer:0,byteOffset:length,byteLength:data.length});chunks.push(data);length+=data.length;
  const accessor=doc.accessors.length;doc.accessors.push({bufferView:view,componentType:5125,type:'SCALAR',count:reduced.length,min:[Math.min(...reduced)],max:[Math.max(...reduced)]});primitive.indices=accessor;
  if(welded){
   const geometry=new T.BufferGeometry();geometry.setAttribute('position',new T.BufferAttribute(positions,3));geometry.setIndex(new T.BufferAttribute(new Uint32Array(reduced),1));geometry.computeVertexNormals();
   const normal=Buffer.from(geometry.attributes.normal.array.buffer),v=doc.bufferViews.length;doc.bufferViews.push({buffer:0,byteOffset:length,byteLength:normal.length});chunks.push(normal);length+=normal.length;
   primitive.attributes.NORMAL=doc.accessors.length;doc.accessors.push({bufferView:v,componentType:5126,type:'VEC3',count:position.count});delete primitive.attributes.TANGENT;geometry.dispose();
  }
 }
 const c=doc.asset.extras.lifelensWildlife;c.tier='mid';c.paws=c.paws.map(({sole_vertices,...paw})=>paw);c.triangles=triangles;
 doc.buffers[0].byteLength=length;
 let packed=Buffer.concat(chunks);packed=Buffer.concat([packed,Buffer.alloc((-packed.length)&3)]);
 let json=Buffer.from(JSON.stringify(doc));json=Buffer.concat([json,Buffer.alloc((-json.length)&3,32)]);
 const head=Buffer.alloc(20);head.write('glTF');head.writeUInt32LE(2,4);head.writeUInt32LE(28+json.length+packed.length,8);head.writeUInt32LE(json.length,12);head.writeUInt32LE(0x4e4f534a,16);
 const binHead=Buffer.alloc(8);binHead.writeUInt32LE(packed.length);binHead.writeUInt32LE(0x004e4942,4);
 const output=Buffer.concat([head,json,binHead,packed]);writeFileSync(resolve(root,slug+'-mid.glb'),output);
 reports.push({species:c.species,originalTriangles:total,midTriangles:triangles,error:maxError,sha256:createHash('sha256').update(output).digest('hex')});
 console.log('NATIVE_MID_LOD',JSON.stringify(reports.at(-1)));
}
writeFileSync(resolve(root,'mid-lods.json'),JSON.stringify(reports,null,2)+'\n');
