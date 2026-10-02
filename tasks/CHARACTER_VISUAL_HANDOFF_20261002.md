# 캐릭터 시각 개선 인계 — PR #590

## 재개할 곳

- Draft PR: https://github.com/sjLim91/lifelens-ue/pull/590
- 브랜치: `visual/resident-motion-tools-phenotype-20261002`
- 기준 main: `03a7866ef63c367082d2df0bb3ed05dc1bf97a26`
- 목적: 사용자가 요청한 모션 누락/빈손 운반/도구 미표시/유전 외형 수치 미반영 개선.
- 상세 설계·자산 provenance·실행 명령·한계: `docs/WEB_CHARACTER_PRESENTATION_20261002.md`.
- Core, WASM DTO, 저장/밸런스, 공통 WORK_STATE를 수정하지 않았다. 다른 Chat의 #589와 변경 파일이 겹치지 않는다. main 병합·배포는 하지 말 것.

## 구현 상태

- Plant/Water/Tend/Harvest/Trade 및 확인된 야외 self-care를 semantic motion에 연결.
- 실제 양수 inventory를 기반으로 도구·용기·기록판 등 최대 3종 표시.
- 실제 물+용기가 있을 때만 물주기 용기를 손에 부착. 실제 자재가 있을 때만 이동 carry 모션+손 사이 자재 표시. 예정된 Gather/Retrieve 결과는 만들지 않음.
- 키/체격/피부/머리/눈/얼굴 수치 및 성별 비율 연결. 늦게 도착하는 관찰값 갱신, source geometry/material 보존, 누적 변형 방지.
- CC0 base/UAL1/UAL2 실제 다운로드 완료, fixed Git blob 검증 및 same-origin build vendoring. 유료 자산/서비스 없음. 독립 female mesh는 미도입.
- 10개 appearance/props 회귀 + action-context 회귀 + 실제 리그/4개 clip의 16 포즈 검증. 로컬 Blender contact sheet에서 물주기 손 용기/운반 자재/외형 차이 확인.

## 확인된 검증 checkpoint

- `4810e483a24292fa677c9cdbd4d0d1e8cc1563c4`: 자산 파이프라인. Action 36973765894 성공, artifact 11212457352.
- `5e6e1a72fe01df97bdf24f5a541c359fffe6f20a`: 최초 구현. Web Typecheck 36975143422 성공, Web Runtime Resilience 36975143337 성공, Character Asset Verification 36975134932 성공.
- 위 최초 구현의 Preflight 36975143339는 `next.play().fadeIn(blendSeconds)` exact token assertion 때문에 실패했다. reset을 별도 문장으로 분리해 동작을 보존하며 수정했다. 후속 HEAD 전체 CI 결과를 아래 최종 검증에 기록한다.
- 검토 중 확인한 빈손 물주기/운반 결손은 후속 변경에서 inventory와 hand attachment를 함께 gate하도록 보완했다. 최초 성공 CI만 보고 후속 HEAD도 성공했다고 추정하지 말 것.

## 최종 검증

2026-10-02 재개 후 GitHub에서 구현 HEAD `d5f959279f1d5ceb1b44d1b47294b78201ca1b63`의 모든 check run 성공을 확인했다.

| 검증 | 결과 / 근거 |
|---|---|
| Preflight | [36976216380](https://github.com/sjLim91/lifelens-ue/actions/runs/36976216380) 성공 |
| Web Typecheck (PR) | [36976216395](https://github.com/sjLim91/lifelens-ue/actions/runs/36976216395) 성공 |
| Web Typecheck (push) | [36976210341](https://github.com/sjLim91/lifelens-ue/actions/runs/36976210341) 성공 |
| Web Runtime Resilience | [36976216383](https://github.com/sjLim91/lifelens-ue/actions/runs/36976216383) 성공 |
| Character Asset Verification | [36976210291](https://github.com/sjLim91/lifelens-ue/actions/runs/36976210291) 성공 |

Character Asset Verification의 실제 step에서 CC0 자산 다운로드/고정 해시, 타입·행동·외형 회귀, production build, 실제 GLB 리그·모션 16개 포즈 검증, 검증 artifact 업로드가 모두 성공했다. 이전 Preflight 실패는 후속 구현 HEAD에서 해소되었다.

이번 재개 작업은 위 결과를 확인하여 인계 문서와 PR 설명을 마무리한다. 구현 이후 문서만 추가한 커밋의 검증과 위 구현 SHA의 검증을 구분한다. 이전 세션의 Blender 육안 검토는 위 구현 상태에 기록된 인계 근거이며, 이번 세션에서 새 브라우저/모바일 시각 QA를 수행한 것은 아니다. 브라우저 조명·기기 성능, 여성 전용 모델, 정교한 손가락 IK는 완료로 주장하지 않는다.

PR #590은 Draft 상태를 유지한다. main 병합·배포 없이 현재 구현과 검증 기록 정리를 완료했다.

## 재개 시 주의

1. AGENTS 및 사용자 지정 canonical 문서 순서를 확인하고 actual main/PR/Actions를 다시 조회한다.
2. 이 PR의 diff만 검토한다. stale Core branch wholesale merge 금지. 새로운 balance 축 변경 금지.
3. 동작 모양만 개선한 것으로 장기 고착/생존행동 문제까지 고쳤다고 보고하지 않는다.
4. 무료 공유 기본 mesh 비율 조정과 성별 전용 모델 도입은 다르다. 현재 후자는 미완이다.
5. CI artifact는 만료된다. 검증 스크립트와 asset pinned source로 재생성할 수 있다. 로컬 environment shell 네트워크가 차단되었으나 GitHub CI 다운로드와 artifact 전달은 성공했다.
6. 다음 자산 작업이 필요하면 실제 장착 도구/대상 접촉 계약, 맞는 라이선스, rig 호환성과 그립/IK를 검증한 뒤 진행한다. 예쁜 모션이라는 이유만으로 검 공격을 벌목에 쓰지 않는다.
