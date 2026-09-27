import * as THREE from 'three';
import type { HumanTrace, TerrainWindow } from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { visibleHumanTraces } from '../state/human-traces';
import { createTerrainElevationSampler } from './terrain-geometry';

type FacilityTrace = Extract<HumanTrace, { kind: 'Facility' }>;

const UP = new THREE.Vector3(0, 1, 0);

function clamp01(value: unknown): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function constructionProgress(trace: FacilityTrace): number {
  if (trace.state === 'Operational') return 1;
  if (trace.state === 'Ruined') return 1;
  if (trace.state === 'Planned') return 0.04;
  return clamp01(trace.progress01);
}

export class FacilityLayer {
  readonly group = new THREE.Group();

  private readonly boxGeometry = new THREE.BoxGeometry(1, 1, 1);
  private readonly cylinderGeometry = new THREE.CylinderGeometry(
    0.5,
    0.5,
    1,
    8,
  );
  private readonly taperedCylinderGeometry = new THREE.CylinderGeometry(
    0.38,
    0.56,
    1,
    10,
  );
  private readonly stoneGeometry = new THREE.DodecahedronGeometry(0.5, 0);
  private readonly flameGeometry = new THREE.ConeGeometry(0.42, 0.95, 8);

  private readonly woodMaterial = new THREE.MeshStandardMaterial({
    color: 0x745238,
    roughness: 0.9,
    metalness: 0,
  });
  private readonly darkWoodMaterial = new THREE.MeshStandardMaterial({
    color: 0x4c3526,
    roughness: 0.94,
    metalness: 0,
  });
  private readonly stoneMaterial = new THREE.MeshStandardMaterial({
    color: 0x69675f,
    roughness: 0.97,
    metalness: 0,
  });
  private readonly earthMaterial = new THREE.MeshStandardMaterial({
    color: 0x4d4235,
    roughness: 1,
    metalness: 0,
  });
  private readonly thatchMaterial = new THREE.MeshStandardMaterial({
    color: 0x786c3d,
    roughness: 1,
    metalness: 0,
    side: THREE.DoubleSide,
  });
  private readonly beddingMaterial = new THREE.MeshStandardMaterial({
    color: 0x817963,
    roughness: 1,
    metalness: 0,
  });
  private readonly charMaterial = new THREE.MeshStandardMaterial({
    color: 0x292723,
    roughness: 0.96,
    metalness: 0,
  });
  private readonly flameMaterial = new THREE.MeshStandardMaterial({
    color: 0xffa53c,
    emissive: 0xff5f12,
    emissiveIntensity: 1.45,
    roughness: 0.4,
    metalness: 0,
  });

  private signature = '';

  constructor() {
    this.group.name = 'facilities';
  }

  setTerrain(window: TerrainWindow): void {
    const facilities = visibleHumanTraces(window)
      .filter((trace): trace is FacilityTrace => trace.kind === 'Facility');
    const signature = JSON.stringify([
      window.worldSeed,
      window.centerChunkX,
      window.centerChunkY,
      facilities.map((trace) => [
        trace.id,
        trace.gridX,
        trace.gridY,
        trace.facilityKind,
        trace.state,
        Math.round(clamp01(trace.progress01) * 100),
        trace.deliveredMaterialUnits,
        trace.requiredMaterialUnits,
        trace.active,
        trace.lit,
      ]),
    ]);
    if (signature === this.signature) return;
    this.signature = signature;

    this.group.clear();
    if (facilities.length === 0) {
      this.group.visible = false;
      return;
    }

    const sampleElevation = createTerrainElevationSampler(window);
    const {
      gridCellsPerChunk: span,
      worldUnitsPerChunk: size,
      elevationScale,
    } = WORLD_GRID_CONTRACT;

    for (const trace of facilities) {
      const chunkX = Math.floor(trace.gridX / span);
      const chunkY = Math.floor(trace.gridY / span);
      const localX01 = trace.gridX / span - chunkX;
      const localY01 = trace.gridY / span - chunkY;
      const worldX = (
        trace.gridX / span
        - window.centerChunkX
        - 0.5
      ) * size;
      const worldZ = (
        trace.gridY / span
        - window.centerChunkY
        - 0.5
      ) * size;
      const groundY = sampleElevation(
        chunkX,
        chunkY,
        localX01,
        localY01,
      ) * elevationScale;

      const structure = new THREE.Group();
      structure.name = `facility-${trace.id}`;
      structure.userData.traceId = trace.id;
      structure.position.set(worldX, groundY + 0.025, worldZ);
      structure.rotation.setFromVector3(
        new THREE.Vector3(
          0,
          hash01(`${window.worldSeed ?? '0'}:${trace.id}:yaw`)
            * Math.PI * 2,
          0,
        ),
      );

      this.buildFacility(
        structure,
        trace,
        constructionProgress(trace),
      );
      this.group.add(structure);
    }

    this.group.visible = true;
  }

