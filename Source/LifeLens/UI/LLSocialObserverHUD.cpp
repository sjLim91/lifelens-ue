#include "UI/LLSocialObserverHUD.h"

#include "Characters/LLResidentCharacter.h"
#include "Core/LLTypes.h"
#include "Simulation/LLCoreBridgeSubsystem.h"
#include "UI/LLObservationSubsystem.h"
#include "UI/LLObserverLabels.h"
#include "UI/LLSocialCommunicationText.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

namespace
{
    constexpr int64 SpeechBubbleLifetimeMinutes = 8;
    constexpr int64 EventFeedLifetimeMinutes = 180;
    constexpr int64 ResidentHistoryLifetimeMinutes = 720;
    constexpr int32 MaxVisibleSpeechBubbles = 2;
    constexpr int32 MaxVisibleFeedEntries = 4;
    constexpr int32 MaxVisibleResidentHistoryEntries = 3;

    UFont* SocialHUDFont()
    {
        return GEngine ? GEngine->GetSmallFont() : nullptr;
    }

    ALLResidentCharacter* FindSocialResidentActor(UWorld* World, FGuid ResidentId)
    {
        if (!World || !ResidentId.IsValid())
        {
            return nullptr;
        }
        for (TActorIterator<ALLResidentCharacter> It(World); It; ++It)
        {
            if (It->GetResidentId() == ResidentId)
            {
                return *It;
            }
        }
        return nullptr;
    }

    ULLCoreBridgeSubsystem* FindSocialCoreBridge(UWorld* World)
    {
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        return GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
    }

    ULLObservationSubsystem* FindSocialObservation(UWorld* World)
    {
        UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
        return GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    }

    bool SocialRectsOverlap(const FBox2D& A, const FBox2D& B)
    {
        return A.Min.X < B.Max.X && A.Max.X > B.Min.X
            && A.Min.Y < B.Max.Y && A.Max.Y > B.Min.Y;
    }

    FString SocialRelativeTimeLabel(int64 AgeMinutes)
    {
        if (AgeMinutes < 1)
        {
            return TEXT("방금");
        }
        if (AgeMinutes < 60)
        {
            return FString::Printf(TEXT("%lld분 전"), static_cast<long long>(AgeMinutes));
        }
        if (AgeMinutes < 1440)
        {
            return FString::Printf(TEXT("%lld시간 전"), static_cast<long long>(AgeMinutes / 60));
        }
        return FString::Printf(TEXT("%lld일 전"), static_cast<long long>(AgeMinutes / 1440));
    }
}

void ALLSocialObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GetWorld())
    {
        return;
    }

    ULLCoreBridgeSubsystem* Bridge = FindSocialCoreBridge(GetWorld());
    if (!Bridge || !Bridge->IsCoreRunning())
    {
        return;
    }

    const TArray<FLLCoreSocialEventObservation> Events = Bridge->GetRecentSocialEvents(24);
    if (Events.Num() == 0)
    {
        return;
    }

    DrawSocialOverlays(Events, Bridge->GetWorldObservation().SimulationMinute);
}

FString ALLSocialObserverHUD::CurrentActionFor(const FLLResidentData& Resident) const
{
    if (ULLCoreBridgeSubsystem* Bridge = FindSocialCoreBridge(GetWorld()))
    {
        FLLCoreResidentObservation Observation;
        if (Bridge->GetResidentObservation(Resident.ResidentId, Observation))
        {
            FString Label = LLSocialCommunicationText::ActivityLabel(Observation.ActivityLabel);
            if (Observation.ActivityKind == ELLCoreObservedActivityKind::Social
                && !Observation.ActivityTargetName.IsEmpty())
            {
                Label += TEXT(" · ") + Observation.ActivityTargetName;
            }
            return Label;
        }
    }

    if (const ALLResidentCharacter* Actor = FindSocialResidentActor(GetWorld(), Resident.ResidentId))
    {
        return LLObserverLabels::IntentToString(Actor->GetCurrentIntent());
    }
    return FString();
}

