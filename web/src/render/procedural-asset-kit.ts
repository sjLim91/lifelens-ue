import * as THREE from 'three';

/** Presentation RNG only. Never consumes the Core RNG or a frame clock. */
export function assetRandom(seed: string): () => number {
  let state = 2166136261;
  for (const c of `lifelens:asset:v1:${seed}`) state = Math.imul(state ^ c.charCodeAt(0), 16777619);
  return () => {
    state += 0x6d2b79f5;
    let t = Math.imul(state ^ (state >>> 15), 1 | state);
    t ^= t + Math.imul(t ^ (t >>> 7), 61 | t);
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}

/** Small indexed mesh writer shared by branches, blades, leaves and raw timber. */
export class AssetMeshBuilder {
  private positions: number[] = [];
  private colors: number[] = [];
  private indices: number[] = [];
  private ranks: number[] = [];

  vertex(point: THREE.Vector3, color: THREE.Color, rank = 0): number {
    const index = this.positions.length / 3;
    this.positions.push(point.x, point.y, point.z);
    this.colors.push(color.r, color.g, color.b);
    this.ranks.push(rank);
    return index;
  }
  triangle(a: number, b: number, c: number): void { this.indices.push(a, b, c); }

  tube(points: THREE.Vector3[], radii: number[], sides: number, color: THREE.Color): void {
    const rings: number[][] = [];
    const axis = new THREE.Vector3(), u = new THREE.Vector3(), v = new THREE.Vector3();
    points.forEach((point, ring) => {
      axis.subVectors(points[Math.min(points.length - 1, ring + 1)], points[Math.max(0, ring - 1)]).normalize();
      u.crossVectors(axis, Math.abs(axis.y) > 0.9 ? new THREE.Vector3(1, 0, 0) : new THREE.Vector3(0, 1, 0)).normalize();
      v.crossVectors(axis, u).normalize();
      rings.push(Array.from({ length: sides }, (_, side) => {
        const angle = side / sides * Math.PI * 2;
        const radius = radii[ring] * (1 + Math.sin(side * 5.1 + ring * 0.4) * 0.07);
        const p = point.clone().addScaledVector(u, Math.cos(angle) * radius).addScaledVector(v, Math.sin(angle) * radius);
        return this.vertex(p, color.clone().multiplyScalar(0.86 + side / sides * 0.24));
      }));
    });
    for (let ring = 1; ring < rings.length; ring++) for (let side = 0; side < sides; side++) {
      const next = (side + 1) % sides, a = rings[ring - 1], b = rings[ring];
      this.triangle(a[side], a[next], b[side]); this.triangle(b[side], a[next], b[next]);
    }
    // Seal ends: cut timber shows a real end face rather than a hollow cylinder.
    for (const [ring, reverse] of [[0, true], [points.length - 1, false]] as const) {
      const center = this.vertex(points[ring], color.clone().multiplyScalar(1.16));
      for (let side = 0; side < sides; side++) {
        const a = rings[ring][side], b = rings[ring][(side + 1) % sides];
        this.triangle(center, reverse ? b : a, reverse ? a : b);
      }
    }
  }

  /** Folded pointed leaf, with a ridge that catches light from both directions. */
  leaf(center: THREE.Vector3, length: number, width: number, yaw: number, tilt: number,
    color: THREE.Color, rank = 0): void {
    const axis = new THREE.Vector3(Math.cos(yaw) * Math.cos(tilt), Math.sin(tilt), Math.sin(yaw) * Math.cos(tilt));
    const across = new THREE.Vector3(-Math.sin(yaw), 0, Math.cos(yaw));
    const points = [center.clone().addScaledVector(axis, -length * 0.5),
      center.clone().addScaledVector(across, width * 0.5),
      center.clone().addScaledVector(axis, length * 0.5),
      center.clone().addScaledVector(across, -width * 0.5),
      center.clone().add(new THREE.Vector3(0, width * 0.14, 0))];
    const ids = points.map(p => this.vertex(p, color, rank));
    for (let i = 0; i < 4; i++) this.triangle(ids[i], ids[(i + 1) % 4], ids[4]);
  }

  geometry(): THREE.BufferGeometry {
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.positions, 3));
    g.setAttribute('color', new THREE.Float32BufferAttribute(this.colors, 3));
    g.setAttribute('llLeafRank', new THREE.Float32BufferAttribute(this.ranks, 1));
    g.setIndex(this.indices); g.computeVertexNormals(); g.computeBoundingBox(); g.computeBoundingSphere();
    return g;
  }
}

export function createTimberGeometry(seed: string, sides = 9): THREE.BufferGeometry {
  const random = assetRandom(seed), b = new AssetMeshBuilder();
  const points = Array.from({ length: 5 }, (_, i) => new THREE.Vector3(
    i === 0 || i === 4 ? 0 : (random() - 0.5) * 0.055, i / 4 - 0.5,
    i === 0 || i === 4 ? 0 : (random() - 0.5) * 0.055));
  b.tube(points, [0.5, 0.49, 0.47, 0.46, 0.44], sides, new THREE.Color(0xffffff));
  return b.geometry();
}

