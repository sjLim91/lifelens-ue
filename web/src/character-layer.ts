import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { clone as cloneSkeleton } from 'three/addons/utils/SkeletonUtils.js';

export type ResidentSex = 'Male' | 'Female';

export interface ResidentPlacement {
  id: string;
  name: string;
  sex: ResidentSex;
  activityLabel: string;
  x: number;
  y: number;
  index: number;
  heightPx: number;
}

const BASE_MODEL_COMMIT = 'ddd5fc34a445bcded3cf9836607aaeebc19a5c78';
const BASE_MODEL_URL =
  `https://raw.githubusercontent.com/programasweights/avatar/${BASE_MODEL_COMMIT}/public/assets/character.glb`;
const ANIMATION_COMMIT = 'aa02a4e6d8337a0604d2da131bcbbeb1f01badf0';
const ANIMATION_URL =
  `https://raw.githubusercontent.com/Seyamalam/blood-league-kickoff/${ANIMATION_COMMIT}/public/assets/vendor/quaternius/universal-animation-library.glb`;

interface Palette {
  skin: string;
  shirt: string;
  pants: string;
  shoes: string;
  hair: string;
}

type MotionName = 'idle' | 'walk' | 'talk' | 'sit' | 'interact';

const RESIDENT_VISIBILITY_GRACE_MS = 10000;

interface Actor {
  root: THREE.Group;
  variantSeed: number;
  sex: ResidentSex;
  activityLabel: string;
  mixer: THREE.AnimationMixer;
  idle?: THREE.AnimationAction;
  walk?: THREE.AnimationAction;
  talk?: THREE.AnimationAction;
  sit?: THREE.AnimationAction;
  interact?: THREE.AnimationAction;
  active: MotionName | '';
  currentX: number;
  currentY: number;
  targetX: number;
  targetY: number;
  lastTargetX: number;
  lastTargetY: number;
  movingUntil: number;
  initialized: boolean;
  lastSeenAt: number;
  heightPx: number;
}

const PALETTES: Palette[] = [
  { skin: '#d5a17f', shirt: '#506f78', pants: '#303943', shoes: '#272622', hair: '#2d211b' },
  { skin: '#b97957', shirt: '#775d4f', pants: '#37343d', shoes: '#262220', hair: '#1d1715' },
  { skin: '#edc3a1', shirt: '#6f7651', pants: '#3c4350', shoes: '#302a25', hair: '#4a3020' },
  { skin: '#9f694d', shirt: '#635c7b', pants: '#2f3940', shoes: '#201f1f', hair: '#201715' },
  { skin: '#c88b66', shirt: '#7a5966', pants: '#35413b', shoes: '#2a2724', hair: '#3b281f' },
  { skin: '#e0ad88', shirt: '#55705a', pants: '#47404d', shoes: '#2d2925', hair: '#261b17' },
];

function stableHash(value: string): number {
  let hash = 2166136261 >>> 0;
  for (let i = 0; i < value.length; i += 1) {
    hash ^= value.charCodeAt(i);
    hash = Math.imul(hash, 16777619) >>> 0;
  }
  hash ^= hash >>> 16;
  return hash >>> 0;
}

function findClip(clips: THREE.AnimationClip[], exact: string, contains: string): THREE.AnimationClip | undefined {
  const lowerExact = exact.toLowerCase();
  return clips.find((clip) => clip.name.toLowerCase() === lowerExact)
    ?? clips.find((clip) => clip.name.toLowerCase().includes(contains.toLowerCase()));
}

function dominantBoneName(mesh: THREE.SkinnedMesh, vertexIndex: number): string {
  const skinIndex = mesh.geometry.getAttribute('skinIndex');
  const skinWeight = mesh.geometry.getAttribute('skinWeight');
  if (!skinIndex || !skinWeight || !mesh.skeleton) return '';

  const indices = [
    skinIndex.getX(vertexIndex),
    skinIndex.getY(vertexIndex),
    skinIndex.getZ(vertexIndex),
    skinIndex.getW(vertexIndex),
  ];
  const weights = [
    skinWeight.getX(vertexIndex),
    skinWeight.getY(vertexIndex),
    skinWeight.getZ(vertexIndex),
    skinWeight.getW(vertexIndex),
  ];

  let slot = 0;
  for (let i = 1; i < weights.length; i += 1) {
    if (weights[i] > weights[slot]) slot = i;
  }
  return mesh.skeleton.bones[Math.round(indices[slot])]?.name.toLowerCase() ?? '';
}

