#include "UI/LLObserverHUD.h"
#include "UI/LLObservationSubsystem.h"
#include "Simulation/LLSimulationSubsystem.h"
#include "Characters/LLResidentCharacter.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"

namespace
{
    FString IntentToString(ELLActionIntent Intent)
    {
        switch (Intent)
        {
            case ELLActionIntent::Eat: return TEXT("Eating");
            case ELLActionIntent::Sleep: return TEXT("Sleeping");
            case ELLActionIntent::Socialize: return TEXT("Socializing");
            case ELLActionIntent::Hygiene: return TEXT("Hygiene");
            case ELLActionIntent::Toilet: return TEXT("Toilet");
            case ELLActionIntent::HaveFun: return TEXT("Leisure");
            case ELLActionIntent::Idle:
            default: return TEXT("Idle");
        }
    }

    FString RelationshipStageToString(ELLRelationshipStage Stage)
    {
        switch (Stage)
        {
            case ELLRelationshipStage::Acquaintance: return TEXT("Acquaintance");
            case ELLRelationshipStage::Friend: return TEXT("Friend");
            case ELLRelationshipStage::Dating: return TEXT("Dating");
            case ELLRelationshipStage::Partner: return TEXT("Partner");
            case ELLRelationshipStage::Engaged: return TEXT("Engaged");
            case ELLRelationshipStage::Married: return TEXT("Married");
            case ELLRelationshipStage::Estranged: return TEXT("Estranged");
            case ELLRelationshipStage::Stranger:
            default: return TEXT("Stranger");
        }
    }

