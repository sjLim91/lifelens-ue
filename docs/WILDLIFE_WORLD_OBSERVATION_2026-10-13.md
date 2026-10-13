# Core 야생동물 관측 연결

신규 v4 게임의 실제 materialized 자연 청크에 기존 독립 WildlifePopulation을 등록한다. Core가 하루마다 기존 먹이·이동·번식·포식·자연사 전이를 실행하며 주민 RNG와 자원 인벤토리를 변경하지 않는다. 최대 64개 청크를 유지하며 관찰 카메라 이동은 개체군을 생성하지 않는다.

NaturalEcology 선택적 확장을 v2로 저장하고 기존 개체군 코덱의 checksum과 개체/수용력 검증을 재사용한다. 이전 v1/확장 없는 저장은 야생동물 없는 상태로 유지한다. 주민 GUID/기존 저장 필드와 초기 성인 4명·시설 0개 규칙을 보존한다.

terrainWindowJson 및 경량 humanTracesWindowJson에 동일한 wildlife DTO를 제공한다. 실제 살아 있는 개체의 chunk-qualified ID, 종류, 성별, 나이, 위치만 전달한다. 지리/지역 기후 provenance는 false다. 사냥·길들이기·고기/가죽 자원·지역 간 이동은 연결하지 않았다.

검증: 전체 Core 7개 translation unit을 g++ C++17로 연결하고 신규 world observation 테스트 통과. 저장/복원 후 3,000분 동일 continuation, 손상/다른 seed 거부, 기존 저장에서 개체 생성 금지, 읽기 전용 관측과 RNG 보존을 검사했다. 기존 wildlife ecology/codec, natural ecology lifecycle, snapshot codec, continuous terrain, web bridge 테스트도 통과했다.

최종 PR head `9bb19f4443d25f3f1a3ece2f0ff8f2b9aec699b1`의 CMake/CTest·결정론·365일 검증·WASM·공통 Web·생태/저장/좌표/audit 등 GitHub 9개 workflow PASS. PR #710은 `db4d93163948edc0c69f4c50f749a154a4769663`으로 main에 병합했다. 해당 Core runtime release와 Pages runtime checksum 검증도 PASS. 병합 후 main CTest·결정론 PASS, 추가 365일 run 38384160386은 배포 closeout 시점에 진행 중이었다.
