#include "UI/LLObservationSubsystem.h"

void ULLObservationSubsystem::ObserveResident(FGuid ResidentId)
{
    if (!ResidentId.IsValid())
    {
        ClearObservedResident();
        return;
    }

    if (ObservedResidentId == ResidentId)
    {
        return;
    }

    ObservedResidentId = ResidentId;
    OnObservedResidentChanged.Broadcast(ObservedResidentId);
    SetLevel(ELLObservationLevel::Quick);
}

void ULLObservationSubsystem::ObserveHistoricalResident(FGuid ResidentId)
{
    if (!ResidentId.IsValid())
    {
        ClearObservedResident();
        return;
    }

    if (ObservedResidentId != ResidentId)
    {
        ObservedResidentId = ResidentId;
        OnObservedResidentChanged.Broadcast(ObservedResidentId);
    }

    // Historical/genealogy cards retain the stable identity, but physical
    // resident Quick/Detail is only meaningful for a living projected actor.
    SetLevel(ELLObservationLevel::World);
}

void ULLObservationSubsystem::ClearObservedResident()
{
    if (!ObservedResidentId.IsValid())
    {
        SetLevel(ELLObservationLevel::World);
        return;
    }

    ObservedResidentId.Invalidate();
    OnObservedResidentChanged.Broadcast(ObservedResidentId);
    SetLevel(ELLObservationLevel::World);
}

void ULLObservationSubsystem::OpenDetail()
{
    if (!ObservedResidentId.IsValid())
    {
        return;
    }
    SetLevel(ELLObservationLevel::Detail);
}

void ULLObservationSubsystem::CloseDetail()
{
    if (Level == ELLObservationLevel::Detail)
    {
        SetLevel(ELLObservationLevel::Quick);
    }
}

void ULLObservationSubsystem::StepBack()
{
    switch (Level)
    {
        case ELLObservationLevel::Detail:
            SetLevel(ELLObservationLevel::Quick);
            break;
        case ELLObservationLevel::Quick:
            ClearObservedResident();
            break;
        case ELLObservationLevel::World:
            // LEVEL 0 may still retain a historical resident identity for the
            // lifecycle card. Empty-space back clears that history selection.
            if (ObservedResidentId.IsValid())
            {
                ClearObservedResident();
            }
            break;
        default:
            break;
    }
}

void ULLObservationSubsystem::SetLevel(ELLObservationLevel NewLevel)
{
    if (Level == NewLevel)
    {
        return;
    }
    Level = NewLevel;
    OnObservationLevelChanged.Broadcast(Level);
}
