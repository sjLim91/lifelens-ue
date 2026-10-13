import * as THREE from 'three';
import type { TerrainWindow, WildlifeSpecies, WildlifeAnimalObservation } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT as grid } from '../runtime/lifelens-contract';
import { createWildlifeGeometry } from './procedural-wildlife';
import { ResidentGroundSurface } from './resident-grounding';
import { buildFlowWaterSurfaceGeometry, createVisibleWaterFootprintTester } from './water-geometry';
import { facilityPresentationFootprints, outsideFacilityFootprints } from './facility-layer';
import { terrainDressingSignature } from './terrain-dressing-signature';

const SPECIES: WildlifeSpecies[] = ['Hare', 'Deer', 'Wolf', 'FreshwaterFish', 'Tiger'];
const LIMIT=128;
const motionName=(animal: WildlifeAnimalObservation): string =>
  animal.species==='Deer' && animal.sex==='Male' ? 'DeerMale' : animal.species;
const behaviorCode=(name: WildlifeAnimalObservation['behavior']): number =>
  name==='Roam'?1:name==='Forage'?2:name==='Drink'?3:name==='Flee'?4
    :name==='Hunt'?5:name==='Swim'?6:0;
type Visible = {
  id: string; species: WildlifeSpecies; key: string; behavior: WildlifeAnimalObservation['behavior'];
  x: number; z: number; goalX: number; goalZ: number; heading: number; phase: number;
  scale: number; motion: number; fish: boolean;
};
const stablePhase=(id: string): number => {
  let h=2166136261>>>0;
  for(let i=0;i<id.length;i++) {h^=id.charCodeAt(i);h=Math.imul(h,16777619)>>>0;}
  return (h>>>0)/4294967296*Math.PI*2;
};

/** Core alone determines births, deaths, world coordinates and behavior.
 * Observer smooths ONLY between two confirmed Core positions, and animates
 * muscles/ears/fins in place without inventing movement or new animals.
 * Six batched geometries: deer male antlers use a separate instanced batch.
 */
