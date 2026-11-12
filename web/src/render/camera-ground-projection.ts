import * as THREE from 'three';

/** Read-only camera/ground intersection, never a simulation terrain query. */
export class CameraGroundProjection {
  private readonly plane = new THREE.Plane(new THREE.Vector3(0, 1, 0), 0);
  private readonly ray = new THREE.Raycaster();
  private readonly pointer = new THREE.Vector2();
  private readonly hit = new THREE.Vector3();

  point(camera: THREE.Camera, x: number, y: number, groundHeight: number): { x: number; z: number } | null {
    if (![x, y, groundHeight].every(Number.isFinite)) return null;
    this.plane.constant = -groundHeight;
    this.pointer.set(x, y);
    camera.updateMatrixWorld();
    this.ray.setFromCamera(this.pointer, camera);
    if (!this.ray.ray.intersectPlane(this.plane, this.hit)) return null;
    // Near-horizon rays must never translate the observer thousands of chunks.
    if (this.hit.distanceTo(camera.position) > 500) return null;
    return { x: this.hit.x, z: this.hit.z };
  }
}
