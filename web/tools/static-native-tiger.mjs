/** Bake bounded far poses from the same verified native body, not a proxy shape. */
import * as T from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
export async function bakeStaticFar(raw){
 const size=raw.readUInt32LE(12),original=JSON.parse(raw.subarray(20,20+size)),binary=raw.subarray(28+size);
 const loader=new GLTFLoader().register(()=>({name:'FAR_BAKE_NO_TEXTURE_RENDER',loadTexture:()=>Promise.resolve(new T.Texture())}));
 const gltf=await loader.parseAsync(raw.buffer.slice(raw.byteOffset,raw.byteOffset+raw.byteLength),'');
 const contract=structuredClone(original.asset.extras.lifelensTiger);
 let skin;gltf.scene.traverse(o=>{if(o.isSkinnedMesh)skin=o;});if(!skin)throw Error('Far bake requires the real weighted source body');
 const sourceGeometry=skin.geometry,sourceIndices=Array.from(sourceGeometry.index.array),used=Array.from(new Set(sourceIndices));
 const remap=new Map(used.map((index,i)=>[index,i])),indices=sourceIndices.map(i=>remap.get(i));
 const bones=skin.skeleton.bones,weights=sourceGeometry.attributes.skinWeight,joints=sourceGeometry.attributes.skinIndex;
 const families=contract.paws.map(p=>{const upper=bones.find(b=>b.name===p.upper);if(!upper)throw Error('Far limb mask lost its native joint');const family=new Set();upper.traverse(b=>{if(b.isBone)family.add(bones.indexOf(b));});return family;});
 const legWeights=used.flatMap(index=>families.map(family=>{let total=0;for(let j=0;j<4;j++)if(family.has(joints.getComponent(index,j)))total+=weights.getComponent(index,j);return total;}));
 const doc={asset:structuredClone(original.asset),scene:0,scenes:[],nodes:[],meshes:[],materials:structuredClone(original.materials),images:[],textures:structuredClone(original.textures),samplers:structuredClone(original.samplers??[]),buffers:[{byteLength:0}],bufferViews:[],accessors:[]};
 const chunks=[];let length=0;
 function view(bytes){const pad=(4-length%4)%4;if(pad){chunks.push(Buffer.alloc(pad));length+=pad;}const index=doc.bufferViews.length;doc.bufferViews.push({buffer:0,byteOffset:length,byteLength:bytes.length});chunks.push(bytes);length+=bytes.length;return index;}
 function accessor(values,type,integer=false){const data=integer?new Uint16Array(values):new Float32Array(values);const index=doc.accessors.length;const count=values.length/({SCALAR:1,VEC2:2,VEC3:3,VEC4:4}[type]);const entry={bufferView:view(Buffer.from(data.buffer)),componentType:integer?5123:5126,count,type};if(type==='VEC3'){entry.min=[0,1,2].map(i=>Math.min(...values.filter((_,j)=>j%3===i)));entry.max=[0,1,2].map(i=>Math.max(...values.filter((_,j)=>j%3===i)));}doc.accessors.push(entry);return index;}
 for(const image of original.images){const v=original.bufferViews[image.bufferView],offset=v.byteOffset??0;doc.images.push({...image,bufferView:view(binary.subarray(offset,offset+v.byteLength))});}
 const indexAccessor=accessor(indices,'SCALAR',true),uvAccessor=accessor(used.flatMap(i=>[sourceGeometry.attributes.uv.getX(i),sourceGeometry.attributes.uv.getY(i)]),'VEC2'),legAccessor=accessor(legWeights,'VEC4');
 const mixer=new T.AnimationMixer(gltf.scene);
 for(const pose of ['idle','rest','forage']){
  mixer.stopAllAction();const clip=gltf.animations.find(c=>c.name===contract.clips[pose].name);if(!clip)throw Error('Far pose has no authored source');mixer.clipAction(clip).reset().play();mixer.setTime(0);gltf.scene.updateMatrixWorld(true);skin.skeleton.update();
  const positions=[],normals=[];
  for(const index of used){
   const point=new T.Vector3();skin.getVertexPosition(index,point);point.applyMatrix4(skin.matrixWorld);point.toArray(positions,positions.length);
   const p=new T.Vector3().fromBufferAttribute(sourceGeometry.attributes.position,index),n=new T.Vector3().fromBufferAttribute(sourceGeometry.attributes.normal,index);p.addScaledVector(n,.01);skin.applyBoneTransform(index,p);p.applyMatrix4(skin.matrixWorld).sub(point).normalize().toArray(normals,normals.length);
  }
  const name='LifeLensTigerFar'+pose[0].toUpperCase()+pose.slice(1),mesh=doc.meshes.length;
  doc.meshes.push({name,primitives:[{attributes:{POSITION:accessor(positions,'VEC3'),NORMAL:accessor(normals,'VEC3'),TEXCOORD_0:uvAccessor,_LIFELENS_LEG:legAccessor},indices:indexAccessor,material:0}]});doc.scenes.push({name,nodes:[doc.nodes.length]});doc.nodes.push({name,mesh});
 }
 contract.source_clips=contract.clips;contract.clips={};contract.paws=[];contract.head_joint='';contract.source_bone_count=47;contract.render_mode='instanced-static';contract.bone_count=0;contract.static_poses=['idle','rest','forage'];contract.triangles=indices.length/3;contract.asset_triangle_count=contract.triangles*3;contract.maximum_instances=128;doc.asset.extras.lifelensTiger=contract;
 doc.buffers[0].byteLength=length;let packed=Buffer.concat(chunks);packed=Buffer.concat([packed,Buffer.alloc((4-packed.length%4)%4)]);let json=Buffer.from(JSON.stringify(doc));json=Buffer.concat([json,Buffer.alloc((4-json.length%4)%4,32)]);
 const h=Buffer.alloc(20);h.write('glTF');h.writeUInt32LE(2,4);h.writeUInt32LE(28+json.length+packed.length,8);h.writeUInt32LE(json.length,12);h.writeUInt32LE(0x4e4f534a,16);const b=Buffer.alloc(8);b.writeUInt32LE(packed.length);b.writeUInt32LE(0x004e4942,4);
 return {bytes:Buffer.concat([h,json,b,packed]),contract};
}