export function createStoneGeometry(seed: string, radius = 1): THREE.BufferGeometry {
  const g = new THREE.IcosahedronGeometry(radius, 1), pos = g.getAttribute('position');
  // Coordinate hash keeps duplicated face vertices welded without a random seam.
  for (let i = 0; i < pos.count; i++) {
    const x = pos.getX(i), y = pos.getY(i), z = pos.getZ(i);
    const r = assetRandom(`${seed}:${x.toFixed(4)}:${y.toFixed(4)}:${z.toFixed(4)}`)();
    const scale = 0.83 + r * 0.22;
    pos.setXYZ(i, x * scale, y * scale, z * scale);
  }
  g.computeVertexNormals(); g.computeBoundingBox(); g.computeBoundingSphere(); return g;
}

export function createPlankGeometry(): THREE.BufferGeometry {
  const shape = new THREE.Shape();
  shape.moveTo(-.475, -.475); shape.lineTo(.475, -.475);
  shape.lineTo(.475, .475); shape.lineTo(-.475, .475); shape.closePath();
  const g = new THREE.ExtrudeGeometry(shape, { depth: .95, bevelEnabled: true,
    bevelThickness: .025, bevelSize: .025, bevelSegments: 1, steps: 1 });
  g.translate(0, 0, -.475); g.computeBoundingBox(); g.computeBoundingSphere(); return g;
}

/** Open storage vessel with a thick lip and inner wall; no downloaded model. */
export function createVesselGeometry(): THREE.BufferGeometry {
  const profile = [[0, -.5], [.3, -.5], [.43, -.38], [.5, -.05], [.43, .3],
    [.34, .43], [.36, .5], [.29, .5], [.27, .4], [.36, .25], [.42, -.04], [.35, -.32], [0, -.38]];
  const g = new THREE.LatheGeometry(profile.map(([x, y]) => new THREE.Vector2(x, y)), 12);
  g.computeBoundingBox(); g.computeBoundingSphere(); return g;
}

export function createGrassGeometry(seed: string, blades = 7): THREE.BufferGeometry {
  const random = assetRandom(seed), b = new AssetMeshBuilder(), white = new THREE.Color(0xffffff);
  for (let i = 0; i < blades; i++) {
    const yaw = random() * Math.PI * 2, height = 0.35 + random() * 0.48, width = 0.025 + random() * 0.04;
    const base = new THREE.Vector3((random() - 0.5) * 0.23, 0, (random() - 0.5) * 0.23);
    const across = new THREE.Vector3(Math.cos(yaw), 0, Math.sin(yaw));
    const bend = new THREE.Vector3(Math.sin(yaw), 0, -Math.cos(yaw)).multiplyScalar(height * 0.35);
    const left = b.vertex(base.clone().addScaledVector(across, -width), white.clone().multiplyScalar(0.75));
    const right = b.vertex(base.clone().addScaledVector(across, width), white.clone().multiplyScalar(0.75));
    const mid = base.clone().addScaledVector(bend, 0.25); mid.y = height * 0.55;
    const ml = b.vertex(mid.clone().addScaledVector(across, -width * 0.62), white);
    const mr = b.vertex(mid.clone().addScaledVector(across, width * 0.62), white);
    const tip = base.clone().add(bend); tip.y = height;
    const top = b.vertex(tip, white.clone().multiplyScalar(1.08));
    b.triangle(left, right, ml); b.triangle(right, mr, ml); b.triangle(ml, mr, top);
  }
  return b.geometry();
}

/** Grain is analytical and local to each assembled component: no texture fetch. */
export function installWoodGrain(material: THREE.MeshStandardMaterial): void {
  material.userData.llGrain = true;
  const previous = material.onBeforeCompile;
  const previousKey = material.customProgramCacheKey();
  material.onBeforeCompile = (shader, renderer) => {
    previous.call(material, shader, renderer);
    shader.vertexShader = 'varying vec3 llGrainPosition;\n' + shader.vertexShader;
    shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>',
      '#include <begin_vertex>\nllGrainPosition = position;');
    shader.fragmentShader = 'varying vec3 llGrainPosition;\n' + shader.fragmentShader;
    shader.fragmentShader = shader.fragmentShader.replace('#include <color_fragment>', `
      #include <color_fragment>
      float llGrain = sin(llGrainPosition.x * 75.0 + sin(llGrainPosition.y * 7.0) * 1.6 + llGrainPosition.z * 43.0);
      diffuseColor.rgb *= 0.91 + 0.09 * llGrain;
    `);
  };
  material.customProgramCacheKey = () => `${previousKey}:wood-grain-v1`;
}

export function installFiberWeave(material: THREE.MeshStandardMaterial): void {
  material.userData.llWeave = true;
  material.onBeforeCompile = shader => {
    shader.vertexShader = 'varying vec3 llFiberPosition;\n' + shader.vertexShader;
    shader.vertexShader = shader.vertexShader.replace('#include <begin_vertex>',
      '#include <begin_vertex>\nllFiberPosition = position;');
    shader.fragmentShader = 'varying vec3 llFiberPosition;\n' + shader.fragmentShader;
    shader.fragmentShader = shader.fragmentShader.replace('#include <color_fragment>', `
      #include <color_fragment>
      float llWarp = sin(llFiberPosition.x * 110.0), llWeft = sin(llFiberPosition.z * 92.0);
      diffuseColor.rgb *= 0.89 + 0.06 * llWarp + 0.05 * llWeft;
    `);
  };
  material.customProgramCacheKey = () => 'lifelens-fiber-weave-v1';
}