  pickTrace(raycaster: THREE.Raycaster): string | null {
    if (!this.group.visible) return null;
    const hits = raycaster.intersectObjects(this.group.children, true);
    for (const hit of hits) {
      let current: THREE.Object3D | null = hit.object;
      while (current && current !== this.group) {
        if (typeof current.userData.traceId === 'string') {
          return current.userData.traceId;
        }
        current = current.parent;
      }
    }
    return null;
  }

  dispose(): void {
    this.group.clear();
    this.boxGeometry.dispose();
    this.cylinderGeometry.dispose();
    this.taperedCylinderGeometry.dispose();
    this.stoneGeometry.dispose();
    this.flameGeometry.dispose();
    this.woodMaterial.dispose();
    this.darkWoodMaterial.dispose();
    this.stoneMaterial.dispose();
    this.earthMaterial.dispose();
    this.thatchMaterial.dispose();
    this.beddingMaterial.dispose();
    this.charMaterial.dispose();
    this.flameMaterial.dispose();
  }

  private buildFacility(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    if (progress < 0.18) {
      this.addPlanStakes(group, trace);
    } else if (trace.state === 'UnderConstruction') {
      this.addMaterialPile(group, trace);
    }

    switch (trace.facilityKind) {
      case 'PrimitiveStorage':
        this.buildStorage(group, trace, progress);
        break;
      case 'FirePit':
        this.buildFirePit(group, trace, progress);
        break;
      case 'WorkSurface':
        this.buildWorkSurface(group, trace, progress);
        break;
      case 'SleepingPlace':
        this.buildSleepingPlace(group, trace, progress);
        break;
      case 'Shelter':
        this.buildShelter(group, trace, progress);
        break;
      case 'Furnace':
        this.buildFurnace(group, trace, progress);
        break;
      default:
        this.buildUnknownFacility(group, trace, progress);
        break;
    }
  }

  private addPlanStakes(
    group: THREE.Group,
    trace: FacilityTrace,
  ): void {
    const corners = [
      [-1.35, -1.1],
      [1.35, -1.1],
      [1.35, 1.1],
      [-1.35, 1.1],
    ] as const;
    corners.forEach(([x, z], index) => {
      this.addCylinder(
        group,
        trace,
        this.woodMaterial,
        [x, 0.24, z],
        [0.08, 0.48, 0.08],
        [0, 0, 0],
        index,
      );
    });
  }

  private addMaterialPile(
    group: THREE.Group,
    trace: FacilityTrace,
  ): void {
    const required = Math.max(1, Number(trace.requiredMaterialUnits) || 1);
    const delivered = Math.max(0, Number(trace.deliveredMaterialUnits) || 0);
    const count = Math.min(4, Math.ceil((delivered / required) * 4));
    for (let index = 0; index < count; index += 1) {
      this.addBox(
        group,
        trace,
        this.woodMaterial,
        [-1.55 + index * 0.34, 0.11 + index * 0.08, 1.25],
        [0.52, 0.18, 0.22],
        [0, 0.12 * (index % 2 ? 1 : -1), 0.06 * index],
        90 + index,
      );
    }
  }

  private buildStorage(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.16,
      this.darkWoodMaterial,
      [0, 0.12, 0],
      [2.4, 0.24, 1.65],
      [0, 0, 0],
      10,
    );

