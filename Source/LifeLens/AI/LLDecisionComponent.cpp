#include "AI/LLDecisionComponent.h"

ULLDecisionComponent::ULLDecisionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FLLDecisionResult ULLDecisionComponent::ChooseAction(const FLLResidentData& Resident) const
{
    FLLDecisionResult Best;
    Best.Intent = ELLActionIntent::Idle;
    Best.Score = 8.0f;

    const auto Consider = [&Best](ELLActionIntent Intent, float Score)
    {
        if (Score > Best.Score)
        {
            Best.Intent = Intent;
            Best.Score = Score;
        }
    };

    Consider(ELLActionIntent::Eat, Deficit(Resident.Needs.Hunger) * 1.25f);
    Consider(ELLActionIntent::Sleep, Deficit(Resident.Needs.Energy) * 1.35f);
    Consider(ELLActionIntent::Toilet, Deficit(Resident.Needs.Bladder) * 1.65f);
    Consider(ELLActionIntent::Hygiene, Deficit(Resident.Needs.Hygiene) * 1.05f);

    const float SocialWeight = FMath::Lerp(0.75f, 1.35f, Resident.Personality.Extraversion / 100.0f);
    Consider(ELLActionIntent::Socialize, Deficit(Resident.Needs.Social) * SocialWeight);

    const float FunWeight = FMath::Lerp(0.85f, 1.20f, Resident.Personality.Openness / 100.0f);
    Consider(ELLActionIntent::HaveFun, Deficit(Resident.Needs.Fun) * FunWeight);

    return Best;
}

float ULLDecisionComponent::Deficit(float NeedValue)
{
    return 100.0f - FMath::Clamp(NeedValue, 0.0f, 100.0f);
}
