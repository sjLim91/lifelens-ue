import * as THREE from 'three';

/**
 * Clamp a Core-authorized gather gesture to the actual resident's arm range.
 * This is NOT a resource pickup, interaction target, Core position, or reach
 * permission. The resident still faces the real node, which is kept separate.
 * Caller owns all vectors; update never allocates a vector per resident/frame.
 */
export function boundedVisualWorkReach(
  shoulder: THREE.Vector3,
  actualWorkPoint: THREE.Vector3,
  residentHeight: number,
  output: THREE.Vector3,
): THREE.Vector3 | null {
  if (!Number.isFinite(shoulder.x + shoulder.y + shoulder.z)
    || !Number.isFinite(actualWorkPoint.x + actualWorkPoint.y + actualWorkPoint.z)
    || !Number.isFinite(residentHeight) || residentHeight <= 0) return null;
  const maxReach = residentHeight * 0.58;
  output.subVectors(actualWorkPoint, shoulder);
  const distanceSq = output.lengthSq();
  if (distanceSq > maxReach * maxReach) {
    output.multiplyScalar(maxReach / Math.sqrt(distanceSq));
  }
  return output.add(shoulder);
}
