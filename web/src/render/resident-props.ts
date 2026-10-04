import { WORLD_PRESENTATION } from './world-presentation-config';
import * as THREE from 'three';
import { knownProductionMaterial, ownedMaterialUnits, PRODUCTION_MATERIALS } from './production-material-presentation';
import { registeredProductionDirective } from './resident-production-context';
import type { ResidentCivilizationItem, ResidentPresentationDirective } from '../runtime/core-types';

const DISPLAY_ITEMS = [
  'DiggingStick', 'StoneCuttingTool', 'StoneHammer', 'BronzeAxe', 'BronzePick',
  'SimpleContainer', 'SharpFlake', 'Cordage', 'FuelBundle', 'RecordTablet',
] as const;

/** Possession, not equipped-tool authority. Unknown inventories show nothing. */
export function residentVisibleItems(inventory: ResidentCivilizationItem[] | undefined): string[] {
  const held = new Set((inventory ?? [])
    .filter(stack => Number.isFinite(stack.quantity) && (stack.quantity ?? 0) > 0)
    .map(stack => stack.item));
  // Bound draw calls and avoid a fan of every historic possession. Order is stable.
  return DISPLAY_ITEMS.filter(item => held.has(item)).slice(0, 3);
}

/** Actual possession sample, never a promise of the directive's output/cargo. */
export function residentCarriedMaterial(inventory: ResidentCivilizationItem[] | undefined, p: ResidentPresentationDirective | null | undefined): string | null {
  if (!p?.active || !['Moving','Interacting'].includes(p.phase ?? '')
    || !['Civilization','Trade','Physical','Social','Parenting','KnowledgeTeaching'].includes(p.kind ?? '')) return null;
  if (p.kind === 'Civilization' && (!registeredProductionDirective(p) || (p.civilizationMaterial && p.civilizationMaterial !== 'Unknown' && !knownProductionMaterial(p.civilizationMaterial)))) return null;
  if (p.phase === 'Interacting' && (p.kind !== 'Civilization'
    || !(p.civilizationIntent === 'Store' || ['DeliverMaterial','Fuel','LoadSmeltCharge','Plant','Water'].includes(p.facilityAction ?? '')))) return null;
  const usable = (material: string): boolean => ownedMaterialUnits(inventory, material) > 0
    && (material !== 'Water' || (inventory ?? []).some(s => s.item === 'SimpleContainer' && Number.isFinite(s.quantity) && s.quantity! > 0));
  if (p.kind === 'Civilization' && knownProductionMaterial(p.civilizationMaterial)
    && usable(p.civilizationMaterial)) return p.civilizationMaterial;
  if (p.phase === 'Interacting') {
    const material = p.facilityAction === 'Water' ? 'Water' : p.facilityAction === 'Plant' ? 'PlantFood' : null;
    return material && usable(material) ? material : null;
  }
  return Object.keys(PRODUCTION_MATERIALS).find(usable) ?? null;
}

export function residentPouringWater(inventory: ResidentCivilizationItem[] | undefined, p: ResidentPresentationDirective | null | undefined): boolean {
  return Boolean(p?.active && p.kind === 'Civilization' && p.phase === 'Interacting'
    && p.facilityKind === 'CultivatedPlot' && p.facilityAction === 'Water'
    && inventory?.some(s => s.item === 'SimpleContainer' && Number.isFinite(s.quantity) && (s.quantity ?? 0) > 0)
    && inventory?.some(s => s.item === 'RawMaterial' && s.material === 'Water' && Number.isFinite(s.quantity) && (s.quantity ?? 0) > 0));
}

