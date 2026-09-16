#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/Simulation.h"
#include "lifelens/SocialCommunicationReadModel.h"

namespace
{
ELLCoreSocialEventType ToUnrealSocialEventType(lifelens::SocialEventType Type)
{
    switch (Type)
    {
        case lifelens::SocialEventType::PositiveInteraction: return ELLCoreSocialEventType::PositiveInteraction;
        case lifelens::SocialEventType::Help: return ELLCoreSocialEventType::Help;
        case lifelens::SocialEventType::Comfort: return ELLCoreSocialEventType::Comfort;
        case lifelens::SocialEventType::Conflict: return ELLCoreSocialEventType::Conflict;
        case lifelens::SocialEventType::Betrayal: return ELLCoreSocialEventType::Betrayal;
        case lifelens::SocialEventType::Rejection: return ELLCoreSocialEventType::Rejection;
        case lifelens::SocialEventType::Apology: return ELLCoreSocialEventType::Apology;
        case lifelens::SocialEventType::Intimacy: return ELLCoreSocialEventType::Intimacy;
        case lifelens::SocialEventType::Commitment: return ELLCoreSocialEventType::Commitment;
    }
    return ELLCoreSocialEventType::PositiveInteraction;
}

ELLCoreSocialPresentationLevel ToUnrealPresentationLevel(lifelens::SocialPresentationLevel Level)
{
    switch (Level)
    {
        case lifelens::SocialPresentationLevel::Meaningful:
            return ELLCoreSocialPresentationLevel::Meaningful;
        case lifelens::SocialPresentationLevel::Important:
            return ELLCoreSocialPresentationLevel::Important;
        case lifelens::SocialPresentationLevel::Everyday:
        default:
            return ELLCoreSocialPresentationLevel::Everyday;
    }
}
}

TArray<FLLCoreSocialEventObservation> ULLCoreBridgeSubsystem::GetRecentSocialEvents(int32 MaxEvents) const
{
    TArray<FLLCoreSocialEventObservation> Result;
    if (!CoreSimulation || MaxEvents <= 0)
    {
        return Result;
    }

    const std::vector<lifelens::SocialCommunicationObservation> CoreEvents =
        CoreSimulation->observeRecentSocialEvents(static_cast<std::size_t>(MaxEvents));
    Result.Reserve(CoreEvents.size() > static_cast<std::size_t>(MAX_int32)
        ? MAX_int32
        : static_cast<int32>(CoreEvents.size()));

    for (const lifelens::SocialCommunicationObservation& CoreEvent : CoreEvents)
    {
        FLLCoreSocialEventObservation Event;
        Event.Sequence = static_cast<int64>(CoreEvent.sequence);
        Event.ActorResidentId = MakeStableResidentGuid(static_cast<uint64>(CoreEvent.actor));
        Event.TargetResidentId = MakeStableResidentGuid(static_cast<uint64>(CoreEvent.target));
        Event.Type = ToUnrealSocialEventType(CoreEvent.type);
        Event.PresentationLevel = ToUnrealPresentationLevel(CoreEvent.presentationLevel);
        Event.Intensity = static_cast<float>(CoreEvent.intensity);
        Event.Importance = static_cast<float>(CoreEvent.importance);
        Event.SimulationMinute = CoreEvent.minute;
        Event.Location = UTF8_TO_TCHAR(CoreEvent.where.c_str());
        Event.bSuccessful = CoreEvent.successful;

        if (const lifelens::Character* Actor =
            lifelens::findObservedCharacter(CoreSimulation->world(), CoreEvent.actor))
        {
            Event.ActorName = UTF8_TO_TCHAR(Actor->name.c_str());
        }
        if (const lifelens::Character* Target =
            lifelens::findObservedCharacter(CoreSimulation->world(), CoreEvent.target))
        {
            Event.TargetName = UTF8_TO_TCHAR(Target->name.c_str());
        }

        Result.Add(MoveTemp(Event));
    }

    return Result;
}
