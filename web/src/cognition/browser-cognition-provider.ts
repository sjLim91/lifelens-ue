import {
  parseCognitiveProposal,
  cognitiveProposalJsonSchema,
  type CognitiveProposalDto,
  type CognitiveRequestDto,
} from './cognitive-contract';
import type { CognitiveProvider } from './cognitive-provider';
import { CognitiveProviderError } from './cognitive-provider';
import { cognitiveSystemPrompt } from './local-cognition-provider';

// Open-source quantized model. One shared browser engine reasons about each
// resident's SEPARATE authoritative Core identity and memory context.
// The first download can be large; WebGPU availability does not guarantee
// enough VRAM. This is not a paid API, cloud language service or second NPC.
export const BUILTIN_RESIDENT_MODEL =
  'Qwen3-1.7B-q4f16_1-MLC';
export const BUILTIN_RESIDENT_MODEL_FP32 = 'Qwen3-1.7B-q4f32_1-MLC';

// Same Qwen3-1.7B weights/architecture; only GPU arithmetic precision differs.
// WebGPU presence alone does not imply optional shader-f16 support.
export function builtinResidentModelForFeatures(features: ReadonlySet<string>): string {
  return features.has('shader-f16') ? BUILTIN_RESIDENT_MODEL : BUILTIN_RESIDENT_MODEL_FP32;
}

interface BrowserChatRequest {
  messages: Array<{ role: 'system' | 'user'; content: string }>;
  temperature: number;
  max_tokens: number;
  response_format: { type: 'json_object'; schema: string };
  extra_body: { enable_thinking: false };
}

export interface BrowserChatEngine {
  readonly modelId?: string;
  // Stops WebLLM decoding after the current GPU operation completes. It
  // cannot synchronously cancel a submitted prefill/kernel dispatch.
  interruptGenerate?(): Promise<void>;
  // Native WebLLM MLCEngine.unload frees the model's GPU pipelines/buffers.
  // Optional for small deterministic fake engines used by the tests.
  unload?(): Promise<void>;
  chat: {
    completions: {
      create(request: BrowserChatRequest): Promise<{
        choices?: Array<{ message?: { content?: unknown } }>;
      }>;
    };
  };
}

export class BrowserCognitionProvider implements CognitiveProvider {
  readonly name = 'resident-browser-webgpu';
  readonly modelId: string;
  private disposed = false;
  private inFlight = false;
  constructor(private readonly engine: BrowserChatEngine) {
    this.modelId = engine.modelId ?? BUILTIN_RESIDENT_MODEL;
  }

  async dispose(): Promise<void> {
    if (this.disposed) return;
    this.disposed = true;
    if (this.inFlight) this.interruptPending();
    await this.engine.unload?.();
  }

  private interruptPending(): void {
    try {
      const interruption = this.engine.interruptGenerate?.();
      if (interruption) void interruption.catch(() => {});
    } catch {
      // A lost device must not throw from an AbortSignal listener or allow
      // the obsolete result to authorize a resident decision.
    }
  }

