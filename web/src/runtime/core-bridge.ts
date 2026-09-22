import { api } from '@appdeploy/client';
import type {
  CoreModule,
  Resident,
  ResidentsPayload,
  RuntimeClient,
  TerrainWindow,
  WorldOverview,
} from './core-types';

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

export class LifeLensCoreBridge {
  private constructor(private readonly client: RuntimeClient) {}

  static async connect(): Promise<LifeLensCoreBridge> {
    const response = await api.get<RuntimePayload>('/api/runtime/core');
    const payload = response.data;
    if (!payload?.js || !payload?.wasmBase64) {
      throw new Error('LifeLensCore runtime payload unavailable');
    }

    const jsUrl = URL.createObjectURL(
      new Blob([payload.js], { type: 'text/javascript' }),
    );
    const wasmBytes = decodeBase64(payload.wasmBase64);
    const wasmUrl = URL.createObjectURL(
      new Blob([wasmBytes], { type: 'application/wasm' }),
    );

    try {
      const wasmModule = await import(
        /* @vite-ignore */
        jsUrl
      ) as { default?: (options?: unknown) => Promise<CoreModule> };

      if (typeof wasmModule.default !== 'function') {
        throw new Error('Emscripten module factory not found');
      }

      const module = await wasmModule.default({
        locateFile(path: string): string {
          if (path.endsWith('.wasm')) return wasmUrl;
          return path;
        },
      });

      if (!module.LifeLensWebClient) {
        throw new Error('LifeLensWebClient binding unavailable');
      }

      return new LifeLensCoreBridge(new module.LifeLensWebClient());
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
    return parseJson<WorldOverview>(this.client.worldOverviewJson(), {});
  }

  residents(): ResidentsPayload {
    return parseJson<ResidentsPayload>(
      this.client.residentsJson(),
      { available: false, residents: [] },
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