export class WildlifeLayer {
  readonly group=new THREE.Group();
  private readonly time={value:0};
  private readonly material=new THREE.MeshStandardMaterial({vertexColors:true,roughness:.91});
  private readonly meshes=new Map<string,THREE.InstancedMesh>();
  private readonly ground=new ResidentGroundSurface();
  private readonly transform=new THREE.Object3D();
  private terrainSignature='';
  private centerChunkX=0;
  private centerChunkY=0;
  private water: THREE.Mesh | null=null;
  private readonly waterMaterial=new THREE.MeshBasicMaterial({side:THREE.DoubleSide});
  private readonly ray=new THREE.Raycaster();
  private readonly rayOrigin=new THREE.Vector3();
  private readonly down=new THREE.Vector3(0,-1,0);
  private animals=new Map<string,Visible>();
  private readonly scratch=new Map<string,number>();
  instanceCount=0;
  constructor(){
    // GPU articulation keeps visual fidelity without creating one actor/rig
    // per animal, even when 128 are near the mobile observer.
    this.material.onBeforeCompile=shader=>{
      shader.uniforms.wildlifeTime=this.time;
      shader.vertexShader=shader.vertexShader.replace('#include <common>',
        `#include <common>
attribute float wildlifePart;
attribute float wildlifePhase;
attribute float wildlifeMotion;
attribute float wildlifeBehavior;
uniform float wildlifeTime;`);
      shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>',
        `#include <begin_vertex>
float beat=wildlifeTime*8.0+wildlifePhase;
float gait=wildlifeMotion;
if(wildlifePart>0.5 && wildlifePart<2.5) {
  float alternate=wildlifePart<1.5?1.0:-1.0;
  transformed.z+=gait*0.095*sin(beat+alternate*1.5707963)
    *clamp(1.1-position.y,0.0,1.0);
  transformed.y+=gait*0.045*abs(sin(beat+alternate*1.5707963))
    *clamp(1.0-position.y,0.0,1.0);
} else if(wildlifePart>2.5 && wildlifePart<3.5) {
  transformed.x+=0.055*sin(wildlifeTime*3.0+wildlifePhase+position.z*2.0);
} else if(wildlifePart>3.5 && wildlifePart<4.5) {
  float forage=step(1.5,wildlifeBehavior)*(1.0-step(3.5,wildlifeBehavior));
  transformed.y+=forage*0.045*sin(wildlifeTime*2.5+wildlifePhase);
  transformed.x+=0.015*sin(wildlifeTime*1.7+wildlifePhase);
} else if(wildlifePart>4.5) {
  transformed.x+=0.065*sin(wildlifeTime*5.0+wildlifePhase+position.z*6.0);
}`);
    };
    this.material.customProgramCacheKey=()=> 'lifelens-wildlife-v2-articulation';
    for(const species of [...SPECIES,'DeerMale'] as string[]){
      const male=species==='DeerMale';
      const geo=createWildlifeGeometry(male?'Deer':species as WildlifeSpecies,
        male?'Male':'Female');
      geo.setAttribute('wildlifePhase',new THREE.InstancedBufferAttribute(new Float32Array(LIMIT),1));
      geo.setAttribute('wildlifeMotion',new THREE.InstancedBufferAttribute(new Float32Array(LIMIT),1));
      geo.setAttribute('wildlifeBehavior',new THREE.InstancedBufferAttribute(new Float32Array(LIMIT),1));
      const mesh=new THREE.InstancedMesh(geo,this.material,LIMIT);
      mesh.count=0;
      mesh.frustumCulled=false;
      mesh.castShadow=species!=='FreshwaterFish';
      mesh.receiveShadow=true;
      this.meshes.set(species,mesh);
      this.group.add(mesh);
    }
  }
  setTerrain(terrain: TerrainWindow): void {
    const signature=terrainDressingSignature(terrain);
    if(signature!==this.terrainSignature) {
      this.terrainSignature=signature;
      this.ground.setTerrain(terrain,terrain.centerChunkX,terrain.centerChunkY);
      this.water?.geometry.dispose();
      this.water=new THREE.Mesh(buildFlowWaterSurfaceGeometry(terrain),this.waterMaterial);
      this.water.updateMatrixWorld();
    }
    this.centerChunkX=terrain.centerChunkX;
    this.centerChunkY=terrain.centerChunkY;
    const insideWater=createVisibleWaterFootprintTester(terrain);
    const footprints=facilityPresentationFootprints(terrain);
    const input=terrain.available && terrain.wildlife?.available===true
      && Array.isArray(terrain.wildlife.animals)?terrain.wildlife.animals:[];
    const distance=(a:WildlifeAnimalObservation)=>
      (a.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)**2+
      (a.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)**2;
    const next=new Map<string,Visible>();
    for(const animal of input.slice().sort((a,b)=>distance(a)-distance(b)||a.id.localeCompare(b.id))) {
      if(next.size>=LIMIT)break;
      const key=motionName(animal);
      if(!this.meshes.has(key)||!animal.id||next.has(animal.id)
        ||!Number.isFinite(animal.gridX)||!Number.isFinite(animal.gridY)
        ||!Number.isFinite(animal.ageDays)||animal.ageDays<0)continue;
      const x=(animal.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
      const z=(animal.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
      if(this.ground.height(x,z)===null)continue;
      const fish=animal.species==='FreshwaterFish';
      if(fish!==insideWater(x,z)||(!fish&&!outsideFacilityFootprints(x,z,footprints,.35)))continue;
      const maturity=animal.species==='Deer'?420:animal.species==='Wolf'?600
        :animal.species==='Tiger'?900:animal.species==='Hare'?35:75;
      const scale=.45+.55*Math.min(1,animal.ageDays/maturity);
      const goalX=animal.gridX/grid.gridCellsPerChunk*grid.worldUnitsPerChunk;
      const goalZ=animal.gridY/grid.gridCellsPerChunk*grid.worldUnitsPerChunk;
      const old=this.animals.get(animal.id);
      const pose: Visible=old&&old.key===key?old:{
        id:animal.id,species:animal.species,key,behavior:animal.behavior,
        x:goalX,z:goalZ,goalX,goalZ,heading:0,phase:stablePhase(animal.id),
        scale,motion:0,fish,
      };
      pose.goalX=goalX;pose.goalZ=goalZ;pose.scale=scale;
      pose.behavior=animal.behavior;pose.fish=fish;
      next.set(animal.id,pose);
    }
    this.animals=next;
    this.render();
  }
  update(dtSeconds: number): void {
    const dt=THREE.MathUtils.clamp(Number.isFinite(dtSeconds)?dtSeconds:0,0,.1);
    this.time.value+=dt;
    const speed=2.5;
    for(const pose of this.animals.values()){
      const dx=pose.goalX-pose.x,dz=pose.goalZ-pose.z;
      const distance=Math.hypot(dx,dz);
      if(distance>1e-4){
        const step=Math.min(distance,dt*speed);
        pose.x+=dx/distance*step;pose.z+=dz/distance*step;
        pose.heading=Math.atan2(dx,dz);
        pose.motion=THREE.MathUtils.lerp(pose.motion,Math.min(1,.5+step/Math.max(dt,.0001)*.12),
          1-Math.exp(-dt*9));
      }else{
        pose.x=pose.goalX;pose.z=pose.goalZ;
        pose.motion*=Math.exp(-dt*8);
      }
    }
    this.render();
  }
  private render(): void {
    this.scratch.clear();
    this.instanceCount=0;
    const offsetX=(this.centerChunkX+.5)*grid.worldUnitsPerChunk;
    const offsetZ=(this.centerChunkY+.5)*grid.worldUnitsPerChunk;
    for(const pose of this.animals.values()){
      const mesh=this.meshes.get(pose.key)!;
      const i=this.scratch.get(pose.key)??0;
      if(i>=LIMIT)continue;
      const x=pose.x-offsetX,z=pose.z-offsetZ;
      let y=this.ground.height(x,z);
      if(y===null)continue;
      if(pose.fish) {
        if(!this.water)continue;
        this.rayOrigin.set(x,1000,z);
        this.ray.set(this.rayOrigin,this.down);
        const hit=this.ray.intersectObject(this.water)[0];
        const h=mesh.geometry.boundingBox!.max.y*pose.scale;
        if(!hit||hit.point.y-y<h+.04)continue;
        y=hit.point.y-h-.02;
      }else{
        const width=pose.species==='Hare'?.16:pose.species==='Deer'?.21:.25;
        const length=pose.species==='Hare'?.3:pose.species==='Deer'?.6:.52;
        for(const dx of [-width,width])for(const dz of [-length,length])
          y=Math.max(y,this.ground.height(x+dx,z+dz)??y);
      }
      // Only actual observed movement advances X/Z. Idle ear, tail and fin
      // animation is localized to the existing authoritative animal.
      this.transform.position.set(x,y,z);
      this.transform.rotation.set(0,pose.heading,0);
      this.transform.scale.setScalar(pose.scale);
      this.transform.updateMatrix();
      mesh.setMatrixAt(i,this.transform.matrix);
      const geo=mesh.geometry;
      (geo.getAttribute('wildlifePhase') as THREE.InstancedBufferAttribute).setX(i,pose.phase);
      (geo.getAttribute('wildlifeMotion') as THREE.InstancedBufferAttribute).setX(i,
        pose.fish?.45:pose.motion);
      (geo.getAttribute('wildlifeBehavior') as THREE.InstancedBufferAttribute).setX(i,
        behaviorCode(pose.behavior));
      this.scratch.set(pose.key,i+1);
      this.instanceCount++;
    }
    for(const [key,mesh] of this.meshes){
      mesh.count=this.scratch.get(key)??0;
      mesh.instanceMatrix.needsUpdate=true;
      for(const name of ['wildlifePhase','wildlifeMotion','wildlifeBehavior'])
        (mesh.geometry.getAttribute(name) as THREE.InstancedBufferAttribute).needsUpdate=true;
    }
  }
  dispose(): void {
    for(const mesh of this.meshes.values()){mesh.geometry.dispose();mesh.dispose();}
    this.meshes.clear();this.animals.clear();
    this.water?.geometry.dispose();this.waterMaterial.dispose();this.material.dispose();
    this.group.clear();
  }
}
