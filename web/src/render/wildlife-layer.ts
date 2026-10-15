import * as THREE from 'three';
import type { TerrainWindow, WildlifeSpecies, WildlifeAnimalObservation } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT as grid } from '../runtime/lifelens-contract';
import { createWildlifeGeometry } from './procedural-wildlife';
type WildlifeLod = 'near' | 'mid' | 'far';
import { ResidentGroundSurface } from './resident-grounding';
import { buildFlowWaterSurfaceGeometry, createVisibleWaterFootprintTester } from './water-geometry';
import { facilityPresentationFootprints, outsideFacilityFootprints } from './facility-layer';
import { terrainDressingSignature } from './terrain-dressing-signature';
import { ImportedTigerPool, type TigerAssetLoader } from './imported-tiger';

// Actual 3D anatomy has three levels of detail. Near uses fused surfaces and
// shader fur, mid reduces topology and far preserves just essential silhouette.
const SPECIES:WildlifeSpecies[]=['Hare','Deer','Wolf','FreshwaterFish','Tiger'];
const LODS:WildlifeLod[]=['near','mid','far'];
const LIMIT=128;
const keyOf=(animal:WildlifeAnimalObservation)=>
  animal.species==='Deer'&&animal.sex==='Male'?'DeerMale':animal.species;
const behaviorCode=(name:WildlifeAnimalObservation['behavior'])=>
  name==='Roam'?1:name==='Forage'?2:name==='Drink'?3:name==='Flee'?4
    :name==='Hunt'?5:name==='Swim'?6:0;
type Visible = {
  id:string;species:WildlifeSpecies;key:string;behavior:WildlifeAnimalObservation['behavior'];
  x:number;z:number;goalX:number;goalZ:number;heading:number;phase:number;
  scale:number;motion:number;speed:number;fish:boolean;
};
const stablePhase=(id:string):number=>{
  let h=2166136261>>>0;
  for(let i=0;i<id.length;i++){h^=id.charCodeAt(i);h=Math.imul(h,16777619)>>>0;}
  return (h>>>0)/4294967296*Math.PI*2;
};

