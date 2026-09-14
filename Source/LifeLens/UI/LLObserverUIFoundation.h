#pragma once

#include "CoreMinimal.h"

class UCanvas;
class UFont;

// Observer UI foundation for Android landscape (Issue #24, DQ-01).
// Single source of the layout, typography, touch and density rules the
// observer HUD panels follow. Read-only helpers; no simulation access.
namespace LLObserverUIFoundation
{
    // ---- Scale ------------------------------------------------------------
    // Uniform UI scale from the viewport short side. Phone landscape 1080p
    // (short side 1080) resolves to 2.0; a 1080p desktop window to 2.0 as well.
    constexpr float ReferenceShortSide = 540.0f;
    constexpr float MinUIScale = 1.0f;
    constexpr float MaxUIScale = 2.5f;
    float ComputeUIScale(const UCanvas* Canvas);

    // ---- Safe area --------------------------------------------------------
    // Insets in canvas pixels. Combines the platform title-safe padding
    // (camera cutout, rounded corners, gesture bar) with the screen margin so
    // panels never touch the physical edge.
    struct FSafeArea
    {
        float Left = 0.0f;
        float Top = 0.0f;
        float Right = 0.0f;
        float Bottom = 0.0f;
    };
    FSafeArea GetSafeArea(const UCanvas* Canvas, float UIScale);

    // ---- Typography scale (multipliers of the HUD font) -------------------
    constexpr float TextTitle     = 1.15f; // panel titles, resident name
    constexpr float TextBody      = 1.00f; // status line, current action
    constexpr float TextSecondary = 0.90f; // rows, summaries
    constexpr float TextCaption   = 0.85f; // section headers, strip, tabs
    constexpr float TextHint      = 0.80f; // hints, faint affordances

    // ---- Spacing (unscaled px; multiply by UIScale) -----------------------
    constexpr float Space1 = 4.0f;
    constexpr float Space2 = 8.0f;
    constexpr float Space3 = 12.0f;
    constexpr float Space4 = 16.0f;
    constexpr float Space6 = 24.0f;

    constexpr float ScreenMargin = Space4;
    constexpr float PanelPadX    = Space3;
    constexpr float PanelPadY    = Space2;
    constexpr float LineGap      = 6.0f;
    constexpr float SectionGap   = Space3;

    // ---- Panel width limits (ratio of viewport width) ---------------------
    constexpr float QuickPanelMaxRatio  = 0.42f;
    constexpr float DetailPanelMaxRatio = 0.60f;
    constexpr float QuickPanelMaxWidth  = 360.0f;
    constexpr float QuickPanelMinWidth  = 200.0f;
    constexpr float DetailPanelMaxWidth = 520.0f;
    constexpr float DetailPanelMinWidth = 280.0f;

    // ---- Panel backgrounds (alpha over the world) --------------------------
    constexpr float OverviewBandAlpha = 0.22f;
    constexpr float QuickPanelAlpha   = 0.45f;
    constexpr float DetailPanelAlpha  = 0.60f;

    // ---- Touch targets (unscaled px; Android minimum 48dp) ----------------
    constexpr float MinTouchTarget = 48.0f;
    float TouchTargetSize(float UIScale);
    // Grows Rect symmetrically so both sides are at least the touch target.
    FBox2D ExpandToTouchTarget(const FBox2D& Rect, float UIScale);

    // ---- State tones (colour tokens) ---------------------------------------
    enum class EStateTone : uint8
    {
        Primary,   // titles, primary values
        Secondary, // rows
        Muted,     // strip, identity line
        Hint,      // affordances
        Action,    // current action
        Section,   // section headers
        Attention, // low need
        Critical,  // very low need
        Accent     // active tab underline
    };
    FLinearColor ToneColor(EStateTone Tone);

    // ---- Overflow / scroll policy ------------------------------------------
    // Text rows word-wrap to the panel inner width. Row lists never scroll;
    // rows past the panel bottom are not drawn. Lists longer than the limits
    // below are cut, so panels stay readable on small screens.
    constexpr int32 MaxOverviewStripItems = 8;
    constexpr int32 MaxDetailRows = 24;

    // ---- Information density (SPEC 53-54) ----------------------------------
    // LEVEL 0 draws at most the status line and the resident strip. Need,
    // emotion and relationship values never appear at LEVEL 0.
    constexpr int32 MaxLevel0Lines = 2;

    // ---- Font ---------------------------------------------------------------
    // Korean-capable HUD font asset. Resolved once; when the asset is absent
    // the engine small font is used, so the HUD keeps working before the
    // Content/UI asset exists.
    constexpr const TCHAR* HUDFontAssetPath = TEXT("/Game/UI/Fonts/F_LifeLensHUD.F_LifeLensHUD");
    UFont* GetHUDFont();
}
