# 사회·가족·돌봄·위생 결합 장기 회귀 — 2026-10-12

## 성격
이 브랜치는 **진단 전용 실험 브랜치**이며 main 병합 및 자동 병합을 금지한다.
최종 장기감사 기준 main: `19aa2ad371767779bddcb25b8ee5fc886423d0d9` (#646).
기반: #651 보육 ToiletAssist 위생시설 실제 이동 브랜치 `af50a5f747b6218d657f797da67c97221c9b207e`.
결합 후보: #648 off-cadence 로컬 첫 접촉 사회 활성화 (장기 회귀로 미병합 종료).
메인 Core 생존/사회·가족/정착지 숫자/임계값은 별도 튜닝하지 않는다.

## 목적
#648 단독으로는 seed4242001에서 결혼2, 임신2, 출산2까지 진행했지만 출산 후
adult illness 4명 및 child deprivation 2명으로 전체 붕괴했다.
#651은 ToiletAssist 시 실제 위생시설까지 부모와 아이가 경로 이동한 뒤 폐기물을 기록한다.
이 두 수정을 합친 **실제 Core 장기 실행**에서 출산 후 생활권 병원체 누적과
성인/아이 사망 원인 연쇄가 개선되는지 확인한다.

## 결합 원칙
- #651 부모/아이 동행, 시설에 HumanWaste 기록, 단절 시 fallback, Core route authority 유지.
- #648의 위치 근거 local social candidate 및 off-cadence first-contact probe만 결합.
- 정규 15분 통합 문명 선택에서 bootstrap 예외 적용 금지.
- SocialUtility.h, 관련 3개 테스트는 #648 후보를 그대로 사용. Simulation.cpp에서는
  #651 부모 돌봄 경로를 그대로 두고 `trySocialDecision`의 해당 한 호출부만 결합.
- 사회 미발생의 해결 여부와 출산 후 건강 안정성은 별도 합격 조건. 기능 생성 ≠ 안정성.
- 브랜치 편의상 실험 결과가 좋더라도 기존 폐기된 #648을 바로 main에 병합하지 않는다.

## 필수 검증
1. exact-head Core CTest, Preflight, Web WASM, Weather-Sleep-Era.
2. 두 canonical seed 874213954 / 4242001 각각 day1/7/30/100/365/1000,
   대표 seed 결정론 반복과 save byte identity 확인.
3. 두 seed social>0 여부, 실제 couple/pregnancy/birth 및 사건순서 확인.
4. seed424 출산 후 HumanWaste site/home 분리 및 pathogen/illness/deprivation
   사망 cascade 개선 여부를 baseline #648 단독과 비교.
5. seed874 기존 trade departure/exchange/physical return, 주민 생존, 이동,
   water/health/sleep regression 및 시설/chunk 급증 여부 확인.
6. 결론은 PASS / FAIL / UNKNOWN으로 원인 증거와 함께 분리.

## 실행 트리거
`.github/workflows/current-main-longrun-audit.yml`의
`audit/system-balance-*` push 경로가 이 브랜치에서 Core 파일 변경을 감지한다.
장기 실행의 실제 Actions 결과와 artifact를 확인하기 전에는 성공이라고 기록하지 않는다.