    const posts = [
      [-1.0, -0.62],
      [1.0, -0.62],
      [-1.0, 0.62],
      [1.0, 0.62],
    ] as const;
    posts.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.28 + index * 0.035,
        this.woodMaterial,
        [x, 0.62, z],
        [0.11, 1.24, 0.11],
        [0, 0, 0],
        20 + index,
      );
    });

    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.48,
      this.woodMaterial,
      [0, 0.55, 0],
      [1.85, 0.72, 1.14],
      [0, 0, 0],
      30,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.72,
      this.thatchMaterial,
      [0, 1.23, 0],
      [2.55, 0.18, 1.9],
      [0, 0, -0.05],
      31,
    );
  }

  private buildFirePit(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const stoneCount = 10;
    for (let index = 0; index < stoneCount; index += 1) {
      const angle = index / stoneCount * Math.PI * 2;
      this.addStoneAtProgress(
        group,
        trace,
        progress,
        0.12 + index * 0.025,
        [
          Math.cos(angle) * 0.82,
          0.16,
          Math.sin(angle) * 0.82,
        ],
        [0.42, 0.28, 0.36],
        [0, angle, 0],
        40 + index,
      );
    }

    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.38,
      this.charMaterial,
      [0, 0.22, 0],
      [0.13, 1.3, 0.13],
      [0, 0, Math.PI * 0.5],
      52,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.46,
      this.charMaterial,
      [0, 0.24, 0],
      [0.13, 1.3, 0.13],
      [Math.PI * 0.5, 0, Math.PI * 0.5],
      53,
    );

    if (trace.lit && progress >= 0.78 && trace.state !== 'Ruined') {
      const flame = new THREE.Mesh(
        this.flameGeometry,
        this.flameMaterial,
      );
      flame.position.set(0, 0.74, 0);
      flame.scale.set(0.65, 1.0, 0.65);
      this.prepareMesh(flame, trace, 54);
      group.add(flame);
    }
  }

  private buildWorkSurface(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const legs = [
      [-0.84, -0.42],
      [0.84, -0.42],
      [-0.84, 0.42],
      [0.84, 0.42],
    ] as const;
    legs.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.18 + index * 0.055,
        this.woodMaterial,
        [x, 0.48, z],
        [0.09, 0.96, 0.09],
        [0, 0, 0],
        60 + index,
      );
    });
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.52,
      this.darkWoodMaterial,
      [0, 1.0, 0],
      [2.1, 0.2, 1.15],
      [0, 0, 0],
      65,
    );
    this.addStoneAtProgress(
      group,
      trace,
      progress,
      0.72,
      [0.48, 1.2, 0.08],
      [0.34, 0.18, 0.25],
      [0, 0.4, 0],
      66,
    );
  }

  private buildSleepingPlace(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.18,
      this.darkWoodMaterial,
      [0, 0.16, 0],
      [2.5, 0.2, 1.15],
      [0, 0, 0],
      70,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.44,
      this.beddingMaterial,
      [0, 0.32, 0],
      [2.25, 0.18, 0.98],
      [0, 0, 0],
      71,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.68,
      this.thatchMaterial,
      [-0.82, 0.48, 0],
      [0.42, 0.2, 0.78],
      [0, 0, 0],
      72,
    );
  }

  private buildShelter(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const posts = [
      [-1.45, -1.15],
      [1.45, -1.15],
      [-1.45, 1.15],
      [1.45, 1.15],
    ] as const;
    posts.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.16 + index * 0.055,
        this.woodMaterial,
        [x, 1.35, z],
        [0.12, 2.7, 0.12],
        [0, 0, 0],
        80 + index,
      );
    });

    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.38,
      this.darkWoodMaterial,
      [0, 2.55, -1.15],
      [3.15, 0.15, 0.15],
      [0, 0, 0],
      85,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.46,
      this.darkWoodMaterial,
      [0, 2.55, 1.15],
      [3.15, 0.15, 0.15],
      [0, 0, 0],
      86,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.58,
      this.thatchMaterial,
      [-0.76, 2.88, 0],
      [1.85, 0.17, 2.75],
      [0, 0, -0.27],
      87,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.68,
      this.thatchMaterial,
      [0.76, 2.88, 0],
      [1.85, 0.17, 2.75],
      [0, 0, 0.27],
      88,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.82,
      this.woodMaterial,
      [0, 1.3, 1.12],
      [2.7, 2.35, 0.1],
      [0, 0, 0],
      89,
    );
  }

  private buildFurnace(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.18,
      this.stoneMaterial,
      [0, 0.56, 0],
      [1.1, 1.12, 1.1],
      [0, 0, 0],
      100,
      true,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.48,
      this.earthMaterial,
      [0, 1.32, 0],
      [0.84, 0.7, 0.84],
      [0, 0, 0],
      101,
      true,
    );
    this.addCylinderAtProgress(
      group,
      trace,
      progress,
      0.72,
      this.stoneMaterial,
      [0, 1.98, 0],
      [0.48, 0.78, 0.48],
      [0, 0, 0],
      102,
    );
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.58,
      this.charMaterial,
      [0, 0.62, -0.56],
      [0.66, 0.48, 0.16],
      [0, 0, 0],
      103,
    );

    if (trace.lit && progress >= 0.75 && trace.state !== 'Ruined') {
      const flame = new THREE.Mesh(
        this.flameGeometry,
        this.flameMaterial,
      );
      flame.position.set(0, 0.72, -0.66);
      flame.scale.set(0.36, 0.55, 0.36);
      flame.rotation.x = Math.PI * 0.08;
      this.prepareMesh(flame, trace, 104);
      group.add(flame);
    }
  }

  private buildUnknownFacility(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
  ): void {
    const corners = [
      [-0.9, -0.7],
      [0.9, -0.7],
      [-0.9, 0.7],
      [0.9, 0.7],
    ] as const;
    corners.forEach(([x, z], index) => {
      this.addCylinderAtProgress(
        group,
        trace,
        progress,
        0.2 + index * 0.08,
        this.woodMaterial,
        [x, 0.7, z],
        [0.1, 1.4, 0.1],
        [0, 0, 0],
        110 + index,
      );
    });
    this.addBoxAtProgress(
      group,
      trace,
      progress,
      0.62,
      this.darkWoodMaterial,
      [0, 1.25, 0],
      [2.1, 0.18, 1.7],
      [0, 0, 0],
      115,
    );
  }

  private addBoxAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    this.addBox(
      group,
      trace,
      material,
      position,
      scale,
      this.ruinedRotation(trace, rotation, slot),
      slot,
    );
  }

  private addCylinderAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
    tapered = false,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    const mesh = new THREE.Mesh(
      tapered
        ? this.taperedCylinderGeometry
        : this.cylinderGeometry,
      material,
    );
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...this.ruinedRotation(trace, rotation, slot));
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addStoneAtProgress(
    group: THREE.Group,
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    if (!this.shouldShowPart(trace, progress, threshold, slot)) return;
    const mesh = new THREE.Mesh(
      this.stoneGeometry,
      this.stoneMaterial,
    );
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...this.ruinedRotation(trace, rotation, slot));
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addBox(
    group: THREE.Group,
    trace: FacilityTrace,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    const mesh = new THREE.Mesh(this.boxGeometry, material);
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...rotation);
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private addCylinder(
    group: THREE.Group,
    trace: FacilityTrace,
    material: THREE.Material,
    position: [number, number, number],
    scale: [number, number, number],
    rotation: [number, number, number],
    slot: number,
  ): void {
    const mesh = new THREE.Mesh(this.cylinderGeometry, material);
    mesh.position.set(...position);
    mesh.scale.set(...scale);
    mesh.rotation.set(...rotation);
    this.prepareMesh(mesh, trace, slot);
    group.add(mesh);
  }

  private shouldShowPart(
    trace: FacilityTrace,
    progress: number,
    threshold: number,
    slot: number,
  ): boolean {
    if (trace.state !== 'Ruined') return progress >= threshold;
    return slot % 4 !== 0;
  }

  private ruinedRotation(
    trace: FacilityTrace,
    rotation: [number, number, number],
    slot: number,
  ): [number, number, number] {
    if (trace.state !== 'Ruined') return rotation;
    const direction = slot % 2 === 0 ? 1 : -1;
    return [
      rotation[0] + 0.08 * direction,
      rotation[1],
      rotation[2] + 0.2 * direction,
    ];
  }

  private prepareMesh(
    mesh: THREE.Mesh,
    trace: FacilityTrace,
    slot: number,
  ): void {
    mesh.name = `${trace.facilityKind}-${slot}`;
    mesh.userData.traceId = trace.id;
    mesh.castShadow = false;
    mesh.receiveShadow = true;
  }
}
