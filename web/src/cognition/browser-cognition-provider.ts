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

async function loadBuiltinWebGPUEngine(modelId: string): Promise<BrowserChatEngine> {
  // Keep heavyweight WebLLM out of initial app JS. The first load downloads
  // model weights and caches them in the browser; no native server is needed.
  const webllm = await import('@mlc-ai/web-llm');
  const gpu = (navigator as Navigator & {
    gpu?: { requestAdapter(options: { powerPreference: 'high-performance' }): Promise<{ features: ReadonlySet<string> } | null> };
  }).gpu;
  const adapter = await gpu?.requestAdapter({ powerPreference: 'high-performance' });
  if (!adapter) throw new Error('No WebGPU adapter available for Qwen3-1.7B');
  modelId = builtinResidentModelForFeatures(adapter.features);
  const engine = await webllm.CreateMLCEngine(modelId, {
    appConfig: { ...webllm.prebuiltAppConfig, cacheBackend: 'indexeddb' },
  });
  return {
    modelId,
    interruptGenerate: () => engine.interruptGenerate(),
    unload: () => engine.unload(),
    chat: engine.chat,
  } as unknown as BrowserChatEngine;
}

export async function createBrowserResidentCognition(
  available: () => boolean = browserWebGPUSupported,
  loadEngine: (modelId: string) => Promise<BrowserChatEngine> =
    loadBuiltinWebGPUEngine,
): Promise<BrowserCognitionProvider | null> {
  if (!available()) return null;
  try {
    const engine = await loadEngine(BUILTIN_RESIDENT_MODEL);
    return new BrowserCognitionProvider(engine);
  } catch {
    // Unsupported WebGPU, insufficient mobile VRAM, model download failure,
    // or asset availability must NOT fabricate AI inference or stop Core.
    return null;
  }
}

