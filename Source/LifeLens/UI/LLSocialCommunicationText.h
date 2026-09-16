#pragma once

#include "CoreMinimal.h"
#include "Simulation/LLCoreReadTypes.h"

namespace LLSocialCommunicationText
{
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

    inline FString SpeechLine(
        const FLLCoreSocialEventObservation& Event,
        const FLLCoreResidentObservation* Actor)
    {
        const float Empathy = Actor ? Actor->Personality.Empathy : 0.5f;
        const float Agreeableness = Actor ? Actor->Personality.Agreeableness : 0.5f;
        const float Sociability = Actor ? Actor->Personality.Sociability : 0.5f;
        const float Anger = Actor ? Actor->Emotion.Anger : 0.0f;

        switch (Event.Type)
        {
            case ELLCoreSocialEventType::PositiveInteraction:
                return Sociability >= 0.65f ? TEXT("요즘 어때?") : TEXT("잘 지냈어?");
            case ELLCoreSocialEventType::Help:
                return Empathy >= 0.65f ? TEXT("내가 도와줄게.") : TEXT("도움 필요해?");
            case ELLCoreSocialEventType::Comfort:
                if (Empathy >= 0.70f) return TEXT("괜찮아? 내가 옆에 있을게.");
                if (Agreeableness < 0.40f) return TEXT("힘들면 좀 쉬어.");
                return TEXT("...괜찮아?");
            case ELLCoreSocialEventType::Conflict:
                return Anger >= 0.55f ? TEXT("그건 정말 아니잖아.") : TEXT("그건 아니지.");
            case ELLCoreSocialEventType::Betrayal:
                return TEXT("정말 실망했어.");
            case ELLCoreSocialEventType::Rejection:
                return TEXT("미안해. 같은 마음은 아니야.");
            case ELLCoreSocialEventType::Apology:
                return Agreeableness >= 0.60f ? TEXT("미안해. 내가 잘못했어.") : TEXT("미안해.");
            case ELLCoreSocialEventType::Intimacy:
                return TEXT("너랑 있으면 편해.");
            case ELLCoreSocialEventType::Commitment:
                return TEXT("우리 앞으로도 함께하자.");
        }
        return TEXT("...");
    }

    inline bool ShouldAppearInEventFeed(ELLCoreSocialPresentationLevel Level)
    {
        return Level != ELLCoreSocialPresentationLevel::Everyday;
    }
}
