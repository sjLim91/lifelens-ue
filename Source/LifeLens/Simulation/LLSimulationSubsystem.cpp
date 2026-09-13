#include "Simulation/LLSimulationSubsystem.h"
#include "Save/LLSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"

void ULLSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void ULLSimulationSubsystem::NewGame(int32 OptionalSeed)
{
    WorldSeed = OptionalSeed != 0
        ? OptionalSeed
        : static_cast<int32>(FDateTime::UtcNow().GetTicks() & 0x7fffffff);

    if (WorldSeed == 0)
    {
        WorldSeed = 1;
    }

    SimulationMinute = 8 * 60; // Day 1, 08:00
    Residents.Reset();
    Relationships.Reset();

    GenerateInitialPopulation();
    GenerateInitialRelationships();
    OnSimulationStateChanged.Broadcast();
}

bool ULLSimulationSubsystem::SaveGame(const FString& SlotName)
{
    ULLSaveGame* SaveObject = Cast<ULLSaveGame>(UGameplayStatics::CreateSaveGameObject(ULLSaveGame::StaticClass()));
    if (!SaveObject)
    {
        return false;
    }

    SaveObject->WorldSeed = WorldSeed;
    SaveObject->SimulationMinute = SimulationMinute;
    SaveObject->Residents = Residents;
    SaveObject->Relationships = Relationships;
    return UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
}

bool ULLSimulationSubsystem::LoadGame(const FString& SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        return false;
    }

    ULLSaveGame* SaveObject = Cast<ULLSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!SaveObject)
    {
        return false;
    }

    WorldSeed = SaveObject->WorldSeed;
    SimulationMinute = SaveObject->SimulationMinute;
    Residents = SaveObject->Residents;
    Relationships = SaveObject->Relationships;
    OnSimulationStateChanged.Broadcast();
    return true;
}

void ULLSimulationSubsystem::AdvanceSimulationMinutes(int32 Minutes)
{
    if (Minutes <= 0)
    {
        return;
    }

    SimulationMinute += Minutes;

    const float Hours = static_cast<float>(Minutes) / 60.0f;
    for (FLLResidentData& Resident : Residents)
    {
        Resident.Needs.Hunger = FMath::Clamp(Resident.Needs.Hunger - (2.0f * Hours), 0.0f, 100.0f);
        Resident.Needs.Energy = FMath::Clamp(Resident.Needs.Energy - (1.25f * Hours), 0.0f, 100.0f);
        Resident.Needs.Hygiene = FMath::Clamp(Resident.Needs.Hygiene - (0.75f * Hours), 0.0f, 100.0f);
        Resident.Needs.Bladder = FMath::Clamp(Resident.Needs.Bladder - (3.0f * Hours), 0.0f, 100.0f);
        Resident.Needs.Social = FMath::Clamp(Resident.Needs.Social - (0.8f * Hours), 0.0f, 100.0f);
        Resident.Needs.Fun = FMath::Clamp(Resident.Needs.Fun - (0.6f * Hours), 0.0f, 100.0f);
    }

    OnSimulationStateChanged.Broadcast();
}

bool ULLSimulationSubsystem::FindResidentById(FGuid ResidentId, FLLResidentData& OutResident) const
{
    for (const FLLResidentData& Resident : Residents)
    {
        if (Resident.ResidentId == ResidentId)
        {
            OutResident = Resident;
            return true;
        }
    }
    return false;
}

bool ULLSimulationSubsystem::GetRelationship(FGuid A, FGuid B, FLLRelationshipData& OutRelationship) const
{
    for (const FLLRelationshipData& Relation : Relationships)
    {
        const bool bSamePair = (Relation.A == A && Relation.B == B) || (Relation.A == B && Relation.B == A);
        if (bSamePair)
        {
            OutRelationship = Relation;
            return true;
        }
    }
    return false;
}

bool ULLSimulationSubsystem::ApplySocialInteraction(FGuid A, FGuid B, float AffinityDelta, float TrustDelta, float RomanceDelta)
{
    if (!A.IsValid() || !B.IsValid() || A == B)
    {
        return false;
    }

    for (FLLRelationshipData& Relation : Relationships)
    {
        const bool bSamePair = (Relation.A == A && Relation.B == B) || (Relation.A == B && Relation.B == A);
        if (!bSamePair)
        {
            continue;
        }

        Relation.Affinity = FMath::Clamp(Relation.Affinity + AffinityDelta, -100.0f, 100.0f);
        Relation.Trust = FMath::Clamp(Relation.Trust + TrustDelta, -100.0f, 100.0f);
        Relation.Romance = FMath::Clamp(Relation.Romance + RomanceDelta, -100.0f, 100.0f);

        if (Relation.Stage == ELLRelationshipStage::Stranger && Relation.Affinity >= 10.0f)
        {
            Relation.Stage = ELLRelationshipStage::Acquaintance;
        }
        if (Relation.Stage == ELLRelationshipStage::Acquaintance && Relation.Affinity >= 35.0f && Relation.Trust >= 20.0f)
        {
            Relation.Stage = ELLRelationshipStage::Friend;
        }
        if ((Relation.Stage == ELLRelationshipStage::Friend || Relation.Stage == ELLRelationshipStage::Acquaintance)
            && Relation.Affinity >= 55.0f && Relation.Trust >= 35.0f && Relation.Romance >= 45.0f)
        {
            Relation.Stage = ELLRelationshipStage::Dating;
        }

        OnSimulationStateChanged.Broadcast();
        return true;
    }
    return false;
}

