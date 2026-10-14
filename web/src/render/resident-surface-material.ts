import * as THREE from 'three';
import type { ResidentAppearanceProfile } from './resident-appearance';

// Q1-A: deterministic procedural skin, not painted-on primitive clothing.
// This uses only the established Core-derived complexion, identity and age;
// a shared shader program keeps mobile compilation predictable.
export const SKIN_QUALITY_NAME='LifeLensResidentSkinQ1';
export function createResidentSkinMaterial(profile:ResidentAppearanceProfile):THREE.MeshStandardMaterial {
  const material=new THREE.MeshStandardMaterial({
    color:profile.skinColor,roughness:.78,metalness:0,side:THREE.FrontSide,
  });
  material.name=SKIN_QUALITY_NAME;
  const uniforms={
    llAge:{value:profile.ageWeathering01},
    llSeed:{value:(profile.seed%104729)/104729},
    llQuality:{value:1},
  };
  material.userData.llSkinUniforms=uniforms;
  material.onBeforeCompile=shader=>{
    Object.assign(shader.uniforms,uniforms);
    shader.vertexShader=shader.vertexShader.replace('#include <common>',
      `#include <common>
varying vec3 llSkinLocal;`);
    shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>',
      `#include <begin_vertex>
llSkinLocal=position;`);
    shader.fragmentShader=shader.fragmentShader.replace('#include <common>',
      `#include <common>
varying vec3 llSkinLocal;
uniform float llAge;
uniform float llSeed;
uniform float llQuality;
float llSkinHash(vec3 p){return fract(sin(dot(p,vec3(127.1,311.7,74.7)))*43758.5453);}
float llSkinNoise(vec3 p){
 vec3 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);
 return mix(mix(mix(llSkinHash(i),llSkinHash(i+vec3(1,0,0)),f.x),
   mix(llSkinHash(i+vec3(0,1,0)),llSkinHash(i+vec3(1,1,0)),f.x),f.y),
   mix(mix(llSkinHash(i+vec3(0,0,1)),llSkinHash(i+vec3(1,0,1)),f.x),
   mix(llSkinHash(i+vec3(0,1,1)),llSkinHash(i+vec3(1,1,1)),f.x),f.y),f.z);
}`);
    shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',
      `#include <color_fragment>
float freckles=llSkinNoise(llSkinLocal*74.0+llSeed*13.0);
float pores=llSkinNoise(llSkinLocal*218.0+llSeed*31.0);
float mottling=llSkinNoise(llSkinLocal*11.0+llSeed*19.0);
vec3 complexion=vec3(1.0);
complexion.r+=.032*(mottling-.5);
complexion.g+=.012*(pores-.5);
complexion.b-=.024*(mottling-.5);
float smallSpots=smoothstep(.84,.975,freckles);
complexion*=1.0-smallSpots*(.023+.035*llAge)*llQuality;
diffuseColor.rgb*=mix(vec3(1.0),complexion,llQuality);
`);
    shader.fragmentShader=shader.fragmentShader.replace('#include <roughnessmap_fragment>',
      `#include <roughnessmap_fragment>
roughnessFactor*=1.0+(llSkinNoise(llSkinLocal*145.0+llSeed*7.0)-.5)*.08*llQuality;
`);
  };
  material.customProgramCacheKey=()=> 'LifeLens-Resident-Skin-Quality-Q1-v1';
  return material;
}
/** LOD changes shader details only, never Core bodies, bones or equipment. */
export function setResidentSkinMaterialQuality(model:THREE.Object3D,cameraDistance:number):void {
  const body=model.getObjectByName('LifeLensAuthoredAnatomicalBody');
  if(!(body instanceof THREE.SkinnedMesh))return;
  const material=Array.isArray(body.material)?body.material[0]:body.material;
  const uniforms=material.userData.llSkinUniforms as {llQuality:{value:number}}|undefined;
  if(uniforms)uniforms.llQuality.value=cameraDistance<7?1:cameraDistance<18?.5:0;
}
