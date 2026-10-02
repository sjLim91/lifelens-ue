# 캐릭터 행동·소지품·유전 외형 시각 개선 인계

- 기준 main: `03a7866ef63c367082d2df0bb3ed05dc1bf97a26`.
- 브랜치: `visual/resident-motion-tools-phenotype-20261002`.
- 사용자 요청: 부자연스러운 행동/모션 누락/도구 표시 보완, 무료 자산 다운로드, 성별과 개별 외형 수치 반영.
- #589의 Core 생존·위생 수정과 분리. Core/save/Need/선택 정책 변경 및 main 병합·배포 금지.
- 확인 결손: 외형이 Genetics 대신 ID hash 중심; 전체 주민에게 SuperHero_Male 기본 mesh; 실제 소지품 attachment 없음; Trade/일부 cultivation/야외 self-care가 neutral로 빠짐.
- 무료 자산은 기존 pinned Quaternius CC0를 우선 사용하고 재배포 가능한 license/source/hash를 보존. Ruth2 AGPL 모델은 이번 범위에 도입하지 않는다.
- game-dev CLI 미설치로 해당 skill의 package admission/receipt 경로는 사용 불가. 기존 저장소의 자산 로딩·CI 다운로드 방식으로 사용자가 요청한 무료 자산 확인/구현을 진행한다. 게임-dev 검증을 받았다고 보고하지 않는다.
- 로컬 shell 네트워크가 막혀 공개 URL 다운로드 실패. GitHub CI에서 pinned 다운로드/검증·inventory를 확인한다. 미확인 clip 이름을 runtime에 연결하지 않는다.
- 현재 checkpoint: 분석 및 자산 검증 준비. 구현/검증 완료와 남은 한계는 후속 커밋에서 갱신한다.