type BodyRegion = 'skin' | 'shirt' | 'pants' | 'shoes';

function bodyRegion(boneName: string): BodyRegion {
  if (
    boneName.includes('foot')
    || boneName.includes('ball')
    || boneName.includes('toe')
  ) return 'shoes';

  if (
    boneName.includes('pelvis')
    || boneName.includes('thigh')
    || boneName.includes('calf')
  ) return 'pants';

  if (
    boneName.includes('spine')
    || boneName.includes('clavicle')
    || boneName.includes('upperarm')
  ) return 'shirt';

  return 'skin';
}

function regionColor(boneName: string, palette: Palette): THREE.Color {
  switch (bodyRegion(boneName)) {
    case 'shirt': return new THREE.Color(palette.shirt);
    case 'pants': return new THREE.Color(palette.pants);
    case 'shoes': return new THREE.Color(palette.shoes);
    default: return new THREE.Color(palette.skin);
  }
}

function deformResidentGeometry(
  mesh: THREE.SkinnedMesh,
  geometry: THREE.BufferGeometry,
  sex: ResidentSex,
  variantSeed: number,
): void {
  const position = geometry.getAttribute('position');
  const normal = geometry.getAttribute('normal');
  if (!position) return;

  const shoulderBias = ((((variantSeed >>> 3) % 9) - 4) * 0.008);
  const hipBias = ((((variantSeed >>> 11) % 9) - 4) * 0.009);
  const depthBias = ((((variantSeed >>> 19) % 7) - 3) * 0.008);

  for (let i = 0; i < position.count; i += 1) {
    const boneName = dominantBoneName(mesh, i);
    const region = bodyRegion(boneName);
    let x = position.getX(i);
    let y = position.getY(i);
    let z = position.getZ(i);

    let widthScale = 1;
    let depthScale = 1;

    if (sex === 'Female') {
      if (boneName.includes('clavicle') || boneName.includes('upperarm')) {
        widthScale = 0.89 + shoulderBias;
        depthScale = 0.94 + depthBias;
      } else if (boneName.includes('spine')) {
        widthScale = 0.91 + (shoulderBias * 0.45);
        depthScale = 0.93 + depthBias;
      } else if (boneName.includes('pelvis')) {
        widthScale = 1.09 + hipBias;
        depthScale = 0.96 + depthBias;
      } else if (boneName.includes('thigh')) {
        widthScale = 1.055 + (hipBias * 0.55);
        depthScale = 0.97 + depthBias;
      } else if (boneName.includes('calf')) {
        widthScale = 0.985 + (hipBias * 0.25);
        depthScale = 0.97 + depthBias;
      } else if (boneName.includes('head') || boneName.includes('neck')) {
        widthScale = 0.96;
        depthScale = 0.96;
      }
    } else {
      if (boneName.includes('clavicle') || boneName.includes('upperarm')) {
        widthScale = 1.025 + shoulderBias;
        depthScale = 1.015 + depthBias;
      } else if (boneName.includes('spine')) {
        widthScale = 1.01 + (shoulderBias * 0.4);
        depthScale = 1.01 + depthBias;
      } else if (boneName.includes('pelvis')) {
        widthScale = 0.985 + hipBias;
      } else if (boneName.includes('thigh')) {
        widthScale = 1.0 + (hipBias * 0.35);
      }
    }

    x *= widthScale;
    z *= depthScale;

    if (normal && region !== 'skin') {
      const garmentThickness = region === 'shirt' ? 0.012
        : region === 'pants' ? 0.009
          : 0.006;
      x += normal.getX(i) * garmentThickness;
      y += normal.getY(i) * garmentThickness;
      z += normal.getZ(i) * garmentThickness;
    }

    position.setXYZ(i, x, y, z);
  }

  position.needsUpdate = true;
  geometry.computeVertexNormals();
  geometry.computeBoundingBox();
  geometry.computeBoundingSphere();
}