void ALLSocialObserverHUD::DrawSocialOverlays(
    const TArray<FLLCoreSocialEventObservation>& Events,
    int64 CurrentSimulationMinute)
{
    // Speech belongs to the world itself, so it remains visible at every
    // observation depth. Bottom panels adapt to the base HUD density instead of
    // competing with LEVEL 1/2 resident chrome.
    DrawSpeechBubbles(Events, CurrentSimulationMinute);

    ULLObservationSubsystem* Observation = FindSocialObservation(GetWorld());
    if (!Observation)
    {
        DrawEventFeed(Events, CurrentSimulationMinute);
        return;
    }

    if (Observation->GetObservationLevel() == ELLObservationLevel::Detail)
    {
        // LEVEL 2 already spends most of the available mobile-safe width on
        // resident detail. Avoid stacking two more bottom panels over it.
        return;
    }

    if (Observation->HasObservedResident())
    {
        DrawObservedResidentHistory(Events, CurrentSimulationMinute);
        return;
    }

    DrawEventFeed(Events, CurrentSimulationMinute);
}

void ALLSocialObserverHUD::DrawSpeechBubbles(
    const TArray<FLLCoreSocialEventObservation>& Events,
    int64 CurrentSimulationMinute)
{
    APlayerController* PlayerController = GetOwningPlayerController();
    UFont* Font = SocialHUDFont();
    ULLCoreBridgeSubsystem* Bridge = FindSocialCoreBridge(GetWorld());
    ULLObservationSubsystem* Observation = FindSocialObservation(GetWorld());
    if (!PlayerController || !Font || !Bridge)
    {
        return;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    PlayerController->GetViewportSize(ViewportX, ViewportY);
    const FVector2D ViewportSize(ViewportX, ViewportY);
    const float UIScale = FMath::Clamp(
        FMath::Min(Canvas->ClipX, Canvas->ClipY) / 540.0f,
        1.0f,
        2.5f);
    const float TextScale = 0.82f * UIScale;
    const float SpeakerScale = 0.68f * UIScale;
    // Named apart from the PadX/PadY constants in LLObserverHUD.cpp: both files
    // can land in the same unity translation unit, where identical names trip
    // -Werror,-Wshadow and break the build.
    const float BubblePadX = 9.0f * UIScale;
    const float BubblePadY = 6.0f * UIScale;

    TSet<FGuid> PresentedActors;
    TArray<FBox2D> OccupiedBubbleRects;
    int32 VisibleCount = 0;

    for (int32 Index = Events.Num() - 1;
         Index >= 0 && VisibleCount < MaxVisibleSpeechBubbles;
         --Index)
    {
        const FLLCoreSocialEventObservation& Event = Events[Index];
        if (!Event.bSuccessful || !Event.ActorResidentId.IsValid())
        {
            continue;
        }

        const int64 AgeMinutes = FMath::Max<int64>(
            0,
            CurrentSimulationMinute - Event.SimulationMinute);
        if (AgeMinutes > SpeechBubbleLifetimeMinutes
            || PresentedActors.Contains(Event.ActorResidentId))
        {
            continue;
        }

        ALLResidentCharacter* Actor = FindSocialResidentActor(GetWorld(), Event.ActorResidentId);
        if (!Actor)
        {
            continue;
        }

        FLLCoreResidentObservation ActorObservation;
        const FLLCoreResidentObservation* ActorContext =
            Bridge->GetResidentObservation(Event.ActorResidentId, ActorObservation)
                ? &ActorObservation
                : nullptr;
        FString Speech = LLSocialCommunicationText::SpeechLine(Event, ActorContext);
        if (Speech.IsEmpty())
        {
            continue;
        }
        if (Speech.Len() > 36)
        {
            Speech = Speech.Left(35) + TEXT("…");
        }
        const FString Speaker = ActorContext && !ActorContext->DisplayName.IsEmpty()
            ? ActorContext->DisplayName
            : Actor->GetResidentDisplayName().ToString();
        const bool bSelectedSpeaker = Observation && Observation->HasObservedResident()
            && Observation->GetObservedResidentId() == Event.ActorResidentId;

        FVector BoundsOrigin = FVector::ZeroVector;
        FVector BoundsExtent = FVector::ZeroVector;
        Actor->GetActorBounds(false, BoundsOrigin, BoundsExtent, false);
        const FVector BubbleWorldLocation = BoundsOrigin
            + FVector(0.0f, 0.0f, BoundsExtent.Z + 32.0f);

        FVector2D ScreenPosition;
        if (!PlayerController->ProjectWorldLocationToScreen(
                BubbleWorldLocation, ScreenPosition, false))
        {
            continue;
        }
        const FVector2D CanvasPosition = ViewportToCanvas(ScreenPosition, ViewportSize);

        float TextW = 0.0f;
        float TextH = 0.0f;
        GetTextSize(Speech, TextW, TextH, Font, TextScale);
        float SpeakerW = 0.0f;
        float SpeakerH = 0.0f;
        GetTextSize(Speaker, SpeakerW, SpeakerH, Font, SpeakerScale);

        const float BubbleW = FMath::Max(TextW, SpeakerW) + BubblePadX * 2.0f;
        const float BubbleH = SpeakerH + TextH + BubblePadY * 2.0f + 2.0f * UIScale;
        const float BubbleX = FMath::Clamp(
            CanvasPosition.X - BubbleW * 0.5f,
            4.0f,
            FMath::Max(4.0f, Canvas->ClipX - BubbleW - 4.0f));
        float BubbleY = FMath::Clamp(
            CanvasPosition.Y - BubbleH,
            4.0f,
            FMath::Max(4.0f, Canvas->ClipY - BubbleH - 4.0f));

        bool bOverlapsExisting = false;
        for (int32 Attempt = 0; Attempt < 4; ++Attempt)
        {
            const FBox2D Candidate(
                FVector2D(BubbleX, BubbleY),
                FVector2D(BubbleX + BubbleW, BubbleY + BubbleH));
            bOverlapsExisting = false;
            for (const FBox2D& Existing : OccupiedBubbleRects)
            {
                if (SocialRectsOverlap(Candidate, Existing))
                {
                    bOverlapsExisting = true;
                    break;
                }
            }
            if (!bOverlapsExisting)
            {
                OccupiedBubbleRects.Add(Candidate);
                break;
            }

            BubbleY = FMath::Clamp(
                BubbleY - BubbleH - 5.0f * UIScale,
                4.0f,
                FMath::Max(4.0f, Canvas->ClipY - BubbleH - 4.0f));
        }
        if (bOverlapsExisting)
        {
            continue;
        }

        const float Fade = FMath::Clamp(
            1.0f - static_cast<float>(AgeMinutes)
                / static_cast<float>(SpeechBubbleLifetimeMinutes + 1),
            0.25f,
            1.0f);
        // Background and text fade independently. Multiplying both by Fade let
        // an old bubble drop to 0.195 background alpha, at which point the
        // generated-world vegetation shows through and the text stops being
        // readable. The panel now stays close to opaque and only the text
        // fades, so age still reads without costing legibility.
        const FLinearColor BubbleColor = bSelectedSpeaker
            ? FLinearColor(0.025f, 0.09f, 0.13f, 0.92f + 0.05f * Fade)
            : FLinearColor(0.02f, 0.025f, 0.035f, 0.88f + 0.07f * Fade);
        FLinearColor TextColor = Event.PresentationLevel == ELLCoreSocialPresentationLevel::Important
            ? FLinearColor(1.0f, 0.88f, 0.68f, Fade)
            : FLinearColor(1.0f, 1.0f, 1.0f, Fade);

        const float BubbleShadow = 3.0f * UIScale;
        DrawRect(
            FLinearColor(0.0f, 0.0f, 0.0f, 0.26f * Fade),
            BubbleX + BubbleShadow,
            BubbleY + BubbleShadow,
            BubbleW,
            BubbleH);
        DrawRect(BubbleColor, BubbleX, BubbleY, BubbleW, BubbleH);

        const FLinearColor AccentColor = bSelectedSpeaker
            ? FLinearColor(0.38f, 0.90f, 1.0f, 0.95f * Fade)
            : (Event.PresentationLevel == ELLCoreSocialPresentationLevel::Important
                ? FLinearColor(1.0f, 0.72f, 0.36f, 0.90f * Fade)
                : FLinearColor(0.42f, 0.82f, 1.0f, 0.55f * Fade));
        DrawRect(
            AccentColor,
            BubbleX,
            BubbleY,
            BubbleW,
            FMath::Max(bSelectedSpeaker ? 2.0f : 1.0f, (bSelectedSpeaker ? 2.0f : 1.0f) * UIScale));

        // A short leader ties the bubble back to its resident when multiple
        // people stand close together. It is presentation-only and clipped to
        // a modest length so it never becomes a screen-spanning line.
        const FVector2D BubbleAnchor(BubbleX + BubbleW * 0.5f, BubbleY + BubbleH);
        FVector2D LeaderDelta = CanvasPosition - BubbleAnchor;
        const float LeaderLength = LeaderDelta.Size();
        if (LeaderLength > 6.0f * UIScale)
        {
            const float MaxLeaderLength = 76.0f * UIScale;
            LeaderDelta = LeaderDelta.GetSafeNormal() * FMath::Min(LeaderLength, MaxLeaderLength);
            const FVector2D LeaderEnd = BubbleAnchor + LeaderDelta;
            DrawLine(
                BubbleAnchor.X,
                BubbleAnchor.Y,
                LeaderEnd.X,
                LeaderEnd.Y,
                AccentColor * 0.72f,
                FMath::Max(1.0f, UIScale));
        }
        const FLinearColor SpeakerColor = bSelectedSpeaker
            ? FLinearColor(0.50f, 0.94f, 1.0f, Fade)
            : (Event.PresentationLevel == ELLCoreSocialPresentationLevel::Important
                ? FLinearColor(1.0f, 0.75f, 0.46f, Fade)
                : FLinearColor(0.50f, 0.84f, 1.0f, Fade));
        DrawText(
            Speaker,
            SpeakerColor,
            BubbleX + BubblePadX,
            BubbleY + BubblePadY,
            Font,
            SpeakerScale,
            false);
        DrawText(
            Speech,
            TextColor,
            BubbleX + BubblePadX,
            BubbleY + BubblePadY + SpeakerH + 2.0f * UIScale,
            Font,
            TextScale,
            false);

        PresentedActors.Add(Event.ActorResidentId);
        ++VisibleCount;
    }
}

void ALLSocialObserverHUD::DrawObservedResidentHistory(
    const TArray<FLLCoreSocialEventObservation>& Events,
    int64 CurrentSimulationMinute)
{
    UFont* Font = SocialHUDFont();
    ULLObservationSubsystem* Observation = FindSocialObservation(GetWorld());
    if (!Font || !Observation || !Observation->HasObservedResident())
    {
        return;
    }

    const FGuid ObservedId = Observation->GetObservedResidentId();
    TArray<const FLLCoreSocialEventObservation*> HistoryEvents;
    for (int32 Index = Events.Num() - 1;
         Index >= 0 && HistoryEvents.Num() < MaxVisibleResidentHistoryEntries;
         --Index)
    {
        const FLLCoreSocialEventObservation& Event = Events[Index];
        const bool bInvolvesObserved = Event.ActorResidentId == ObservedId
            || Event.TargetResidentId == ObservedId;
        const int64 AgeMinutes = FMath::Max<int64>(
            0,
            CurrentSimulationMinute - Event.SimulationMinute);
        if (!Event.bSuccessful || !bInvolvesObserved || AgeMinutes > ResidentHistoryLifetimeMinutes)
        {
            continue;
        }
        HistoryEvents.Add(&Event);
    }

    if (HistoryEvents.Num() == 0)
    {
        return;
    }

    const float UIScale = FMath::Clamp(
        FMath::Min(Canvas->ClipX, Canvas->ClipY) / 540.0f,
        1.0f,
        2.5f);
    const float TitleScale = 0.76f * UIScale;
    const float RowScale = 0.70f * UIScale;
    const float BubblePadX = 10.0f * UIScale;
    const float BubblePadY = 8.0f * UIScale;
    const float Gap = 5.0f * UIScale;
    const float PanelWidth = FMath::Min(330.0f * UIScale, Canvas->ClipX * 0.44f);

    float TitleW = 0.0f;
    float TitleH = 0.0f;
    GetTextSize(TEXT("최근 상호작용"), TitleW, TitleH, Font, TitleScale);

    TArray<FString> Lines;
    TArray<float> Heights;
    float ContentHeight = TitleH;
    for (const FLLCoreSocialEventObservation* Event : HistoryEvents)
    {
        const bool bWasActor = Event->ActorResidentId == ObservedId;
        const FString OtherName = bWasActor ? Event->TargetName : Event->ActorName;
        const int64 AgeMinutes = FMath::Max<int64>(
            0,
            CurrentSimulationMinute - Event->SimulationMinute);
        FString Line = FString::Printf(
            TEXT("%s %s · %s · %s"),
            bWasActor ? TEXT("→") : TEXT("←"),
            OtherName.IsEmpty() ? TEXT("상대") : *OtherName,
            *LLSocialCommunicationText::EventLabel(Event->Type),
            *SocialRelativeTimeLabel(AgeMinutes));
        if (Line.Len() > 38)
        {
            Line = Line.Left(37) + TEXT("…");
        }

        float W = 0.0f;
        float H = 0.0f;
        GetTextSize(Line, W, H, Font, RowScale);
        Lines.Add(MoveTemp(Line));
        Heights.Add(H);
        ContentHeight += Gap + H;
    }

    const float PanelHeight = ContentHeight + BubblePadY * 2.0f;
    const FSafeInsets Insets = SafeInsets(UIScale);
    const float PanelX = FMath::Max(
        Insets.Left,
        Canvas->ClipX - Insets.Right - PanelWidth);
    const float PanelY = FMath::Max(
        Insets.Top,
        Canvas->ClipY - Insets.Bottom - PanelHeight);

    const float HistoryShadowOffset = 4.0f * UIScale;
    const float HistoryAccentWidth = FMath::Max(3.0f, 3.0f * UIScale);
    DrawRect(
        FLinearColor(0.0f, 0.0f, 0.0f, 0.24f),
        PanelX + HistoryShadowOffset,
        PanelY + HistoryShadowOffset,
        PanelWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.018f, 0.028f, 0.044f, 0.82f),
        PanelX,
        PanelY,
        PanelWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.42f, 0.82f, 1.0f, 0.80f),
        PanelX,
        PanelY,
        HistoryAccentWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.44f, 0.63f, 0.82f, 0.22f),
        PanelX,
        PanelY,
        PanelWidth,
        FMath::Max(1.0f, UIScale));

    float CursorY = PanelY + BubblePadY;
    DrawText(
        TEXT("최근 상호작용"),
        FLinearColor(0.82f, 0.92f, 1.0f, 0.92f),
        PanelX + BubblePadX,
        CursorY,
        Font,
        TitleScale,
        false);
    CursorY += TitleH + Gap;
    DrawRect(
        FLinearColor(0.44f, 0.63f, 0.82f, 0.18f),
        PanelX + BubblePadX,
        CursorY - Gap * 0.5f,
        FMath::Max(0.0f, PanelWidth - BubblePadX * 2.0f),
        FMath::Max(1.0f, UIScale));

    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        DrawText(
            Lines[Index],
            FLinearColor(0.93f, 0.95f, 0.98f, 0.88f),
            PanelX + BubblePadX,
            CursorY,
            Font,
            RowScale,
            false);
        CursorY += Heights[Index] + Gap;
    }
}

