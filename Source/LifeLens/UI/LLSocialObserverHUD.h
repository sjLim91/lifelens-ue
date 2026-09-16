#pragma once

#include "UI/LLObserverHUD.h"
#include "Simulation/LLCoreReadTypes.h"
#include "LLSocialObserverHUD.generated.h"

struct FLLResidentData;

/**
 * Social Communication v1 presentation layer.
 *
 * The base observer HUD remains responsible for normal world/resident chrome.
 * This layer only presents authoritative typed social events supplied by Core:
 * short world-space speech bubbles and a compact meaningful/important event feed.
 */
UCLASS()
class LIFELENS_API ALLSocialObserverHUD : public ALLObserverHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    virtual FString CurrentActionFor(const FLLResidentData& Resident) const override;

    void DrawSocialOverlays(
        const TArray<FLLCoreSocialEventObservation>& Events,
        int64 CurrentSimulationMinute);
    void DrawSpeechBubbles(
        const TArray<FLLCoreSocialEventObservation>& Events,
        int64 CurrentSimulationMinute);
    void DrawEventFeed(
        const TArray<FLLCoreSocialEventObservation>& Events,
        int64 CurrentSimulationMinute);
};
