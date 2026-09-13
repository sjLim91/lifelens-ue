#include "UI/LLObservationSubsystem.h"

void ULLObservationSubsystem::ObserveResident(FGuid ResidentId)
{
    if (ObservedResidentId == ResidentId)
    {
        return;
    }

    ObservedResidentId = ResidentId;
    OnObservedResidentChanged.Broadcast(ObservedResidentId);
}

void ULLObservationSubsystem::ClearObservedResident()
{
    if (!ObservedResidentId.IsValid())
    {
        return;
    }

    ObservedResidentId.Invalidate();
    OnObservedResidentChanged.Broadcast(ObservedResidentId);
}
