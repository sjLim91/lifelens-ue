import * as THREE from 'three';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import type { WildlifeSpecies } from '../runtime/core-types';

/** Original, locally authored meshes. Metres, forward +Z, soles at Y=0.
 * Shared vertex colours keep each species in one instanced draw call. */
export function createWildlifeGeometry(species: WildlifeSpecies): THREE.BufferGeometry {
  const parts: THREE.BufferGeometry[] = [];
  const oval = (color: number, x: number, y: number, z: number,
    sx: number, sy: number, sz: number, tilt = 0): void => {
    const geometry = new THREE.SphereGeometry(1, 10, 6);
    geometry.scale(sx, sy, sz);
    geometry.rotateZ(tilt);
    geometry.translate(x, y, z);
    const tint = new THREE.Color(color);
    const colors = new Float32Array(geometry.getAttribute('position').count * 3);
    for (let i = 0; i < colors.length; i += 3) tint.toArray(colors, i);
    geometry.setAttribute('color', new THREE.BufferAttribute(colors, 3));
    parts.push(geometry);
  };
  const eyes = (width: number, y: number, z: number): void => {
    for (const side of [-1, 1]) oval(0x181711, side * width, y, z, .018, .022, .02);
  };
  if (species === 'Hare') {
    oval(0x8b7760, 0, .22, -.04, .14, .18, .25);
    oval(0xb5a48a, 0, .31, .2, .1, .105, .12);
    for (const side of [-1, 1]) {
      oval(0x89715b, side * .052, .51, .17, .028, .16, .038, side * -.12);
      oval(0xc9aaa0, side * .052, .52, .198, .012, .12, .009, side * -.12);
      oval(0x705f4c, side * .085, .065, .14, .042, .065, .085);
      oval(0x9c896f, side * .12, .085, -.15, .065, .085, .11);
    }
    oval(0xe2d8c1, 0, .22, -.29, .065, .065, .055);
    oval(0x795b51, 0, .3, .312, .02, .018, .016);
    eyes(.086, .344, .238);
  } else if (species === 'Deer') {
    oval(0x94704c, 0, .76, -.1, .22, .27, .52);
    oval(0xb38b5e, 0, 1.04, .37, .12, .33, .15);
    oval(0x99724f, 0, 1.3, .48, .13, .14, .24);
    oval(0x39332c, 0, 1.26, .69, .09, .06, .04);
    oval(0xd9c8a5, 0, 1.22, .53, .08, .055, .18);
    for (const side of [-1, 1]) {
      oval(0xb68c61, side * .18, 1.4, .38, .14, .045, .07, side * .4);
      for (const z of [-.43, .25]) {
        oval(0x856242, side * .14, .36, z, .045, .34, .052);
        oval(0x403a31, side * .14, .035, z + .015, .048, .035, .07);
      }
    }
    oval(0xe0d2b5, 0, .91, -.63, .07, .12, .06, -.4);
    eyes(.117, 1.335, .53);
  } else if (species === 'Wolf') {
    oval(0x787a72, 0, .56, -.08, .19, .23, .46);
    oval(0x85877d, 0, .69, .33, .18, .23, .19);
    oval(0x6f746d, 0, .8, .48, .135, .13, .18);
    oval(0xb7b7a3, 0, .74, .64, .085, .065, .16);
    oval(0x222622, 0, .755, .79, .05, .04, .035);
    for (const side of [-1, 1]) {
      oval(0x575e56, side * .09, .97, .43, .048, .12, .048, side * -.18);
      for (const z of [-.36, .27]) {
        oval(0x797d72, side * .12, .25, z, .048, .23, .06);
        oval(0xa1a28e, side * .12, .035, z + .045, .065, .035, .095);
      }
    }
    oval(0x60685f, 0, .44, -.59, .09, .12, .32, -.35);
    eyes(.117, .832, .535);
  } else {
    oval(0x799c94, 0, .09, 0, .05, .085, .2);
    oval(0xc4d1b7, 0, .052, .03, .043, .038, .17);
    for (const side of [-1, 1]) oval(0x52786c, side * .035, .105, -.22, .025, .11, .055, side * .2);
    oval(0x52786c, 0, .172, -.03, .013, .075, .09);
    eyes(.045, .11, .125);
  }
  const merged = mergeGeometries(parts);
  for (const part of parts) part.dispose();
  if (!merged) throw new Error('Wildlife geometry merge failed');
  merged.computeBoundingBox();
  merged.translate(0, -merged.boundingBox!.min.y, 0);
  merged.computeBoundingBox();
  merged.computeBoundingSphere();
  return merged;
}
