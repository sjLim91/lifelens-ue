import * as THREE from 'three';
import type { WaterKind } from '../runtime/core-types';

interface WaterSurfaceProfile {
  deep: number;
  shallow: number;
  sky: number;
  opacity: number;
  waveScale: number;
  flowSpeed: number;
}

const WATER_PROFILES: Record<Exclude<WaterKind, 'None'>, WaterSurfaceProfile> = {
  Spring: {
    deep: 0x326e78,
    shallow: 0x6fb7b8,
    sky: 0xaed0d3,
    opacity: 0.76,
    waveScale: 0.46,
    flowSpeed: 1.45,
  },
  Stream: {
    deep: 0x285f70,
    shallow: 0x65aebe,
    sky: 0xa8cbd2,
    opacity: 0.78,
    waveScale: 0.5,
    flowSpeed: 1.6,
  },
  River: {
    deep: 0x24576b,
    shallow: 0x4f91a9,
    sky: 0xa2c3cb,
    opacity: 0.8,
    waveScale: 0.58,
    flowSpeed: 1.25,
  },
  Lake: {
    deep: 0x244f63,
    shallow: 0x4d8596,
    sky: 0xa0bec4,
    opacity: 0.82,
    waveScale: 0.34,
    flowSpeed: 0.42,
  },
  Wetland: {
    deep: 0x355b55,
    shallow: 0x5e8070,
    sky: 0x9bb2a6,
    opacity: 0.7,
    waveScale: 0.22,
    flowSpeed: 0.28,
  },
  Coast: {
    deep: 0x1f5268,
    shallow: 0x4e8798,
    sky: 0xa2c2c8,
    opacity: 0.84,
    waveScale: 0.62,
    flowSpeed: 0.62,
  },
  Ocean: {
    deep: 0x163f5a,
    shallow: 0x39778c,
    sky: 0x9cbcc7,
    opacity: 0.88,
    waveScale: 0.78,
    flowSpeed: 0.72,
  },
};

export interface WaterSurfaceEnvironment {
  wind01: number;
  rain01: number;
  daylight01: number;
}

