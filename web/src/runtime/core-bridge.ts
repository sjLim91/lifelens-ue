import {
  parseAutonomousGoalPreview,
  type AutonomousGoalPreview,
} from '../cognition/autonomous-goal-preview';
import {
  parseCognitiveRequest,
  type CognitiveRequestDto,
  type CognitiveProposalDto,
  type CognitiveTrigger,
} from '../cognition/cognitive-contract';
import type {
  CivilizationWorldPayload,
  CoreModule,
  DynamicEnvironment,
  HumanTracesWindowPayload,
  RecentSocialEventsPayload,
  Resident,
  ResidentsPayload,
  RuntimeClient,
  TerrainWindow,
  WorldObjectsPayload,
  WorldOverview,
} from './core-types';

import {
  connectRuntimeSources,
  DEFAULT_RUNTIME_LOAD_POLICY,
  emscriptenRuntimeOptions,
  readRuntimeResponse,
  withRuntimeDeadline,
  type RuntimeBytes,
  type RuntimeLoadPolicy,
} from './runtime-loading';
import { readResidents, readWorldOverview } from './core-response';
import { readCoreEarthScenario, UNBOUND_EARTH_SCENARIO,
  type CoreEarthScenarioObservation } from '../earth/core-earth-scenario';
import { readCoreEarthSolar, UNBOUND_EARTH_SOLAR,
  type CoreEarthSolarObservation } from '../earth/core-earth-solar';

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
  return { js, wasmBuffer, moduleUrl: `${base}runtime/lifelens_core.js` };
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
    // Emscripten accepts an existing Uint8Array as wasmBinary. Supplying the
    // exact validated download avoids a SECOND fetch(blob:...) which can fail
    // in Android in-app browsers / restricted WebViews. It also prevents
    // a mismatched JS/WASM pair being loaded during a Pages publish.
    // Still supply locateFile: Emscripten's blob import.meta.url cannot
    // resolve a relative WASM URL. wasmBinary prevents network fetching.
    const options = emscriptenRuntimeOptions(payload);
    type ModuleFactory = (options?: unknown) => Promise<CoreModule>;
    type FactoryModule = { default?: ModuleFactory };
    let factoryModule: FactoryModule | null = null;
    let directImportFailure: string | null = null;

    // Import a same-origin real URL first. Native module URLs work in mobile
    // browsers that reject executable blob URLs. Loading validated byte pairs
    // remains mandatory; a remote raw GitHub module is NEVER executed here.
    if (payload.moduleUrl && typeof document !== 'undefined') {
      const absoluteUrl = new URL(payload.moduleUrl, document.baseURI);
      if (absoluteUrl.origin === window.location.origin) {
        try {
          factoryModule = await withRuntimeDeadline(
            import(/* @vite-ignore */ absoluteUrl.href) as Promise<FactoryModule>,
            policy.initializationTimeoutMs,
            'LifeLensCore same-origin module import',
          );
        } catch (error) {
          directImportFailure = error instanceof Error ? error.message : String(error);
        }
      }
    }

    let temporaryModuleUrl: string | null = null;
    try {
      if (!factoryModule) {
        temporaryModuleUrl = URL.createObjectURL(
          new Blob([payload.js], { type: 'text/javascript' }),
        );
        try {
          factoryModule = await withRuntimeDeadline(
            import(/* @vite-ignore */ temporaryModuleUrl) as Promise<FactoryModule>,
            policy.initializationTimeoutMs,
            'LifeLensCore fallback module import',
          );
        } catch (error) {
          const blobFailure = error instanceof Error ? error.message : String(error);
          throw new Error(directImportFailure
            ? `same-origin import: ${directImportFailure}; blob import: ${blobFailure}`
            : `blob import: ${blobFailure}`);
        }
      }
      if (typeof factoryModule.default !== 'function') {
        throw new Error('Emscripten module factory not found');
      }
      const module = await withRuntimeDeadline(
        factoryModule.default(options),
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
      if (temporaryModuleUrl) URL.revokeObjectURL(temporaryModuleUrl);
    }
  }

  createWorld(
    worldSeed: string,
    populationSeed = '',
    generationVersion = 4,
  ): boolean {
    return this.client.newGame(worldSeed, populationSeed, generationVersion);
  }

  createEarthWorld(
    seed: string, latitudeDeg: number, longitudeDeg: number,
    generationVersion = 4,
  ): boolean {
    if (typeof this.client.newEarthGame !== 'function') return false;
    return this.client.newEarthGame(
      seed, latitudeDeg, longitudeDeg, generationVersion,
    );
  }

  earthScenario(overview: WorldOverview): CoreEarthScenarioObservation {
    if (typeof this.client.earthScenarioJson !== 'function')
      return UNBOUND_EARTH_SCENARIO;
    try {
      return readCoreEarthScenario(
        JSON.parse(this.client.earthScenarioJson()) as unknown, overview,
      );
    } catch {
      return UNBOUND_EARTH_SCENARIO;
    }
  }

  earthSolar(
    overview: WorldOverview,
    scenario: CoreEarthScenarioObservation,
  ): CoreEarthSolarObservation {
    if (typeof this.client.earthSolarJson !== 'function')
      return UNBOUND_EARTH_SOLAR;
    try {
      return readCoreEarthSolar(
        JSON.parse(this.client.earthSolarJson()) as unknown, overview, scenario,
      );
    } catch {
      return UNBOUND_EARTH_SOLAR;
    }
  }

  runMinutes(minutes: number): void {
    const wholeMinutes = Math.max(0, Math.floor(minutes));
    if (wholeMinutes > 0) this.client.runMinutes(wholeMinutes);
  }

  // Exact Core snapshot; no observer-derived fake reconstruction.
  saveSnapshotBase64(): string {
    if (typeof this.client.saveSnapshotBase64 !== 'function') {
      throw Error('현재 Core 런타임은 영구 월드 저장을 지원하지 않습니다.');
    }
    const encoded = this.client.saveSnapshotBase64();
    if (!encoded || encoded.length < 16) throw Error('Core 월드 저장에 실패했습니다.');
    return encoded;
  }

  restoreSnapshotBase64(encoded: string): boolean {
    if (typeof this.client.restoreSnapshotBase64 !== 'function') {
      throw Error('현재 Core 런타임은 월드 복원을 지원하지 않습니다.');
    }
    return this.client.restoreSnapshotBase64(encoded);
  }

  worldOverview(): WorldOverview {
    return readWorldOverview(this.client.worldOverviewJson());
  }

  residents(): ResidentsPayload {
    return readResidents(this.client.residentsJson());
  }

  cognitiveRequest(
    residentId: string,
    trigger: CognitiveTrigger,
  ): CognitiveRequestDto | null {
    if (typeof this.client.cognitiveRequestJson !== 'function') return null;
    try {
      return parseCognitiveRequest(
        JSON.parse(this.client.cognitiveRequestJson(residentId, trigger)) as unknown,
      );
    } catch {
      return null;
    }
  }

  // Core checks the issued observation receipt and validates the preference
  // against current legal actions before recording both contexts.
  submitLocalCognitiveProposal(
    request: CognitiveRequestDto,
    proposal: CognitiveProposalDto,
    observedWorldSeed: string,
  ): boolean {
    if (typeof this.client.submitLocalCognitiveProposal !== 'function'
        || request.worldSeed !== observedWorldSeed
        || !request.contextFingerprint
        || !request.populationSeed
        || request.generationVersion === undefined) return false;
    let digest = 2166136261;
    for (const point of proposal.rationale) {
      digest = Math.imul(digest ^ (point.codePointAt(0) ?? 0), 16777619) >>> 0;
    }
    try {
      return this.client.submitLocalCognitiveProposal(
        request.actor, request.minute, observedWorldSeed,
        proposal.intent, proposal.priority, proposal.targetResident ?? '',
        `local-${digest.toString(16).padStart(8, '0')}`, request.trigger,
        request.contextFingerprint,
      );
    } catch {
      return false;
    }
  }

  // Pure Core observation; it never supplies a plan or an execution
  // command. Older WASM builds without B3c safely return null.
  autonomousGoalCandidates(
    residentId: string,
    trigger: CognitiveTrigger = 'Reflection',
  ): AutonomousGoalPreview | null {
    if (typeof this.client.autonomousGoalCandidatesJson !== 'function') return null;
    try {
      const preview = parseAutonomousGoalPreview(
        JSON.parse(this.client.autonomousGoalCandidatesJson(residentId, trigger)) as unknown,
      );
      return preview.actor === residentId ? preview : null;
    } catch {
      return null;
    }
  }

  residentRuntime(): ResidentsPayload {
    if (typeof this.client.residentRuntimeJson !== 'function') {
      return this.residents();
    }
    return readResidents(this.client.residentRuntimeJson());
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

  recentSocialEvents(maxEvents = 32): RecentSocialEventsPayload {
    const fallback: RecentSocialEventsPayload = {
      available: false,
      count: 0,
      events: [],
    };
    if (typeof this.client.recentSocialEventsJson !== 'function') {
      return fallback;
    }
    return parseJson<RecentSocialEventsPayload>(
      this.client.recentSocialEventsJson(
        Math.max(0, Math.min(64, Math.floor(maxEvents))),
      ),
      fallback,
    );
  }

  civilizationWorld(
    maxRecentDiscoveries = 32,
  ): CivilizationWorldPayload {
    const fallback: CivilizationWorldPayload = {
      available: false,
      resources: [],
      storages: [],
      facilities: [],
      recentDiscoveries: [],
    };
    if (typeof this.client.civilizationWorldJson !== 'function') {
      return fallback;
    }
    return parseJson<CivilizationWorldPayload>(
      this.client.civilizationWorldJson(
        Math.max(0, Math.min(64, Math.floor(maxRecentDiscoveries))),
      ),
      fallback,
    );
  }

  civilizationWorldWindow(
    maxRecentDiscoveries: number,
    centerChunkX: number,
    centerChunkY: number,
    radiusChunks: number,
  ): CivilizationWorldPayload {
    if (typeof this.client.civilizationWorldWindowJson !== 'function') {
      return this.civilizationWorld(maxRecentDiscoveries);
    }

    const fallback: CivilizationWorldPayload = {
      available: false,
      resources: [],
      storages: [],
      facilities: [],
      recentDiscoveries: [],
    };
    return parseJson<CivilizationWorldPayload>(
      this.client.civilizationWorldWindowJson(
        Math.max(0, Math.min(64, Math.floor(maxRecentDiscoveries))),
        Math.trunc(centerChunkX),
        Math.trunc(centerChunkY),
        Math.max(0, Math.min(16, Math.floor(radiusChunks))),
      ),
      fallback,
    );
  }

  worldObjects(): WorldObjectsPayload {
    const fallback: WorldObjectsPayload = {
      available: false,
      smartObjects: [],
      sanitationSites: [],
    };
    if (typeof this.client.worldObjectsJson !== 'function') {
      return fallback;
    }
    return parseJson<WorldObjectsPayload>(
      this.client.worldObjectsJson(),
      fallback,
    );
  }

  humanTracesWindow(
    x: number,
    y: number,
    radius: number,
  ): HumanTracesWindowPayload {
    if (typeof this.client.humanTracesWindowJson !== 'function') {
      const terrain = this.terrainWindow(x, y, radius);
      return {
        available: terrain.available,
        centerChunkX: terrain.centerChunkX,
        centerChunkY: terrain.centerChunkY,
        radiusChunks: terrain.radiusChunks,
        humanTraces: terrain.humanTraces,
        ...(terrain.wildlife === undefined ? {} : { wildlife: terrain.wildlife }),
      };
    }

    return parseJson<HumanTracesWindowPayload>(
      this.client.humanTracesWindowJson(x, y, radius),
      {
        available: false,
        centerChunkX: x,
        centerChunkY: y,
        radiusChunks: radius,
        humanTraces: { total: 0, entries: [] },
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