export class WildlifeLayer {
  readonly group=new THREE.Group();
  private readonly time={value:0};
  private readonly material=new THREE.MeshStandardMaterial({
    vertexColors:true,roughness:.86,metalness:0,side:THREE.FrontSide,
  });
  private readonly batches=new Map<string,THREE.InstancedMesh>();
  private readonly ground=new ResidentGroundSurface();
  private readonly groundHeight=(x:number,z:number)=>this.ground.height(x,z);
  private readonly transform=new THREE.Object3D();
  private terrainSignature='';
  private centerChunkX=0;
  private centerChunkY=0;
  private water:THREE.Mesh|null=null;
  private readonly waterMaterial=new THREE.MeshBasicMaterial({side:THREE.DoubleSide});
  private readonly ray=new THREE.Raycaster();
  private readonly rayOrigin=new THREE.Vector3();
  private readonly down=new THREE.Vector3(0,-1,0);
  private animals=new Map<string,Visible>();
  private readonly scratch=new Map<string,number>();
  private readonly cameraPosition=new THREE.Vector3();
  private readonly cameraAbsolute=new THREE.Vector3();
  private hasCamera=false;
  private readonly tigers:ImportedTigerPool;
  instanceCount=0;
  constructor(tigerLoader?:TigerAssetLoader){
    this.tigers=new ImportedTigerPool(tigerLoader);
    // Fragment-space stripes and fur variation belong to the animal surface
    // itself. The earlier tiger's black tubular stripes looked like cords.
    this.material.onBeforeCompile=shader=>{
      shader.uniforms.wildlifeTime=this.time;
      shader.vertexShader=shader.vertexShader.replace('#include <common>',
        `#include <common>
attribute float wildlifePart;
attribute float wildlifeSpeciesId;
attribute float wildlifePhase;
attribute float wildlifeMotion;
attribute float wildlifeBehavior;
varying vec3 vWildlifeLocal;
varying float vWildlifeSpecies;
varying float vWildlifePart;
uniform float wildlifeTime;`);
      shader.vertexShader=shader.vertexShader.replace('#include <begin_vertex>',
        `#include <begin_vertex>
vWildlifeLocal=position;
vWildlifeSpecies=wildlifeSpeciesId;
vWildlifePart=wildlifePart;
float beat=wildlifeTime*8.0+wildlifePhase;
float gait=wildlifeMotion;
if(wildlifePart>0.5&&wildlifePart<2.5){
  float alternate=wildlifePart<1.5?1.0:-1.0;
  float attachment=(1.0-smoothstep(0.1,0.99,position.y));
  transformed.z+=gait*0.065*sin(beat+alternate*1.5707963)*attachment;
  transformed.y+=gait*0.029*abs(sin(beat+alternate*1.5707963))*attachment;
}else if(wildlifePart>2.5&&wildlifePart<3.5){
  transformed.x+=0.055*sin(wildlifeTime*2.6+wildlifePhase+position.z*2.0);
}else if(wildlifePart>3.5&&wildlifePart<4.5){
  float forage=step(1.5,wildlifeBehavior)*(1.0-step(3.5,wildlifeBehavior));
  transformed.y+=forage*0.027*sin(wildlifeTime*2.5+wildlifePhase);
}else if(wildlifePart>4.5){
  transformed.x+=0.052*sin(wildlifeTime*5.0+wildlifePhase+position.z*6.0);
}`);
      shader.fragmentShader=shader.fragmentShader.replace('#include <common>',
        `#include <common>
varying vec3 vWildlifeLocal;
varying float vWildlifeSpecies;
varying float vWildlifePart;`);
      shader.fragmentShader=shader.fragmentShader.replace('#include <color_fragment>',
        `#include <color_fragment>
// Aperiodic coat micro-variation: fur rather than a monochrome wooden shell.
// All calculations are deterministic from the generated body's local surface.
float micro=sin(dot(vWildlifeLocal,vec3(227.3,347.7,151.1)))
  *sin(dot(vWildlifeLocal,vec3(137.7,229.3,311.7)));
diffuseColor.rgb*=1.0+micro*0.062;
if(vWildlifeSpecies>3.5){
  // Tiger: stripes are a pigmentation mask on the skinned continuous coat,
// not freestanding dark tubes. Vary width/phase and taper over shoulders.
  float axis=vWildlifeLocal.z*43.0+vWildlifeLocal.y*14.5
    +sin(vWildlifeLocal.x*9.0+vWildlifeLocal.z*5.0)*1.05;
  float stripes=smoothstep(.44,.73,sin(axis));
  float underside=smoothstep(.43,.63,vWildlifeLocal.y);
  stripes*=underside;
  diffuseColor.rgb=mix(diffuseColor.rgb,
    diffuseColor.rgb*vec3(.17,.14,.12),stripes*.94);
}else if(vWildlifeSpecies>1.5&&vWildlifeSpecies<2.5){
  // Grey-backed wolf with pale chest, subtle irregular guard hair.
  diffuseColor.rgb*=mix(1.075,.88,
    smoothstep(.48,1.05,vWildlifeLocal.y));
}else if(vWildlifeSpecies>2.5&&vWildlifeSpecies<3.5){
  float scales=.025*sin(vWildlifeLocal.z*124.0+sin(vWildlifeLocal.y*94.0));
  diffuseColor.rgb*=1.0+scales;
}
// Face detail sits on the existing fused skull surface rather than hovering
// as separate bead eyes. Symmetric masks include an iris and small highlight.
vec3 eyeCenter=vec3(.115,.42,.305);
vec3 eyeSize=vec3(.05,.06,.09);
vec3 noseCenter=vec3(0.0,.33,.43);
if(vWildlifeSpecies>3.5){
  eyeCenter=vec3(.195,1.22,.68);eyeSize=vec3(.070,.075,.13);
  noseCenter=vec3(0.0,.97,.94);
}else if(vWildlifeSpecies>2.5){
  eyeCenter=vec3(.083,.2,.255);eyeSize=vec3(.044,.048,.082);
  noseCenter=vec3(0.0,.16,.34);
}else if(vWildlifeSpecies>1.5){
  eyeCenter=vec3(.17,.99,.66);eyeSize=vec3(.060,.070,.11);
  noseCenter=vec3(0.0,.83,.94);
}else if(vWildlifeSpecies>.5){
  eyeCenter=vec3(.16,1.43,.76);eyeSize=vec3(.062,.075,.11);
  noseCenter=vec3(0.0,1.34,.98);
}
vec3 eyeOffset=vec3(abs(vWildlifeLocal.x),vWildlifeLocal.y,vWildlifeLocal.z)-eyeCenter;
float eyeDistance=length(eyeOffset/eyeSize);
float iris=1.0-smoothstep(.60,.96,eyeDistance);
float sparkle=1.0-smoothstep(.10,.31,length((eyeOffset-vec3(-eyeSize.x*.18,eyeSize.y*.24,eyeSize.z*.3))/eyeSize));
diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.065,.058,.038),iris*.90);
diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.98,.89,.73),sparkle*iris*.75);
vec3 noseDist=(vWildlifeLocal-noseCenter)/vec3(.115,.075,.13);
float noseMask=1.0-smoothstep(.43,.9,length(noseDist));
diffuseColor.rgb=mix(diffuseColor.rgb,vec3(.17,.145,.13),noseMask*.74);
`);
    };
    this.material.customProgramCacheKey=()=> 'lifelens-q1-fused-wildlife-triplanar-v1';
    // Stable gallery/test order: the near instances remain the first 6 meshes.
    for(const lod of LODS)for(const species of [...SPECIES,'DeerMale']){
      const male=species==='DeerMale';
      const real=male?'Deer':species as WildlifeSpecies;
      const geometry=createWildlifeGeometry(real,male?'Male':'Female',lod);
      for(const attr of ['wildlifePhase','wildlifeMotion','wildlifeBehavior'])
        geometry.setAttribute(attr,new THREE.InstancedBufferAttribute(new Float32Array(LIMIT),1));
      const mesh=new THREE.InstancedMesh(geometry,this.material,LIMIT);
      mesh.name=`LifeLensWildlife:${species}:${lod}`;
      mesh.count=0;mesh.frustumCulled=false;
      mesh.castShadow=real!=='FreshwaterFish';mesh.receiveShadow=true;
      this.batches.set(`${species}:${lod}`,mesh);
      this.group.add(mesh);
    }
  }
  setTerrain(terrain:TerrainWindow):void {
    const signature=terrainDressingSignature(terrain);
    if(signature!==this.terrainSignature){
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
    const input=terrain.available&&terrain.wildlife?.available===true
      &&Array.isArray(terrain.wildlife.animals)?terrain.wildlife.animals:[];
    const distance=(a:WildlifeAnimalObservation)=>
      (a.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)**2
      +(a.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)**2;
    const next=new Map<string,Visible>();
    for(const animal of input.slice().sort((a,b)=>distance(a)-distance(b)||a.id.localeCompare(b.id))){
      if(next.size>=LIMIT)break;
      const key=keyOf(animal);
      if(!this.batches.has(`${key}:near`)||!animal.id||next.has(animal.id)
        ||!Number.isFinite(animal.gridX)||!Number.isFinite(animal.gridY)
        ||!Number.isFinite(animal.ageDays)||animal.ageDays<0)continue;
      const relativeX=(animal.gridX/grid.gridCellsPerChunk-terrain.centerChunkX-.5)*grid.worldUnitsPerChunk;
      const relativeZ=(animal.gridY/grid.gridCellsPerChunk-terrain.centerChunkY-.5)*grid.worldUnitsPerChunk;
      if(this.ground.height(relativeX,relativeZ)===null)continue;
      const fish=animal.species==='FreshwaterFish';
      if(fish!==insideWater(relativeX,relativeZ)
        ||(!fish&&!outsideFacilityFootprints(relativeX,relativeZ,footprints,.35)))continue;
      const maturity=animal.species==='Deer'?420:animal.species==='Wolf'?600
        :animal.species==='Tiger'?900:animal.species==='Hare'?35:75;
      const scale=.45+.55*Math.min(1,animal.ageDays/maturity);
      const goalX=animal.gridX/grid.gridCellsPerChunk*grid.worldUnitsPerChunk;
      const goalZ=animal.gridY/grid.gridCellsPerChunk*grid.worldUnitsPerChunk;
      const old=this.animals.get(animal.id);
      const pose:Visible=old&&old.key===key?old:{
        id:animal.id,species:animal.species,key,behavior:animal.behavior,
        x:goalX,z:goalZ,goalX,goalZ,heading:0,phase:stablePhase(animal.id),
        scale,motion:0,speed:0,fish,
      };
      pose.goalX=goalX;pose.goalZ=goalZ;pose.scale=scale;
      pose.behavior=animal.behavior;pose.fish=fish;
      next.set(animal.id,pose);
    }
    this.animals=next;
    this.render();
  }
  update(dtSeconds:number,camera?:THREE.Camera):void {
    const dt=THREE.MathUtils.clamp(Number.isFinite(dtSeconds)?dtSeconds:0,0,.1);
    this.time.value+=dt;
    const speed=grid.worldUnitsPerChunk/grid.gridCellsPerChunk*.75;
    for(const pose of this.animals.values()){
      const dx=pose.goalX-pose.x,dz=pose.goalZ-pose.z;
      const distance=Math.hypot(dx,dz);
      if(distance>1e-4){
        const step=Math.min(distance,dt*speed);
        pose.speed=dt>0?step/dt:0;
        pose.x+=dx/distance*step;pose.z+=dz/distance*step;
        pose.heading=Math.atan2(dx,dz);
        pose.motion=THREE.MathUtils.lerp(pose.motion,
          Math.min(1,.5+step/Math.max(dt,.0001)*.12),1-Math.exp(-dt*9));
      }else{
        pose.x=pose.goalX;pose.z=pose.goalZ;
        pose.speed=0;
        pose.motion*=Math.exp(-dt*8);
      }
    }
    this.render(camera,dt);
  }
  private render(camera?:THREE.Camera,dt=0):void {
    this.scratch.clear();this.instanceCount=0;
    this.tigers.begin();
    const offsetX=(this.centerChunkX+.5)*grid.worldUnitsPerChunk;
    const offsetZ=(this.centerChunkY+.5)*grid.worldUnitsPerChunk;
    if(camera){
      camera.updateWorldMatrix(true,false);camera.getWorldPosition(this.cameraAbsolute);
      this.cameraAbsolute.x+=offsetX;this.cameraAbsolute.z+=offsetZ;this.hasCamera=true;
    }
    this.cameraPosition.copy(this.cameraAbsolute);
    this.cameraPosition.x-=offsetX;this.cameraPosition.z-=offsetZ;
    // Spend the bounded skinned-model budget on the animals nearest the observer.
    const visible=[...this.animals.values()];
    if(this.hasCamera)visible.sort((a,b)=>
      Math.hypot(a.x-offsetX-this.cameraPosition.x,a.z-offsetZ-this.cameraPosition.z)
      -Math.hypot(b.x-offsetX-this.cameraPosition.x,b.z-offsetZ-this.cameraPosition.z));
    for(const pose of visible){
      const x=pose.x-offsetX,z=pose.z-offsetZ;
      let y=this.ground.height(x,z);
      if(y===null)continue;
      const cameraDistance=this.hasCamera
        ?Math.hypot(x-this.cameraPosition.x,z-this.cameraPosition.z,
          this.cameraPosition.y-y-.5):0;
      const lod:WildlifeLod=cameraDistance>25?'far':cameraDistance>11?'mid':'near';
      const mesh=this.batches.get(`${pose.key}:${lod}`)!;
      const i=this.scratch.get(mesh.name)??0;
      if(i>=LIMIT)continue;
      if(pose.fish){
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
      if(pose.species==='Tiger'&&lod!=='far'&&this.tigers.draw({
        id:pose.id,x,y,z,heading:pose.heading,scale:pose.scale,speed:pose.speed,
        phase:pose.phase,behavior:pose.behavior,
      },lod,dt,this.groundHeight)){this.instanceCount++;continue;}
      this.transform.position.set(x,y,z);
      this.transform.rotation.set(0,pose.heading,0);
      this.transform.scale.setScalar(pose.scale);
      this.transform.updateMatrix();
      mesh.setMatrixAt(i,this.transform.matrix);
      const geometry=mesh.geometry;
      (geometry.getAttribute('wildlifePhase') as THREE.InstancedBufferAttribute).setX(i,pose.phase);
      (geometry.getAttribute('wildlifeMotion') as THREE.InstancedBufferAttribute).setX(i,
        pose.fish?.45:pose.motion);
      (geometry.getAttribute('wildlifeBehavior') as THREE.InstancedBufferAttribute).setX(i,
        behaviorCode(pose.behavior));
      this.scratch.set(mesh.name,i+1);this.instanceCount++;
    }
    this.tigers.end(this.group);
    for(const mesh of this.batches.values()){
      mesh.count=this.scratch.get(mesh.name)??0;
      mesh.instanceMatrix.needsUpdate=true;
      for(const key of ['wildlifePhase','wildlifeMotion','wildlifeBehavior'])
        (mesh.geometry.getAttribute(key) as THREE.InstancedBufferAttribute).needsUpdate=true;
    }
  }
  dispose():void {
    this.tigers.dispose();
    for(const mesh of this.batches.values()){mesh.geometry.dispose();mesh.dispose();}
    this.batches.clear();this.animals.clear();
    this.water?.geometry.dispose();this.waterMaterial.dispose();this.material.dispose();
    this.group.clear();
  }
}

