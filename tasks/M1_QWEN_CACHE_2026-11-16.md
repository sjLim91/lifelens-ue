# M1-07 Qwen 모델 캐시 로딩 수정

기준 main `a2c9c2830e409addbe26f0d532ed714f11ee9065`. M1-01~06 공개 배포 뒤 실제 모델 초기화 실패를 추적한 후속 작업이다. 모델 정책·24초 제품 추론 제한·Core 권위는 유지한다.

## 원인과 수정

`Qwen3-1.7B-q4f32_1-MLC`의 원래 embedding 조각은 **155,582,464 bytes**다. 공식 manifest의 MD5 `c738b02f2921806bec6241c056c7d89f`와 내려받은 파일이 일치한다. 같은 실제 파일을 로컬 HTTP로 제공한 브라우저 분리 실험에서 IndexedDB는 **133,169,152 bytes** 단일 값 제한을 이유로 거부했고, Cache API는 1,607ms에 저장했다.

WebLLM asset cache를 `indexeddb`에서 라이브러리 기본 지원인 `cache`로 변경한다. 기존 저장 게임과 기존 모델 캐시는 삭제하지 않는다. 다른 모델이나 서버 추론으로 바꾸지 않는다.

## 실제 검증과 경계

- 공식 모델 전체 **30조각**을 HTTPS로 내려받고 소프트웨어 WebGPU에서 초기화 완료: 약 **78.895초**. 캐시 재사용 후 실제 제품 factory로 초기화 **7,435ms**.
- 실제 WASM 주민의 **7,018 byte Core 문맥**으로 제품과 같은 24초 scheduler 추론을 실행했다. **24,069ms 시간 초과**, 그동안 Core heartbeat **20분** 진행·추론 직후 Core 계속 진행. 가짜 provider가 아니며 성공 제안은 없었다.
- 제품 설정을 변경하지 않은 별도 **180초 진단**도 180,018ms 시간 초과, Core heartbeat 174분 진행이었다. 성공 추론/품질 또는 제품 시간 예산 통과를 의미하지 않는다. [원시 결과](evidence/m1-qwen-cache-2026-11-16/software-webgpu-extended.json).
- Typecheck, 기존 cognition adapter **40개**·Shadow 계약 **16개**·live controller 회귀 및 production build 통과. Core 소스는 변경하지 않는다. 정확한 head의 원격 CI 및 배포 상태는 PR/후속 배포 기록을 기준으로 확인한다.
- QA 브라우저에서만 executor proxy 인증서 예외와 SwiftShader를 사용했다. 제품 TLS·모델 파일·단일 모델 정책은 변경하지 않았다. 물리 GPU/기기/발열 또는 실제 신경망 결과 품질의 증거로 계산하지 않는다.

[원래 모델·저장소 비교](evidence/m1-qwen-cache-2026-11-16/asset-cache.json) · [초기화 원시 로그](evidence/m1-qwen-cache-2026-11-16/initialization.log) · [제품 제한의 실제 추론](evidence/m1-qwen-cache-2026-11-16/software-webgpu-24s.json).

M1-07의 성공 추론 인수는 여전히 미입증이다. **M1 6/7을 유지**한다.
