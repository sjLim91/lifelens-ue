import * as THREE from 'three';
import type {
  TerrainWindow,
  WorldFacility,
  WorldPresentationSnapshot,
  WorldResourceNode,
  WorldResidue,
  WorldSanitationSite,
  WorldStorageSite,
} from '../runtime/core-types';
import { WORLD_GRID_CONTRACT } from '../runtime/lifelens-contract';
import { createTerrainElevationSampler } from './terrain-geometry';
import { gridToWorldPosition } from './resident-world-coordinates';

function clamp01(value: number | undefined): number {
  return Math.max(0, Math.min(1, Number(value) || 0));
}

function hash01(value: string, salt = 0): number {
  let hash = (2166136261 ^ salt) >>> 0;
  for (let index = 0; index < value.length; index += 1) {
    hash ^= value.charCodeAt(index);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return (hash >>> 0) / 4294967295;
}

function facilityLabel(
  facility: WorldFacility,
  storedUnits?: number,
): string {
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
  const storageSuffix = facility.kind === 'PrimitiveStorage'
    && Number(storedUnits) > 0
    ? ` · 보관 ${storedUnits}`
    : '';
  return facility.state === 'UnderConstruction'
    ? `${kinds[facility.kind]} · ${states[facility.state]} ${progress}%`
    : `${kinds[facility.kind]} · ${states[facility.state]}${storageSuffix}`;
}

function makeLabelSprite(
  text: string,
  priority = 1,
): THREE.Sprite {
  const canvas = document.createElement('canvas');
  canvas.width = 512;
  canvas.height = 112;
  const context = canvas.getContext('2d');
  if (!context) return new THREE.Sprite();

  context.fillStyle = 'rgba(7, 13, 9, 0.70)';
  context.roundRect(14, 16, 484, 80, 20);
  context.fill();
  context.strokeStyle = 'rgba(185, 205, 187, 0.36)';
  context.lineWidth = 2;
  context.stroke();
  context.fillStyle = '#edf4ed';
  context.font = '600 31px sans-serif';
  context.textAlign = 'center';
  context.textBaseline = 'middle';
  context.fillText(text, 256, 56, 452);

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
  sprite.scale.set(5.5, 1.18, 1);
  sprite.userData.lifeLensContextLabel = true;
  sprite.userData.lifeLensLabelPriority = priority;
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

function addCylinder(
  group: THREE.Group,
  radiusTop: number,
  radiusBottom: number,
  height: number,
  pos: [number, number, number],
  material: THREE.Material,
  rotation: [number, number, number] = [0, 0, 0],
  radialSegments = 8,
): THREE.Mesh {
  const mesh = new THREE.Mesh(
    new THREE.CylinderGeometry(
      radiusTop,
      radiusBottom,
      height,
      radialSegments,
    ),
    material,
  );
  mesh.position.set(...pos);
  mesh.rotation.set(...rotation);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  group.add(mesh);
  return mesh;
}

function addLog(
  group: THREE.Group,
  length: number,
  radius: number,
  pos: [number, number, number],
  yaw: number,
  material: THREE.Material,
): void {
  addCylinder(
    group,
    radius * 0.86,
    radius,
    length,
    pos,
    material,
    [Math.PI * 0.5, yaw, 0],
    7,
  );
}

function addStone(
  group: THREE.Group,
  radius: number,
  pos: [number, number, number],
  material: THREE.Material,
  seed: number,
): void {
  const mesh = new THREE.Mesh(
    new THREE.DodecahedronGeometry(radius, 0),
    material,
  );
  mesh.position.set(...pos);
  mesh.scale.set(
    0.82 + (seed % 7) * 0.035,
    0.62 + (seed % 5) * 0.04,
    0.9 + (seed % 3) * 0.05,
  );
  mesh.rotation.set(
    (seed % 11) * 0.07,
    (seed % 17) * 0.11,
    (seed % 13) * 0.05,
  );
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  group.add(mesh);
}

function addConstructionMaterials(
  group: THREE.Group,
  facility: WorldFacility,
  wood: THREE.Material,
  stone: THREE.Material,
  fiber: THREE.Material,
): void {
  if (
    facility.state !== 'Planned'
    && facility.state !== 'UnderConstruction'
  ) {
    return;
  }

  const progress = clamp01(facility.workProgress);
  const woodCount = 2 + Math.floor(progress * 4);
  const stoneCount = 2 + Math.floor(progress * 3);
  for (let index = 0; index < woodCount; index += 1) {
    const angle = -0.9 + index * 0.26;
    addLog(
      group,
      1.3 + (index % 2) * 0.35,
      0.09 + (index % 3) * 0.015,
      [
        -2.5 + (index % 3) * 0.32,
        0.12 + Math.floor(index / 3) * 0.17,
        1.65 + (index % 2) * 0.22,
      ],
      angle,
      wood,
    );
  }
  for (let index = 0; index < stoneCount; index += 1) {
    addStone(
      group,
      0.22 + (index % 2) * 0.05,
      [
        2.1 + (index % 3) * 0.28,
        0.15 + Math.floor(index / 3) * 0.12,
        -1.45 + (index % 2) * 0.32,
      ],
      stone,
      index + 93,
    );
  }

  if (progress > 0.32) {
    const bundle = new THREE.Mesh(
      new THREE.BoxGeometry(1.25, 0.18, 0.92),
      fiber,
    );
    bundle.position.set(-1.95, 0.14, -1.65);
    bundle.rotation.y = -0.24;
    bundle.castShadow = true;
    group.add(bundle);
  }
}

function addSmokePuffs(
  group: THREE.Group,
  baseY: number,
  baseZ: number,
): void {
  const smokeMaterial = new THREE.MeshBasicMaterial({
    color: 0xb5bab6,
    transparent: true,
    opacity: 0.13,
    depthWrite: false,
  });
  for (let index = 0; index < 4; index += 1) {
    const puff = new THREE.Mesh(
      new THREE.SphereGeometry(0.25 + index * 0.08, 8, 6),
      smokeMaterial.clone(),
    );
    puff.position.set(
      (index % 2 ? 1 : -1) * 0.1,
      baseY + index * 0.46,
      baseZ + (index % 2 ? 0.06 : -0.04),
    );
    puff.userData.lifeLensSmoke = true;
    puff.userData.baseY = puff.position.y;
    puff.userData.phase = index * 1.7;
    puff.renderOrder = 6;
    group.add(puff);
  }
}

function addFacilityGroundWear(
  group: THREE.Group,
  facility: WorldFacility,
): void {
  if (facility.state === 'Ruined') return;
  const construction = facility.state === 'Planned'
    || facility.state === 'UnderConstruction';
  const radius = facility.kind === 'Shelter'
    ? 3.2
    : facility.kind === 'WorkSurface'
      ? 2.7
      : 2.35;
  const wear = new THREE.Mesh(
    new THREE.CircleGeometry(radius, 36),
    new THREE.MeshStandardMaterial({
      color: construction ? 0x65513a : 0x5a4b38,
      roughness: 1,
      transparent: true,
      opacity: construction ? 0.22 : 0.12,
      depthWrite: false,
    }),
  );
  wear.rotation.x = -Math.PI * 0.5;
  wear.position.y = 0.025;
  wear.renderOrder = 1;
  group.add(wear);
}

function addStorageContents(
  group: THREE.Group,
  storage: WorldStorageSite | undefined,
): void {
  if (!storage?.inventory?.length) return;

  const wood = disposableMaterial(0x73563a);
  const stone = disposableMaterial(0x77736a);
  const fiber = disposableMaterial(0x887d55);
  const clay = disposableMaterial(0x8d6550);
  const charcoal = disposableMaterial(0x292824);
  const metal = disposableMaterial(0xa96f50);

  let slot = 0;
  for (const stack of storage.inventory.slice(0, 6)) {
    const quantity = Math.max(0, Number(stack.quantity) || 0);
    if (quantity <= 0) continue;
    const x = -1.0 + (slot % 3) * 0.95;
    const z = -0.45 + Math.floor(slot / 3) * 0.95;
    const visibleCount = Math.max(1, Math.min(4, Math.ceil(quantity / 3)));

    for (let index = 0; index < visibleCount; index += 1) {
      const ox = x + (index % 2) * 0.18;
      const oz = z + Math.floor(index / 2) * 0.2;
      const y = 0.34 + Math.floor(index / 2) * 0.14;
      switch (stack.material) {
        case 'Wood':
          addLog(
            group,
            0.82,
            0.07,
            [ox, y, oz],
            0.32 + index * 0.26,
            wood,
          );
          break;
        case 'Fiber':
        case 'PlantFood': {
          const bundle = new THREE.Mesh(
            new THREE.IcosahedronGeometry(0.16, 1),
            fiber,
          );
          bundle.position.set(ox, y, oz);
          bundle.scale.set(1.25, 0.72, 0.92);
          group.add(bundle);
          break;
        }
        case 'Clay': {
          const lump = new THREE.Mesh(
            new THREE.SphereGeometry(0.15, 8, 6),
            clay,
          );
          lump.position.set(ox, y, oz);
          lump.scale.y = 0.72;
          group.add(lump);
          break;
        }
        case 'Charcoal': {
          const lump = new THREE.Mesh(
            new THREE.DodecahedronGeometry(0.12, 0),
            charcoal,
          );
          lump.position.set(ox, y, oz);
          group.add(lump);
          break;
        }
        case 'CopperMetal': {
          const ingot = new THREE.Mesh(
            new THREE.BoxGeometry(0.32, 0.08, 0.16),
            metal,
          );
          ingot.position.set(ox, y, oz);
          group.add(ingot);
          break;
        }
        default:
          addStone(
            group,
            0.13,
            [ox, y, oz],
            stone,
            slot * 11 + index,
          );
          break;
      }
    }
    slot += 1;
  }
}

function facilityLabelPriority(
  facility: WorldFacility,
): number {
  if (
    facility.state === 'Planned'
    || facility.state === 'UnderConstruction'
  ) return 4;
  if (
    (facility.kind === 'FirePit' || facility.kind === 'Furnace')
    && facility.lit
  ) return 4;
  if (facility.kind === 'Shelter') return 3;
  if (
    facility.kind === 'PrimitiveStorage'
    || facility.kind === 'WorkSurface'
    || facility.kind === 'Furnace'
  ) return 2;
  return 1;
}

function addFacilityShape(
  group: THREE.Group,
  facility: WorldFacility,
): void {
  const progress = facility.state === 'Operational'
    ? 1
    : Math.max(0.16, clamp01(facility.workProgress));
  const ruined = facility.state === 'Ruined';
  const construction = facility.state === 'Planned'
    || facility.state === 'UnderConstruction';
  const wood = disposableMaterial(
    ruined ? 0x4c453a : construction ? 0x876f4e : 0x675137,
    { opacity: construction ? 0.78 : 1 },
  );
  const freshWood = disposableMaterial(
    ruined ? 0x51493e : 0x866744,
    { opacity: construction ? 0.76 : 1 },
  );
  const stone = disposableMaterial(ruined ? 0x4f4d49 : 0x77756e);
  const fiber = disposableMaterial(
    ruined ? 0x625b49 : 0x9a8b5f,
    { opacity: construction ? 0.78 : 1 },
  );
  const earth = disposableMaterial(0x564333);
  addFacilityGroundWear(group, facility);

  const fire = disposableMaterial(
    0x5f2c16,
    facility.lit
      ? { emissive: 0xff6b12, emissiveIntensity: 2.8 }
      : {},
  );

  addConstructionMaterials(group, facility, freshWood, stone, fiber);

  switch (facility.kind) {
    case 'PrimitiveStorage': {
      addBox(group, [3.0, 0.18, 2.35], [0, 0.13, 0], earth);
      for (const z of [-0.82, -0.28, 0.28, 0.82]) {
        addLog(
          group,
          2.65,
          0.14,
          [0, 0.38 + progress * 0.18, z],
          Math.PI * 0.5,
          wood,
        );
      }
      for (const x of [-1.22, 1.22]) {
        addCylinder(
          group,
          0.11,
          0.14,
          1.5 * progress,
          [x, 0.75 * progress, -0.92],
          wood,
        );
        addCylinder(
          group,
          0.11,
          0.14,
          1.5 * progress,
          [x, 0.75 * progress, 0.92],
          wood,
        );
      }
      if (progress > 0.52) {
        const cover = new THREE.Mesh(
          new THREE.BoxGeometry(2.75, 0.16, 2.0),
          fiber,
        );
        cover.position.set(0, 1.25 * progress, 0);
        cover.rotation.z = -0.05;
        cover.castShadow = true;
        group.add(cover);
      }
      break;
    }
    case 'FirePit': {
      for (let index = 0; index < 11; index += 1) {
        const angle = (Math.PI * 2 * index) / 11;
        addStone(
          group,
          0.33,
          [
            Math.cos(angle) * 1.18,
            0.22,
            Math.sin(angle) * 1.18,
          ],
          stone,
          index + Number(facility.id),
        );
      }
      addLog(group, 1.65, 0.13, [0, 0.28, 0], Math.PI * 0.25, freshWood);
      addLog(group, 1.65, 0.13, [0, 0.31, 0], -Math.PI * 0.25, freshWood);
      const ember = new THREE.Mesh(
        new THREE.CircleGeometry(0.82, 28),
        fire,
      );
      ember.rotation.x = -Math.PI * 0.5;
      ember.position.y = 0.16;
      group.add(ember);
      if (facility.lit) {
        const flame = new THREE.Mesh(
          new THREE.ConeGeometry(0.38, 1.05, 7),
          disposableMaterial(
            0xff8a22,
            { emissive: 0xff5b0a, emissiveIntensity: 3.4, opacity: 0.82 },
          ),
        );
        flame.position.y = 0.72;
        flame.scale.z = 0.72;
        group.add(flame);
        const light = new THREE.PointLight(0xff8b32, 2.25, 22, 2);
        light.position.y = 1.25;
        light.castShadow = false;
        light.userData.lifeLensFireLight = true;
        light.userData.baseIntensity = 2.25;
        flame.userData.lifeLensFireFlame = true;
        flame.userData.baseScaleY = 1;
        group.add(light);
        addSmokePuffs(group, 1.45, 0);
      }
      break;
    }
    case 'WorkSurface': {
      const topY = 1.55 * progress;
      addBox(group, [3.55, 0.24, 2.05], [0, topY, 0], freshWood);
      for (const x of [-1.35, 1.35]) {
        for (const z of [-0.68, 0.68]) {
          addCylinder(
            group,
            0.12,
            0.16,
            1.55 * progress,
            [x, 0.78 * progress, z],
            wood,
          );
        }
      }
      if (progress > 0.7) {
        addStone(group, 0.32, [-0.72, topY + 0.25, 0.2], stone, 11);
        addLog(group, 1.15, 0.08, [0.55, topY + 0.22, -0.22], 0.32, wood);
      }
      break;
    }
    case 'SleepingPlace': {
      addLog(group, 3.4, 0.15, [0, 0.22, -0.82], Math.PI * 0.5, wood);
      addLog(group, 3.4, 0.15, [0, 0.22, 0.82], Math.PI * 0.5, wood);
      addBox(
        group,
        [3.25, 0.22, 1.55],
        [0, 0.34 + progress * 0.08, 0],
        fiber,
      );
      if (progress > 0.65) {
        addCylinder(
          group,
          0.28,
          0.28,
          1.1,
          [-1.0, 0.62, 0],
          fiber,
          [0, 0, Math.PI * 0.5],
          10,
        );
      }
      break;
    }
    case 'Shelter': {
      const frameHeight = 4.5 * progress;
      for (const z of [-1.9, 1.9]) {
        addCylinder(
          group,
          0.12,
          0.16,
          5.15 * progress,
          [-1.75, 2.05 * progress, z],
          wood,
          [0, 0, -0.72],
        );
        addCylinder(
          group,
          0.12,
          0.16,
          5.15 * progress,
          [1.75, 2.05 * progress, z],
          wood,
          [0, 0, 0.72],
        );
      }
      addLog(
        group,
        4.3,
        0.15,
        [0, frameHeight, 0],
        0,
        freshWood,
      );
      if (progress > 0.38) {
        const roofMaterial = disposableMaterial(
          ruined ? 0x554c3d : 0x756f47,
          { opacity: construction ? 0.82 : 1 },
        );
        for (const side of [-1, 1]) {
          const roof = new THREE.Mesh(
            new THREE.BoxGeometry(2.75, 0.16, 4.4),
            roofMaterial,
          );
          roof.position.set(side * 1.13, 3.2 * progress, 0);
          roof.rotation.z = side * 0.72;
          roof.castShadow = true;
          roof.receiveShadow = true;
          group.add(roof);
        }
      }
      addBox(group, [3.4, 0.16, 3.55], [0, 0.1, 0], earth);
      break;
    }
    case 'Furnace': {
      const body = new THREE.Mesh(
        new THREE.CylinderGeometry(
          1.32,
          1.62,
          2.65 * progress,
          14,
        ),
        stone,
      );
      body.position.y = 1.33 * progress;
      body.castShadow = true;
      body.receiveShadow = true;
      group.add(body);
      if (progress > 0.55) {
        const chimney = new THREE.Mesh(
          new THREE.CylinderGeometry(0.62, 0.78, 1.45 * progress, 12),
          stone,
        );
        chimney.position.y = 3.18 * progress;
        chimney.castShadow = true;
        group.add(chimney);
      }
      const mouth = new THREE.Mesh(
        new THREE.CircleGeometry(0.55, 18),
        fire,
      );
      mouth.position.set(0, 0.82, 1.49);
      group.add(mouth);
      for (let index = 0; index < 8; index += 1) {
        const angle = (Math.PI * 2 * index) / 8;
        addStone(
          group,
          0.34,
          [Math.cos(angle) * 1.45, 0.24, Math.sin(angle) * 1.45],
          stone,
          index + 31,
        );
      }
      if (facility.lit) {
        const light = new THREE.PointLight(0xff7921, 2.45, 20, 2);
        light.position.set(0, 1.05, 1.65);
        light.userData.lifeLensFireLight = true;
        light.userData.baseIntensity = 2.45;
        group.add(light);
        addSmokePuffs(group, 3.85 * progress, 0);
      }
      break;
    }
  }

  if (construction) {
    const footprint = new THREE.Mesh(
      new THREE.RingGeometry(2.1, 2.35, 28),
      new THREE.MeshBasicMaterial({
        color: 0xd3b277,
        transparent: true,
        opacity: facility.state === 'Planned' ? 0.48 : 0.24,
        side: THREE.DoubleSide,
        depthWrite: false,
      }),
    );
    footprint.rotation.x = -Math.PI * 0.5;
    footprint.position.y = 0.045;
    group.add(footprint);

    for (let index = 0; index < 4; index += 1) {
      const angle = Math.PI * 0.25 + (Math.PI * 0.5 * index);
      addCylinder(
        group,
        0.055,
        0.075,
        1.15,
        [
          Math.cos(angle) * 2.28,
          0.575,
          Math.sin(angle) * 2.28,
        ],
        freshWood,
        [0, 0, 0.03 * (index % 2 ? -1 : 1)],
        6,
      );
    }
  }
}

function addSanitationShape(
  group: THREE.Group,
  site: WorldSanitationSite,
): void {
  const wood = disposableMaterial(0x705638);
  const earth = disposableMaterial(0x4f3d2f);
  const rim = disposableMaterial(0x6f6758);
  const radius = site.kind === 'DugPit' ? 1.28 : 2.0;

  if (site.kind === 'DugPit') {
    const pit = new THREE.Mesh(
      new THREE.CircleGeometry(1.08, 30),
      new THREE.MeshStandardMaterial({
        color: 0x271f19,
        roughness: 1,
      }),
    );
    pit.rotation.x = -Math.PI * 0.5;
    pit.position.y = 0.035;
    group.add(pit);
    for (let index = 0; index < 10; index += 1) {
      const angle = (Math.PI * 2 * index) / 10;
      addStone(
        group,
        0.28,
        [
          Math.cos(angle) * 1.32,
          0.18,
          Math.sin(angle) * 1.32,
        ],
        rim,
        index + Number(site.id),
      );
    }
  } else {
    const worn = new THREE.Mesh(
      new THREE.CircleGeometry(radius, 30),
      new THREE.MeshStandardMaterial({
        color: 0x62513b,
        roughness: 1,
        transparent: true,
        opacity: 0.32,
        depthWrite: false,
      }),
    );
    worn.rotation.x = -Math.PI * 0.5;
    worn.position.y = 0.035;
    group.add(worn);
    for (let index = 0; index < 4; index += 1) {
      const angle = Math.PI * 0.25 + (Math.PI * 0.5 * index);
      addCylinder(
        group,
        0.055,
        0.075,
        1.45,
        [
          Math.cos(angle) * radius,
          0.725,
          Math.sin(angle) * radius,
        ],
        wood,
        [0, 0, 0.04 * (index % 2 ? -1 : 1)],
        6,
      );
    }
  }

  if (site.kind === 'DugPit') {
    addBox(group, [2.9, 0.12, 0.35], [0, 0.15, -1.75], earth);
  }

  const label = makeLabelSprite(
    site.kind === 'DugPit'
      ? `위생 구덩이 · 사용 ${site.useCount ?? 0}회`
      : `지정 위생구역 · 사용 ${site.useCount ?? 0}회`,
    site.kind === 'DugPit' ? 3 : 2,
  );
  label.scale.multiplyScalar(0.82);
  label.position.y = 2.75;
  group.add(label);
}

function addResourceShape(
  group: THREE.Group,
  resource: WorldResourceNode,
): void {
  const quantityRatio = Math.max(
    0.18,
    Math.min(
      1,
      (Number(resource.quantity) || 0)
      / Math.max(1, Number(resource.maxQuantity) || Number(resource.quantity) || 1),
    ),
  );
  const materialColors: Record<string, number> = {
    Stone: 0x77756f,
    Flint: 0x4f5455,
    Wood: 0x6f5536,
    Fiber: 0x83945b,
    Clay: 0x936b52,
    Water: 0x3c86a0,
    PlantFood: 0x7b9147,
    Bone: 0xc8c1a7,
    Hide: 0x8a6948,
    CopperOre: 0x8c684d,
    TinOre: 0x7d8586,
    IronOre: 0x635d59,
    Charcoal: 0x292a28,
    CopperMetal: 0xb46d43,
  };
  const color = materialColors[resource.material] ?? 0x77756f;
  group.rotation.y = hash01(resource.id, 19) * Math.PI * 2;

  if (resource.material === 'Wood') {
    const logMaterial = disposableMaterial(color);
    addLog(
      group,
      1.75 * quantityRatio + 0.6,
      0.16,
      [0, 0.22, 0],
      0.35,
      logMaterial,
    );
    addLog(
      group,
      1.45 * quantityRatio + 0.55,
      0.13,
      [0.22, 0.28, -0.18],
      -0.58,
      logMaterial,
    );
  } else if (resource.material === 'Fiber' || resource.material === 'PlantFood') {
    const tuftMaterial = disposableMaterial(color);
    for (let index = 0; index < 3; index += 1) {
      const tuft = new THREE.Mesh(
        new THREE.IcosahedronGeometry(
          0.34 + quantityRatio * 0.22,
          1,
        ),
        tuftMaterial,
      );
      tuft.position.set(
        (index - 1) * 0.36,
        0.28 + index * 0.08,
        index % 2 ? 0.22 : -0.12,
      );
      tuft.scale.y = 0.85 + index * 0.08;
      group.add(tuft);
    }
  } else if (resource.material === 'Water') {
    const water = new THREE.Mesh(
      new THREE.CircleGeometry(0.92 + quantityRatio * 0.34, 28),
      new THREE.MeshStandardMaterial({
        color,
        roughness: 0.18,
        metalness: 0.02,
        transparent: true,
        opacity: 0.72,
      }),
    );
    water.rotation.x = -Math.PI * 0.5;
    water.position.y = 0.05;
    group.add(water);
  } else {
    const rockMaterial = disposableMaterial(color);
    for (let index = 0; index < 3; index += 1) {
      const rock = new THREE.Mesh(
        new THREE.DodecahedronGeometry(
          (0.4 + index * 0.12) * quantityRatio + 0.16,
          0,
        ),
        rockMaterial,
      );
      rock.scale.set(1.15, 0.68, 0.9);
      rock.position.set(
        (index - 1) * 0.42,
        0.28 + index * 0.07,
        index % 2 ? 0.24 : -0.18,
      );
      rock.rotation.y = index * 0.72;
      group.add(rock);
    }
  }
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
    cellSize * 0.7,
    (Number(residue.radiusTiles) || 1) * cellSize,
  );
  const patchMaterial = new THREE.MeshStandardMaterial({
    color: 0x493622,
    roughness: 1,
    transparent: true,
    opacity: Math.min(0.54, 0.12 + intensity * 0.42),
    depthWrite: false,
  });

  for (let index = 0; index < 4; index += 1) {
    const seed = hash01(residue.id, index * 41);
    const angle = seed * Math.PI * 2;
    const spread = index === 0 ? 0 : radius * (0.16 + seed * 0.28);
    const patchRadius = radius * (
      index === 0
        ? 0.72
        : 0.28 + hash01(residue.id, index * 67) * 0.25
    );
    const patch = new THREE.Mesh(
      new THREE.CircleGeometry(patchRadius, 28),
      patchMaterial,
    );
    patch.rotation.x = -Math.PI * 0.5;
    patch.position.set(
      Math.cos(angle) * spread,
      0.045 + index * 0.002,
      Math.sin(angle) * spread,
    );
    patch.renderOrder = 3;
    group.add(patch);
  }

  if (intensity >= 0.48) {
    const darkSpot = new THREE.Mesh(
      new THREE.CircleGeometry(radius * 0.22, 22),
      new THREE.MeshBasicMaterial({
        color: 0x241b14,
        transparent: true,
        opacity: 0.5 + intensity * 0.16,
        depthWrite: false,
      }),
    );
    darkSpot.rotation.x = -Math.PI * 0.5;
    darkSpot.position.y = 0.058;
    darkSpot.renderOrder = 4;
    group.add(darkSpot);
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
  private lastSignature = '';
  private animationTime = 0;
  private cameraZoom = 1.25;

  setCameraZoom(zoom: number): void {
    this.cameraZoom = Math.max(0.1, Number(zoom) || 1);
    this.refreshLabelVisibility();
  }

  private refreshLabelVisibility(): void {
    const zoom = this.cameraZoom;
    const minPriority = zoom < 0.9
      ? 4
      : zoom < 1.3
        ? 3
        : zoom < 2.0
          ? 2
          : 1;
    const scaleFactor = Math.max(
      0.72,
      Math.min(1, 0.74 + zoom * 0.12),
    );

    this.group.traverse((object) => {
      if (
        !(object instanceof THREE.Sprite)
        || !object.userData.lifeLensContextLabel
      ) return;
      const priority = Number(object.userData.lifeLensLabelPriority) || 1;
      object.visible = priority >= minPriority;
      object.scale.set(
        5.5 * scaleFactor,
        1.18 * scaleFactor,
        1,
      );
    });
  }

  update(deltaSeconds: number): void {
    this.animationTime += Math.min(0.05, Math.max(0, deltaSeconds));
    const slow = Math.sin(this.animationTime * 8.1);
    const fast = Math.sin(this.animationTime * 17.3 + 0.7);

    this.group.traverse((object) => {
      if (
        object instanceof THREE.PointLight
        && object.userData.lifeLensFireLight
      ) {
        const base = Number(object.userData.baseIntensity) || 1;
        object.intensity = base * (0.9 + slow * 0.055 + fast * 0.035);
      }
      if (
        object instanceof THREE.Mesh
        && object.userData.lifeLensFireFlame
      ) {
        const baseScaleY = Number(object.userData.baseScaleY) || 1;
        object.scale.y = baseScaleY * (0.94 + fast * 0.08);
        object.rotation.y = slow * 0.08;
      }
      if (
        object instanceof THREE.Mesh
        && object.userData.lifeLensSmoke
      ) {
        const baseY = Number(object.userData.baseY) || 0;
        const phase = Number(object.userData.phase) || 0;
        const cycle = (
          this.animationTime * 0.34
          + phase * 0.11
        ) % 1;
        object.position.y = baseY + cycle * 1.35;
        object.position.x = Math.sin(
          this.animationTime * 0.7 + phase,
        ) * 0.18;
        object.scale.setScalar(0.8 + cycle * 0.72);
        const material = object.material;
        if (material instanceof THREE.MeshBasicMaterial) {
          material.opacity = 0.13 * (1 - cycle);
        }
      }
    });
  }

  setSnapshot(
    snapshot: WorldPresentationSnapshot | null,
    terrain: TerrainWindow,
    centerX: number,
    centerY: number,
  ): void {
    const signature = snapshot?.available
      ? [
          centerX,
          centerY,
          terrain.worldSeed ?? '0',
          ...(snapshot.resources ?? []).map((item) => (
            `r:${item.id}:${item.quantity}:${item.gridX}:${item.gridY}`
          )),
          ...(snapshot.facilities ?? []).map((item) => (
            `f:${item.id}:${item.state}:${item.workProgress ?? 0}:${item.durability ?? 0}:${item.lit ? 1 : 0}:${item.gridX}:${item.gridY}`
          )),
          ...(snapshot.storages ?? []).map((item) => (
            `s:${item.id}:${item.totalUnits ?? 0}:${item.gridX}:${item.gridY}`
          )),
          ...(snapshot.sanitationSites ?? []).map((item) => (
            `t:${item.id}:${item.kind}:${item.useCount ?? 0}:${item.improvementProgress ?? 0}:${item.gridX}:${item.gridY}`
          )),
          ...(snapshot.residues ?? []).map((item) => (
            `w:${item.id}:${item.amount ?? 0}:${item.intensity ?? 0}:${item.radiusTiles ?? 0}:${item.gridX}:${item.gridY}`
          )),
        ].join('|')
      : 'unavailable';

    if (signature === this.lastSignature) return;
    this.lastSignature = signature;

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

    const visibleChunks = new Set(
      terrain.chunks.map((chunk) => `${chunk.x}:${chunk.y}`),
    );
    const cells = WORLD_GRID_CONTRACT.gridCellsPerChunk;
    for (const resource of snapshot.resources ?? []) {
      const chunkX = Math.floor(resource.gridX / cells);
      const chunkY = Math.floor(resource.gridY / cells);
      if (!visibleChunks.has(`${chunkX}:${chunkY}`)) continue;
      const object = new THREE.Group();
      object.position.copy(positionAt(resource.gridX, resource.gridY));
      addResourceShape(object, resource);
      this.group.add(object);
    }

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

    const storageByGrid = new Map(
      (snapshot.storages ?? []).map((storage) => [
        `${storage.gridX}:${storage.gridY}`,
        storage,
      ]),
    );
    for (const facility of snapshot.facilities ?? []) {
      const object = new THREE.Group();
      object.position.copy(positionAt(facility.gridX, facility.gridY));
      addFacilityShape(object, facility);
      const storage = storageByGrid.get(
        `${facility.gridX}:${facility.gridY}`,
      );
      if (facility.kind === 'PrimitiveStorage') {
        addStorageContents(object, storage);
      }
      const label = makeLabelSprite(
        facilityLabel(facility, Number(storage?.totalUnits) || 0),
        facilityLabelPriority(facility),
      );
      label.position.y = facility.kind === 'Shelter' ? 5.35 : 3.25;
      object.add(label);
      this.group.add(object);
    }

    this.refreshLabelVisibility();
  }

  dispose(): void {
    this.lastSignature = '';
    for (const child of [...this.group.children]) {
      this.group.remove(child);
      disposeObject(child);
    }
  }
}