function styleModel(
  model: THREE.Group,
  palette: Palette,
  sex: ResidentSex,
  variantSeed: number,
): void {
  model.traverse((object) => {
    if (!(object instanceof THREE.Mesh)) return;

    const meshName = object.name.toLowerCase();
    if (meshName.includes('eye')) {
      object.material = new THREE.MeshStandardMaterial({
        color: '#e9ece7',
        roughness: 0.38,
        metalness: 0,
      });
      return;
    }

    if (meshName.includes('hair') || meshName.includes('brow')) {
      object.material = new THREE.MeshStandardMaterial({
        color: palette.hair,
        roughness: 0.78,
        metalness: 0,
      });
      return;
    }

    if (!(object instanceof THREE.SkinnedMesh)) {
      if (Array.isArray(object.material)) {
        object.material = object.material.map((material) => material.clone());
      } else {
        object.material = object.material.clone();
      }
      return;
    }

    const geometry = object.geometry.clone();
    const position = geometry.getAttribute('position');
    if (!position) return;

    deformResidentGeometry(object, geometry, sex, variantSeed);

    const colors = new Float32Array(position.count * 3);
    for (let i = 0; i < position.count; i += 1) {
      const color = regionColor(dominantBoneName(object, i), palette);
      colors[(i * 3)] = color.r;
      colors[(i * 3) + 1] = color.g;
      colors[(i * 3) + 2] = color.b;
    }
    geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
    object.geometry = geometry;
    object.material = new THREE.MeshStandardMaterial({
      vertexColors: true,
      roughness: 0.78,
      metalness: 0.01,
    });
  });
}

function findHeadBone(model: THREE.Object3D): THREE.Bone | null {
  let found: THREE.Bone | null = null;
  model.traverse((object) => {
    if (found || !(object instanceof THREE.Bone)) return;
    const name = object.name.toLowerCase();
    if (name === 'head' || name.endsWith(':head') || name.includes('head')) found = object;
  });
  return found;
}

function createHairOverlay(sex: ResidentSex, variantSeed: number, palette: Palette): THREE.Group {
  const group = new THREE.Group();
  const material = new THREE.MeshStandardMaterial({
    color: palette.hair,
    roughness: 0.82,
    metalness: 0,
  });
  const style = variantSeed % 3;

  const cap = new THREE.Mesh(
    new THREE.SphereGeometry(0.105, 14, 9, 0, Math.PI * 2, 0, Math.PI * 0.62),
    material,
  );
  cap.position.set(0, 0.935, -0.008);
  cap.scale.set(sex === 'Female' ? 1.03 : 0.96, 0.72, 0.94);
  group.add(cap);

  if (sex === 'Female' && style === 0) {
    const back = new THREE.Mesh(new THREE.CapsuleGeometry(0.075, 0.18, 4, 10), material);
    back.position.set(0, 0.82, -0.06);
    back.scale.set(1.06, 1.0, 0.56);
    group.add(back);
  } else if (sex === 'Female' && style === 1) {
    const bun = new THREE.Mesh(new THREE.SphereGeometry(0.055, 12, 8), material);
    bun.position.set(0, 1.005, -0.05);
    group.add(bun);
  } else if (sex === 'Male' && style === 1) {
    const side = new THREE.Mesh(new THREE.SphereGeometry(0.04, 10, 6), material);
    side.position.set(-0.055, 0.965, 0);
    side.scale.set(1.2, 0.7, 0.75);
    group.add(side);
  }

  return group;
}

export class CharacterLayer {
  private readonly canvas: HTMLCanvasElement;
  private readonly renderer: THREE.WebGLRenderer;
  private readonly scene = new THREE.Scene();
  private readonly camera = new THREE.OrthographicCamera(0, 1, 1, 0, -100, 100);
  private readonly loader = new GLTFLoader();
  private readonly actors = new Map<string, Actor>();
  private readonly onReady?: () => void;

  private template: THREE.Group | null = null;
  private clips: THREE.AnimationClip[] = [];
  private pending: ResidentPlacement[] = [];
  private width = 1;
  private height = 1;
  private readyState = false;
  private failedState = false;
  private lastFrame = performance.now();

  constructor(canvas: HTMLCanvasElement, onReady?: () => void) {
    this.canvas = canvas;
    this.onReady = onReady;
    this.renderer = new THREE.WebGLRenderer({
      canvas,
      alpha: true,
      antialias: true,
      premultipliedAlpha: true,
      powerPreference: 'low-power',
    });
    this.renderer.setClearColor(0x000000, 0);
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;

    this.camera.position.set(0, 0, 10);
    this.camera.lookAt(0, 0, 0);

    this.scene.add(new THREE.HemisphereLight(0xf5f6ef, 0x2f3c34, 2.1));
    const key = new THREE.DirectionalLight(0xffffff, 2.0);
    key.position.set(-3, 6, 8);
    this.scene.add(key);
    const fill = new THREE.DirectionalLight(0xb8c9ff, 0.7);
    fill.position.set(4, 2, 4);
    this.scene.add(fill);

    void this.load();
    requestAnimationFrame(this.animate);
  }