  async reason(
    request: CognitiveRequestDto,
    signal: AbortSignal,
  ): Promise<CognitiveProposalDto | null> {
    if (this.disposed || signal.aborted
      || request.allowedIntents.length === 0) return null;
    // An earlier timeout releases the scheduler slot but NOT the WebGPU
    // kernel. Reporting the still-occupied engine as a harmless "no
    // proposal" resets the recovery fault counter forever, stranding all
    // residents behind one hung inference. Keep the GPU call exclusive,
    // but surface a genuine provider fault so automatic recovery can unload
    // the stuck engine after bounded failures.
    if (this.inFlight) {
      throw new CognitiveProviderError(
        'http_error', 'on-device WebGPU inference remains in flight');
    }
    // The scheduler releases its slot on timeout, even if GPU prefill is
    // still running. Retain this engine slot until the actual call settles
    // so later residents cannot accumulate an invisible WebLLM lock queue.
    this.inFlight = true;
    const onAbort = () => this.interruptPending();
    signal.addEventListener('abort', onAbort, { once: true });
    let reply: { choices?: Array<{ message?: { content?: unknown } }> };
    try {
      reply = await this.engine.chat.completions.create({
        messages: [
          { role: 'system', content: cognitiveSystemPrompt() },
          { role: 'user', content: JSON.stringify(request) },
        ],
        temperature: 0,
        max_tokens: 180,
        // Constrain generation itself, then still validate against Core.
        response_format: {
          type: 'json_object',
          schema: JSON.stringify(cognitiveProposalJsonSchema(request)),
        },
        // Real-time residents need a short decision, not an unbounded
        // reasoning transcript consuming the JSON output budget.
        extra_body: { enable_thinking: false },
      });
    } catch {
      if (this.disposed || signal.aborted) return null;
      throw new CognitiveProviderError(
        'http_error', 'on-device WebGPU inference failed');
    } finally {
      signal.removeEventListener('abort', onAbort);
      this.inFlight = false;
    }
    // A late GPU inference must not re-authorize a disposed old runtime.
    if (this.disposed || signal.aborted) return null;
    const raw = reply.choices?.[0]?.message?.content;
    if (typeof raw !== 'string' || raw.length === 0 || raw.length > 8192) {
      throw new CognitiveProviderError(
        'malformed_response', 'on-device model returned no JSON string');
    }
    let json: unknown;
    try { json = JSON.parse(raw); }
    catch {
      throw new CognitiveProviderError(
        'invalid_json', 'on-device model response was not JSON');
    }
    try { return parseCognitiveProposal(json, request); }
    catch {
      // Even local neural inference is never allowed to invent Core truth.
      throw new CognitiveProviderError(
        'invalid_schema', 'on-device proposal violated the Core contract');
    }
  }
}

export function browserWebGPUSupported(): boolean {
  return typeof navigator !== 'undefined'
    && typeof (navigator as Navigator & { gpu?: unknown }).gpu !== 'undefined';
}

// A WebGPU driver can stop responding during model initialization or unload.
// The worker is the containment boundary: Core and the observer must never
// wait indefinitely for a GPU device teardown. This is *not* a model fallback.
export const QWEN_WORKER_INIT_TIMEOUT_MS = 8 * 60 * 1000;
export const QWEN_WORKER_UNLOAD_TIMEOUT_MS = 2000;

/** Own exactly one worker alongside the shared single Qwen engine. */
export function workerBackedBrowserEngine(
  worker: Pick<Worker, 'terminate'>,
  engine: BrowserChatEngine,
  modelId: string,
  unloadTimeoutMs = QWEN_WORKER_UNLOAD_TIMEOUT_MS,
): BrowserChatEngine {
  let disposed = false;
  return {
    modelId,
    chat: engine.chat,
    interruptGenerate: async () => {
      if (!disposed) await engine.interruptGenerate?.();
    },
    unload: async () => {
      if (disposed) return;
      disposed = true;
      let timer: ReturnType<typeof setTimeout> | undefined;
      try {
        await Promise.race([
          Promise.resolve().then(() => engine.unload?.()),
          new Promise<void>((resolve) => {
            timer = setTimeout(resolve, unloadTimeoutMs);
          }),
        ]);
      } finally {
        if (timer !== undefined) clearTimeout(timer);
        // Termination also works when unload never resolves after device loss.
        worker.terminate();
      }
    },
  };
}

