import type {
  CoreModule,
  DynamicEnvironment,
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

const STATIC_RUNTIME_TIMEOUT_MS = 6000;
const MODULE_INIT_TIMEOUT_MS = 12000;
const RAW_GITHUB_RUNTIME_BASE =
  'https://raw.githubusercontent.com/sjLim91/lifelens-ue/gh-pages/';

async function fetchWithTimeout(
  url: string,
  timeoutMs = STATIC_RUNTIME_TIMEOUT_MS,
): Promise<Response> {
  const controller = new AbortController();
  const timer = window.setTimeout(() => controller.abort(), timeoutMs);

  try {
    return await fetch(url, {
      cache: 'no-store',
      signal: controller.signal,
    });
  } catch (error) {
    if (controller.signal.aborted) {
      throw new Error(`Core runtime request timed out after ${timeoutMs}ms: ${url}`);
    }
    throw error;
  } finally {
    window.clearTimeout(timer);
  }
}

async function withTimeout<T>(
  promise: Promise<T>,
  timeoutMs: number,
  label: string,
): Promise<T> {
  let timer: number | null = null;

  try {
    return await Promise.race([
      promise,
      new Promise<T>((_, reject) => {
        timer = window.setTimeout(
          () => reject(new Error(`${label} timed out after ${timeoutMs}ms`)),
          timeoutMs,
        );
      }),
    ]);
  } finally {
    if (timer !== null) window.clearTimeout(timer);
  }
}

async function loadRuntimeFromBase(base: string): Promise<{
  js: string;
  wasmBuffer: ArrayBuffer;
}> {
  const [jsResponse, wasmResponse] = await Promise.all([
    fetchWithTimeout(`${base}runtime/lifelens_core.js`),
    fetchWithTimeout(`${base}runtime/lifelens_core.wasm`),
  ]);

  if (!jsResponse.ok || !wasmResponse.ok) {
    throw new Error(
      `Core runtime unavailable from ${base}: JS ${jsResponse.status}, WASM ${wasmResponse.status}`,
    );
  }

  return {
    js: await jsResponse.text(),
    wasmBuffer: await wasmResponse.arrayBuffer(),
  };
}

async function loadStaticRuntime(): Promise<{
  js: string;
  wasmBuffer: ArrayBuffer;
}> {
  const base = import.meta.env.BASE_URL || './';
  return loadRuntimeFromBase(base);
}

async function loadRawGitHubRuntime(): Promise<{
  js: string;
  wasmBuffer: ArrayBuffer;
}> {
  return loadRuntimeFromBase(RAW_GITHUB_RUNTIME_BASE);
}

async function loadBackendRuntime(): Promise<{
  js: string;
  wasmBuffer: ArrayBuffer;
}> {
  const response = await fetchWithTimeout('/api/runtime/core');

  if (!response.ok) {
    throw new Error(
      `Backend Core runtime unavailable: ${response.status}`,
    );
  }

  const payload = await response.json() as RuntimePayload;
  if (!payload?.js || !payload?.wasmBase64) {
    throw new Error('LifeLensCore runtime payload unavailable');
  }

  const bytes = decodeBase64(payload.wasmBase64);
  const wasmBuffer = bytes.buffer.slice(
    bytes.byteOffset,
    bytes.byteOffset + bytes.byteLength,
  ) as ArrayBuffer;

  return {
    js: payload.js,
    wasmBuffer,
  };
}

async function loadRuntime(): Promise<{
  js: string;
  wasmBuffer: ArrayBuffer;
}> {
  const failures: string[] = [];

  try {
    return await loadStaticRuntime();
  } catch (staticError) {
    failures.push(`same-origin: ${String(staticError)}`);
    console.info(
      'LifeLens same-origin runtime unavailable; trying GitHub raw runtime',
      staticError,
    );
  }

  try {
    return await loadRawGitHubRuntime();
  } catch (rawError) {
    failures.push(`github-raw: ${String(rawError)}`);
    console.info(
      'LifeLens GitHub raw runtime unavailable; trying backend runtime',
      rawError,
    );
  }

  try {
    return await loadBackendRuntime();
  } catch (backendError) {
    failures.push(`backend: ${String(backendError)}`);
    throw new Error(
      `LifeLensCore runtime loading failed. ${failures.join(' | ')}`,
    );
  }
}

export class LifeLensCoreBridge {
  private constructor(private readonly client: RuntimeClient) {}

  static async connect(): Promise<LifeLensCoreBridge> {
    const payload = await loadRuntime();

    const jsUrl = URL.createObjectURL(
      new Blob([payload.js], { type: 'text/javascript' }),
    );
    const wasmUrl = URL.createObjectURL(
      new Blob([payload.wasmBuffer], { type: 'application/wasm' }),
    );

    try {
      const wasmModule = await withTimeout(
        import(
          /* @vite-ignore */
          jsUrl
        ) as Promise<{ default?: (options?: unknown) => Promise<CoreModule> }>,
        MODULE_INIT_TIMEOUT_MS,
        'LifeLensCore JavaScript module import',
      );

      if (typeof wasmModule.default !== 'function') {
        throw new Error('Emscripten module factory not found');
      }

      const module = await withTimeout(
        wasmModule.default({
          locateFile(path: string): string {
            if (path.endsWith('.wasm')) return wasmUrl;
            return path;
          },
        }),
        MODULE_INIT_TIMEOUT_MS,
        'LifeLensCore WASM initialization',
      );

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

  dynamicEnvironment(x: number, y: number): DynamicEnvironment {
    return parseJson<DynamicEnvironment>(
      this.client.dynamicEnvironmentJson(x, y),
      {
        available: false,
        centerChunkX: x,
        centerChunkY: y,
        precipitationType: 'None',
        summary: 'Clear',
      },
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
