import * as THREE from 'three';
import type { Resident } from '../runtime/core-types';
import type { SocialEventAnchorResolver } from './social-event-presentation';
import { WORLD_PRESENTATION } from './world-presentation-config';
import { ResidentLifeEventPresentation, type LifeCue } from './resident-life-event-presentation';
const C = WORLD_PRESENTATION.lifeEvents;
const RIBBON_VERTICES = [[0,-1],[1,-1],[1,1],[0,-1],[1,1],[0,1]] as const;
type Stroke = (ax:number,ay:number,az:number,bx:number,by:number,bz:number)=>void;
export function lifeEventStrokes(c: LifeCue, p: number, stroke: Stroke): void {
  const ring=(x:number,y:number,z:number,r:number,broken=false)=>{
    for(let i=0;i<C.ringSegments;i++) {
      if(broken && i%4===0) continue;
      const a=i/C.ringSegments*Math.PI*2,b=(i+1)/C.ringSegments*Math.PI*2;
      stroke(x+Math.cos(a)*r,y,z+Math.sin(a)*r,x+Math.cos(b)*r,y,z+Math.sin(b)*r);
    }
  };
  const a=c.actor,b=c.target,y=a.y+C.height;
  const radius=C.radius*(0.7+p*0.7);
  if(c.kind==='relationship' && b) {
    for(let i=0;i<16;i++) {
      const t=i/16,n=(i+1)/16;
      stroke(a.x+(b.x-a.x)*t,y+Math.sin(t*Math.PI)*.4,a.z+(b.z-a.z)*t,
        a.x+(b.x-a.x)*n,y+Math.sin(n*Math.PI)*.4,a.z+(b.z-a.z)*n);
    }
    ring((a.x+b.x)/2,y,(a.z+b.z)/2,C.radius*.6);
  } else if(c.kind==='separation' && b) {
    const dx=b.x-a.x,dz=b.z-a.z,gap=.1+p*.3;
    stroke(a.x,y,a.z,a.x+dx*(.5-gap),y,a.z+dz*(.5-gap));
    stroke(a.x+dx*(.5+gap),y,a.z+dz*(.5+gap),b.x,b.y+C.height,b.z);
  } else {
    ring(a.x,y,a.z,radius,c.kind==='loss'||c.kind==='separation');
    if(c.kind==='birth' && b) {
      stroke(a.x,y,a.z,b.x,b.y+C.height,b.z);
      ring(a.x,y,a.z,radius*.55);
    }
    if(c.kind==='growth')ring(a.x,y+.12*p,a.z,radius*.6);
  }
}
export class ResidentLifeEventLayer {
  readonly group = new THREE.Group();
  readonly presentation: ResidentLifeEventPresentation;
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
  private readonly positions = new Float32Array(C.maxActive * C.segmentsPerCue * 6 * 3);
  private readonly colors = new Float32Array(this.positions.length);
  private readonly opacities = new Float32Array(this.positions.length / 3);
  private readonly color = new THREE.Color();
  private readonly direction = new THREE.Vector3();
  private readonly view = new THREE.Vector3();
  private readonly normal = new THREE.Vector3();
  private disposed = false;
  private elapsed = 0;
  private paused = false;

  constructor(resolve: SocialEventAnchorResolver, nowSeconds?: () => number) {
    this.presentation = new ResidentLifeEventPresentation(resolve, nowSeconds ?? (() => this.elapsed));
    this.geometry.setAttribute('position', new THREE.BufferAttribute(this.positions, 3).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setAttribute('cueColor', new THREE.BufferAttribute(this.colors, 3).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setAttribute('cueOpacity', new THREE.BufferAttribute(this.opacities, 1).setUsage(THREE.DynamicDrawUsage));
    this.geometry.setDrawRange(0, 0);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = 3;
    this.mesh.visible = false;
    this.group.add(this.mesh);
  }

  setSimulationSpeed(speed: number): void { this.paused = speed <= 0; }
  captureDepartures(residents: Resident[]): void {this.presentation.captureDepartures(residents);}
  observe(residents: Resident[], minute: number): void {
    if (!this.disposed) this.presentation.observe(residents, minute);
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

  update(camera: THREE.Camera, deltaSeconds: number): void {
    if (!this.paused) this.elapsed += Math.max(0, deltaSeconds);
    if (this.disposed) return;
    this.presentation.update();
    const now = this.presentation.now();
    let vertex = 0;
    for (const cue of this.presentation.activeCues) {
      const progress = Math.max(0, Math.min(1, (now - cue.start) / cue.duration));
      const opacity = C.opacity * Math.min(1, progress * 8) * Math.min(1, (1 - progress) * 4);
      this.color.setHex(C.colors[cue.kind]);
      let segments = 0;
      lifeEventStrokes(cue, progress, (ax, ay, az, bx, by, bz) => {
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

