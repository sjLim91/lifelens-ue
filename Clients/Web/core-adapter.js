const GENERATED_MODULE_URL = "./generated/lifelens_core.js";

export class LifeLensCoreClient {
  constructor() {
    this.module = null;
    this.runtime = null;
  }

  async load() {
    const wasmModule = await import(GENERATED_MODULE_URL);
    const createModule = wasmModule.default;
    if (typeof createModule !== "function") {
      throw new Error("Emscripten module factory not found");
    }

    this.module = await createModule({
      locateFile(path) {
        return new URL(`./generated/${path}`, import.meta.url).href;
      },
    });

    if (!this.module.LifeLensWebClient) {
      throw new Error("LifeLensWebClient WASM binding not found");
    }

    this.runtime = new this.module.LifeLensWebClient();
  }

  get available() {
    return Boolean(this.runtime);
  }

  newGame(worldSeed, populationSeed = "", generationVersion = 2) {
    if (!this.runtime) throw new Error("LifeLensCore is not loaded");
    return this.runtime.newGame(
      String(worldSeed),
      String(populationSeed),
      Number(generationVersion),
    );
  }

  runMinutes(minutes) {
    if (!this.runtime) return;
    this.runtime.runMinutes(Number(minutes));
  }

  worldOverview() {
    return this.#parse(this.runtime?.worldOverviewJson(), { available: false });
  }

  residents() {
    return this.#parse(this.runtime?.residentsJson(), {
      available: false,
      residents: [],
    });
  }

  terrainWindow(centerChunkX, centerChunkY, radiusChunks = 8) {
    return this.#parse(
      this.runtime?.terrainWindowJson(
        Number(centerChunkX),
        Number(centerChunkY),
        Number(radiusChunks),
      ),
      { available: false, chunks: [] },
    );
  }

  destroy() {
    if (this.runtime && typeof this.runtime.delete === "function") {
      this.runtime.delete();
    }
    this.runtime = null;
  }

  #parse(text, fallback) {
    if (!text) return fallback;
    try {
      return JSON.parse(text);
    } catch {
      return fallback;
    }
  }
}