/** Original low-poly props, sized in normalized resident-height units. */
export function createResidentProp(item: string): THREE.Group {
  const group = new THREE.Group();
  if (!DISPLAY_ITEMS.some(known => known === item)) return group;
  group.name = `LifeLensCarried_${item}`;
  const wood = 0x715038, stone = 0x858a89, bronze = 0xa58648, fiber = 0xb69c6b;
  function part(geometry: THREE.BufferGeometry, color: number, x=0, y=0, z=0): THREE.Mesh {
    const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color, roughness: .85 }));
    mesh.position.set(x,y,z); mesh.castShadow = true;
    mesh.userData.residentOwnsGeometry = true;
    group.add(mesh); return mesh;
  }
  if (item === 'SimpleContainer') {
    const profile = [[0,-.075],[.05,-.07],[.068,-.025],[.058,.035],[.033,.065],[.033,.08],[.023,.08],[.024,.057],[.045,.027],[.052,-.025],[.04,-.057],[0,-.06]];
    part(new THREE.LatheGeometry(profile.map(([x,y]) => new THREE.Vector2(x,y)), 12), 0xa16e4d);
  } else if (item === 'SharpFlake') {
    const flake = part(new THREE.OctahedronGeometry(.065), stone);
    flake.scale.set(.55,1.4,.22);
  } else if (item === 'Cordage') {
    part(new THREE.TorusGeometry(.043,.012,5,16), fiber);
    part(new THREE.TorusGeometry(.036,.008,5,16), fiber,0,0,.014);
  } else if (item === 'FuelBundle') {
    for (let i=0;i<3;i++) part(new THREE.CylinderGeometry(.018,.021,.21,6),wood,(i-1)*.033);
    const tie=part(new THREE.TorusGeometry(.053,.007,4,12),fiber);tie.rotation.x=Math.PI/2;
  } else if (item === 'RecordTablet') {
    part(new THREE.BoxGeometry(.105,.14,.025),0xad8561);
    for(let i=0;i<3;i++) part(new THREE.BoxGeometry(.065,.005,.002),0x795b42,0,(i-1)*.027,.013);
  } else {
    const long = item === 'DiggingStick';
    part(new THREE.CylinderGeometry(.012,.009,long ? .57 : .29,7),wood);
    if (long) {
      const tip=part(new THREE.ConeGeometry(.012,.075,7),0x473427,0,-.32);
      tip.rotation.z=Math.PI;
    } else if (item === 'StoneHammer') {
      const head=part(new THREE.DodecahedronGeometry(.07,0),stone,0,.12);head.scale.set(1.3,.7,.65);
    } else if (item === 'BronzePick') {
      const head=part(new THREE.ConeGeometry(.033,.20,4),bronze,.025,.12);head.rotation.z=Math.PI/2;
    } else {
      const head=part(new THREE.OctahedronGeometry(.085),item==='BronzeAxe'?bronze:stone,.035,.11);
      head.scale.set(1,.8,.25);head.rotation.z=.45;
    }
    if (!long) {
      const tie=part(new THREE.TorusGeometry(.023,.006,4,10),fiber,0,.1);tie.rotation.x=Math.PI/2;
    }
  }
  return group;
}

