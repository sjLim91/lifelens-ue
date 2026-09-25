import {readFileSync,mkdtempSync,writeFileSync,rmSync} from 'node:fs';
import {pathToFileURL,fileURLToPath} from 'node:url';
import {join} from 'node:path';
import * as THREE from 'three';
import {GLTFLoader} from 'three/addons/loaders/GLTFLoader.js';
import ts from 'typescript';
const [modelPath, animationPath] = process.argv.slice(2);
if (!modelPath || !animationPath) throw new Error('Provide paths to the pinned model and animation GLBs');
const loader=new GLTFLoader();
async function load(path){const b=readFileSync(path);return loader.parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),'');}
const base=await load(modelPath), library=await load(animationPath);
const dir=mkdtempSync(fileURLToPath(new URL('./compiled-',import.meta.url)));
try {
 const out=join(dir,'appearance.mjs');writeFileSync(out,ts.transpileModule(readFileSync(new URL('../../src/render/resident-appearance.ts',import.meta.url),'utf8'),{compilerOptions:{module:ts.ModuleKind.ES2022}}).outputText);
 const api=await import(pathToFileURL(out));
 const source=base.scene;source.updateMatrixWorld(true);const box=new THREE.Box3().setFromObject(source),size=box.getSize(new THREE.Vector3()),center=box.getCenter(new THREE.Vector3());source.position.sub(new THREE.Vector3(center.x,box.min.y,center.z));const model=new THREE.Group();model.add(source);model.scale.setScalar(1/size.y);const visual=new THREE.Group();visual.add(model);
 const profile=api.createResidentAppearanceProfile({id:'actual-asset-check',sex:'Female',ageYears:26});api.applyResidentMaterialVariant(model,profile);
 if(visual.getObjectByName('LifeLensResidentHairVariant'))throw Error('rigid hair proxy remains');
 const walk=library.animations.find(c=>c.name==='Walk_Loop');const mixer=new THREE.AnimationMixer(visual);mixer.clipAction(walk).play();const head=visual.getObjectByName('Head');const first=head.getWorldPosition(new THREE.Vector3());mixer.update(0.2);visual.updateMatrixWorld(true);const next=head.getWorldPosition(new THREE.Vector3());
 const body=visual.getObjectByName('SuperHero_Male');const colors=body.geometry.getAttribute('color');if(!colors||!body.material.vertexColors)throw Error('actual body skin colors missing');
 const hairColor=new THREE.Color(profile.hairColor);let scalpVertices=0;for(let i=0;i<colors.count;i++){if(Math.abs(colors.getX(i)-hairColor.r)<1e-5&&Math.abs(colors.getY(i)-hairColor.g)<1e-5)scalpVertices++;}if(scalpVertices<10)throw Error('scalp coverage missing');
 console.log(JSON.stringify({modelSize:size.toArray(),scalpVertices,rigidHairProxy:false,headDisplacement:first.distanceTo(next),bodyColorVertices:colors.count,walkDuration:walk.duration,rootTracks:walk.tracks.filter(t=>t.name.startsWith('root.')).map(t=>({name:t.name,first:[...t.values.slice(0,4)],last:[...t.values.slice(-4)]}))},null,2));
} finally {rmSync(dir,{recursive:true,force:true});}
