# Balance Audit Checklist

기준 브랜치: `audit/simulation-balance-20260930`  
기준 main: `0e409b53089739f737856ad508bfbdd99c80e8cb`

## 원칙

- #539 실험값을 기준선에 섞지 않는다.
- 숫자를 먼저 중앙화하지 않는다. **현재 의미와 상호작용을 먼저 계측**한다.
- 수정은 한 축씩 하고 동일 seed/동일 기간으로 전후 비교한다.
- 순간 행동 하나보다 장기 시간예산과 자원수지를 우선한다.

## 전수검사 상태

- [x] Need 기본 증가율
- [x] Physical fallback 효과/시간
- [x] Sleep 순회복률
- [x] Social/Civilization 임계치와 호출부 hard gate
- [x] Physical/Social/Civilization 계획 cadence
- [x] 실패 backoff / context timeout
- [x] Water / PlantFood 자연 재생량
- [x] 야외 용변 Hygiene burden
- [x] 식량 부패
- [x] Cultivation 성장/수확 상한
- [x] 저장소 인식/압력 계수
- [x] 시설 wear / 수면 효율
- [x] Social utility 가중치
- [x] Relationship 변화량
- [x] Family/romance/pregnancy 주요 threshold
- [x] Parenting action 효과
- [x] Environment 추가 Need pressure
- [x] 장기 계측 harness 추가
- [x] 1/7/30/100일 baseline 실측
- [x] 365일 baseline 실측
- [x] 500일 baseline 실측
- [x] 1000일 baseline 실측
- [x] 1/7/30/100일 다중 seed 편차 확인
- [ ] 365/500/1000일 추가 seed 편차 확인
- [ ] 정책 숫자 중앙화 후보 확정
- [ ] Balance Contract v1 확정
- [x] P0 행동 commitment 장기 A/B
- [x] P0 생활수지(Eat/Drink/Sleep/Wash + headless sanitation) 1차 A/B
- [x] P0 KnowledgeTeaching plan-steal 장기 A/B
- [ ] P0 위생 발전 catch-22 장기 A/B (진행 중)
- [ ] P0 생존경제 튜닝
- [ ] P0 Social starvation 튜닝
- [ ] P1 Sleep 튜닝
- [ ] P1 cadence/backoff 튜닝
- [ ] 전 구간 회귀
- [ ] main 병합

## 변경 순서 잠금

1. 계측 결과 고정
2. Resource/Physical budget
3. Decision bands
4. Social/Civilization 경쟁
5. Sleep
6. Cadence/backoff
7. Family/lifecycle 장기 빈도
8. 1000일 회귀

한 단계가 녹색이 되기 전에 다음 단계의 정책 숫자를 동시에 바꾸지 않는다.
