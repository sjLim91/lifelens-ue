#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/LLObservationSubsystem.h"
#include "LLObserverHUD.generated.h"

class ULLSimulationSubsystem;
class ULLObservationSubsystem;
class ULLCoreBridgeSubsystem;
class ALLResidentCharacter;
struct FLLResidentData;

// Tabs of the LEVEL 2 detail panel (SPEC 57). The first four preserve the
// original Dagyeom HUD layout; the additional tabs consume authoritative Core
// read DTOs only and never own simulation state.
UENUM()
enum class ELLDetailTab : uint8
{
    Overview,
    Needs,
    Personality,
    TraitsSkills,
    Emotion,
    Relationships,
    Family,
    Civilization,

    Count UMETA(Hidden)
};

// Observer-first HUD (SPEC 53-57, 81).
//
// LEVEL 0: nothing selected. A thin status line plus a faint strip of
//          residents and their current action. No need or relationship values.
// LEVEL 1: one resident selected. A compact quick inspector: name and age,
//          current action, one-line status summary, one line of personality
//          words. Numbers are never shown here. Tapping the card opens LEVEL 2.
// LEVEL 2: tabbed detail panel. Core-backed emotion / relationship / family /
//          civilization detail is available here without cluttering LEVEL 0.
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
    // ScreenPosition is in viewport pixels; ViewportSize is the viewport the
    // position was measured in.
    bool HandleTap(const FVector2D& ScreenPosition, const FVector2D& ViewportSize = FVector2D::ZeroVector);

    // Safe-area insets in canvas pixels: normal LifeLens margin plus the
    // platform title-safe padding (camera cutout, rounded corners, gesture bar).
    struct FSafeInsets
    {
        float Left = 0.0f;
        float Top = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
    };
    FSafeInsets SafeInsets(float UIScale) const;

    // Viewport pixels -> canvas pixels. The canvas is the scene view rect,
    // which is smaller than the viewport when the camera constrains the aspect
    // ratio (letterbox); its origin is the view rect's top-left.
    FVector2D ViewportToCanvas(const FVector2D& ViewportPosition, const FVector2D& ViewportSize) const;

    // ---- Tap-target helpers shared with the player controller -------------
    // All rectangles are in viewport pixels.

    // Touch-target radius: 48 logical px times the Slate DPI scale.
    static float TouchTargetRadiusPixels(const UObject* WorldContext);

    // Projects the resident's render bounds (all visible components) to a
    // screen-space rectangle, and the same rectangle grown by RadiusPixels.
    // Returns false when the resident is off screen or behind the camera.
    static bool ProjectResidentTapRect(const APlayerController* PlayerController, const ALLResidentCharacter* Resident,
        float RadiusPixels, FBox2D& OutBoundsRect, FBox2D& OutTapRect);

    static constexpr int32 DetailTabCount = 8; // keep equal to ELLDetailTab::Count

private:
    // Uniform scale so text stays readable on phones and desktops alike.
    float ComputeUIScale() const;

    // LEVEL 0. Returns the Y just below the drawn overview. The selection hint is
    // only drawn when nothing is selected. bDimStrip de-emphasizes the strip
    // while LEVEL 2 is open.
    float DrawOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents,
        float UIScale, bool bShowHint, bool bDimStrip);

    // Lightweight presentation-only feedback from legacy DQ-05, reimplemented
    // on current main without changing observation/simulation authority.
    void UpdateFeedbackState(const ULLObservationSubsystem* Observation);
    void DrawSelectionFeedback(const FLLResidentData& Selected, float UIScale);
    FLinearColor Faded(const FLinearColor& Color) const
    {
        FLinearColor Result = Color;
        Result.A *= PanelFade;
        return Result;
    }

    // World overview panel (SPEC 61). Opened by tapping the overview band at
    // LEVEL 0. Only values available from the read API are drawn.
    void DrawWorldOverview(const ULLSimulationSubsystem& Simulation, const TArray<FLLResidentData>& Residents, float UIScale, float TopY);

    // LEVEL 1. TopY is the Y just below the overview band.
    void DrawQuickInspector(const FLLResidentData& Resident, float UIScale, float TopY);

    // LEVEL 2. TopY is the Y just below the overview band.
    void DrawDetailPanel(const FLLResidentData& Resident, float UIScale, float TopY);

    // Greedy word wrap against MaxWidth using the HUD font metrics.
    TArray<FString> WrapText(const FString& Text, float MaxWidth, float Scale) const;

    // ll.DebugTapTargets: outlines of each resident's projected bounds and tap
    // rectangle, for checking the pick against what is rendered.
    void DrawDebugTapTargets(const FVector2D& ViewportSize);

    FString CurrentActionFor(const FLLResidentData& Resident) const;

    ULLObservationSubsystem* GetObservation() const;
    ULLCoreBridgeSubsystem* GetCoreBridge() const;

    // Chrome rectangles from the last DrawHUD, in canvas pixels.
    FBox2D OverviewBandRect;
    FBox2D WorldOverviewRect;
    FBox2D QuickInspectorRect;
    FBox2D DetailPanelRect;
    FBox2D DetailBackRect;
    FBox2D DetailTabRects[DetailTabCount];

    ELLDetailTab ActiveTab = ELLDetailTab::Overview;
    FGuid LastDetailResidentId;
    bool bWorldOverviewOpen = false;

    // Presentation-only feedback state.
    ELLObservationLevel LastLevel = ELLObservationLevel::World;
    FGuid LastObservedId;
    float LevelChangeTime = -100.0f;
    float SelectionChangeTime = -100.0f;
    float PanelFade = 1.0f;

    // Canvas size and view-rect origin of the last DrawHUD, for mapping taps.
    FVector2D LastCanvasSize = FVector2D::ZeroVector;
    FVector2D LastViewRectMin = FVector2D::ZeroVector;
    bool bLastViewRectKnown = false;
};