async function loadBuiltinWebGPUEngine(
  modelId: string, signal?: AbortSignal,
): Promise<BrowserChatEngine> {
  if (signal?.aborted) throw new Error('Qwen initialization cancelled');
  // Only the WebLLM RPC client runs on the React/Three.js main thread.
  // GPU compilation, model prefill, decoding and heavyweight loading happen
  // in the single dedicated Worker. This preserves on-device/free Qwen.
  const gpu = (navigator as Navigator & {
    gpu?: { requestAdapter(options: { powerPreference: 'high-performance' }): Promise<{ features: ReadonlySet<string> } | null> };
  }).gpu;
  const adapter = await gpu?.requestAdapter({ powerPreference: 'high-performance' });
  if (signal?.aborted) throw new Error('Qwen initialization cancelled');
  if (!adapter) throw new Error('No WebGPU adapter available for Qwen3-1.7B');
  modelId = builtinResidentModelForFeatures(adapter.features);
  const webllm = await import('@mlc-ai/web-llm');
  if (signal?.aborted) throw new Error('Qwen initialization cancelled');

  // Vite statically emits the worker bundle from this relative URL. It must
  // stay on the same origin as LifeLens, not a cloud inference endpoint.
  const worker = new Worker(
    new URL('./qwen-webgpu.worker.ts', import.meta.url),
    { type: 'module', name: 'lifelens-resident-qwen' },
  );
  let timeout: ReturnType<typeof setTimeout> | undefined;
  // A replaced Core world must terminate its OLD download/compile immediately,
  // not keep a second 2GB-class GPU model alive until the eight-minute timeout.
  // The worker is still the only WebGPU execution owner.
  let cancelInitialization: () => void = () => {};
  const cancelled = new Promise<never>((_resolve, reject) => {
    cancelInitialization = () => reject(new Error('Qwen initialization cancelled'));
  });
  const onAbort = () => cancelInitialization();
  signal?.addEventListener('abort', onAbort, { once: true });
  if (signal?.aborted) onAbort();
  const onError = () => fail(new Error('Qwen WebGPU worker crashed'));
  const onMessageError = () => fail(new Error('Qwen WebGPU worker message failed'));
  let fail: (error: Error) => void = () => {};
  const crashed = new Promise<never>((_resolve, reject) => { fail = reject; });
  worker.addEventListener('error', onError);
  worker.addEventListener('messageerror', onMessageError);
  try {
    const engine = await Promise.race([
      webllm.CreateWebWorkerMLCEngine(worker, modelId, {
        appConfig: { ...webllm.prebuiltAppConfig, cacheBackend: 'indexeddb' },
      }),
      crashed,
      cancelled,
      new Promise<never>((_resolve, reject) => {
        timeout = setTimeout(
          () => reject(new Error('Qwen WebGPU worker initialization timed out')),
          QWEN_WORKER_INIT_TIMEOUT_MS,
        );
      }),
    ]);
    return workerBackedBrowserEngine(worker, {
      chat: engine.chat,
      interruptGenerate: async () => { engine.interruptGenerate(); },
      unload: () => engine.unload(),
    }, modelId);
  } catch (error) {
    worker.terminate();
    throw error;
  } finally {
    if (timeout !== undefined) clearTimeout(timeout);
    worker.removeEventListener('error', onError);
    worker.removeEventListener('messageerror', onMessageError);
    signal?.removeEventListener('abort', onAbort);
  }
}
export async function createBrowserResidentCognition(
  available: () => boolean = browserWebGPUSupported,
  loadEngine: (modelId: string, signal?: AbortSignal) => Promise<BrowserChatEngine> =
    loadBuiltinWebGPUEngine,
  signal?: AbortSignal,
): Promise<BrowserCognitionProvider | null> {
  if (signal?.aborted || !available()) return null;
  try {
    const engine = await loadEngine(BUILTIN_RESIDENT_MODEL, signal);
    if (signal?.aborted) {
      // Handles an injected loader which settled concurrently with a world
      // replacement, as well as any future platform that ignores abort.
      try { await engine.unload?.(); } catch { /* stale GPU is not Core authority */ }
      return null;
    }
    return new BrowserCognitionProvider(engine);
  } catch {
    // Unsupported WebGPU, insufficient mobile VRAM, model download failure,
    // or asset availability must NOT fabricate AI inference or stop Core.
    return null;
  }
}

