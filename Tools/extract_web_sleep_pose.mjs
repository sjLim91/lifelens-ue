// Reproduce the zero-download sleep endpoint derivative after preparing pinned assets.
import { createHash } from 'node:crypto';
import { readFileSync,writeFileSync } from 'node:fs';
import { GLTFLoader } from '../web/node_modules/three/examples/jsm/loaders/GLTFLoader.js';
const b=readFileSync(new URL('../web/public/vendor/characters/ual2.glb',import.meta.url));const digest=createHash('sha1').update(Buffer.from(`blob ${b.length}\0`)).update(b).digest('hex');
if(digest!=='dc684c2a664927964307e8eb7b27b0000ebf6a18')throw Error('Pinned UAL2 blob mismatch');
const gltf=await new GLTFLoader().parseAsync(b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength),'');
const clip=gltf.animations.find(c=>c.name==='LayToIdle');
const tracks=clip.tracks.map(t=>{const size=t.getValueSize(),f=t.createInterpolant(),start=Array.from(f.evaluate(0)),end=Array.from(f.evaluate(clip.duration));return {name:t.name,type:t.ValueTypeName,times:[0,clip.duration],values:[...start,...end],size};});
const destination=new URL('../web/src/render/resident-sleep-pose.ts',import.meta.url);
const generated='// Two endpoint poses extracted from the pinned CC0 UAL2 LayToIdle.\n// Source blob dc684c2a664927964307e8eb7b27b0000ebf6a18; see docs/RESIDENT_MOTION_PASS.md.\n// Used only when the animation library is unavailable; no standing sleep fallback.\nexport const RESIDENT_SLEEP_ENDPOINTS = '+JSON.stringify({duration:clip.duration,tracks},null,0)+' as const;\n';
if(process.argv.includes('--check')){if(readFileSync(destination,'utf8')!==generated)throw Error('Sleep endpoint derivative differs from pinned GLB');}
else writeFileSync(destination,generated);console.log('Extracted',tracks.length,'verified endpoint tracks; duration',clip.duration);