void ALLSocialObserverHUD::DrawEventFeed(
    const TArray<FLLCoreSocialEventObservation>& Events,
    int64 CurrentSimulationMinute)
{
    UFont* Font = SocialHUDFont();
    if (!Font)
    {
        return;
    }

    TArray<const FLLCoreSocialEventObservation*> FeedEvents;
    for (int32 Index = Events.Num() - 1;
         Index >= 0 && FeedEvents.Num() < MaxVisibleFeedEntries;
         --Index)
    {
        const FLLCoreSocialEventObservation& Event = Events[Index];
        const int64 AgeMinutes = FMath::Max<int64>(
            0,
            CurrentSimulationMinute - Event.SimulationMinute);
        if (!Event.bSuccessful
            || AgeMinutes > EventFeedLifetimeMinutes
            || !LLSocialCommunicationText::ShouldAppearInEventFeed(Event.PresentationLevel))
        {
            continue;
        }
        FeedEvents.Add(&Event);
    }

    if (FeedEvents.Num() == 0)
    {
        return;
    }

    const float UIScale = FMath::Clamp(
        FMath::Min(Canvas->ClipX, Canvas->ClipY) / 540.0f,
        1.0f,
        2.5f);
    const float TitleScale = 0.78f * UIScale;
    const float RowScale = 0.72f * UIScale;
    const float BubblePadX = 10.0f * UIScale;
    const float BubblePadY = 8.0f * UIScale;
    const float Gap = 5.0f * UIScale;
    const float PanelWidth = FMath::Min(330.0f * UIScale, Canvas->ClipX * 0.48f);

    float TitleW = 0.0f;
    float TitleH = 0.0f;
    GetTextSize(TEXT("사회 사건"), TitleW, TitleH, Font, TitleScale);

    TArray<FString> Lines;
    TArray<float> Heights;
    float ContentHeight = TitleH;
    for (const FLLCoreSocialEventObservation* Event : FeedEvents)
    {
        FString Line = LLSocialCommunicationText::FeedLine(*Event);
        if (Line.Len() > 38)
        {
            Line = Line.Left(37) + TEXT("…");
        }
        float W = 0.0f;
        float H = 0.0f;
        GetTextSize(Line, W, H, Font, RowScale);
        Lines.Add(MoveTemp(Line));
        Heights.Add(H);
        ContentHeight += Gap + H;
    }

    const float PanelHeight = ContentHeight + BubblePadY * 2.0f;
    const FSafeInsets Insets = SafeInsets(UIScale);
    const float PanelX = Insets.Left;
    const float PanelY = FMath::Max(
        Insets.Top,
        Canvas->ClipY - Insets.Bottom - PanelHeight);

    const float FeedShadowOffset = 4.0f * UIScale;
    const float FeedAccentWidth = FMath::Max(3.0f, 3.0f * UIScale);
    DrawRect(
        FLinearColor(0.0f, 0.0f, 0.0f, 0.24f),
        PanelX + FeedShadowOffset,
        PanelY + FeedShadowOffset,
        PanelWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.018f, 0.028f, 0.044f, 0.84f),
        PanelX,
        PanelY,
        PanelWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.42f, 0.82f, 1.0f, 0.78f),
        PanelX,
        PanelY,
        FeedAccentWidth,
        PanelHeight);
    DrawRect(
        FLinearColor(0.44f, 0.63f, 0.82f, 0.22f),
        PanelX,
        PanelY,
        PanelWidth,
        FMath::Max(1.0f, UIScale));

    float CursorY = PanelY + BubblePadY;
    DrawText(
        TEXT("사회 사건"),
        FLinearColor(0.80f, 0.90f, 1.0f, 0.92f),
        PanelX + BubblePadX,
        CursorY,
        Font,
        TitleScale,
        false);
    CursorY += TitleH + Gap;
    DrawRect(
        FLinearColor(0.44f, 0.63f, 0.82f, 0.18f),
        PanelX + BubblePadX,
        CursorY - Gap * 0.5f,
        FMath::Max(0.0f, PanelWidth - BubblePadX * 2.0f),
        FMath::Max(1.0f, UIScale));

    for (int32 Index = 0; Index < Lines.Num(); ++Index)
    {
        const bool bImportant = FeedEvents[Index]->PresentationLevel
            == ELLCoreSocialPresentationLevel::Important;
        const FLinearColor RowColor = bImportant
            ? FLinearColor(1.0f, 0.83f, 0.62f, 0.95f)
            : FLinearColor(0.92f, 0.94f, 0.98f, 0.88f);
        if (bImportant)
        {
            DrawRect(
                FLinearColor(1.0f, 0.72f, 0.36f, 0.90f),
                PanelX + BubblePadX - 6.0f * UIScale,
                CursorY,
                FMath::Max(2.0f, 2.0f * UIScale),
                FMath::Max(2.0f, Heights[Index]));
        }
        DrawText(
            Lines[Index],
            RowColor,
            PanelX + BubblePadX,
            CursorY,
            Font,
            RowScale,
            false);
        CursorY += Heights[Index] + Gap;
    }
}