  get ready(): boolean {
    return this.readyState;
  }

  get failed(): boolean {
    return this.failedState;
  }

  clearResidents(): void {
    this.pending = [];
    for (const actor of this.actors.values()) actor.root.visible = false;
  }

  resize(width: number, height: number, dpr: number): void {
    this.width = Math.max(1, width);
    this.height = Math.max(1, height);
    this.renderer.setPixelRatio(Math.min(Math.max(dpr, 1), 2));
    this.renderer.setSize(this.width, this.height, false);

    this.camera.left = 0;
    this.camera.right = this.width;
    this.camera.top = this.height;
    this.camera.bottom = 0;
    this.camera.updateProjectionMatrix();

    for (const actor of this.actors.values()) this.applyActorScale(actor);
  }

  setResidents(placements: ResidentPlacement[]): void {
    this.pending = placements;
    if (!this.readyState || !this.template) return;

    const activeIds = new Set(placements.map((placement) => placement.id));
    const now = performance.now();
    for (const [id, actor] of this.actors) {
      if (!activeIds.has(id) && now - actor.lastSeenAt > RESIDENT_VISIBILITY_GRACE_MS) {
        actor.root.visible = false;
      }
    }

    for (const placement of placements) {
      const actor = this.ensureActor(placement);
      const tx = placement.x;
      const ty = this.height - placement.y;
      if (actor.initialized && Math.hypot(tx - actor.lastTargetX, ty - actor.lastTargetY) > 1.5) {
        actor.movingUntil = now + 900;
        const dx = tx - actor.lastTargetX;
        actor.root.rotation.y = Math.PI + Math.max(-0.34, Math.min(0.34, dx * 0.01));
      }
      actor.activityLabel = placement.activityLabel || 'Idle';
      actor.lastSeenAt = now;
      actor.heightPx = placement.heightPx;
      this.applyActorScale(actor);
      actor.targetX = tx;
      actor.targetY = ty;
      actor.lastTargetX = tx;
      actor.lastTargetY = ty;
      if (!actor.initialized) {
        actor.currentX = tx;
        actor.currentY = ty;
        actor.initialized = true;
      }
      actor.root.visible = true;
    }
  }

  private async load(): Promise<void> {
    try {
      const base = await this.loader.loadAsync(BASE_MODEL_URL);
      let animationClips: THREE.AnimationClip[] = [];
      try {
        const animationAsset = await this.loader.loadAsync(ANIMATION_URL);
        animationClips = animationAsset.animations;
      } catch (error) {
        console.warn('LifeLens character animation asset unavailable; using static human model', error);
      }

      const source = base.scene;
      source.updateMatrixWorld(true);
      const box = new THREE.Box3().setFromObject(source);
      const size = box.getSize(new THREE.Vector3());
      const center = box.getCenter(new THREE.Vector3());
      const modelHeight = Math.max(0.001, size.y);

      source.position.x -= center.x;
      source.position.y -= box.min.y;
      source.position.z -= center.z;
      source.rotation.y = Math.PI;

      const normalized = new THREE.Group();
      normalized.add(source);
      normalized.scale.setScalar(1 / modelHeight);
      this.template = normalized;
      this.clips = animationClips.length > 0 ? animationClips : base.animations;
      this.readyState = true;
      this.failedState = false;
      this.setResidents(this.pending);
      this.onReady?.();
    } catch (error) {
      this.failedState = true;
      this.readyState = false;
      console.warn('LifeLens 3D resident asset unavailable; keeping compact fallback markers', error);
      this.onReady?.();
    }
  }