/** One material and a finite catalog per ResidentWorldLayer, never per refresh. */
export class ResidentPropResources {
  readonly material = new THREE.MeshStandardMaterial({vertexColors:true,roughness:.85});
  readonly empty = new THREE.BufferGeometry();
  private readonly geometries = new Map<string,THREE.BufferGeometry>();
  private disposed = false;
  constructor() {
    this.empty.setAttribute('position', new THREE.Float32BufferAttribute([], 3));
    this.empty.setAttribute('normal', new THREE.Float32BufferAttribute([], 3));
    this.empty.setAttribute('color', new THREE.Float32BufferAttribute([], 3));
    for (const item of DISPLAY_ITEMS) this.geometries.set(item, this.merge(createResidentProp(item)));
    for (const [material,profile] of Object.entries(PRODUCTION_MATERIALS)) {
      for (let pieces=1;pieces<=WORLD_PRESENTATION.load.maxPieces;pieces++) {
        if (profile.shape === 'water') continue;
        const group=new THREE.Group();
        const geometry = profile.shape === 'wood' || profile.shape === 'bone'
          ? new THREE.CylinderGeometry(.035,.038,.36,7)
          : profile.shape === 'metal' || profile.shape === 'hide'
            ? new THREE.BoxGeometry(.16,profile.shape === 'hide'?.018:.055,.12)
            : profile.shape === 'fiber' ? new THREE.TorusGeometry(.065,.018,5,12)
              : new THREE.DodecahedronGeometry(profile.shape === 'food'?.07:.08,0);
        const tint=new THREE.MeshStandardMaterial({color:profile.color});
        for(let n=0;n<pieces;n++) {
          const mesh=new THREE.Mesh(geometry,tint);mesh.position.set(0,-.025,(n-(pieces-1)/2)*WORLD_PRESENTATION.load.spacing);
          if(profile.shape === 'wood' || profile.shape === 'bone')mesh.rotation.z=Math.PI/2;
          group.add(mesh);
        }
        this.geometries.set(`${material}:${pieces}`,this.merge(group));
      }
    }
  }
  get(item:string,pieces=1): THREE.BufferGeometry {
    return this.geometries.get(item === 'Water' ? 'SimpleContainer' : knownProductionMaterial(item) ? `${item}:${pieces}` : item) ?? this.empty;
  }
  get geometryCount(): number {return this.geometries.size+1;}
  private merge(group:THREE.Group): THREE.BufferGeometry {
    const positions:number[]=[],normals:number[]=[],colors:number[]=[];
    const point=new THREE.Vector3(),normal=new THREE.Vector3(),nm=new THREE.Matrix3();
    const ownedGeometry=new Set<THREE.BufferGeometry>(),ownedMaterial=new Set<THREE.Material>();
    group.updateMatrixWorld(true);
    group.traverse(o=>{
      if(!(o instanceof THREE.Mesh))return;
      const geometry=o.geometry.index ? o.geometry.toNonIndexed() : o.geometry;
      const p=geometry.getAttribute('position'),n=geometry.getAttribute('normal');
      const material=Array.isArray(o.material)?o.material[0]:o.material;
      const color=(material as THREE.MeshStandardMaterial).color;nm.getNormalMatrix(o.matrixWorld);
      for(let v=0;v<p.count;v++) {
        point.fromBufferAttribute(p,v).applyMatrix4(o.matrixWorld);positions.push(point.x,point.y,point.z);
        normal.fromBufferAttribute(n,v).applyMatrix3(nm).normalize();normals.push(normal.x,normal.y,normal.z);
        colors.push(color.r,color.g,color.b);
      }
      ownedGeometry.add(o.geometry);ownedGeometry.add(geometry);
      for(const m of Array.isArray(o.material)?o.material:[o.material])ownedMaterial.add(m);
    });
    for(const g of ownedGeometry)g.dispose();for(const m of ownedMaterial)m.dispose();
    const merged=new THREE.BufferGeometry();
    merged.setAttribute('position',new THREE.Float32BufferAttribute(positions,3));
    merged.setAttribute('normal',new THREE.Float32BufferAttribute(normals,3));
    merged.setAttribute('color',new THREE.Float32BufferAttribute(colors,3));
    return merged;
  }
  dispose(): void {
    if(this.disposed)return;this.disposed=true;
    for(const geometry of this.geometries.values())geometry.dispose();this.geometries.clear();
    this.empty.dispose();this.material.dispose();
  }
}

