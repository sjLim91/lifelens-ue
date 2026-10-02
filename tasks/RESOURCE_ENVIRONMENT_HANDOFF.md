# 자원 환경 통합 작업 checkpoint

- 사용자 승인: 최신 main에서 분석/구현/검증 후 PR 생성. main merge 금지. 추가 승인 재요청하지 않는다.
- 기준 main: c6a357686266b6a8cc35d433e8af978d8614793c (2026-10-02 확인).
- 저장소: sjLim91/lifelens-ue. 로컬 /workspace/lifelens는 GitHub connector로 받은 소스 snapshot이며 git clone이 아니다.
- shell proxy 연결 실패. 네트워크 권한 질문은 사용자가 취소했다. 같은 요청 반복 금지. GitHub connector로 읽기/branch/commit/PR 가능.
- AGENTS.md와 canonical runtime/spec/milestone/web architecture/work state 확인.
- 분석: vegetation/ground detail은 terrainDressingSignature + coverage만 사용. spatial target은 accessGrid에 9 resource meshes로 별도 표현. quantity 0 필터로 자연 식생을 지우지 못함. WorldScene.setAuthoritativeSpatialTargets 경로에 공통 resource projection 공급 필요.
- 설계: 고갈 노드도 포함하는 immutable grid 기반 영역. 기존 instanced tree asset/palette 및 ground meshes에 고정 후보 + quantity ratio에 따른 visibility/size 적용. baseline은 영역 바깥 유지. accessGrid clearance, 물/시설 guard 유지. 별도 resource meshes 제거, sanitation 유지. 지형 geometry 재생성 없음.
- 다음: 공통 projection 구현 → 레이어 및 scene 연결 → validator/회귀/typecheck/build → connector commit/PR. 완료/검증 결과를 이 파일에 갱신.

## 구현/로컬 검증 checkpoint

- 브랜치: work/web-resource-environment-20261002. 구현 완료, PR CI 검증 예정.
- main #601 갱신 반영: c6a357686266b6a8cc35d433e8af978d8614793c. Web/수정 파일 overlap 없음.
- 별도 자연 자원 프록시 mesh 9개 제거. sanitation와 accessGrid projection helper 유지.
- NaturalResourceProjection: 고갈 영역/인접 chunk masking, 고정 slots, quantity 비례 수/크기, accessGrid 회피, 동일 seed/state 재현.
- 모바일 나무 6/풀14/관목4/돌5 per chunk 및 기존 global capacity 유지. draw call 추가 없음.
- 로컬: projection 회귀 10/10, spatial validator PASS, ecology validator PASS.
- 다음: PR exact-head Runtime Resilience (전체 presentation/typecheck/build 포함), Preflight 결과 확인/오류 수정. main merge 금지.
