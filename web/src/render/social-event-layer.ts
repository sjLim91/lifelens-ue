import * as THREE from 'three';
import type { RecentSocialEventsPayload } from '../runtime/core-types';
import { SOCIAL_EVENT_PRESENTATION_CONTRACT as C } from '../runtime/lifelens-contract';
import {
  SocialEventPresentation,
  type ActiveSocialEventCue,
  type SocialEventAnchor,
  type SocialEventAnchorResolver,
} from './social-event-presentation';

type Stroke = (ax: number, ay: number, az: number, bx: number, by: number, bz: number) => void;
const RIBBON_VERTICES = [[0, -1], [1, -1], [1, 1], [0, -1], [1, 1], [0, 1]] as const;

// One semantic cue per exact event. All animation is geometry, never a resident
// transform or a statement about relationship deltas/marriage.
export function socialEventStrokes(cue: ActiveSocialEventCue, progress: number, stroke: Stroke): void {
  const a = cue.actor, b = cue.target, scale = cue.visual.scale;
  const dx = b.x - a.x, dz = b.z - a.z;
  const distance = Math.hypot(dx, dz);
  const ux = distance > 0 ? dx / distance : 1;
  const uz = distance > 0 ? dz / distance : 0;
  const nx = -uz, nz = ux;
  const point = (t: number): SocialEventAnchor => ({
    x: a.x + dx * t, y: a.y + (b.y - a.y) * t + C.connectorHeight, z: a.z + dz * t,
  });
  const line = (from: number, to: number, side = 0): void => {
    const p = point(from), q = point(to);
    stroke(p.x + nx * side, p.y, p.z + nz * side, q.x + nx * side, q.y, q.z + nz * side);
  };
  const ring = (t: number, radius: number, start = 0, end = Math.PI * 2): void => {
    const p = point(t);
    for (let i = 0; i < C.pulseSegments; i++) {
      const r = start + (end - start) * i / C.pulseSegments;
      const s = start + (end - start) * (i + 1) / C.pulseSegments;
      stroke(p.x + Math.cos(r) * radius, p.y, p.z + Math.sin(r) * radius,
        p.x + Math.cos(s) * radius, p.y, p.z + Math.sin(s) * radius);
    }
  };
  const arc = (side: number): void => {
    for (let i = 0; i < C.pulseSegments; i++) {
      const t = i / C.pulseSegments, next = (i + 1) / C.pulseSegments;
      const p = point(t), q = point(next);
      const lift = Math.sin(Math.PI * t), nextLift = Math.sin(Math.PI * next);
      stroke(p.x + nx * side * lift, p.y + C.arcLift * lift, p.z + nz * side * lift,
        q.x + nx * side * nextLift, q.y + C.arcLift * nextLift, q.z + nz * side * nextLift);
    }
  };
  switch (cue.visual.kind) {
    case 'PositiveInteraction':
      ring(0.5, scale * (0.5 + progress));
      break;
    case 'Help':
      ring(Math.min(cue.visual.successful ? 1 : 0.7, progress * 1.5), scale * 0.65);
      break;
    case 'Comfort':
      ring(0.7, scale * (1.4 - progress * 0.65), Math.PI * 0.15, Math.PI * 1.85);
      break;
    case 'Conflict':
      for (let i = 0; i < 8; i++) {
        const p = point(i / 8), q = point((i + 1) / 8);
        const offset = (i % 2 ? 1 : -1) * C.conflictAmplitude * Math.sin(Math.PI * i / 8);
        const next = (i % 2 ? -1 : 1) * C.conflictAmplitude * Math.sin(Math.PI * (i + 1) / 8);
        stroke(p.x + nx * offset, p.y, p.z + nz * offset,
          q.x + nx * next, q.y, q.z + nz * next);
      }
      break;
    case 'Betrayal': {
      const gap = 0.08 + progress * 0.3;
      line(0, 0.5 - gap); line(0.5 + gap, 1);
      ring(1, scale * (0.6 + progress), Math.PI * 0.2, Math.PI * 1.7);
      break;
    }
    case 'Rejection': {
      const forward = progress < 0.4;
      const t = forward ? progress / 0.4 * 0.85 : 0.85 * (1 - (progress - 0.4) / 0.6);
      const p = point(t), sign = forward ? 1 : -1;
      const length = C.arrowLength;
      stroke(p.x, p.y, p.z, p.x - ux * sign * length + nx * length / 2,
        p.y, p.z - uz * sign * length + nz * length / 2);
      stroke(p.x, p.y, p.z, p.x - ux * sign * length - nx * length / 2,
        p.y, p.z - uz * sign * length - nz * length / 2);
      const barrier = point(0.9);
      stroke(barrier.x - nx * scale, barrier.y, barrier.z - nz * scale,
        barrier.x + nx * scale, barrier.y, barrier.z + nz * scale);
      break;
    }
    case 'Apology': {
      const reach = Math.min(cue.visual.successful ? 0.5 : 0.36, 0.12 + progress * 0.6);
      line(0, reach); line(1 - reach, 1);
      // Recovery family, without claiming Core conflict has been resolved.
      break;
    }
    case 'Intimacy':
      ring(0.15 + progress * 0.22, scale * 0.7);
      ring(0.85 - progress * 0.22, scale * 0.7);
      break;
    case 'Commitment':
      arc(scale * 0.6); arc(-scale * 0.6);
      break;
  }
}