export class ResidentInventoryProps {
  readonly root = new THREE.Group();
  private inventory: ResidentCivilizationItem[] | undefined;
  private presentation: ResidentPresentationDirective | null | undefined;
  private visuallyMoving = false;
  private interacting = true;
  private disposed = false;
  private readonly loadRoot = new THREE.Group();
  private readonly rightHand: THREE.Object3D | undefined;
  private readonly leftHand: THREE.Object3D | undefined;
  private readonly otherHand = new THREE.Vector3();
  private readonly slots: THREE.Mesh[] = [];
  private readonly load: THREE.Mesh;
  hasCarriedLoad = false;
  hasWaterContainer = false;
  private readonly pelvis: THREE.Object3D | undefined;
  private readonly point = new THREE.Vector3();
  private readonly resources: ResidentPropResources;
  private readonly ownsResources: boolean;
  constructor(private readonly visual: THREE.Group, model: THREE.Object3D, resources?: ResidentPropResources) {
    this.resources=resources ?? new ResidentPropResources();this.ownsResources=!resources;
    this.pelvis=model.getObjectByName('pelvis');this.rightHand=model.getObjectByName('hand_r');this.leftHand=model.getObjectByName('hand_l');
    this.root.name='LifeLensInventoryProps';this.loadRoot.name='LifeLensObservedLoad';
    this.root.visible=false;this.loadRoot.visible=false;visual.add(this.root,this.loadRoot);
    for(let i=0;i<WORLD_PRESENTATION.production.maxEquipmentSlots;i++) {
      const mesh=new THREE.Mesh(this.resources.empty,this.resources.material);mesh.visible=false;mesh.castShadow=true;
      mesh.position.set(i===1?-.14:.14,-.06,i===2?-.095:.02);mesh.rotation.z=i===1?-.15:.15;
      this.slots.push(mesh);this.root.add(mesh);
    }
    this.load=new THREE.Mesh(this.resources.empty,this.resources.material);this.load.castShadow=true;
    this.load.scale.setScalar(WORLD_PRESENTATION.production.loadScale);this.loadRoot.add(this.load);
  }
  setInventory(inventory: ResidentCivilizationItem[] | undefined, presentation?: ResidentPresentationDirective | null): void {
    if(this.disposed)return;this.inventory=inventory;this.presentation=presentation;this.applyInventory();
  }
  setInteractionAllowed(allowed:boolean): void {if(this.interacting!==allowed){this.interacting=allowed;this.applyInventory();}}
  setVisuallyMoving(moving: boolean): void {if(this.visuallyMoving!==moving){this.visuallyMoving=moving;this.applyInventory();}}
  private applyInventory(): void {
    const inventory=this.inventory;
    const p=this.visuallyMoving && this.presentation?.phase === 'Interacting'
      ? {...this.presentation,phase:'Moving' as const}:this.presentation;
    const material=p?.phase === 'Interacting' && !this.interacting ? null : residentCarriedMaterial(inventory,p);
    this.hasWaterContainer=Boolean(this.rightHand) && this.interacting && !this.visuallyMoving && residentPouringWater(inventory,p);
    this.hasCarriedLoad=Boolean(material && this.rightHand && this.leftHand);
    const jug=this.hasWaterContainer || material === 'Water';
    const items=residentVisibleItems(inventory).filter(i=>!(i === 'SimpleContainer' && jug));
    this.slots.forEach((slot,i)=>{slot.visible=!!items[i];slot.geometry=this.resources.get(items[i] ?? '');});
    this.root.visible=Boolean(this.pelvis) && items.length>0;
    this.loadRoot.visible=this.hasCarriedLoad || this.hasWaterContainer;
    const quantity=material ? ownedMaterialUnits(inventory,material):0;
    const pieces=Math.min(WORLD_PRESENTATION.load.maxPieces,Math.max(1,Math.ceil(quantity)));
    this.load.geometry=this.resources.get(jug?'Water':material ?? '',pieces);
    this.load.rotation.x=this.hasWaterContainer ? .65 : 0;this.load.position.y=jug?-.055:0;
  }
  update(): void {
    if(this.disposed || (!this.root.visible && !this.loadRoot.visible))return;
    this.visual.updateWorldMatrix(true,true);
    if(this.pelvis){this.pelvis.getWorldPosition(this.point);this.root.position.copy(this.visual.worldToLocal(this.point));}
    if(this.loadRoot.visible && this.rightHand) {
      this.rightHand.getWorldPosition(this.point);
      if(!this.hasWaterContainer && this.leftHand && (this.visuallyMoving || this.presentation?.phase === 'Moving')) {
        this.leftHand.getWorldPosition(this.otherHand);this.point.add(this.otherHand).multiplyScalar(.5);
      }
      this.loadRoot.position.copy(this.visual.worldToLocal(this.point));
    }
  }
  reset(): void {this.inventory=undefined;this.presentation=null;this.visuallyMoving=false;this.hasCarriedLoad=false;this.hasWaterContainer=false;this.root.visible=false;this.loadRoot.visible=false;for(const slot of this.slots)slot.visible=false;}
  dispose(): void {if(this.disposed)return;this.disposed=true;this.reset();this.root.removeFromParent();this.loadRoot.removeFromParent();if(this.ownsResources)this.resources.dispose();}
}
