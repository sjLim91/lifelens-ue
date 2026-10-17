# LifeLens 소스 전체 감사 — 2026-10-17 / 1차 기준선

**상태: SOURCE REVIEW + AUTOMATED EVIDENCE GATE. 전체 동적 검증 완료가 아님.**

- 기준: main@42191487d0412806ef26333478a13b29ffae915e (2026-10-14 commit).
- 대상: GitHub main의 추적 파일 719개 / 6,255,227 bytes (소스 트리와 메타데이터 확인).
- 현재 브랜치: audit/full-source-causal-20261017 (진단/회귀만, Core truth 및 밸런스 불변).
- 별도 PR의 미병합 변경은 main 동작으로 주장하지 않는다.
- 실행: python3 Tools/full_source_audit.py --out full-source-audit
- 결과: full-source-audit/source-inventory.json 및 SOURCE_AUDIT.md. PR CI 스모크 로그도 보관.

## 1. 소스 검토 발견 사항

| ID | 중요도 | 소스 근거 | 판정 및 추가 검증 |
|---|---|---|---|
| COG-01 | P1 | Simulation.h / Simulation.cpp::acceptCognitiveProposal | 승인된 외부 AI 제안은 journal에 기록되며 실제 goal/action 영향은 아직 구현되지 않음. COG-4 연결 검증 필수. |
| COG-02 | P1 | WebClientBridge.h / WASM bindings / core-types.ts | Core proposal 제출 API가 Web에 노출되지 않음. Shadow는 읽기 전용. |
| SOC-01 | P1 | SocialUtility.h::chooseSocialUtilityDecision | Global 후보 선택에서 사망 target을 제외하지 않음. trySocialDecision에서 실행 직전 거부하므로 사망자가 실제 행동한다는 의미는 아님. |
| EARTH-01 | P1 | WebClientBridge.cpp::earthScenarioJson | snapshotPersistence:false. 지역 위경도 결속은 저장·복원이 아직 미완성. |
| EARTH-02 | P1 | WebClientBridge.cpp::newEarthGame | terrainGeographicallyConditioned:false. 실제 위치의 지형·기후로 생성되는 단계 아님. |
| ECO-01 | P2 | WildlifeEcology.h::stepWildlifeDay | 동물은 독립 개체·포식·번식·사망을 계산하지만, 이동은 청크 경계 내 제한됨. |
| VIS-01 | P2 | web/src/render/wildlife-layer.ts | main은 procedural geometry를 사용. 고품질 GLB 관련 #725/#727/#728 미병합·출하 품질 검증 전. |
| PERF-01 | P2 | Simulation.cpp::runMinutes | 시뮬레이션 분마다 step()을 반복함. 장기/대규모에서 비용 측정 필요. |

우선순위는 정적 근거에 기반한 감사 우선순위다. P1이라는 이름이 곧 검증 완료된 런타임 장애를 의미하지 않는다.

## 2. 반드시 다시 실행할 인과 테스트

2026-10-06 당시 장기 감사에서 seed 874213954, 4242001은 1,000일간 social 0%, 출생 0명, 각각 초기 주민 1명 사망이었다. **현재 main 재실행 결과는 아니다.**

- main에서 1/7/30/100/365/1000일을 새로 실행해 생존/사회/가족/시설/교역 비율 검증.
- 초기 접촉/결혼/임신/출산/영아 위생/성인 질병 사망의 원인-결과 사건 검증.
- 주민의 모델 판단 → 실제 이동·자원·상호작용 → 결과 → 경험 학습의 동일 토큰 추적.
- 저장·복원·결정론·일수 건너뛰기 비교. CI PASS만으로 창발이나 삶의 질을 승인하지 않는다.
- 미병합 #651/#652/#656 등의 후속 연구 결과를 main과 섞지 않는다.

## 3. 자동 진단/범위

Tools/full_source_audit.py는 Git 추적 파일 전부를 열거해 SHA256, 크기, 줄 수, UTF-8 상태, C++ 로컬 include/TS 상대 import, Core tests-CMake 등록, 대형 파일 및 TODO/FIXME 등을 기계 판독 결과로 기록한다.

CI는 스캐너 테스트, Core determinism·snapshot·social 회귀, 실제 소규모 시드 스모크를 실행해 증거를 저장한다. 진단은 읽기 전용이며 물리/Needs/utility/가족/환경 규칙을 변경하지 않는다.

**이 감사 PR만으로 완결되지 않는 항목:** 소스 모든 줄의 사람 검토, 1000일 다중 시드 최신 실행, 주민 100/1000명 벤치마크, 실제 모바일 GPU/렌더 시각 검수, 지구 지역 실제 데이터 동등성.

정확한 PR head CI 및 산출물 검증 전에는 Draft 유지하고 main에 병합하지 않는다.
