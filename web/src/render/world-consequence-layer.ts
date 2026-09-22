import * as THREE from 'three';
import type {
  TerrainWindow,
  WorldFacility,
  WorldPresentationSnapshot,
  WorldResidue,
  WorldSanitationSite,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import { gridToWorldPosition } from './resident-world-coordinates';

function clamp01(value: number | undefined): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function facilityLabel(facility: WorldFacility): string {
  const kinds: Record<WorldFacility['kind'], string> = {
    PrimitiveStorage: '원시 저장소',
    FirePit: '화덕',
    WorkSurface: '작업대',
    SleepingPlace: '잠자리',
    Shelter: '쉼터',
    Furnace: '용광로',
  };
  const states: Record<WorldFacility['state'], string> = {
    Planned: '계획',
    UnderConstruction: '건설 중',
    Operational: '완성',
    Ruined: '폐허',
  };
  const progress = Math.round(clamp01(facility.workProgress) * 100);
  return facility.state === 'UnderConstruction'
    ? `${kinds[facility.kind]} · ${states[facility.state]} ${progress}%`
    : `${kinds[facility.kind]} · ${states[facility.state]}`;
}

function makeLabelSprite(text: string): THREE.Sprite {
  const canvas = document.createElement('canvas');
  canvas.width = 512;
  canvas.height = 112;
  const context = canvas.getContext('2d');
  if (!context) return new THREE.Sprite();

  context.fillStyle = 'rgba(7, 13, 9, 0.82)';
  context.roundRect(8, 8, 496, 96, 22);
  context.fill();
  context.strokeStyle = 'rgba(191, 213, 193, 0.55)';
  context.lineWidth = 3;
  context.stroke();
  context.fillStyle = '#eef5ee';
  context.font = '600 36px sans-serif';
  context.textAlign = 'center';
  context.textBaseline = 'middle';
  context.fillText(text, 256, 56, 470);

  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.SRGBColorSpace;
  texture.minFilter = THREE.LinearFilter;
  const material = new THREE.SpriteMaterial({
    map: texture,
    transparent: true,
    depthTest: false,
    depthWrite: false,
  });
  const sprite = new THREE.Sprite(material);
  sprite.scale.set(7.5, 1.65, 1);
  sprite.renderOrder = 12;
  return sprite;
}

function disposableMaterial(
  color: number,
  options: {
    opacity?: number;
    emissive?: number;
    emissiveIntensity?: number;
  } = {},
): THREE.MeshStandardMaterial {
  const opacity = options.opacity ?? 1;
  return new THREE.MeshStandardMaterial({
    color,
    roughness: 0.82,
    metalness: 0.02,
    transparent: opacity < 1,
    opacity,
    emissive: options.emissive ?? 0x000000,
    emissiveIntensity: options.emissiveIntensity ?? 0,
  });
}

function addBox(
  group: THREE.Group,
  size: [number, number, number],
  pos: [number, number, number],
  material: THREE.Material,
): void {
  const mesh = new THREE.Mesh(new THREE.BoxGeometry(...size), material);
  mesh.position.set(...pos);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  group.add(mesh);
}

function addFacilityShape(
  group: THREE.Group,
  facility: WorldFacility,
): void {
  const progress = facility.state === 'Operational'
    ? 1
    : Math.max(0.18, clamp01(facility.workProgress));
  const ruined = facility.state === 'Ruined';
  const construction = facility.state === 'Planned'
    || facility.state === 'UnderConstruction';
  const main = disposableMaterial(
    ruined ? 0x554f46 : construction ? 0x9b815d : 0x766040,
    { opacity: construction ? 0.72 : 1 },
  );
  const stone = disposableMaterial(ruined ? 0x55514b : 0x77746b);
  const fiber = disposableMaterial(ruined ? 0x625b49 : 0x9a8d61);
  const fire = disposableMaterial(
    0x5f2c16,
    facility.lit
      ? { emissive: 0xff7a1a, emissiveIntensity: 2.2 }
      : {},
  );

  switch (facility.kind) {
    case 'PrimitiveStorage':
      addBox(group, [2.8, 0.5, 2.2], [0, 0.3, 0], main);
      addBox(group, [2.4, 0.9 * progress, 1.8], [0, 0.75 * progress, 0], fiber);
      break;
    case 'FirePit': {
      const ring = new THREE.Mesh(
        new THREE.TorusGeometry(1.2, 0.28, 10, 24),
        stone,
      );
      ring.rotation.x = Math.PI * 0.5;
      ring.position.y = 0.18;
      group.add(ring);
      const ember = new THREE.Mesh(
        new THREE.CircleGeometry(0.9, 24),
        fire,
      );
      ember.rotation.x = -Math.PI * 0.5;
      ember.position.y = 0.21;
      group.add(ember);
      if (facility.lit) {
        const light = new THREE.PointLight(0xff8b32, 1.7, 16);
        light.position.y = 1.2;
        group.add(light);
      }
      break;
    }
    case 'WorkSurface':
      addBox(group, [3.4, 0.35, 2.1], [0, 1.1 * progress, 0], main);
      for (const x of [-1.3, 1.3]) {
        for (const z of [-0.7, 0.7]) {
          addBox(group, [0.28, 2 * progress, 0.28], [x, progress, z], main);
        }
      }
      break;
    case 'SleepingPlace':
      addBox(group, [3.2, 0.32, 1.8], [0, 0.25, 0], fiber);
      addBox(group, [3.0, 0.18, 1.6], [0, 0.48 * progress, 0], main);
      break;
    case 'Shelter': {
      const frame = new THREE.Mesh(
        new THREE.ConeGeometry(3.1, 4.2 * progress, 4),
        fiber,
      );
      frame.position.y = 2.1 * progress;
      frame.rotation.y = Math.PI * 0.25;
      group.add(frame);
      break;
    }
    case 'Furnace': {
      const body = new THREE.Mesh(
        new THREE.CylinderGeometry(1.5, 1.8, 2.8 * progress, 14),
        stone,
      );
      body.position.y = 1.4 * progress;
      group.add(body);
      const mouth = new THREE.Mesh(
        new THREE.CircleGeometry(0.58, 18),
        fire,
      );
      mouth.position.set(0, 0.8, 1.55);
      group.add(mouth);
      if (facility.lit) {
        const light = new THREE.PointLight(0xff7921, 2.1, 18);
        light.position.set(0, 1.2, 1.4);
        group.add(light);
      }
      break;
    }
  }

  if (construction) {
    const footprint = new THREE.Mesh(
      new THREE.RingGeometry(2.2, 2.55, 28),
      new THREE.MeshBasicMaterial({
        color: 0xd7b878,
        transparent: true,
        opacity: 0.6,
        side: THREE.DoubleSide,
        depthWrite: false,
      }),
    );
    footprint.rotation.x = -Math.PI * 0.5;
    footprint.position.y = 0.05;
    group.add(footprint);
  }
}

function addSanitationShape(
  group: THREE.Group,
  site: WorldSanitationSite,
): void {
  const ringMaterial = new THREE.MeshStandardMaterial({
    color: site.kind === 'DugPit' ? 0x5e4935 : 0x8a815c,
    roughness: 0.95,
  });
  const ring = new THREE.Mesh(
    new THREE.RingGeometry(
      site.kind === 'DugPit' ? 1.2 : 1.8,
      site.kind === 'DugPit' ? 1.7 : 2.2,
      32,
    ),
    ringMaterial,
  );
  ring.rotation.x = -Math.PI * 0.5;
  ring.position.y = 0.06;
  group.add(ring);

  if (site.kind === 'DugPit') {
    const pit = new THREE.Mesh(
      new THREE.CircleGeometry(1.12, 28),
      new THREE.MeshStandardMaterial({
        color: 0x30271f,
        roughness: 1,
      }),
    );
    pit.rotation.x = -Math.PI * 0.5;
    pit.position.y = 0.04;
    group.add(pit);
  } else {
    for (let i = 0; i < 4; i += 1) {
      const angle = (Math.PI * 2 * i) / 4;
      addBox(
        group,
        [0.15, 1.4, 0.15],
        [Math.cos(angle) * 2, 0.7, Math.sin(angle) * 2],
        ringMaterial,
      );
    }
  }

  const label = makeLabelSprite(
    site.kind === 'DugPit'
      ? `구덩이식 위생시설 · 사용 ${site.useCount ?? 0}회`
      : `지정 위생구역 · 사용 ${site.useCount ?? 0}회`,
  );
  label.position.y = 3.2;
  group.add(label);
}

function addResidueShape(
  group: THREE.Group,
  residue: WorldResidue,
): void {
  const intensity = Math.max(0.08, clamp01(residue.intensity));
  const cellSize =
    WORLD_GRID_CONTRACT.worldUnitsPerChunk
    / WORLD_GRID_CONTRACT.gridCellsPerChunk;
  const radius = Math.max(
    cellSize * 0.8,
    (Number(residue.radiusTiles) || 1) * cellSize,
  );
  const patch = new THREE.Mesh(
    new THREE.CircleGeometry(radius, 36),
    new THREE.MeshStandardMaterial({
      color: 0x493622,
      roughness: 1,
      transparent: true,
      opacity: Math.min(0.68, 0.16 + intensity * 0.52),
      depthWrite: false,
    }),
  );
  patch.rotation.x = -Math.PI * 0.5;
  patch.position.y = 0.055;
  patch.renderOrder = 3;
  group.add(patch);

  if (intensity >= 0.36) {
    const inner = new THREE.Mesh(
      new THREE.CircleGeometry(radius * 0.46, 28),
      new THREE.MeshBasicMaterial({
        color: 0x2c2017,
        transparent: true,
        opacity: Math.min(0.72, 0.24 + intensity * 0.45),
        depthWrite: false,
      }),
    );
    inner.rotation.x = -Math.PI * 0.5;
    inner.position.y = 0.065;
    inner.renderOrder = 4;
    group.add(inner);
  }
}

function disposeObject(object: THREE.Object3D): void {
  object.traverse((child) => {
    if (child instanceof THREE.Mesh) {
      child.geometry.dispose();
      const materials = Array.isArray(child.material)
        ? child.material
        : [child.material];
      for (const material of materials) material.dispose();
    }
    if (child instanceof THREE.Sprite) {
      const material = child.material;
      material.map?.dispose();
      material.dispose();
    }
  });
}

export class WorldConsequenceLayer {
  readonly group = new THREE.Group();

  setSnapshot(
    snapshot: WorldPresentationSnapshot | null,
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    for (const child of [...this.group.children]) {
      this.group.remove(child);
      disposeObject(child);
    }
    if (!snapshot?.available) return;

    const sampleElevation = createTerrainElevationSampler(terrain);
    const positionAt = (gridX: number, gridY: number): THREE.Vector3 => {
      const cells = WORLD_GRID_CONTRACT.gridCellsPerChunk;
      const chunkX = Math.floor(gridX / cells);
      const chunkY = Math.floor(gridY / cells);
      const localX = (gridX - chunkX * cells) / cells;
      const localY = (gridY - chunkY * cells) / cells;
      const elevation = sampleElevation(chunkX, chunkY, localX, localY);
      const pos = gridToWorldPosition(
        gridX,
        gridY,
        centerX,
        centerY,
        elevation,
      );
      return new THREE.Vector3(pos.x, pos.y, pos.z);
    };

    for (const residue of snapshot.residues ?? []) {
      const object = new THREE.Group();
      object.position.copy(positionAt(residue.gridX, residue.gridY));
      addResidueShape(object, residue);
      this.group.add(object);
    }

    for (const site of snapshot.sanitationSites ?? []) {
      if (site.active === false) continue;
      const object = new THREE.Group();
      object.position.copy(positionAt(site.gridX, site.gridY));
      addSanitationShape(object, site);
      this.group.add(object);
    }

    for (const facility of snapshot.facilities ?? []) {
      const object = new THREE.Group();
      object.position.copy(positionAt(facility.gridX, facility.gridY));
      addFacilityShape(object, facility);
      const label = makeLabelSprite(facilityLabel(facility));
      label.position.y = facility.kind === 'Shelter' ? 5.1 : 3.7;
      object.add(label);
      this.group.add(object);
    }
  }

  dispose(): void {
    for (const child of [...this.group.children]) {
      this.group.remove(child);
      disposeObject(child);
    }
  }
}
