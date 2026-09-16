# LifeLens Social Communication & Localization v1

이 문서는 LifeLens의 **사용자 표시 언어와 주민 간 사회적 소통의 관찰 표현**에 대한 canonical companion spec이다.

## 1. 제품 목표

LifeLens는 주민들이 내부 수치만 바꾸는 사회 시뮬레이션이 아니라, 플레이어가 실제로 **누가 누구에게 어떤 의도로 접근하고, 대화하고, 위로하고, 다투고, 사과하고, 호감을 표현하는지 관찰할 수 있는 세계**여야 한다.

Observer 화면에서 사회행동은 관계 수치 변화만으로 끝나면 안 된다. Core에서 실제로 발생한 사회 사건을 Presentation이 사람답게 보여줘야 한다.

## 2. 언어 원칙

- 기본 사용자 표시 언어는 **한국어**로 한다.
- Core enum / action id / event id / protocol value는 언어 중립적인 식별자를 유지한다. 예: `UseToilet`, `Comfort`, `Argue`, `Confess`.
- UI에 raw English identifier를 직접 출력하지 않는다.
- Presentation/localization layer가 식별자를 현재 locale의 표시 문자열로 변환한다.
- 한국어 기본 제공 후 다른 언어를 추가할 수 있는 구조로 만든다.
- UI 문구를 여러 C++ 파일에 흩어진 문자열 literal로 복제하지 않는다. String Table / localization-ready data source 등 일관된 표시 계층을 사용한다.

예:

- `UseToilet` → `화장실 이용`
- `Comfort` → `위로하기`
- `Argue` → `말다툼`
- `Confess` → `고백하기`
- `VeryLow` → `매우 낮음`

## 3. Core / Presentation authority boundary

Core가 결정하는 것:
- actor / target
- social intent / action type
- 발생 시각 / 위치
- 관계/감정/기억 등의 입력 상태
- success / rejection / interruption / outcome
- authoritative relationship / emotion / memory 변화

Presentation이 결정하는 것:
- 해당 사건을 어떤 말풍선/아이콘/애니메이션/시선/카메라로 보여줄지
- 사용자 언어로 어떤 문장을 표시할지
- 화면 혼잡을 피하기 위해 어느 수준으로 축약할지

Presentation은 Core에서 발생하지 않은 고백/싸움/위로/관계 변화를 임의로 만들어서는 안 된다.

## 4. 사회행동 표현 단계

모든 대화를 큰 말풍선으로 표시하지 않는다. Observer-first 가독성을 위해 중요도별로 표현한다.

### Level A — 일상적 소통
예: 인사, 가벼운 잡담, 짧은 질문.

표현:
- 서로 접근/정렬
- 시선 또는 몸 방향을 상대에게 맞춤
- talking gesture / idle talking animation
- 작은 `💬` 또는 짧은 상태 표시
- 필요하면 매우 짧은 말풍선

### Level B — 의미 있는 사회행동
예: 위로, 도움 요청, 감사, 사과, 칭찬, 비판, 정보 공유, 중요한 부탁.

표현:
- 명확한 상대 지정
- 적절한 표정/몸짓/거리
- 짧은 한국어 말풍선
- 관계/감정 변화가 있으면 Observer detail/history에 기록

### Level C — 중요 사건
예: 큰 말다툼, 화해, 호감 표현, 고백, 거절, 이별, 청혼, 결혼 관련 대화, 중대한 경고/폭로.

표현:
- 실제 대화/반응이 관찰 가능해야 함
- 짧은 한국어 대사 교환
- Event Feed 기록
- Importance에 따라 중요 사건 알림 및 Event Camera 후보
- Relationship/Life History에 적절히 기록

## 5. 대화 생성 원칙

초기 구현은 유료 생성형 AI API에 의존하지 않는다.