void ULLSimulationSubsystem::GenerateInitialPopulation()
{
    FRandomStream Random(WorldSeed);
    TSet<FString> UsedNames;

    Residents.Reserve(4);
    Residents.Add(GenerateAdult(Random, ELLSex::Male, UsedNames));
    Residents.Add(GenerateAdult(Random, ELLSex::Male, UsedNames));
    Residents.Add(GenerateAdult(Random, ELLSex::Female, UsedNames));
    Residents.Add(GenerateAdult(Random, ELLSex::Female, UsedNames));
}

FLLResidentData ULLSimulationSubsystem::GenerateAdult(FRandomStream& Random, ELLSex Sex, TSet<FString>& UsedNames)
{
    static const TArray<FString> MaleNames = {
        TEXT("민준"), TEXT("도윤"), TEXT("서준"), TEXT("지호"), TEXT("현우"), TEXT("태윤"), TEXT("준호"), TEXT("시우")
    };
    static const TArray<FString> FemaleNames = {
        TEXT("서윤"), TEXT("하윤"), TEXT("지아"), TEXT("수아"), TEXT("민서"), TEXT("예린"), TEXT("채원"), TEXT("나은")
    };
    static const TArray<FName> TraitPool = {
        TEXT("Calm"), TEXT("Ambitious"), TEXT("Romantic"), TEXT("Curious"), TEXT("Practical"), TEXT("Playful"), TEXT("Independent"), TEXT("Empathetic")
    };
    static const TArray<FName> PreferencePool = {
        TEXT("Nature"), TEXT("Music"), TEXT("Fitness"), TEXT("Cooking"), TEXT("Games"), TEXT("Reading"), TEXT("Travel"), TEXT("Socializing")
    };
    static const TArray<FString> BackgroundPool = {
        TEXT("Urban"), TEXT("Suburban"), TEXT("Rural"), TEXT("Academic"), TEXT("WorkingClass"), TEXT("Creative")
    };

    const TArray<FString>& NamePool = Sex == ELLSex::Male ? MaleNames : FemaleNames;

    FString Name;
    for (int32 Attempt = 0; Attempt < 16; ++Attempt)
    {
        Name = NamePool[Random.RandRange(0, NamePool.Num() - 1)];
        if (!UsedNames.Contains(Name))
        {
            break;
        }
    }
    if (UsedNames.Contains(Name))
    {
        Name += FString::Printf(TEXT("%02d"), Random.RandRange(10, 99));
    }
    UsedNames.Add(Name);

    FLLResidentData Resident;
    Resident.ResidentId = MakeDeterministicGuid(Random);
    Resident.DisplayName = Name;
    Resident.Sex = Sex;
    Resident.LifeStage = ELLLifeStage::Adult;
    Resident.AgeYears = Random.RandRange(20, 34);

    Resident.Personality.Extraversion = RollPercent(Random);
    Resident.Personality.Agreeableness = RollPercent(Random);
    Resident.Personality.Conscientiousness = RollPercent(Random);
    Resident.Personality.Openness = RollPercent(Random);
    Resident.Personality.EmotionalStability = RollPercent(Random);

    TArray<FName> ShuffledTraits = TraitPool;
    for (int32 Index = ShuffledTraits.Num() - 1; Index > 0; --Index)
    {
        ShuffledTraits.Swap(Index, Random.RandRange(0, Index));
    }
    Resident.Traits.Append(ShuffledTraits.GetData(), 3);

    Resident.Skills.Add(TEXT("Social"), RollPercent(Random, 20.0f, 80.0f));
    Resident.Skills.Add(TEXT("Cooking"), RollPercent(Random, 10.0f, 75.0f));
    Resident.Skills.Add(TEXT("Fitness"), RollPercent(Random, 10.0f, 75.0f));
    Resident.Skills.Add(TEXT("Career"), RollPercent(Random, 15.0f, 85.0f));

    TArray<FName> ShuffledPreferences = PreferencePool;
    for (int32 Index = ShuffledPreferences.Num() - 1; Index > 0; --Index)
    {
        ShuffledPreferences.Swap(Index, Random.RandRange(0, Index));
    }
    Resident.Preferences.Append(ShuffledPreferences.GetData(), 3);

    Resident.BackgroundTag = BackgroundPool[Random.RandRange(0, BackgroundPool.Num() - 1)];
    return Resident;
}

void ULLSimulationSubsystem::GenerateInitialRelationships()
{
    Relationships.Reset();
    for (int32 I = 0; I < Residents.Num(); ++I)
    {
        for (int32 J = I + 1; J < Residents.Num(); ++J)
        {
            FLLRelationshipData Relation;
            Relation.A = Residents[I].ResidentId;
            Relation.B = Residents[J].ResidentId;
            Relation.Affinity = 0.0f;
            Relation.Trust = 0.0f;
            Relation.Romance = 0.0f;
            Relation.Stage = ELLRelationshipStage::Stranger;
            Relationships.Add(Relation);
        }
    }
}

FGuid ULLSimulationSubsystem::MakeDeterministicGuid(FRandomStream& Random)
{
    return FGuid(
        static_cast<uint32>(Random.GetUnsignedInt()),
        static_cast<uint32>(Random.GetUnsignedInt()),
        static_cast<uint32>(Random.GetUnsignedInt()),
        static_cast<uint32>(Random.GetUnsignedInt()));
}

float ULLSimulationSubsystem::RollPercent(FRandomStream& Random, float Min, float Max)
{
    return Random.FRandRange(Min, Max);
}
