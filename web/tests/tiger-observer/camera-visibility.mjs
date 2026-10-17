import {Mesh,Matrix4,Raycaster,Vector3,FrontSide} from 'three';

// Camera-only review aid. Nearby obstacle copies share geometry but are never
// added to the live scene; no vegetation, animal, material or Core data changes.
export function chooseTigerCamera(scene,actor,view){
 const origin=actor.position.clone(),yaw=actor.rotation.y,scale=actor.scale.x;
 const points=[];actor.updateMatrixWorld(true);
 actor.traverse(object=>{
  if(object.isBone&&/Bip01_(Head_|Pelvis_|[LR]_(Hand_|Foot_))/.test(object.name))
   points.push(object.getWorldPosition(new Vector3()).add(new Vector3(0,.08*scale,0)));
 });
 if(!points.length)return null;
 const target=points.reduce((sum,p)=>sum.add(p),new Vector3()).multiplyScalar(1/points.length);
 const colliders=[],matrix=new Matrix4();
 scene.traverse(object=>{
  if(!object.isInstancedMesh||!object.geometry.getAttribute('llLeafRank')||object.material.side!==FrontSide)return;
  for(let i=0;i<object.count;i++){
   object.getMatrixAt(i,matrix);matrix.premultiply(object.matrixWorld);
   const p=new Vector3().setFromMatrixPosition(matrix);
   if(Math.hypot(p.x-origin.x,p.z-origin.z)>10)continue;
   const copy=new Mesh(object.geometry,object.material);copy.matrixAutoUpdate=false;copy.matrixWorld.copy(matrix);colliders.push(copy);
  }
 });
 const angle=yaw+({front:0,side:-Math.PI/2,quarter:-Math.PI/4,back:Math.PI}[view]??0);
 const ray=new Raycaster(),direction=new Vector3();let best=null;
 for(const deviation of [0,-.2,.2])for(const radius of [2.7,3.5])for(const height of [1.5,2.5]){
  const position=origin.clone().add(new Vector3(Math.sin(angle+deviation)*radius,height,Math.cos(angle+deviation)*radius));
  let clear=0;
  for(const point of points){direction.subVectors(point,position);ray.set(position,direction.clone().normalize());ray.near=.02;ray.far=direction.length()-.12;if(!ray.intersectObjects(colliders,false).length)clear++;}
  const score=clear-.1*Math.abs(deviation)-.001*height;
  if(!best||score>best.score)best={score,clear,total:points.length,position:position.sub(origin).toArray(),target:target.clone().sub(origin).toArray(),deviation};
 }
 return best;
}