대사는 아래 authoritative context를 조합해 **비용 0의 deterministic/data-driven 방식**으로 생성할 수 있다.
- intent / social action
- actor personality
- target relationship
- current emotion
- relevant memory / belief
- success/rejection/outcome
- situational context

같은 `Comfort`라도 성격과 관계에 따라 표현이 달라질 수 있다.

예:
- 다정한 성향: `괜찮아? 내가 옆에 있을게.`
- 무뚝뚝한 성향: `힘들면 좀 쉬어.`
- 어색한 관계: `...괜찮아?`

문장은 Presentation 콘텐츠이지만, **사건의 의미와 결과는 Core authority**다.

## 6. 최소 사회행동 taxonomy 방향

사회행동은 단일 `Chat` 하나로 뭉개지 않는다. 최소 다음 범주로 확장 가능해야 한다.

- Greeting
- SmallTalk
- AskQuestion
- ShareInformation
- Request
- OfferHelp
- Comfort
- Thank
- Apologize
- Joke
- Compliment
- Criticize
- Argue
- Reconcile
- Flirt
- ExpressAffection
- Confess
- Propose
- Teach
- Warn

실제 도입 순서는 milestone별로 나눌 수 있지만, 데이터/인터페이스가 향후 taxonomy 확장을 막지 않아야 한다.

## 7. Observer UI 요구사항

- 기본 월드 화면은 계속 깨끗하게 유지한다.
- 모든 주민의 대화를 상시 텍스트로 도배하지 않는다.
- 캐릭터 선택 시 최근 사회행동/대화 이력을 확인할 수 있어야 한다.
- Relationship 상세에서 해당 관계의 최근 상호작용을 확인할 수 있어야 한다.
- Event Feed는 의미 있는 사회사건을 한국어로 표시한다.

예:
- `태윤이 서준을 위로했습니다.`
- `하윤과 나은의 친밀도가 높아졌습니다.`
- `서준이 태윤에게 사과했습니다.`
- `나은이 하윤의 고백을 거절했습니다.`

## 8. Animation / spatial acceptance

사회행동 표현 중 다음을 피한다.
- 서로 등을 돌린 채 대화
- 지나치게 먼 거리에서 대화
- 캐릭터 겹침
- 이동 중 대화 animation이 미끄러짐
- 동일 talking animation만 무한 반복
- Core target과 다른 캐릭터를 바라봄

사회행동 시작/유지/종료 시 이동, facing, gaze, personal space, animation transition이 일관되어야 한다.

## 9. Acceptance Criteria

다음이 충족되어야 이 요구사항을 완료로 본다.

1. 일반 사용자용 Observer UI의 주요 문구가 한국어 기본 표시이며 raw action enum이 직접 노출되지 않는다.
2. 두 주민 사이의 실제 Core social event가 월드에서 시각적으로 식별 가능하다.
3. 적어도 일상/의미 있음/중요 사건의 3단계 presentation 정책이 작동한다.
4. 중요한 사회행동에는 상황에 맞는 짧은 한국어 대사가 표시된다.
5. 대사는 actor personality / relationship / emotion / context에 따라 변형될 수 있다.
6. Presentation이 존재하지 않는 사회사건이나 관계 변화를 꾸며내지 않는다.
7. 최근 사회행동이 Character/Relationship detail 또는 Event history에서 확인 가능하다.
8. Android에서 말풍선/UI가 월드 관찰을 과도하게 가리지 않는다.
9. 유료 LLM/API 없이 기본 기능이 완전하게 작동한다.

## 10. Ownership boundary

- **Jjun/Core lane:** social action/event authority, outcome/read contract, deterministic context needed by presentation.
- **Dagyeom presentation lane:** Korean UI text/localization presentation, speech bubble/event feed presentation, gaze/body/animation expression.
- shared contract 변경이 필요할 때만 Integration Request를 연다.

이 문서는 `docs/LIFELENS_SPEC_v1.1.md`의 대화/관계/Observer-first 요구사항을 구체화하는 companion contract다.
