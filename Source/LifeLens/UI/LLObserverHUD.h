#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LLObserverHUD.generated.h"

class ULLSimulationSubsystem;
class ULLObservationSubsystem;
struct FLLResidentData;

// Tabs of the LEVEL 2 detail panel (SPEC 57). Only tabs backed by a read API
// exist; Emotion / Memory / Relationships / Family etc. are added when the
// simulation side exposes their data.
UENUM()
enum class ELLDetailTab : uint8
{
    Overview,
    Needs,
    Personality,
    TraitsSkills,

    Count UMETA(Hidden)
};

// Observer-first HUD (SPEC 53-57, 81).
//
// LEVEL 0: nothing selected. A thin status line plus a faint strip of
//          residents and their current action. No need or relationship values.
// LEVEL 1: one resident selected. A compact quick inspector: name and age,
//          current action, one-line status summary, one line of personality
//          words. Numbers are never shown here. Tapping the card opens LEVEL 2.
// LEVEL 2: tabbed detail panel. Overview / Needs / Personality / Traits & Skills.
//
// The HUD owns the on-screen rectangles of its own chrome so the player
// controller can ask whether a tap landed on UI before hit-testing the world.
UCLASS()
class LIFELENS_API ALLObserverHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    // Returns true if the tap landed on HUD chrome and was handled here.
    bool HandleTap(const FVector2D& ScreenPosition);

    static constexpr int32 DetailTabCount = 4; // keep equal to ELLDetailTab::Count

private:
    // Uniform scale so text stays readable on phones and desktops alike.
    float ComputeUIScale() const;

    // LEVEL 0. Returns the Y just below the drawn overview. The selection hint is
    // only drawn when nothing is selected.
    float DrawOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents, float UIScale, bool bShowHint);

    // LEVEL 1. TopY is the Y just below the overview band.
    void DrawQuickInspector(const FLLResidentData& Resident, float UIScale, float TopY);

    // LEVEL 2. TopY is the Y just below the overview band.
    void DrawDetailPanel(const FLLResidentData& Resident, float UIScale, float TopY);

    // Greedy word wrap against MaxWidth using the HUD font metrics.
    TArray<FString> WrapText(const FString& Text, float MaxWidth, float Scale) const;

    FString CurrentActionFor(const FLLResidentData& Resident) const;

    ULLObservationSubsystem* GetObservation() const;

    // Chrome rectangles from the last DrawHUD, in canvas pixels.
    FBox2D QuickInspectorRect;
    FBox2D DetailPanelRect;
    FBox2D DetailTabRects[DetailTabCount];

    ELLDetailTab ActiveTab = ELLDetailTab::Overview;
    FGuid LastDetailResidentId;
};
