# M10-03 제안 → 실제 행동 → 결과 → 학습

사용자 지시: 발열 검증은 이번 작업에서 제외한다. 기존 M10 관찰 UI/기본복/동물 배속 배포를 보존하고 제안→행동의 전체 인과 연결을 구현한다.

## Core가 만드는 연결

- 승인된 판단의 actor/minute/context fingerprint/intent/target/원문 대신 저장된 digest를 그대로 식별한다. 비슷한 시간이나 같은 의도를 근거로 Web에서 행동을 붙이지 않는다.
- 실제 unified planner가 해당 판단을 선택해 ContextAction token을 발행한 곳에서 연결한다. teaching/care/trade/migration의 실제 선호 점수 반영은 `Preference`로 구별하며, 모델이 없으면 그 행동이 불가능했을 것이라는 반사실적 주장은 하지 않는다.
- 자원 실패·중단 후 동일 목표의 복구 단계는 Core의 실제 이전 토큰을 따라 연결한다. 일반 reflex/독립적인 행동을 승인 기록에 끼워 넣지 않는다.
- 이동 전 발행, 실제 Core 실행, 실행 성공/실패, 경로 실패/기한/생존 중단을 구분한다. 결과 callback 없이 닫힌 토큰은 종료 관측으로 표시하고 성공·실패·학습을 만들어내지 않는다.
- 실제 실행 함수에서 동일 belief의 증거 수/stance를 전후로 측정한다. 실제 evidence count가 증가한 경우만 학습 반영으로 기록한다. 같은 시각의 다른 기억/믿음을 나중에 끼워 맞추지 않는다.
- 즉시 지역 물물교환은 실제 등록 exchange fact로 연결한다. 존재하지 않는 context token을 만들지 않는다. 미승인·차단 제안에는 실행 단계가 생기지 않는다.

## 저장 및 관찰 경계

- 최대 64개 판단, 판단당 24개 실제 단계를 보관하는 Core 소유 관찰 journal이다. planner/utility/physical outcome/RNG는 이 journal을 읽어 결정하지 않는다.
- 기존 cognition snapshot extension v3에 관찰 journal을 보존한다. v1/v2는 계속 읽으며 과거에 없던 행동 결과를 복원하지 않는다. 비어 있는 journal은 기존 저장 형식을 유지한다. actor/승인 원본/단계 식별자/시간/중복/유한 학습 값 검증에 실패하면 저장·복원을 거부한다.
- `cognitiveCausalJournalJson(actor)`는 읽기 전용 schema 1이다. 최신 8개 판단을 내보내며 actor/seed/population/generation과 64비트 번호/지문을 lossless 문자열로 전달한다. 승인 후 미선택·대체·유효기간 만료를 구분한다.
- Web reader는 소유자·세계·시간·토큰·출처·결과·학습 count와 배열/문자열 한도를 검증한다. 확장된 상세를 여는 동안만 1초 간격으로 읽으며 이전 런타임에는 개별 관찰 fallback을 명시한다. 원본 제안→단계 연결을 Web이 생성하지 않는다.

## 검증 경로

1. 실제 controlled Core New Game fixture: 같은 승인 fingerprint의 Gather token 1, Craft token 8, 원천 재고 80→77, 실제 이동 20칸, 각 행동 직접 기억과 belief 증가를 검사한다. 첫 채집 후 binary save/load한 실행이 원래 실행과 byte-exact 일치한다.
2. 실제 다른 Core 주민의 원천 소진: 실패 token 1 → 다른 원천의 회복 token 5 → 제작 token 16, 실제 실패·성공 기억/학습과 직렬화 재개를 검사한다. 자연 무개입 AL-2 인수로 둔갑시키지 않는다.
3. 승인/차단/중복/만료, 잘못된 소유권·미래 시각·손상 저장, legacy journal 및 읽기 전용 bridge 검증.
4. Web DTO lossless ID·중복/가짜 성공·가짜 학습·다른 주민·한도·이전 ABI 회귀, 전체 Web 타입/빌드 및 runtime/cognitive 회귀.
5. 실제 native fixture의 Core JSON을 320/393/915px UI에 표시하는 선택적 브라우저 검사. fixture는 합성된 성공 사건이 아니다.
6. CI에서 actual native checkpoint → 생성된 WASM → 동일 Gather/Craft 결과/학습 DTO 및 중간 save/load 재개를 비교한다. 추가 네트워크 fetch 없이 공급한 WASM bytes로 검증하고 JSON 증거를 보관한다.

기존 Core 전체 CTest/결정론, Web/WASM/Preflight와 병합 후 Runtime Release/Pages/External Probe를 실제 실행 결과에 따라 확인한다. 이 변경은 Qwen의 실제 추론 성능이나 전체 M10 7개 단위의 완료를 대신하지 않는다.
