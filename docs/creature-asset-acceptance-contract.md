# LifeLens — 실제 생명체 자산 수용·출하 기준 (2026-10-15)

## 배경과 변경 금지 사항
Codex는 고품질 사람/호랑이 원본 확보·리깅·Web Observer 통합을 전담한다.
본 작업은 별도 브랜치에서 **인수 검증만** 구현하며, 3D 모델·Runtime/CSS·Core의
시뮬레이션 truth, 결정론, 유전·사회·AI·저장 계약은 변경하지 않는다.
기존 성과가 낮은 Q3/Q4 Blender 절차 모델 (#725/#727)을 성공으로 간주하지 않는다.

## 테스트 도구
`python3 Tools/creature_asset_gate.py PATH_TO_RELEASE_MANIFEST --root CANDIDATE_DIR`

확인 항목:
- 남성, 여성, 호랑이 3종 중 누락 불가. 개발 중 하나만 검증할 때에 한하여
  명시적인 `--allow-partial`을 사용한다.
- 원본 HTTPS URL, 제작자, 원 라이선스 링크, CC0 또는 CC-BY 조건, CC-BY 저작자 표기.
  CC-BY-NC/ND/불명확한 유료 자산을 배제한다.
- 인체 남녀 `near` 모델의 실제 POSITION 정점 형상이 완전히 동일하면 실패.
  (파일명·재질만 다른 가짜 성별 구분 차단, 해부학적 사실감은 별도 검수.)
- 각 개체 `near/mid` 실제 스킨 GLB와 핀된 SHA-256. 헤더·BIN 경계·스킨
  joint/weights, 삼각형 면/normal/색 또는 UV, 데이터 무결성.
- `far`는 저폴리곤 GLB **또는** 기존 Web Observer `InstancedMesh` fallback을 허용.
  fallback은 별도 실행 보고서에 Core ID/위치 보존, 중복 렌더 없음, 실 테스트 성공을 요구한다.
  (Codex #728의 2LOD + far instanced 구조를 의도적으로 지원.)
- 예산: near 85k / mid 30k / far 6k 삼각형 상한, LOD 감소율 각 1.5배 이상.
  GLB 크기 상한 22 / 10 / 4 MiB. 모바일 프레임 레이트는 **별도 실측** 필요.
- 웹 연동 검증 리포트: Core 기반 개체 키, 실제 Three.js runtime, animation clip
  (주민 idle/walk/sleep/work, 호랑이 idle/walk/run), 실패 시 fallback 정상 작동.
- 정면·측면·3/4·모션·모바일 실제 PNG 증거. 모바일은 세로형 화면이어야 한다.

**이 코드는 "아름다움"을 자동 판정하지 않는다. 결과가 성공하더라도
`release_approved: false`, `review_required: true`를 출력한다.**
시각적 사실감, 생체 해부학적 비율, 피부/모발/피모/줄무늬, 발의 접지,
기존 Core action과 애니메이션/지형 IK 호환, 모바일 FPS, 모델 재사용 및
다수 주민 동시 노출은 제품 화면에서 수동 시각/성능 검수를 통해 별도로 승인해야 한다.

## 후보 모델용 폴더 규격
다음 파일이 존재하면 CI에서 자동 검사한다.

`web/public/vendor/creatures/release-manifest.json`

- 후보 단계에서 호랑이 한 종만 납품하는 경우 `candidate-manifest.json`은
  `--allow-partial`로 검사한다. `release-manifest.json`은 최종 남녀·호랑이 3종 필수.
- `web/public/vendor/creatures/` 아래에서만 상대 경로를 허용한다.
- GLB, 증거 PNG, `web-motion.json` 파일들을 포함한다.
- 코드는 GitHub Actions 표준 라이브러리에서 실행되며 유료 API/Blender/Node
  설치 없이 테스트 가능하다. (제작 자체는 Codex Blender/Three.js 파이프라인.)
- 검증 성공은 최종 승인이나 main 병합 명령이 아니다.

### Manifest 예시 (값은 샘플이며 실제 자산을 의미하지 않음)

```json
{
  "schema_version": 1,
  "creatures": [
    {
      "id": "tiger",
      "source": {
        "url": "https://example.org/REAL_MODEL_PAGE",
        "author": "ORIGINAL_AUTHOR",
        "license": "CC-BY-4.0",
        "license_url": "https://creativecommons.org/licenses/by/4.0/",
        "attribution": "ORIGINAL_AUTHOR — MODEL_TITLE (CC BY 4.0), source URL"
      },
      "lods": {
        "near": {"file": "tiger/near.glb", "sha256": "ACTUAL_64_CHAR_SHA256"},
        "mid": {"file": "tiger/mid.glb", "sha256": "ACTUAL_64_CHAR_SHA256"},
        "far": {"file": "tiger/far.glb", "sha256": "ACTUAL_64_CHAR_SHA256"}
      },
      "web_motion_report": "tiger/web-motion.json",
      "review": {
        "front": "tiger/front.png",
        "side": "tiger/side.png",
        "threequarter": "tiger/threequarter.png",
        "motion": "tiger/motion.png",
        "mobile": "tiger/mobile.png"
      }
    }
  ]
}
```

샘플을 그대로 복사하면 파일·실제 해시·출처가 없으므로 실패한다. 실제 인수
단계에서는 3종 모두 필요하다.

### 원거리 기존 InstancedMesh fallback 예시 (선택)

이 구조를 쓰는 개체는 `lods.far`를 생략하고 아래 항목을 추가할 수 있다.

```json
"far_fallback": {
  "mode": "existing-instanced",
  "test_report": "tiger/far-fallback.json"
}
```

`test_report`의 실제 웹 테스트 결과에는
`creature_id`, `runtime: "web-threejs"`, `tested: true`,
`core_id_preserved: true`, `core_transform_preserved: true`,
`no_double_draw: true`가 필요하다.
검증기를 통과시키기 위한 임의 값 입력은 실험 증거로 인정하지 않는다.

### 실제 Web Motion report 예시

```json
{
  "creature_id": "tiger",
  "runtime": "web-threejs",
  "tested": true,
  "fallback_works": true,
  "clips": {
    "idle": "pass",
    "walk": "pass",
    "run": "pass"
  }
}
```

리포트는 Three.js 브라우저에서 실제 렌더/애니메이션 테스트를 실행한 후 기록해야
하며 임의로 `pass`를 써 넣는 행위는 승인 근거가 될 수 없다. 실제 실행 로그와
스크린샷을 PR에 함께 첨부한다. 스크립트는 보고 형식을 검사할 뿐 시험 진실성을
독립적으로 보장하지 않는다.

## 기존 시스템 인수 체크
- `web/src/render/resident-world-layer.ts`, `resident-rig.ts`의 기존 donor
  bone과 주민 28+ 개 행동 매핑을 손상하지 않는다.
- `web/src/render/wildlife-layer.ts`, `wildlife-quality-surface.ts`의
  species/sex/LOD/InstancedMesh와 core-authoritative movement를 유지한다.
- 사람이 앉기/눕기/자기/도구작업/사회활동 중 지형 관통, 뒤로 걷기, 머리·머리카락
  분리, 발 미끄러짐 없는지 실제 장면에서 확인한다.
- 브라우저와 Android에서 실제 1/4/16개 개체, 지형 zoom 거리, 메모리 및 FPS를
  기록한다. 기준 미충족 시 구버전 모델 fallback을 유지한다.
- 최종 시각 비교는 동일 카메라·동일 조명·동일 동작으로 이전/새 모델을
  1:1 비교하며, 사람이 자연스럽게 보이는지 육안 평가한 승인 기록을 남긴다.

## 개발 및 회귀 테스트
`python3 -m unittest discover -s Tools/tests -p 'test_creature_asset_gate.py' -v`

기존 `.github/workflows/character-assets-check.yml`을 건드리지 않고 새
`creature-asset-release-gate.yml`에 포함. 병합 전에 CI 검증과 PR 코드 리뷰 필요.

> 비고: 라이선스 URL의 문구·정품 모델의 저작권 상태는 저장소가 스스로
> 증명할 수 없다. **원저작자 기록과 해당 모델 라이선스가 부합하는지 사람이
> 확인해야 한다.** 자동 검사는 허용 목록/증빙 누락을 차단하기 위한 것이다.
