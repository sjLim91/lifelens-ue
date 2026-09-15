from pathlib import Path

handoff_path = Path('tasks/HANDOFF_LOG.md')
handoff = handoff_path.read_text(encoding='utf-8')
marker = '### 2026-09-15 — PR #80 Dug pit progression + World Genesis canonicalization'
if marker not in handoff:
    entry = """

## 2026-09-15 — sanitation progression / scalable world design closeout

### 2026-09-15 — PR #80 Dug pit progression + World Genesis canonicalization
- 작성자: 쭌 측 AI
- 브랜치/PR: `jjun/primitive-latrine-progression-v1`, PR #80
- 상태: `DONE / main 병합 완료`
- validated head: `fc918693bcd265f3c021f0bd826f6ba5a7113678`
- main merge: `291926cf78d12c1c61284eb9c59a50e7c70e54e7`
- 구현:
  - 개인 `DugSanitationPit` 지식/실험을 추가하고 기존 지정 배변구역을 같은 site ID/GridPos의 `DugPit`으로 개선.
  - 지식만으로 즉시 시설이 바뀌지 않으며 실제 반복 굴착 작업량이 필요함.
  - 현재 runtime에 실제 Dig 도구가 없으므로 v1은 느린 수작업 굴착을 허용하고 향후 Dig 도구가 같은 work contract를 가속하도록 설계.
  - DugPit 완성은 기존 HumanWaste 양을 삭제하지 않고 노출 강도/확산 반경을 줄이는 containment 효과를 적용.
  - 이후 DugPit 사용도 HumanWaste를 계속 생성하지만 open designated area보다 낮은 exposure profile을 사용.
  - snapshot 외부 포맷 v5 유지, primitive sanitation extension만 v2로 확장하고 v1 호환 decode 유지.
  - `DesignatedSanitationArea`와 `DugSanitationPit`이 Core/Unreal civilization read 범위에 모두 포함됨.
- 검증:
  - Structural Preflight run `34926841905` PASS.
  - Core Tests run `34926841895` PASS, **45/45**.
  - deterministic harness smoke PASS.
  - Unreal Linux Compile run `34926841907` PASS; UE 5.6 image verify + UHT + UBT + link PASS.
- 추가 canonical design:
  - `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`를 신규 canonical companion으로 확정.
  - 최종 월드는 작은 고정 arena가 아니라 `WorldSeed -> Macro World -> deterministic lazy chunks -> persistent human/environmental change -> carrying-capacity pressure -> migration -> additional settlements -> regional society/civilization` 구조를 따른다.
  - untouched chunk natural baseline은 `WorldSeed + ChunkCoord`로 탐사 순서와 무관하게 결정론적으로 생성.
  - 생성 후 인간/자연 변화는 persistent history가 권위이며 reload 시 reroll 금지.
  - 인구 증가 시 고정 맵에 압축하지 않고 토지 이용 고도화, 탐사, 이주, 분가/집단 분리, 복수 정착지 형성으로 확장.
  - 초기 자연환경에는 현대/정착 인프라가 없고 생존 가능한 시작 후보지만 평가한다.
  - WorldSeed와 초기 PopulationSeed/stream을 분리하여 같은 자연환경에서 다른 창립자 구성도 향후 지원 가능하게 한다.
  - Android는 논리 세계 전체를 고품질 Actor로 유지하지 않고 active presentation chunks / simulation LOD를 사용한다.
- 다음 쭌 레인:
  - HumanWaste Environmental Visual Feedback — READY_NOW.
  - World Genesis WG-1(seed/macro/chunk contracts)과 WG-2(persistence/streaming boundary)는 production World Visual Environment가 고정 맵 가정에 묶이기 전에 gate로 구현/확정.
- 상대가 알아야 할 점:
  - 다겸 PR #67 / Character/UI/Content 영역은 #80에서 수정하지 않았다.
  - World Visual Environment 작업은 `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`를 반드시 읽고 고정 소형맵을 제품 구조로 굳히지 않는다.
"""
    handoff_path.write_text(handoff.rstrip() + entry + '\n', encoding='utf-8')

spec_path = Path('docs/LIFELENS_SPEC_v1.1.md')
spec = spec_path.read_text(encoding='utf-8')
ref = '> - 확장형 세계 생성/청크/이주 구조는 `docs/WORLD_GENESIS_CHUNK_MIGRATION_v1.md`가 canonical companion이며, 고정 소형 arena를 제품 월드 구조로 사용하지 않는다.\n'
if ref.strip() not in spec:
    anchor = '> - 빌드/검증 방식(72~76절)은 `docs/BUILD_STRATEGY_v1.2.md`가 우선한다. 그 외 모든 섹션은 이 문서가 기준이다.\n'
    if anchor not in spec:
        raise SystemExit('master spec header anchor not found')
    spec = spec.replace(anchor, anchor + ref, 1)
    spec_path.write_text(spec, encoding='utf-8')
