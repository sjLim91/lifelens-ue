#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLCoreReadTypes.h"

namespace LLSocialCommunicationText
{
    enum class ESocialVoiceStyle : uint8
    {
        Warm,
        Reserved,
        Direct,
        Lively,
        Calm
    };

    inline FString EventLabel(ELLCoreSocialEventType Type)
    {
        switch (Type)
        {
            case ELLCoreSocialEventType::PositiveInteraction: return TEXT("대화");
            case ELLCoreSocialEventType::Help:                return TEXT("도움");
            case ELLCoreSocialEventType::Comfort:             return TEXT("위로");
            case ELLCoreSocialEventType::Conflict:            return TEXT("말다툼");
            case ELLCoreSocialEventType::Betrayal:            return TEXT("배신");
            case ELLCoreSocialEventType::Rejection:           return TEXT("거절");
            case ELLCoreSocialEventType::Apology:             return TEXT("사과");
            case ELLCoreSocialEventType::Intimacy:            return TEXT("친밀감 표현");
            case ELLCoreSocialEventType::Commitment:          return TEXT("관계 약속");
        }
        return TEXT("사회적 상호작용");
    }

    inline FString ActivityLabel(const FString& Code)
    {
        if (Code == TEXT("Eat"))       return TEXT("식사 중");
        if (Code == TEXT("Drink"))     return TEXT("물 마시는 중");
        if (Code == TEXT("Sleep"))     return TEXT("수면 중");
        if (Code == TEXT("UseToilet")) return TEXT("용변 보는 중");
        if (Code == TEXT("Wash"))      return TEXT("씻는 중");
        if (Code == TEXT("Idle"))      return TEXT("대기 중");
        if (Code == TEXT("Approach"))  return TEXT("다가가 대화하는 중");
        if (Code == TEXT("Avoid"))     return TEXT("거리를 두는 중");
        if (Code == TEXT("Repair"))    return TEXT("관계를 회복하려는 중");
        if (Code == TEXT("Comfort"))   return TEXT("위로하는 중");
        return TEXT("활동 중");
    }

    inline FString FeedLine(const FLLCoreSocialEventObservation& Event)
    {
        const FString Actor = Event.ActorName.IsEmpty() ? TEXT("주민") : Event.ActorName;
        const FString Target = Event.TargetName.IsEmpty() ? TEXT("상대") : Event.TargetName;
        return FString::Printf(TEXT("%s → %s · %s"), *Actor, *Target, *EventLabel(Event.Type));
    }

    inline const FLLCoreRelationshipSnapshot* FindRelationship(
        const FLLCoreResidentObservation* Actor,
        const FGuid& TargetResidentId)
    {
        if (!Actor || !TargetResidentId.IsValid())
        {
            return nullptr;
        }
        for (const FLLCoreRelationshipSnapshot& Relationship : Actor->Relationships)
        {
            if (Relationship.TargetResidentId == TargetResidentId)
            {
                return &Relationship;
            }
        }
        return nullptr;
    }

    inline ESocialVoiceStyle VoiceStyle(const FLLCoreResidentObservation* Actor)
    {
        if (!Actor)
        {
            return ESocialVoiceStyle::Calm;
        }

        const FLLCorePersonalitySnapshot& P = Actor->Personality;
        if (P.Empathy >= 0.70f && P.Agreeableness >= 0.60f)
        {
            return ESocialVoiceStyle::Warm;
        }
        if (P.Introversion >= 0.68f || P.Sociability <= 0.32f)
        {
            return ESocialVoiceStyle::Reserved;
        }
        if (P.Agreeableness <= 0.38f || P.Impulsiveness >= 0.72f)
        {
            return ESocialVoiceStyle::Direct;
        }
        if (P.Sociability >= 0.70f && P.Introversion <= 0.38f)
        {
            return ESocialVoiceStyle::Lively;
        }
        return ESocialVoiceStyle::Calm;
    }

