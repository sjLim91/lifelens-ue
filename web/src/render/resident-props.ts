import * as THREE from 'three';
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

export function residentCarriedMaterial(inventory: ResidentCivilizationItem[] | undefined, p: ResidentPresentationDirective | null | undefined): string | null {
  if (!p?.active || p.kind !== 'Civilization' || p.phase !== 'Moving'
    || !(p.facilityAction === 'DeliverMaterial' || p.civilizationIntent === 'Store')) return null;
  if (!['Wood','Stone','Flint','Clay','PlantFood','Water'].includes(p.civilizationMaterial ?? '')) return null;
  return inventory?.some(stack => stack.item === 'RawMaterial' && stack.material === p.civilizationMaterial
    && Number.isFinite(stack.quantity) && (stack.quantity ?? 0) > 0) ? p.civilizationMaterial ?? null : null;
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

export class ResidentInventoryProps {
  readonly root = new THREE.Group();
  private signature = '';
  private readonly loadRoot = new THREE.Group();
  private readonly rightHand: THREE.Object3D | undefined;
  private readonly leftHand: THREE.Object3D | undefined;
  private readonly otherHand = new THREE.Vector3();
  hasCarriedLoad = false;
  hasWaterContainer = false;
  private readonly pelvis: THREE.Object3D | undefined;
  private readonly point = new THREE.Vector3();
  constructor(private readonly visual: THREE.Group, model: THREE.Object3D) {
    this.pelvis = model.getObjectByName('pelvis');
    this.rightHand = model.getObjectByName('hand_r');
    this.leftHand = model.getObjectByName('hand_l');
    this.loadRoot.name = 'LifeLensObservedLoad';
    visual.add(this.loadRoot);
    this.root.name = 'LifeLensInventoryProps';
    visual.add(this.root);
  }
  setInventory(inventory: ResidentCivilizationItem[] | undefined, presentation?: ResidentPresentationDirective | null): void {
    const material = residentCarriedMaterial(inventory, presentation);
    this.hasWaterContainer = Boolean(this.rightHand) && residentPouringWater(inventory, presentation);
    const carriedWater = material === 'Water' && inventory?.some(s => s.item === 'SimpleContainer' && Number.isFinite(s.quantity) && (s.quantity ?? 0) > 0);
    this.hasCarriedLoad = Boolean(material && this.rightHand && this.leftHand && (material !== 'Water' || carriedWater));
    const items = residentVisibleItems(inventory).filter(item => !(item === 'SimpleContainer' && (this.hasWaterContainer || carriedWater)));
    const signature = [items.join('|'), this.hasCarriedLoad ? material : '', this.hasWaterContainer].join(':');
    if (signature === this.signature) return;
    this.clear(); this.signature = signature;
    items.forEach((item, index) => {
      const prop = createResidentProp(item);
      prop.position.set(index === 1 ? -.14 : .14, -.06, index === 2 ? -.095 : .02);
      prop.rotation.z = index === 1 ? -.15 : .15;
      this.root.add(prop);
    });
    this.root.visible = Boolean(this.pelvis) && items.length > 0;
    this.loadRoot.visible = this.hasCarriedLoad || this.hasWaterContainer;
    if (this.hasWaterContainer || (this.hasCarriedLoad && material === 'Water')) {
      const jug = createResidentProp('SimpleContainer');
      jug.position.y = -.055;
      if (this.hasWaterContainer) jug.rotation.x = .65;
      this.loadRoot.add(jug);
    } else if (this.hasCarriedLoad) {
      // A compact sample of the actual carried resource, not an invented tool.
      const resource = new THREE.Group();
      const color = material === 'Wood' ? 0x715038 : material === 'PlantFood' ? 0x648448 : material === 'Clay' ? 0xa16e4d : 0x858a89;
      const geometry = material === 'Wood' ? new THREE.CylinderGeometry(.03,.033,.30,7) : new THREE.DodecahedronGeometry(.065,0);
      const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({color,roughness:.85}));
      mesh.userData.residentOwnsGeometry = true;
      mesh.rotation.z = material === 'Wood' ? Math.PI / 2 : 0;
      mesh.position.set(0,-.025,0);
      resource.add(mesh);
      this.loadRoot.add(resource);
    }
  }
  update(): void {
    if (!this.root.visible && !this.loadRoot.visible) return;
    this.visual.updateWorldMatrix(true, true);
    if (this.pelvis) {
      this.pelvis.getWorldPosition(this.point);
      this.root.position.copy(this.visual.worldToLocal(this.point));
    }
    if (this.loadRoot.visible && this.rightHand) {
      this.rightHand.getWorldPosition(this.point);
      if (!this.hasWaterContainer && this.leftHand) {
        this.leftHand.getWorldPosition(this.otherHand);
        this.point.add(this.otherHand).multiplyScalar(.5);
      }
      this.loadRoot.position.copy(this.visual.worldToLocal(this.point));
    }
  }

  private clear(): void {
    for (const root of [this.root, this.loadRoot]) {
      root.traverse(object => {
        if (!(object instanceof THREE.Mesh)) return;
        object.geometry.dispose();
        for (const material of Array.isArray(object.material) ? object.material : [object.material]) material.dispose();
      });
      root.clear();
    }
  }
}
