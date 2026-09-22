import type {
  CoreModule,
  DynamicEnvironment,
  Resident,
  ResidentsPayload,
  RuntimeClient,
  TerrainWindow,
  WorldOverview,
  WorldPresentationSnapshot,
} from './core-types';

import {
  connectRuntimeSources,
  DEFAULT_RUNTIME_LOAD_POLICY,
  readRuntimeResponse,
  withRuntimeDeadline,
  type RuntimeBytes,
  type RuntimeLoadPolicy,
} from './runtime-loading';
import { readResidents, readWorldOverview } from './core-response';

interface RuntimePayload {
  js: string;
  wasmBase64: string;
}

function parseJson<T>(text: string, fallback: T): T {
  try {
    return JSON.parse(text) as T;
  } catch {
    return fallback;
  }
}

function decodeBase64(value: string): Uint8Array {
  const binary = atob(value);
  const bytes = new Uint8Array(binary.length);
  for (let index = 0; index < binary.length; index += 1) {
    bytes[index] = binary.charCodeAt(index);
  }
  return bytes;
}

const RAW_GITHUB_RUNTIME_BASE =
  'https://raw.githubusercontent.com/sjLim91/lifelens-ue/gh-pages/';

async function loadRuntimeFromBase(base: string, policy: RuntimeLoadPolicy): Promise<RuntimeBytes> {
  const [js, wasmBuffer] = await Promise.all([
    readRuntimeResponse(`${base}runtime/lifelens_core.js`, response => response.text(), policy.requestTimeoutMs),
    readRuntimeResponse(`${base}runtime/lifelens_core.wasm`, response => response.arrayBuffer(), policy.requestTimeoutMs),
  ]);
  return { js, wasmBuffer };
}

async function loadBackendRuntime(policy: RuntimeLoadPolicy): Promise<RuntimeBytes> {
  const payload = await readRuntimeResponse('/api/runtime/core', async response => {
    const value: unknown = await response.json();
    if (!value || typeof value !== 'object') throw new Error('Core runtime payload unavailable');
    const candidate = value as RuntimePayload;
    if (typeof candidate.js !== 'string' || typeof candidate.wasmBase64 !== 'string') {
      throw new Error('Core runtime payload unavailable');
    }
    return candidate;
  }, policy.requestTimeoutMs);
  const bytes = decodeBase64(payload.wasmBase64);
  return {
    js: payload.js,
    wasmBuffer: bytes.buffer.slice(bytes.byteOffset, bytes.byteOffset + bytes.byteLength) as ArrayBuffer,
  };
}

export class LifeLensCoreBridge {
  private constructor(private readonly client: RuntimeClient) {}

  static async connect(policy: RuntimeLoadPolicy = DEFAULT_RUNTIME_LOAD_POLICY): Promise<LifeLensCoreBridge> {
    return connectRuntimeSources([
      { name: 'same-origin', load: () => loadRuntimeFromBase(import.meta.env.BASE_URL || './', policy) },
      { name: 'github-raw', load: () => loadRuntimeFromBase(RAW_GITHUB_RUNTIME_BASE, policy) },
      { name: 'backend', load: () => loadBackendRuntime(policy) },
    ], payload => this.initialize(payload, policy));
  }

  private static async initialize(payload: RuntimeBytes, policy: RuntimeLoadPolicy): Promise<LifeLensCoreBridge> {

    const jsUrl = URL.createObjectURL(
      new Blob([payload.js], { type: 'text/javascript' }),
    );
    const wasmUrl = URL.createObjectURL(
      new Blob([payload.wasmBuffer], { type: 'application/wasm' }),
    );

    try {
      const wasmModule = await withRuntimeDeadline(
        import(
          /* @vite-ignore */
          jsUrl
        ) as Promise<{ default?: (options?: unknown) => Promise<CoreModule> }>,
        policy.initializationTimeoutMs,
        'LifeLensCore JavaScript module import',
      );

      if (typeof wasmModule.default !== 'function') {
        throw new Error('Emscripten module factory not found');
      }

      const module = await withRuntimeDeadline(
        wasmModule.default({
          locateFile(path: string): string {
            if (path.endsWith('.wasm')) return wasmUrl;
            return path;
          },
        }),
        policy.initializationTimeoutMs,
        'LifeLensCore WASM initialization',
      );

      if (!module.LifeLensWebClient) {
        throw new Error('LifeLensWebClient binding unavailable');
      }

      const client = new module.LifeLensWebClient();
      try {
        for (const method of ['newGame', 'runMinutes', 'worldOverviewJson', 'residentsJson', 'terrainWindowJson'] as const) {
          if (typeof client[method] !== 'function') throw new Error(`Core binding missing: ${method}`);
        }
        return new LifeLensCoreBridge(client);
      } catch (error) {
        client.delete?.();
        throw error;
      }
    } finally {
      URL.revokeObjectURL(jsUrl);
      URL.revokeObjectURL(wasmUrl);
    }
  }

  createWorld(
    worldSeed: string,
    populationSeed = '',
    generationVersion = 3,
  ): boolean {
    return this.client.newGame(worldSeed, populationSeed, generationVersion);
  }

  runMinutes(minutes: number): void {
    const wholeMinutes = Math.max(0, Math.floor(minutes));
    if (wholeMinutes > 0) this.client.runMinutes(wholeMinutes);
  }

  worldOverview(): WorldOverview {
    return readWorldOverview(this.client.worldOverviewJson());
  }

  residents(): ResidentsPayload {
    return readResidents(this.client.residentsJson());
  }

  worldPresentation(): WorldPresentationSnapshot {
    const fallback: WorldPresentationSnapshot = {
      available: false,
      facilities: [],
      sanitationSites: [],
      residues: [],
      aggregateWasteAmount: 0,
      peakWasteIntensity: 0,
    };

    if (typeof this.client.worldPresentationJson !== 'function') {
      console.warn(
        'LifeLensCore runtime is older than the web client; world consequence projection is temporarily unavailable.',
      );
      return fallback;
    }

    return parseJson<WorldPresentationSnapshot>(
      this.client.worldPresentationJson(),
      fallback,
    );
  }

  dynamicEnvironment(x: number, y: number): DynamicEnvironment {
    const fallback: DynamicEnvironment = {
      available: false,
      centerChunkX: x,
      centerChunkY: y,
      precipitationType: 'None',
      summary: 'Clear',
    };

    if (typeof this.client.dynamicEnvironmentJson !== 'function') {
      console.warn(
        'LifeLensCore runtime is older than the web client; weather projection is temporarily unavailable.',
      );
      return fallback;
    }

    return parseJson<DynamicEnvironment>(
      this.client.dynamicEnvironmentJson(x, y),
      fallback,
    );
  }

  terrainWindow(x: number, y: number, radius: number): TerrainWindow {
    return parseJson<TerrainWindow>(
      this.client.terrainWindowJson(x, y, radius),
      {
        available: false,
        centerChunkX: x,
        centerChunkY: y,
        chunks: [],
      },
    );
  }

  residentList(): Resident[] {
    return this.residents().residents ?? [];
  }

  dispose(): void {
    this.client.delete?.();
  }
}