    ALLResidentCharacter* FindResidentActor(UWorld* World, FGuid ResidentId)
    {
        if (!World)
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
}

void ALLObserverHUD::DrawHUD()
{
    Super::DrawHUD();

    if (!Canvas || !GetWorld())
    {
        return;
    }

    UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    ULLSimulationSubsystem* Simulation = GameInstance ? GameInstance->GetSubsystem<ULLSimulationSubsystem>() : nullptr;
    ULLObservationSubsystem* Observation = GameInstance ? GameInstance->GetSubsystem<ULLObservationSubsystem>() : nullptr;
    if (!Simulation)
    {
        return;
    }

    const TArray<FLLResidentData> Residents = Simulation->GetResidents();
    const int64 TotalMinutes = Simulation->GetSimulationMinute();
    const int64 Day = TotalMinutes / 1440 + 1;
    const int32 MinuteOfDay = static_cast<int32>(TotalMinutes % 1440);
    const int32 Hour = MinuteOfDay / 60;
    const int32 Minute = MinuteOfDay % 60;

    const float LeftX = 24.0f;
    float Y = 22.0f;
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.38f), 14.0f, 12.0f, 330.0f, 48.0f + Residents.Num() * 24.0f);
    DrawText(FString::Printf(TEXT("LifeLens  |  Day %lld  %02d:%02d  |  %d residents"), static_cast<long long>(Day), Hour, Minute, Residents.Num()),
        FLinearColor::White, LeftX, Y, GEngine ? GEngine->GetSmallFont() : nullptr, 1.05f, false);
    Y += 28.0f;

    for (const FLLResidentData& Resident : Residents)
    {
        const ALLResidentCharacter* Character = FindResidentActor(GetWorld(), Resident.ResidentId);
        const FString Action = Character ? IntentToString(Character->GetCurrentIntent()) : TEXT("Not spawned");
        DrawText(FString::Printf(TEXT("%s  -  %s"), *Resident.DisplayName, *Action),
            FLinearColor(0.88f, 0.9f, 0.94f, 1.0f), LeftX, Y, GEngine ? GEngine->GetSmallFont() : nullptr, 0.95f, false);
        Y += 23.0f;
    }

    if (!Observation || !Observation->HasObservedResident())
    {
        DrawText(TEXT("Tap/click a resident for details"), FLinearColor(0.72f, 0.76f, 0.82f, 1.0f), LeftX, Y + 10.0f,
            GEngine ? GEngine->GetSmallFont() : nullptr, 0.9f, false);
        return;
    }

    FLLResidentData Selected;
    if (!Simulation->FindResidentById(Observation->GetObservedResidentId(), Selected))
    {
        return;
    }

    const float PanelWidth = 330.0f;
    const float PanelX = FMath::Max(20.0f, Canvas->ClipX - PanelWidth - 20.0f);
    float DetailY = 24.0f;
    DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, 0.48f), PanelX - 10.0f, 12.0f, PanelWidth, 245.0f);

    const ALLResidentCharacter* SelectedActor = FindResidentActor(GetWorld(), Selected.ResidentId);
    const FString SelectedAction = SelectedActor ? IntentToString(SelectedActor->GetCurrentIntent()) : TEXT("Not spawned");

    DrawText(FString::Printf(TEXT("%s  |  age %d"), *Selected.DisplayName, Selected.AgeYears), FLinearColor::White,
        PanelX, DetailY, GEngine ? GEngine->GetSmallFont() : nullptr, 1.1f, false);
    DetailY += 28.0f;
    DrawText(FString::Printf(TEXT("Now: %s"), *SelectedAction), FLinearColor(0.75f, 0.9f, 1.0f, 1.0f),
        PanelX, DetailY, GEngine ? GEngine->GetSmallFont() : nullptr, 0.95f, false);
    DetailY += 25.0f;

    DrawText(FString::Printf(TEXT("Hunger %.0f   Energy %.0f   Social %.0f"), Selected.Needs.Hunger, Selected.Needs.Energy, Selected.Needs.Social),
        FLinearColor::White, PanelX, DetailY, GEngine ? GEngine->GetSmallFont() : nullptr, 0.9f, false);
    DetailY += 22.0f;
    DrawText(FString::Printf(TEXT("Hygiene %.0f   Bladder %.0f   Fun %.0f"), Selected.Needs.Hygiene, Selected.Needs.Bladder, Selected.Needs.Fun),
        FLinearColor::White, PanelX, DetailY, GEngine ? GEngine->GetSmallFont() : nullptr, 0.9f, false);
    DetailY += 28.0f;

    DrawText(FString::Printf(TEXT("Personality  E %.0f  A %.0f  C %.0f  O %.0f  S %.0f"),
        Selected.Personality.Extraversion,
        Selected.Personality.Agreeableness,
        Selected.Personality.Conscientiousness,
        Selected.Personality.Openness,
        Selected.Personality.EmotionalStability),
        FLinearColor(0.86f, 0.88f, 0.92f, 1.0f), PanelX, DetailY, GEngine ? GEngine->GetSmallFont() : nullptr, 0.82f, false);
    DetailY += 30.0f;

    float BestAffinity = -101.0f;
    FString BestRelationship = TEXT("No meaningful relationship yet");
    for (const FLLRelationshipData& Relationship : Simulation->GetRelationships())
    {
        FGuid OtherId;
        if (Relationship.A == Selected.ResidentId)
        {
            OtherId = Relationship.B;
        }
        else if (Relationship.B == Selected.ResidentId)
        {
            OtherId = Relationship.A;
        }
        else
        {
            continue;
        }

        if (Relationship.Affinity <= BestAffinity)
        {
            continue;
        }

        FLLResidentData Other;
        if (Simulation->FindResidentById(OtherId, Other))
        {
            BestAffinity = Relationship.Affinity;
            BestRelationship = FString::Printf(TEXT("Closest: %s  |  %s  |  affinity %.0f"),
                *Other.DisplayName, *RelationshipStageToString(Relationship.Stage), Relationship.Affinity);
        }
    }

    DrawText(BestRelationship, FLinearColor(1.0f, 0.82f, 0.68f, 1.0f), PanelX, DetailY,
        GEngine ? GEngine->GetSmallFont() : nullptr, 0.88f, false);
}
