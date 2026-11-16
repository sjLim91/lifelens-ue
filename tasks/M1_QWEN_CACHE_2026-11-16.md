# M1-07 Qwen 모델 캐시 로딩 수정

기준 main `a2c9c2830e409addbe26f0d532ed714f11ee9065`. M1-01~06 공개 배포 뒤 실제 모델 초기화 실패를 추적한 후속 작업이다. 모델 정책·24초 제품 추론 제한·Core 권위는 유지한다.

## 원인과 수정

`Qwen3-1.7B-q4f32_1-MLC`의 원래 embedding 조각은 **155,582,464 bytes**다. 공식 manifest의 MD5 `c738b02f2921806bec6241c056c7d89f`와 내려받은 파일이 일치한다. 같은 실제 파일을 로컬 HTTP로 제공한 브라우저 분리 실험에서 IndexedDB는 **133,169,152 bytes** 단일 값 제한을 이유로 거부했고, Cache API는 1,607ms에 저장했다.

WebLLM asset cache를 `indexeddb`에서 라이브러리 기본 지원인 `cache`로 변경한다. 기존 저장 게임과 기존 모델 캐시는 삭제하지 않는다. 다른 모델이나 서버 추론으로 바꾸지 않는다.

## 소프트웨어 GPU 격리

자동 실행 앱의 실제 SwiftShader adapter는 `info.isFallbackAdapter=true`였다. 이 환경에서는 긴 GPU prefill 이후 관찰 검사가 60초 제한에 걸리고 브라우저 종료도 지연됐다. 캐시만 수정해 이 경로를 활성화하지 않도록 **명시적으로 보고된 fallback adapter는 WebLLM import·worker 생성 전에 거부**한다. 옛 WebGPU의 누락된 metadata와 하드웨어 FP32 GPU는 계속 허용한다. 단일 Qwen 정책을 유지하며 다른 모델은 시작하지 않는다.

최종 제품 경로의 실제 브라우저 검사에서 자동 agency는 활성 상태이고 Qwen worker/모델/추론은 0개, Core는 **640→711분(6초)** 진행했다. [원시 관찰](evidence/m1-qwen-cache-2026-11-16/fallback-core.json). 아래 소프트웨어 GPU 초기화·추론 수치는 이 보호 장치 전의 기술 진단이며 최종 제품의 소프트웨어 추론 지원을 뜻하지 않는다.

## 실제 검증과 경계

- 공식 모델 전체 **30조각**을 HTTPS로 내려받고 소프트웨어 WebGPU에서 초기화 완료: 약 **78.895초**. 캐시 재사용 후 실제 제품 factory로 초기화 **7,435ms**.
- 실제 WASM 주민의 **7,018 byte Core 문맥**으로 제품과 같은 24초 scheduler 추론을 실행했다. **24,069ms 시간 초과**, 그동안 Core heartbeat **20분** 진행·추론 직후 Core 계속 진행. 가짜 provider가 아니며 성공 제안은 없었다.
- 제품 설정을 변경하지 않은 별도 **180초 진단**도 180,018ms 시간 초과, Core heartbeat 174분 진행이었다. 성공 추론/품질 또는 제품 시간 예산 통과를 의미하지 않는다. [원시 결과](evidence/m1-qwen-cache-2026-11-16/software-webgpu-extended.json).
- Typecheck, 기존 cognition adapter **41개**·Shadow 계약 **16개**·live controller 회귀 및 production build 통과. Core 소스는 변경하지 않는다. 정확한 head의 원격 CI 및 배포 상태는 PR/후속 배포 기록을 기준으로 확인한다.
- QA 브라우저에서만 executor proxy 인증서 예외와 SwiftShader를 사용했다. 제품 TLS·모델 파일·단일 모델 정책은 변경하지 않았다. 물리 GPU/기기/발열 또는 실제 신경망 결과 품질의 증거로 계산하지 않는다.

[원래 모델·저장소 비교](evidence/m1-qwen-cache-2026-11-16/asset-cache.json) · [초기화 원시 로그](evidence/m1-qwen-cache-2026-11-16/initialization.log) · [제품 제한의 실제 추론](evidence/m1-qwen-cache-2026-11-16/software-webgpu-24s.json).

M1-07의 성공 추론 인수는 여전히 미입증이다. **M1 6/7을 유지**한다.

## 통합·공개 배포

[#1036](https://github.com/sjLim91/lifelens-ue/pull/1036) → UI main `426d428774414bccc038ade61ddb1c3fc39acefe`, 검증 head `664cb8a4`와 tree 동일. 정확한 head의 Web Checks·Web Cognitive Agent·Preflight 세 workflow가 모두 성공했다. Core 소스와 검증 런타임 `9202520`은 동일하다.

Pages Preview `43846345141`, Pages 배포 `43846475604`, External Probe `43846487565` 성공. 공개 JS가 gh-pages `d8af0e08`의 blob `573eb9d5d42631d47bda522713a39c518f701e48`과 같고 두 수정이 포함됨을 확인했다. 공개 실제 fallback GPU는 worker 0개·오류 0개·Core 480→552분(6초) 진행. [원시 배포 증거](evidence/m1-qwen-cache-2026-11-16/release-proof.json).