    inline uint32 StableVariantKey(const FLLCoreSocialEventObservation& Event)
    {
        const uint64 Sequence = static_cast<uint64>(Event.Sequence);
        uint32 Key = static_cast<uint32>(Sequence) ^ static_cast<uint32>(Sequence >> 32);
        Key ^= Event.ActorResidentId.A * 0x9E3779B9u;
        Key ^= Event.ActorResidentId.B * 0x85EBCA6Bu;
        Key ^= Event.TargetResidentId.C * 0xC2B2AE35u;
        Key ^= Event.TargetResidentId.D * 0x27D4EB2Fu;
        Key ^= static_cast<uint32>(Event.Type) * 0x165667B1u;
        Key ^= static_cast<uint32>(Event.SimulationMinute) * 0xD3A2646Cu;
        return Key;
    }

    template <SIZE_T N>
    inline FString PickLine(
        const FLLCoreSocialEventObservation& Event,
        const TCHAR* const (&Lines)[N])
    {
        static_assert(N > 0, "Social dialogue pool must not be empty");
        return FString(Lines[StableVariantKey(Event) % N]);
    }

    inline FString PositiveInteractionLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const bool bTense = R && (R->Conflict >= 0.45f || R->Grudge >= 0.38f || R->Trust <= 0.25f);
        const bool bRomantic = R && (R->RomanticInterest >= 0.60f || R->Commitment >= 0.60f || R->Attraction >= 0.70f);
        const bool bClose = R && (R->SocialBond >= 0.68f || R->Affection >= 0.72f || R->Familiarity >= 0.78f);
        const bool bUnfamiliar = !R || R->Familiarity < 0.25f;