export function createWaterSurfaceMaterial(
  kind: Exclude<WaterKind, 'None'>,
): THREE.ShaderMaterial {
  const profile = WATER_PROFILES[kind];

  return new THREE.ShaderMaterial({
    uniforms: {
      uTime: { value: 0 },
      uDeepColor: { value: new THREE.Color(profile.deep) },
      uShallowColor: { value: new THREE.Color(profile.shallow) },
      uSkyColor: { value: new THREE.Color(profile.sky) },
      uOpacity: { value: profile.opacity },
      uWaveScale: { value: profile.waveScale },
      uFlowSpeed: { value: profile.flowSpeed },
      uFlow: { value: new THREE.Vector2(0, 0) },
      uWind: { value: 0 },
      uRain: { value: 0 },
      uDaylight: { value: 1 },
    },
    vertexShader: `
      varying vec3 vWorldPosition;

      void main() {
        vec4 world = modelMatrix * vec4(position, 1.0);
        vWorldPosition = world.xyz;
        gl_Position = projectionMatrix * viewMatrix * world;
      }
    `,
    fragmentShader: `
      uniform float uTime;
      uniform vec3 uDeepColor;
      uniform vec3 uShallowColor;
      uniform vec3 uSkyColor;
      uniform float uOpacity;
      uniform float uWaveScale;
      uniform float uFlowSpeed;
      uniform vec2 uFlow;
      uniform float uWind;
      uniform float uRain;
      uniform float uDaylight;

      varying vec3 vWorldPosition;

      float waveField(vec2 p, float t) {
        float a = sin(
          dot(p, vec2(1.35, 0.52))
          + t * (0.82 + uFlowSpeed * 0.55)
        );
        float b = sin(
          dot(p, vec2(-0.68, 1.54))
          - t * (0.54 + uFlowSpeed * 0.32)
        );
        float c = sin(
          dot(p, vec2(2.15, -1.32))
          + t * (0.36 + uWind * 1.15)
        );
        return a * 0.48 + b * 0.34 + c * 0.18;
      }

      vec2 waveGradient(vec2 p, float t) {
        float eps = 0.035;
        float center = waveField(p, t);
        return vec2(
          waveField(p + vec2(eps, 0.0), t) - center,
          waveField(p + vec2(0.0, eps), t) - center
        ) / eps;
      }

      void main() {
        float t = uTime;
        vec2 flow = length(uFlow) > 0.001
          ? normalize(uFlow)
          : vec2(0.68, 0.36);
        vec2 p = vWorldPosition.xz * 0.34
          - flow * t * uFlowSpeed * 0.26;

        float wave = waveField(p, t);
        vec2 gradient = waveGradient(p, t);

        float rainMicro = sin(
          (p.x * 6.3 + p.y * 7.1)
          + t * 10.0
        ) * uRain;
        gradient += vec2(rainMicro, -rainMicro) * 0.045;

        vec3 normal = normalize(vec3(
          -gradient.x * uWaveScale * 0.24,
          1.0,
          -gradient.y * uWaveScale * 0.24
        ));
        vec3 viewDirection = normalize(cameraPosition - vWorldPosition);

        float fresnel = pow(
          1.0 - clamp(dot(normal, viewDirection), 0.0, 1.0),
          2.35
        );
        float depthMix = clamp(
          0.48 + wave * 0.12 + uRain * 0.04,
          0.2,
          0.78
        );

        vec3 water = mix(uDeepColor, uShallowColor, depthMix);
        vec3 reflectedSky = mix(
          uSkyColor * 0.58,
          uSkyColor,
          clamp(uDaylight, 0.08, 1.0)
        );
        water = mix(water, reflectedSky, fresnel * 0.58);

        vec3 lightDirection = normalize(vec3(-0.28, 0.92, 0.27));
        vec3 reflected = reflect(-lightDirection, normal);
        float specular = pow(
          max(dot(reflected, viewDirection), 0.0),
          mix(34.0, 76.0, 1.0 - uRain)
        );
        specular *= (0.22 + uDaylight * 0.7)
          * (1.0 - uRain * 0.42);

        float crest = smoothstep(0.58, 0.96, wave) * 0.055;
        water += vec3(specular + crest);

        float alpha = clamp(
          uOpacity + fresnel * 0.08 + uRain * 0.025,
          0.55,
          0.96
        );
        gl_FragColor = vec4(water, alpha);
      }
    `,
    transparent: true,
    depthWrite: false,
    depthTest: true,
    side: THREE.DoubleSide,
  });
}

export function configureWaterSurfaceKind(
  material: THREE.ShaderMaterial,
  kind: Exclude<WaterKind, 'None'>,
): void {
  const profile = WATER_PROFILES[kind];
  (
    material.uniforms.uDeepColor.value as THREE.Color
  ).setHex(profile.deep);
  (
    material.uniforms.uShallowColor.value as THREE.Color
  ).setHex(profile.shallow);
  (
    material.uniforms.uSkyColor.value as THREE.Color
  ).setHex(profile.sky);
  material.uniforms.uOpacity.value = profile.opacity;
  material.uniforms.uWaveScale.value = profile.waveScale;
  material.uniforms.uFlowSpeed.value = profile.flowSpeed;
}

export function configureWaterSurfaceEnvironment(
  material: THREE.ShaderMaterial,
  environment: WaterSurfaceEnvironment,
): void {
  material.uniforms.uWind.value = THREE.MathUtils.clamp(
    environment.wind01,
    0,
    1,
  );
  material.uniforms.uRain.value = THREE.MathUtils.clamp(
    environment.rain01,
    0,
    1,
  );
  material.uniforms.uDaylight.value = THREE.MathUtils.clamp(
    environment.daylight01,
    0,
    1,
  );
}
