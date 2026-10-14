import * as THREE from 'three';
import { WildlifeLayer } from './review/src/render/wildlife-layer.mjs';
import { createGrassGeometry } from './review/src/render/procedural-asset-kit.mjs';
import { createShrubGeometry } from './review/src/render/procedural-tree-generator.mjs';
const scene = new THREE.Scene(); scene.background = new THREE.Color(0xcddcdc);
const camera = new THREE.PerspectiveCamera(38, innerWidth / innerHeight, .1, 100);
camera.position.set(3.4, 3.1, 6.5); camera.lookAt(0, .6, 0);
scene.add(new THREE.HemisphereLight(0xeaf5ff, 0x687750, 2.4));
const sun = new THREE.DirectionalLight(0xffe8c6, 3); sun.position.set(-4, 7, 5); scene.add(sun);
const renderer = new THREE.WebGLRenderer({antialias:true, preserveDrawingBuffer:true});
renderer.setSize(innerWidth, innerHeight); renderer.setPixelRatio(1);
renderer.toneMapping = THREE.ACESFilmicToneMapping; renderer.toneMappingExposure = 1;
document.body.appendChild(renderer.domElement);
const floor = new THREE.Mesh(new THREE.PlaneGeometry(100,100), new THREE.MeshStandardMaterial({color:0x637950,roughness:1}));
floor.rotation.x=-Math.PI/2; scene.add(floor);
const material = new THREE.MeshStandardMaterial({vertexColors:true,roughness:.92});
// Render the ACTUAL instanced material with coat shader, not a simplified
// gallery material that accidentally skips production shader compilation.
const animals=new WildlifeLayer();
for(const [i,species] of ['Hare','Deer','Wolf','FreshwaterFish','Tiger'].entries()){
  const mesh=animals.group.getObjectByName(`LifeLensWildlife:${species}:near`);
  if(!(mesh instanceof THREE.InstancedMesh))throw Error('Missing Q1 '+species);
  const position=new THREE.Object3D();
  position.position.set((i-2)*1.55,0,i===0?.45:0);
  position.rotation.y=.55;position.updateMatrix();
  mesh.setMatrixAt(0,position.matrix);
  mesh.count=1;mesh.instanceMatrix.needsUpdate=true;
  mesh.geometry.getAttribute('wildlifePhase').setX(0,i*.8);
  mesh.geometry.getAttribute('wildlifeMotion').setX(0,.35);
  mesh.geometry.getAttribute('wildlifeBehavior').setX(0,i===4?5:1);
  scene.add(mesh);
}
for(let i=0;i<12;i++) {
  const shrub = new THREE.Mesh(createShrubGeometry('gallery-'+i),material);
  shrub.position.set((i%6-2.5)*1.2,0,-1.8-Math.floor(i/6)); shrub.scale.setScalar(.55); scene.add(shrub);
}
const plants = new THREE.MeshStandardMaterial({vertexColors:true,side:THREE.DoubleSide,roughness:1});
for(let i=0;i<35;i++) {
  const grass = new THREE.Mesh(createGrassGeometry('gallery-'+i,7),plants);
  grass.position.set((i%7-3)*.65,0,-.6-Math.floor(i/7)*.4); grass.scale.setScalar(.5); scene.add(grass);
}
renderer.render(scene,camera);
window.wildlifeReview={ready:true,info:renderer.info.render,
 q1:{lods:3,productionShader:animals.material?.customProgramCacheKey?.()
   ?? animals.group.children[0].material.customProgramCacheKey(),
   species:5}};
