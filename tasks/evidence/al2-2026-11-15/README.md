# AL-2 Core 제작 목표 인수 증거 — 2026-11-15

기능 검증 소스는 `40bf67d80911b38a18886de770e75fdca46965c2`다. 실제 main 병합·배포 출처 및 workflow는 `release-proof.json`을 따른다. AL-2의 Core 제작 목표 납품 체크포인트이며 M2/M3/G1 전체 인수를 뜻하지 않는다. 기존 86단위 장부는 21 완료/20 진행/45 미완·미검증을 유지한다.

- `build-proof.json`: 최종 소스·native audit·로컬 WASM SHA-256. Core는 Release, 로컬 테스트 진입부는 assertion이 켜진 -O0다. 원격 CI는 일반 Release다.
- `closeout-preflight.log`: 완료 문서의 canonical Preflight 32개와 86단위 장부 일치 검사.
- `core-ctest.log`, `ci-runs.json`: 전체 Core 회귀 및 정확한 최종 PR head의 필수 workflow. CI의 deterministic harness도 포함한다.
- `production-unit.log`: 실제 원천 A 고갈 실패 → B까지 이동 → 실제 제작, 같은 원래 목표/기한/중간 binary 재생, 경쟁 도구의 목표 대체 방지, 새 낮은 우선도 생각의 물리 결과 취소 방지, 실제 6명령 예산, 손상/위조/구 저장 검사. 테스트 rules에서 욕구 증가를 멈추고 낮은 욕구·도구 지식·유한 원천·수락된 테스트 생각을 정하며, 경쟁 고갈을 통제한다. 자연 발생 또는 실제 LM 추론으로 계산하지 않는다.
- `production-recovery.log`: 기존 경쟁자 고갈 회귀. 시험이 정한 접근 위치에서 별도 경쟁자의 Core executor가 유한 원천 80을 실제 소비한다. 시험 대상 주민은 Simulation::step만으로 실패→다른 원천까지 6칸 이동→2단위 채집→501분 실제 제작을 수행한다. 경쟁자의 자율 이동/선택을 무개입으로 증명한 시험은 아니다. 원래 목표 480→600분, 실제 토큰 1→5→6과 실패 직전/채집 후 저장 재생 조건을 유지한다.
- `survival-resume.log`: 통제된 실제 보유 음식과 생존 압박, Core 먹기 실행 후 동일 제작 목표 재개. 자연 장기 census에서 생존 중단이 관측됐다는 주장과 구분한다.
- `natural-*-365.jsonl`: 변경하지 않은 setupNewGame과 Simulation::step의 세 세계(874213954/4242001/874213955), 1/7/30/100/365일 census와 실제 native binary 왕복. 재료·욕구·생각·기억·성공을 주입하지 않는다. 완료·실패 후 완료·미완·중단을 구분한다.
- `natural-summary.json`: 위 원시 census의 집계. 시간이나 단계 수만으로 목표 완료를 만들지 않는다.
- `al1-*.log`: 자연 성공 기억 대조는 세 독립 세계 874213954/874213955/42 모두 14일 이내, 정확히 한 직접 기억만 제거/재삽입, 다른 snapshot 바이트 동일, 다른 실제 원천 선택과 양쪽 물리 실행 완료를 요구한다. 기존 4242001은 현재 경로에서 긍정 기회가 없어 추가 진단으로 유지한다. 자연 실패 기억 대조는 원래 시드4/90일/양수 시작 재고/원래 기한/수량·이동·원천 감소를 유지하며 첫 부분 채집에서 멈추지 않고 전체 원래 목표 달성까지 확인한다.
- `fixture/`: 실제 무개입 세계의 첫 다단계 완료 목표 직전/결과 native binary, 경과분, 실제 주민/목표 JSON. 첫 실험 실패 → 실제 Clay 채집 → 실제 SimpleContainer 실험 성공이다. 수락된 모델 생각이 없는 native 출처를 보존한다.
- `local-wasm-proof.json`, `public-wasm-proof.json`: 실제 WASM의 물리/계획/실행 바이트는 native와 동일. 기존 mt19937 텍스트의 libstdc++/libc++ 표현 차이만 별도로 다루며, 같은 실제 codec으로 정규화하면 전체 binary도 동일하다. 중간 저장 재개·읽기 전용 관찰·손상 복원의 원자성과 추가 fetch 0을 검사한다. 로컬과 공개 빌드 바이트 자체가 같다는 뜻은 아니다. 로그의 `Core checkpoint decode failed`는 의도된 잘린 저장 복원 거부이며 직후 PASS와 상태 보존을 확인한다.
- `local-app-proof.json`, `public-app-proof.json`, `production-320.png`: 격리한 새 브라우저 context의 IndexedDB에 위 실제 자연 checkpoint를 복원하고 앱 화면·실제 자동 저장·새로고침을 확인한다. 한국어 목표/실패/재료/성공/실제 토큰, 320/393/915/1440px 오류·overflow 0. 기존 사용자 데이터는 변경하지 않는다.
- `public-proof.json`: 공개 HTML/launcher/bundle/runtime가 고정 gh-pages Git tree와 정확히 같은 blob이며 runtime SHA256 manifest와 실제 main 출처도 일치한다.

재현:

```sh
cmake -S Source/LifeLensCore -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
LIFELENS_PRODUCTION_FIXTURE_DIR=/tmp/lifelens-production-fixture build/ll_production_audit 874213954 7
python Tools/build_web_client.py
node Tools/verify_wasm_production_plan.mjs /tmp/lifelens-production-fixture
python tasks/evidence/al2-2026-11-15/verify-public.py <runtime-main-sha> /tmp/al2-public
```

`review-app.cjs`는 Playwright와 Chromium이 있는 환경에서 `LIFELENS_REVIEW_URL`, `LIFELENS_REVIEW_OUTPUT`, `LIFELENS_PRODUCTION_FIXTURE_DIR`, `LIFELENS_PLAYWRIGHT_MODULE`, `LIFELENS_EXPECTED_RUNTIME_SHA`를 받는다. `verify_wasm_production_plan.mjs`의 두 번째 인자에 다운로드한 `/tmp/al2-public/runtime`를 주면 공개 WASM을 같은 fixture로 검증한다. software 2D fallback 검사는 물리 Android/GPU 검사가 아니다. 실기기·발열 제외는 이전 사용자 지시를 유지하며 실제 Qwen 추론 품질을 주장하지 않는다.