        if (bTense)
        {
            static const TCHAR* const Lines[] = {
                TEXT("우리 계속 이렇게 불편하게 지내진 말자."),
                TEXT("아까 일은 잠깐 내려놓고 얘기해볼래?"),
                TEXT("조금은 편하게 얘기할 수 있으면 좋겠어."),
                TEXT("서로 오해한 게 있는지 한번 얘기해보자."),
                TEXT("싸우자는 건 아니야. 그냥 얘기하고 싶었어.")
            };
            return PickLine(Event, Lines);
        }
        if (bRomantic)
        {
            static const TCHAR* const Lines[] = {
                TEXT("너랑 얘기하면 시간이 금방 간다."),
                TEXT("오늘도 네 얼굴 보니까 좋네."),
                TEXT("이런 얘기는 이상하게 너한테 제일 먼저 하게 돼."),
                TEXT("너랑 있으면 별 얘기 안 해도 편해."),
                TEXT("요즘 자꾸 네 생각이 나더라."),
                TEXT("오늘은 좀 오래 같이 있고 싶다."),
                TEXT("네가 있으면 분위기가 좀 달라져."),
                TEXT("그냥 네 옆에 있으니까 좋네.")
            };
            return PickLine(Event, Lines);
        }
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("오늘은 별일 없었어?"),
                TEXT("아까 그거 봤어? 진짜 웃기더라."),
                TEXT("너한테 얘기할 게 하나 있었어."),
                TEXT("요즘 어때? 얼굴 자주 못 봤네."),
                TEXT("너는 이런 상황이면 어떻게 할 것 같아?"),
                TEXT("오늘도 잘 버티고 있네."),
                TEXT("잠깐 얘기 좀 하자. 별건 아니고."),
                TEXT("그래도 네가 있어서 심심하진 않다.")
            };
            return PickLine(Event, Lines);
        }
        if (bUnfamiliar)
        {
            static const TCHAR* const Lines[] = {
                TEXT("우리 아직 제대로 얘기해본 적 없지?"),
                TEXT("안녕. 요즘 여기 생활은 좀 어때?"),
                TEXT("아직 서로 잘 모르니까 천천히 알아가자."),
                TEXT("혹시 불편한 건 없어?"),
                TEXT("나는 네가 어떤 사람인지 좀 궁금했어."),
                TEXT("같이 지내게 됐으니 인사는 제대로 해야지."),
                TEXT("오늘 하루는 어땠어?")
            };
            return PickLine(Event, Lines);
        }

        switch (Style)
        {
            case ESocialVoiceStyle::Warm:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("오늘 기분은 어때?"), TEXT("잘 지냈어?"), TEXT("무슨 일 있으면 말해도 돼."),
                    TEXT("오늘은 좀 편해 보여서 다행이다."), TEXT("같이 얘기하니까 좋네."), TEXT("오늘 하루도 고생했어.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Reserved:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("...잘 지냈어?"), TEXT("그냥, 잠깐 얘기하고 싶었어."), TEXT("별일 없지?"),
                    TEXT("오늘은 어땠어?"), TEXT("음... 요즘 괜찮아?"), TEXT("잠깐 같이 있어도 될까?")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Direct:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("요즘 뭐 하고 지내?"), TEXT("별일 없지?"), TEXT("잠깐 얘기 좀 하자."),
                    TEXT("오늘 상태는 괜찮아 보여."), TEXT("뭐 필요한 건 없어?"), TEXT("너 생각은 어때?")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Lively:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("야, 오늘 재밌는 일 없었어?"), TEXT("요즘 어때? 뭐 재밌는 거 없어?"), TEXT("잠깐 얘기하자!"),
                    TEXT("오늘 분위기 괜찮은데?"), TEXT("너 아까 뭐 하고 있었어?"), TEXT("심심한데 같이 얘기 좀 하자.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Calm:
            default:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("요즘은 좀 어때?"), TEXT("오늘 하루는 괜찮았어?"), TEXT("잠깐 이야기할까?"),
                    TEXT("잘 지내고 있지?"), TEXT("무슨 생각 하고 있었어?"), TEXT("잠깐 쉬면서 얘기하자.")
                };
                return PickLine(Event, Lines);
            }
        }
    }

    inline FString HelpLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const bool bClose = R && (R->SocialBond >= 0.65f || R->Affection >= 0.70f);
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("그걸 혼자 하고 있었어? 같이 하자."),
                TEXT("됐어, 내가 도와줄게."),
                TEXT("힘들면 바로 말하지 그랬어."),
                TEXT("우리 사이에 이런 걸 왜 혼자 해."),
                TEXT("내가 옆에 있는데 혼자 버티지 마."),
                TEXT("여기까진 내가 할게. 넌 잠깐 쉬어.")
            };
            return PickLine(Event, Lines);
        }

        switch (Style)
        {
            case ESocialVoiceStyle::Warm:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("내가 도와줄게."), TEXT("괜찮아, 같이 하면 금방 끝나."), TEXT("힘들어 보이는데 내가 좀 할까?"),
                    TEXT("혼자 무리하지 마. 같이 하자."), TEXT("필요한 거 있으면 말해."), TEXT("잠깐만, 내가 같이 할게.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Reserved:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("...도움 필요해?"), TEXT("내가 조금 도와도 돼."), TEXT("혼자 하기 힘들면 말해."),
                    TEXT("이쪽은 내가 할게."), TEXT("필요하면 같이 할 수 있어.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Direct:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("비켜봐. 내가 할게."), TEXT("그건 둘이 하는 게 빠르겠다."), TEXT("혼자 붙잡고 있지 말고 나눠."),
                    TEXT("여기 내가 맡을게."), TEXT("필요한 게 뭔지만 말해.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Lively:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("같이 하자! 금방 끝내버리자."), TEXT("오, 이거 내가 도와줄게."), TEXT("혼자 하지 말고 나도 끼워줘."),
                    TEXT("자, 뭐부터 하면 돼?"), TEXT("내가 왔으니까 좀 편해지겠네.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Calm:
            default:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("도움 필요해?"), TEXT("내가 한쪽 맡을게."), TEXT("천천히 하자. 내가 같이 할게."),
                    TEXT("혼자 하기엔 벅차 보여."), TEXT("같이 하면 조금 수월할 거야.")
                };
                return PickLine(Event, Lines);
            }
        }
    }

    inline FString ComfortLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const float Sadness = Actor ? Actor->Emotion.Sadness : 0.0f;
        const float Anxiety = Actor ? Actor->Emotion.Anxiety : 0.0f;
        const float Grief = Actor ? Actor->Emotion.Grief : 0.0f;
        const bool bClose = R && (R->SocialBond >= 0.65f || R->Affection >= 0.70f || R->Comfort >= 0.72f);

        if (Grief >= 0.55f || Event.Intensity >= 0.80f)
        {
            static const TCHAR* const Lines[] = {
                TEXT("지금은 아무 말 안 해도 돼. 그냥 옆에 있을게."),
                TEXT("많이 힘들지. 혼자 감당하려고 하지 마."),
                TEXT("괜찮은 척 안 해도 돼."),
                TEXT("시간이 좀 걸려도 괜찮아. 천천히 가자."),
                TEXT("지금 네가 힘든 거 알아. 여기 있을게."),
                TEXT("말하고 싶을 때 말해. 그때까지 옆에 있을게.")
            };
            return PickLine(Event, Lines);
        }
        if (Anxiety >= 0.55f || Sadness >= 0.55f)
        {
            static const TCHAR* const Lines[] = {
                TEXT("괜찮아. 하나씩 생각해보자."),
                TEXT("지금 당장 다 해결하지 않아도 돼."),
                TEXT("숨 좀 돌려. 내가 여기 있어."),
                TEXT("너무 앞서 걱정하지 말자. 같이 보자."),
                TEXT("힘들면 잠깐 멈춰도 괜찮아."),
                TEXT("천천히 말해. 듣고 있을게.")
            };
            return PickLine(Event, Lines);
        }
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("네 표정 보면 다 티 나. 무슨 일 있어?"),
                TEXT("나한테는 괜찮은 척 안 해도 돼."),
                TEXT("오늘은 내가 옆에 있을게."),
                TEXT("힘든 거 있으면 나눠. 혼자 들고 있지 말고."),
                TEXT("말 안 해도 돼. 그냥 같이 있자."),
                TEXT("너 힘들 때는 내가 챙길게.")
            };
            return PickLine(Event, Lines);
        }

        switch (Style)
        {
            case ESocialVoiceStyle::Warm:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("괜찮아? 내가 옆에 있을게."), TEXT("많이 힘들었어?"), TEXT("천천히 얘기해도 돼."),
                    TEXT("혼자 참지 않아도 돼."), TEXT("네 마음 이해해보려고 할게."), TEXT("조금이라도 편해졌으면 좋겠다.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Reserved:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("...괜찮아?"), TEXT("내가 잘 말은 못 하는데, 옆에 있을게."), TEXT("힘들면... 말해."),
                    TEXT("그냥 혼자 있지 않았으면 해서."), TEXT("잠깐 같이 있어도 될까?")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Direct:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("힘들면 좀 쉬어."), TEXT("혼자 끙끙대지 마."), TEXT("지금 상태 안 좋아 보여."),
                    TEXT("참는다고 해결되는 건 아니잖아."), TEXT("일단 쉬고 다시 생각해.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Lively:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("야, 너무 혼자 처져 있지 마. 나 있잖아."), TEXT("잠깐 바람이라도 쐬자."), TEXT("기분 좀 풀릴 때까지 같이 있자."),
                    TEXT("내가 괜히 웃겨줄까?"), TEXT("오늘만큼은 내가 신경 써줄게.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Calm:
            default:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("괜찮아?"), TEXT("무슨 일인지 천천히 말해봐."), TEXT("조금 쉬었다가 생각하자."),
                    TEXT("지금은 마음부터 가라앉히자."), TEXT("혼자 감당할 필요는 없어.")
                };
                return PickLine(Event, Lines);
            }
        }
    }

    inline FString ConflictLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const float Anger = Actor ? Actor->Emotion.Anger : 0.0f;
        const bool bGrudge = R && R->Grudge >= 0.48f;
        const bool bClose = R && (R->SocialBond >= 0.62f || R->Affection >= 0.68f || R->Commitment >= 0.60f);
        const bool bIntense = Anger >= 0.62f || Event.Intensity >= 0.72f;

        if (bGrudge)
        {
            static const TCHAR* const Lines[] = {
                TEXT("또 이 얘기야? 난 아직 안 잊었어."),
                TEXT("지금 와서 아무 일 없던 것처럼 말하지 마."),
                TEXT("그때 일부터 제대로 얘기해."),
                TEXT("내가 왜 화났는지 정말 모르겠어?"),
                TEXT("이건 한 번으로 끝날 문제가 아니야."),
                TEXT("난 아직 네 말을 그대로 믿을 수 없어.")
            };
            return PickLine(Event, Lines);
        }
        if (bIntense)
        {
            static const TCHAR* const Lines[] = {
                TEXT("그건 정말 아니잖아."),
                TEXT("그만 좀 해. 나도 참을 만큼 참았어."),
                TEXT("지금 그 말을 진심으로 하는 거야?"),
                TEXT("왜 자꾸 사람 화나게 만들어?"),
                TEXT("내 말은 하나도 안 듣고 있잖아."),
                TEXT("됐어. 지금은 더 얘기하면 싸우기만 하겠다."),
                TEXT("그렇게 말하면 나도 가만히 못 있어."),
                TEXT("선을 넘었어. 그건 아니야.")
            };
            return PickLine(Event, Lines);
        }
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("다른 사람도 아니고 네가 그러니까 더 화나는 거야."),
                TEXT("우리가 이 정도로 싸울 일은 아니잖아."),
                TEXT("네가 왜 그런 말을 하는지 모르겠어."),
                TEXT("우리 사이에 이러지 말자."),
                TEXT("나한테는 솔직하게 말해도 되잖아."),
                TEXT("화난 건 맞는데, 관계까지 망치고 싶진 않아.")
            };
            return PickLine(Event, Lines);
        }
        if (Style == ESocialVoiceStyle::Direct)
        {
            static const TCHAR* const Lines[] = {
                TEXT("그건 아니지."), TEXT("그 말은 납득 못 하겠어."), TEXT("틀린 건 틀렸다고 말할게."),
                TEXT("그렇게 넘어갈 생각은 하지 마."), TEXT("난 그 의견에 동의 못 해."), TEXT("말 돌리지 말고 본론부터 얘기해.")
            };
            return PickLine(Event, Lines);
        }

        static const TCHAR* const Lines[] = {
            TEXT("우리 생각이 많이 다른 것 같네."),
            TEXT("잠깐, 그건 좀 아닌 것 같아."),
            TEXT("나도 내 입장은 말해야겠어."),
            TEXT("서로 말 끊지 말고 얘기하자."),
            TEXT("지금은 조금 감정적인 것 같아."),
            TEXT("싸우려는 건 아닌데, 그 부분은 분명히 하고 싶어."),
            TEXT("나는 그렇게 생각하지 않아.")
        };
        return PickLine(Event, Lines);
    }

    inline FString BetrayalLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R)
    {
        const bool bRomantic = R && (R->RomanticInterest >= 0.55f || R->Commitment >= 0.55f || R->Attraction >= 0.65f);
        const bool bDeepTrust = R && R->Trust >= 0.65f;
        const bool bHighGrudge = R && R->Grudge >= 0.50f;

        if (bRomantic)
        {
            static const TCHAR* const Lines[] = {
                TEXT("적어도 너는 나한테 이러면 안 되는 거였잖아."),
                TEXT("내가 널 얼마나 믿었는데."),
                TEXT("우리 사이가 너한텐 이 정도였어?"),
                TEXT("다른 누구보다 네가 그래서 더 아파."),
                TEXT("이제 네 말을 어떻게 믿어."),
                TEXT("나는 우리 관계를 진짜라고 생각했어."),
                TEXT("솔직히 말해줬으면 이렇게까지는 안 됐어.")
            };
            return PickLine(Event, Lines);
        }
        if (bDeepTrust || bHighGrudge)
        {
            static const TCHAR* const Lines[] = {
                TEXT("정말 실망했어."), TEXT("너라서 믿었는데."), TEXT("이건 쉽게 넘어갈 수 없을 것 같아."),
                TEXT("내가 널 잘못 봤나 봐."), TEXT("앞으로는 예전처럼 믿기 어려울 것 같아."), TEXT("왜 나한테 그런 선택을 한 거야?")
            };
            return PickLine(Event, Lines);
        }

        static const TCHAR* const Lines[] = {
            TEXT("그렇게까지 할 줄은 몰랐어."), TEXT("이건 너무하잖아."), TEXT("설명은 듣고 싶어."),
            TEXT("지금은 네 말을 바로 믿기 어렵다."), TEXT("왜 그랬는지는 말해줘."), TEXT("한동안은 거리를 두고 싶어.")
        };
        return PickLine(Event, Lines);
    }

    inline FString RejectionLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        ESocialVoiceStyle Style)
    {
        switch (Style)
        {
            case ESocialVoiceStyle::Warm:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("미안해. 네 마음을 가볍게 생각하는 건 아니야."),
                    TEXT("고맙지만 나는 같은 마음은 아닌 것 같아."),
                    TEXT("상처 주고 싶진 않은데 솔직하게 말하는 게 맞는 것 같아."),
                    TEXT("네가 싫은 건 아니야. 다만 그 마음에 답해주긴 어려워."),
                    TEXT("말해줘서 고마워. 그런데 나는 조금 달라."),
                    TEXT("미안해. 괜히 기대하게 만들고 싶진 않아.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Reserved:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("...미안해. 나는 그런 마음은 아니야."), TEXT("고마워. 그런데 나는 어렵겠다."),
                    TEXT("미안해. 어떻게 말해야 할지 모르겠는데... 아니야."), TEXT("네 마음은 알겠어. 하지만 나는 아니야."),
                    TEXT("괜히 오해하게 했다면 미안해.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Direct:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("미안하지만 난 같은 마음 아니야."), TEXT("기대하게 만들고 싶진 않아. 아니야."),
                    TEXT("솔직히 말할게. 나는 그렇게 생각하지 않아."), TEXT("우리 관계는 지금 그대로가 좋아."),
                    TEXT("애매하게 말하는 것보다 확실히 말할게.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Lively:
            case ESocialVoiceStyle::Calm:
            default:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("미안해. 같은 마음은 아니야."), TEXT("네 마음은 고마운데 나는 다르게 생각해."),
                    TEXT("우리 사이가 어색해지진 않았으면 좋겠어."), TEXT("고마워. 하지만 그 마음을 받아주긴 어려워."),
                    TEXT("솔직하게 말해줘서 고마워. 나도 솔직하게 말할게.")
                };
                return PickLine(Event, Lines);
            }
        }
    }

    inline FString ApologyLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const bool bSerious = Event.Importance >= 0.65f || (R && (R->Conflict >= 0.50f || R->Grudge >= 0.40f));
        if (bSerious)
        {
            static const TCHAR* const Lines[] = {
                TEXT("미안해. 변명하지 않을게. 내가 잘못했어."),
                TEXT("내가 선을 넘었어. 정말 미안해."),
                TEXT("네가 왜 화났는지 이제 알 것 같아. 미안해."),
                TEXT("한 번에 풀리지 않아도 이해해. 그래도 사과하고 싶었어."),
                TEXT("내 행동 때문에 상처받은 거 알아. 미안해."),
                TEXT("내가 잘못한 부분은 분명해. 다시 그러지 않을게."),
                TEXT("용서해달라고 재촉하진 않을게. 그냥 미안하다고 말하고 싶었어.")
            };
            return PickLine(Event, Lines);
        }

        switch (Style)
        {
            case ESocialVoiceStyle::Warm:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("미안해. 내가 조금 더 생각했어야 했어."), TEXT("기분 상하게 했다면 정말 미안해."),
                    TEXT("내가 잘못했어. 괜찮다면 다시 얘기하고 싶어."), TEXT("미안해. 네 입장도 생각했어야 했는데."),
                    TEXT("내 말이 너무 앞섰어. 미안해."), TEXT("마음 상하게 하려던 건 아니었어. 미안해.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Reserved:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("...미안해."), TEXT("아까는 내가 좀 심했어."), TEXT("말을 잘못했어. 미안해."),
                    TEXT("계속 마음에 걸려서... 사과하고 싶었어."), TEXT("내가 먼저 말했어야 했는데. 미안해.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Direct:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("내가 잘못했다. 미안해."), TEXT("아까 건 내 실수야."), TEXT("그 부분은 내가 틀렸어."),
                    TEXT("변명 안 할게. 미안."), TEXT("내가 너무 세게 말했어.")
                };
                return PickLine(Event, Lines);
            }
            case ESocialVoiceStyle::Lively:
            case ESocialVoiceStyle::Calm:
            default:
            {
                static const TCHAR* const Lines[] = {
                    TEXT("미안해. 아까는 내가 잘못했어."), TEXT("우리 이대로 불편하게 있지 말자. 미안해."),
                    TEXT("생각해보니까 내가 좀 심했더라."), TEXT("내가 먼저 사과할게."), TEXT("아까 일은 미안해.")
                };
                return PickLine(Event, Lines);
            }
        }
    }

    inline FString IntimacyLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const bool bRomantic = R && (R->RomanticInterest >= 0.55f || R->Commitment >= 0.50f || R->Attraction >= 0.65f);
        const bool bClose = R && (R->SocialBond >= 0.68f || R->Comfort >= 0.72f || R->Affection >= 0.72f);

        if (bRomantic)
        {
            static const TCHAR* const Lines[] = {
                TEXT("너랑 있으면 마음이 편해."),
                TEXT("이상하게 너한테는 솔직해지고 싶어."),
                TEXT("요즘 내 하루에서 네가 차지하는 게 꽤 커."),
                TEXT("네가 가까이 있으면 좋더라."),
                TEXT("너한테는 좋은 모습만 보이고 싶었는데, 이제는 그냥 나여도 괜찮을 것 같아."),
                TEXT("너랑 같이 있는 시간이 제일 편하다."),
                TEXT("사실 나는 네가 생각하는 것보다 널 더 많이 신경 쓰고 있어."),
                TEXT("네가 있어서 참 다행이라고 생각해."),
                TEXT("이런 말 잘 안 하는데, 너 정말 소중해."),
                TEXT("앞으로도 네 얘기를 제일 가까이서 듣고 싶어.")
            };
            return PickLine(Event, Lines);
        }
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("너한테는 웬만한 얘기 다 할 수 있을 것 같아."),
                TEXT("내 편이 있다는 느낌이 이런 건가 봐."),
                TEXT("우리가 이렇게 가까워질 줄은 몰랐네."),
                TEXT("네 앞에서는 굳이 괜찮은 척 안 해도 돼서 좋아."),
                TEXT("이런 얘기 들어주는 사람이 있어서 다행이다."),
                TEXT("나는 네가 꽤 믿음직해."),
                TEXT("앞으로도 서로 챙기면서 지내자.")
            };
            return PickLine(Event, Lines);
        }
        if (Style == ESocialVoiceStyle::Reserved)
        {
            static const TCHAR* const Lines[] = {
                TEXT("이런 말 잘 안 하는데... 너랑 있으면 편해."), TEXT("너한테는 좀 솔직해도 될 것 같아."),
                TEXT("나는... 네가 꽤 편한 사람인 것 같아."), TEXT("그냥, 네가 옆에 있으면 덜 불편해."),
                TEXT("너한테는 말해도 괜찮을 것 같았어.")
            };
            return PickLine(Event, Lines);
        }

        static const TCHAR* const Lines[] = {
            TEXT("너랑 있으면 편해."), TEXT("요즘 너랑 이야기하는 게 좋더라."), TEXT("조금씩 서로 알아가는 것 같네."),
            TEXT("너는 생각보다 믿을 만한 사람이네."), TEXT("이런 얘기 나누는 거 나쁘지 않다."), TEXT("앞으로 더 자주 얘기하자.")
        };
        return PickLine(Event, Lines);
    }

    inline FString CommitmentLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor,
        const FLLCoreRelationshipSnapshot* R,
        ESocialVoiceStyle Style)
    {
        const bool bRomantic = R && (R->RomanticInterest >= 0.55f || R->Commitment >= 0.55f || R->Attraction >= 0.65f);
        const bool bClose = R && (R->SocialBond >= 0.68f || R->Trust >= 0.72f || R->Affection >= 0.72f);
        const bool bImportant = Event.PresentationLevel == ELLCoreSocialPresentationLevel::Important || Event.Importance >= 0.75f;

        if (bRomantic && bImportant)
        {
            static const TCHAR* const Lines[] = {
                TEXT("앞으로 어떤 일이 있어도 우리 같이 버텨보자."),
                TEXT("나는 앞으로도 네 옆에 있고 싶어."),
                TEXT("좋을 때만이 아니라 힘들 때도 같이 가고 싶어."),
                TEXT("우리 관계를 가볍게 생각하지 않아. 계속 함께하고 싶어."),
                TEXT("네가 내 미래에 있었으면 좋겠어."),
                TEXT("앞으로의 일들을 너랑 같이 만들고 싶어."),
                TEXT("나는 너를 선택할게. 앞으로도."),
                TEXT("우리 둘이 같이 살아갈 방법을 계속 찾아보자."),
                TEXT("너랑 함께라면 다음도 생각해볼 수 있을 것 같아."),
                TEXT("우리, 여기서 끝내지 말고 더 오래 가자.")
            };
            return PickLine(Event, Lines);
        }
        if (bRomantic)
        {
            static const TCHAR* const Lines[] = {
                TEXT("우리 앞으로도 함께하자."), TEXT("나는 이 관계 계속 이어가고 싶어."), TEXT("앞으로도 네 옆에 있을게."),
                TEXT("우리 천천히 오래 가보자."), TEXT("나는 너랑 계속 같이 있고 싶어."), TEXT("서로 힘들 때도 놓지 말자."),
                TEXT("우리 관계, 나는 진지하게 생각하고 있어."), TEXT("앞으로도 서로한테 솔직하자.")
            };
            return PickLine(Event, Lines);
        }
        if (bClose)
        {
            static const TCHAR* const Lines[] = {
                TEXT("앞으로도 서로 믿고 지내자."), TEXT("무슨 일 생기면 서로 먼저 챙겨주자."), TEXT("우리 계속 같은 편이었으면 좋겠다."),
                TEXT("앞으로도 필요한 순간엔 내가 있을게."), TEXT("서로 등 돌리지 말자."), TEXT("여기서 같이 살아가는 동안은 서로 믿자.")
            };
            return PickLine(Event, Lines);
        }
        if (Style == ESocialVoiceStyle::Reserved)
        {
            static const TCHAR* const Lines[] = {
                TEXT("...앞으로도 같이 지냈으면 좋겠어."), TEXT("나는 우리 관계를 계속 이어가고 싶어."),
                TEXT("말로 잘 못 하지만, 네 편이고 싶어."), TEXT("앞으로도 서로 믿을 수 있으면 좋겠다."),
                TEXT("나중에도 지금처럼 얘기할 수 있었으면 좋겠어.")
            };
            return PickLine(Event, Lines);
        }

        static const TCHAR* const Lines[] = {
            TEXT("앞으로도 서로 챙기자."), TEXT("우리 사이 오래 갔으면 좋겠다."), TEXT("필요할 때 서로 믿고 의지하자."),
            TEXT("앞으로도 같이 잘 지내보자."), TEXT("우리 약속한 건 지키자."), TEXT("서로한테 믿을 만한 사람이 되자.")
        };
        return PickLine(Event, Lines);
    }

    inline FString SpeechLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor)
    {
        const FLLCoreRelationshipSnapshot* Relationship = FindRelationship(Actor, Event.TargetResidentId);
        const ESocialVoiceStyle Style = VoiceStyle(Actor);

        switch (Event.Type)
        {
            case ELLCoreSocialEventType::PositiveInteraction:
                return PositiveInteractionLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Help:
                return HelpLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Comfort:
                return ComfortLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Conflict:
                return ConflictLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Betrayal:
                return BetrayalLine(Event, Actor, Relationship);
            case ELLCoreSocialEventType::Rejection:
                return RejectionLine(Event, Actor, Style);
            case ELLCoreSocialEventType::Apology:
                return ApologyLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Intimacy:
                return IntimacyLine(Event, Actor, Relationship, Style);
            case ELLCoreSocialEventType::Commitment:
                return CommitmentLine(Event, Actor, Relationship, Style);
        }
        return TEXT("...");
    }

    inline bool ShouldAppearInEventFeed(ELLCoreSocialPresentationLevel Level)
    {
        return Level != ELLCoreSocialPresentationLevel::Everyday;
    }
}