export class SocialEventLayer {
  readonly group = new THREE.Group();
  readonly presentation: SocialEventPresentation;
  private readonly geometry = new THREE.BufferGeometry();
  private readonly material = new THREE.ShaderMaterial({
    transparent: true, depthWrite: false, depthTest: true,
    side: THREE.DoubleSide, toneMapped: false,
    vertexShader: `
      attribute vec3 cueColor;
      attribute float cueOpacity;
      varying vec3 vCueColor;
      varying float vCueOpacity;
      void main() {
        vCueColor = cueColor; vCueOpacity = cueOpacity;
        gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
      }`,
    fragmentShader: `
      varying vec3 vCueColor;
      varying float vCueOpacity;
      void main() {
        gl_FragColor = vec4(vCueColor, vCueOpacity);
        #include <colorspace_fragment>
      }`,
  });
  private readonly mesh = new THREE.Mesh(this.geometry, this.material);
  private readonly positions = new Float32Array(C.maxActiveCues * C.segmentsPerCue * 6 * 3);
  private readonly colors = new Float32Array(this.positions.length);
  private readonly opacities = new Float32Array(this.positions.length / 3);
  private readonly color = new THREE.Color();
  private readonly direction = new THREE.Vector3();
  private readonly view = new THREE.Vector3();
  private readonly normal = new THREE.Vector3();
  private disposed = false;

  constructor(private readonly resolve: SocialEventAnchorResolver, nowSeconds?: () => number) {
    this.presentation = new SocialEventPresentation(nowSeconds);
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.positions, 3).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setAttribute('cueColor', new THREE.BufferAttribute(this.colors, 3).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setAttribute('cueOpacity', new THREE.BufferAttribute(this.opacities, 1).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setDrawRange(0, 0);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = C.renderOrder;
    this.mesh.visible = false;
    this.group.add(this.mesh);
  }

  observe(payload: RecentSocialEventsPayload, minute: number): void {
    if (!this.disposed) this.presentation.observe(payload, minute, this.resolve);
  }
  reset(): void {
    this.presentation.reset();
    this.geometry.setDrawRange(0, 0);
    this.mesh.visible = false;
  }
  clearActive(): void {
    this.presentation.clearActive();
    this.geometry.setDrawRange(0, 0);
    this.mesh.visible = false;
  }
  rebase(dx: number, dz: number): void { this.presentation.rebase(dx, dz); }

  update(camera: THREE.Camera): void {
    if (this.disposed) return;
    this.presentation.update(this.resolve);
    const now = this.presentation.now();
    let vertex = 0;
    for (const cue of this.presentation.activeCues) {
      const progress = Math.max(0, Math.min(1, (now - cue.startedAtSeconds) / cue.visual.durationSeconds));
      const opacity = cue.visual.opacity * Math.min(1, progress * 8) * Math.min(1, (1 - progress) * 4);
      this.color.setHex(cue.visual.color);
      let segments = 0;
      socialEventStrokes(cue, progress, (ax, ay, az, bx, by, bz) => {
        if (segments++ >= C.segmentsPerCue) return;
        this.direction.set(bx - ax, by - ay, bz - az);
        this.view.set(camera.position.x - (ax + bx) / 2, camera.position.y - (ay + by) / 2,
          camera.position.z - (az + bz) / 2);
        this.normal.crossVectors(this.direction, this.view).normalize().multiplyScalar(C.strokeWidth / 2);
        for (const [end, side] of RIBBON_VERTICES) {
          const offset = vertex * 3;
          this.positions[offset] = (end ? bx : ax) + this.normal.x * side;
          this.positions[offset + 1] = (end ? by : ay) + this.normal.y * side;
          this.positions[offset + 2] = (end ? bz : az) + this.normal.z * side;
          this.colors[offset] = this.color.r; this.colors[offset + 1] = this.color.g;
          this.colors[offset + 2] = this.color.b; this.opacities[vertex++] = opacity;
        }
      });
    }
    this.geometry.setDrawRange(0, vertex);
    this.mesh.visible = vertex > 0;
    if (vertex > 0) {
      for (const attribute of Object.values(this.geometry.attributes)) attribute.needsUpdate = true;
    }
  }

  dispose(): void {
    if (this.disposed) return;
    this.disposed = true;
    this.reset();
    this.group.clear();
    this.geometry.dispose();
    this.material.dispose();
  }
}
