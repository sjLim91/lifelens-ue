#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LLObserverHUD.generated.h"

class ULLSimulationSubsystem;
class ULLObservationSubsystem;
struct FLLResidentData;

// Observer-first HUD (SPEC 53-56, 81).
//
// LEVEL 0: nothing selected. A thin status line plus a faint strip of
//          residents and their current action. No need or relationship values.
// LEVEL 1: one resident selected. A compact quick inspector: name and age,
//          current action, one-line status summary, one line of personality
//          words. Numbers are never shown here.
// LEVEL 2 (full detail) is not implemented yet.
UCLASS()
class LIFELENS_API ALLObserverHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

private:
    // Uniform scale so text stays readable on phones and desktops alike.
    float ComputeUIScale() const;

    // LEVEL 0. Returns the Y just below the drawn overview. The selection hint is
    // only drawn when nothing is selected.
    float DrawOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents, float UIScale, bool bShowHint);

    // LEVEL 1. TopY is the Y just below the overview band.
    void DrawQuickInspector(const FLLResidentData& Resident, float UIScale, float TopY);

    // Greedy word wrap against MaxWidth using the HUD font metrics.
    TArray<FString> WrapText(const FString& Text, float MaxWidth, float Scale) const;

    FString CurrentActionFor(const FLLResidentData& Resident) const;
};