  private ensureActor(placement: ResidentPlacement): Actor {
    const existing = this.actors.get(placement.id);
    if (existing) return existing;
    if (!this.template) throw new Error('Character template is not loaded');

    const variantSeed = stableHash(placement.id);
    const palette = PALETTES[variantSeed % PALETTES.length];
    const root = new THREE.Group();
    const normalizedModel = cloneSkeleton(this.template) as THREE.Group;
    styleModel(normalizedModel, palette, placement.sex, variantSeed);
    root.add(normalizedModel);

    const hair = createHairOverlay(placement.sex, variantSeed, palette);
    root.add(hair);
    root.updateMatrixWorld(true);
    const headBone = findHeadBone(normalizedModel);
    if (headBone) headBone.attach(hair);

    root.visible = false;
    this.scene.add(root);

    const mixer = new THREE.AnimationMixer(root);
    const idleClip = findClip(this.clips, 'Idle_Loop', 'idle');
    const walkClip = findClip(this.clips, 'Walk_Loop', 'walk');
    const talkClip = findClip(this.clips, 'Idle_Talking_Loop', 'talking');
    const sitClip = findClip(this.clips, 'Sitting_Idle_Loop', 'sitting_idle');
    const interactClip = findClip(this.clips, 'Interact', 'interact');
    const idle = idleClip ? mixer.clipAction(idleClip, root) : undefined;
    const walk = walkClip ? mixer.clipAction(walkClip, root) : undefined;
    const talk = talkClip ? mixer.clipAction(talkClip, root) : undefined;
    const sit = sitClip ? mixer.clipAction(sitClip, root) : undefined;
    const interact = interactClip ? mixer.clipAction(interactClip, root) : undefined;
    [idle, walk, talk, sit, interact].forEach((action) => action?.setLoop(THREE.LoopRepeat, Infinity));
    idle?.play();
    if (idle) idle.time = ((variantSeed % 997) / 997) * Math.max(0.001, idle.getClip().duration);

    const actor: Actor = {
      root,
      variantSeed,
      sex: placement.sex,
      activityLabel: placement.activityLabel || 'Idle',
      mixer,
      idle,
      walk,
      talk,
      sit,
      interact,
      active: idle ? 'idle' : '',
      currentX: 0,
      currentY: 0,
      targetX: 0,
      targetY: 0,
      lastTargetX: 0,
      lastTargetY: 0,
      movingUntil: 0,
      initialized: false,
      lastSeenAt: performance.now(),
      heightPx: placement.heightPx,
    };
    this.actors.set(placement.id, actor);
    this.applyActorScale(actor);
    return actor;
  }

  private applyActorScale(actor: Actor): void {
    const baseHeight = Math.max(12, Math.min(64, actor.heightPx));
    const statureJitter = ((actor.variantSeed >>> 8) % 9 - 4) * 0.012;
    const stature = 1 + statureJitter;
    const individualWidth = (((actor.variantSeed >>> 16) % 7) - 3) * 0.012;
    const width = 1 + individualWidth;
    actor.root.scale.set(baseHeight * width, baseHeight * stature, baseHeight * width);
  }

  private actionFor(actor: Actor, name: MotionName): THREE.AnimationAction | undefined {
    switch (name) {
      case 'walk': return actor.walk;
      case 'talk': return actor.talk;
      case 'sit': return actor.sit;
      case 'interact': return actor.interact;
      default: return actor.idle;
    }
  }

  private restMotion(actor: Actor): MotionName {
    const label = actor.activityLabel;
    if ((label === 'Sleep' || label === 'UseToilet') && actor.sit) return 'sit';
    if ((label === 'Eat' || label === 'Drink' || label === 'Wash') && actor.interact) return 'interact';
    if ((label === 'Approach' || label === 'Repair' || label === 'Comfort') && actor.talk) return 'talk';
    return 'idle';
  }

  private setAction(actor: Actor, moving: boolean): void {
    const desired: MotionName = moving && actor.walk ? 'walk' : this.restMotion(actor);
    if (actor.active === desired) return;

    const previous = actor.active ? this.actionFor(actor, actor.active) : undefined;
    const next = this.actionFor(actor, desired) ?? actor.idle;
    if (!next) return;

    previous?.fadeOut(0.18);
    next.reset().fadeIn(0.18).play();
    const duration = next.getClip().duration;
    if (duration > 0) next.time = ((actor.variantSeed % 997) / 997) * duration;
    actor.active = desired;
  }

  private readonly animate = (frameTime: number): void => {
    const dt = Math.min(0.05, Math.max(0, (frameTime - this.lastFrame) / 1000));
    this.lastFrame = frameTime;
    const ease = 1 - Math.exp(-dt * 10);

    for (const actor of this.actors.values()) {
      if (!actor.root.visible) continue;
      actor.currentX += (actor.targetX - actor.currentX) * ease;
      actor.currentY += (actor.targetY - actor.currentY) * ease;
      actor.root.position.set(actor.currentX, actor.currentY, 0);
      this.setAction(actor, frameTime < actor.movingUntil);
      actor.mixer.update(dt);
    }

    this.renderer.render(this.scene, this.camera);
    requestAnimationFrame(this.animate);
  };
}
