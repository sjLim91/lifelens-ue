#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/ObserverReadModelV2.h"
#include "lifelens/Simulation.h"

namespace
{
uint64 MixStable64(uint64 Value)
{
    Value += 0x9E3779B97F4A7C15ull;
    Value = (Value ^ (Value >> 30)) * 0xBF58476D1CE4E5B9ull;
    Value = (Value ^ (Value >> 27)) * 0x94D049BB133111EBull;
    return Value ^ (Value >> 31);
}

float CoreScalar(double Value)
{
    return static_cast<float>(Value);
}

int32 SafeCount(std::size_t Count)
{
    return Count > static_cast<std::size_t>(MAX_int32)
        ? MAX_int32
        : static_cast<int32>(Count);
}
}

void ULLCoreBridgeSubsystem::Deinitialize()
{
    ResetRuntime();
    Super::Deinitialize();
}

void ULLCoreBridgeSubsystem::StartCoreObserverDemo(int32 Seed, bool bSocialDemo)
{
    ResetRuntime();

    ActiveSeed = Seed == 0 ? 42 : Seed;
    const uint64 CoreSeed = static_cast<uint64>(static_cast<uint32>(ActiveSeed));

    CoreSimulation = new lifelens::Simulation(CoreSeed);
    CoreSimulation->onEvent([this](const std::string& Line)
    {
        PushCoreEvent(UTF8_TO_TCHAR(Line.c_str()));
    });

    if (bSocialDemo)
    {
        CoreSimulation->setupSocialDemo();
    }
    else
    {
        CoreSimulation->setupDemo();
    }

    RebuildGuidIndex();
    OnCoreRuntimeStateChanged.Broadcast();
}

void ULLCoreBridgeSubsystem::StopCoreObserverDemo()
{
    const bool bWasRunning = CoreSimulation != nullptr;
    ResetRuntime();
    if (bWasRunning)
    {
        OnCoreRuntimeStateChanged.Broadcast();
    }
}

void ULLCoreBridgeSubsystem::AdvanceCoreMinutes(int32 Minutes)
{
    if (!CoreSimulation || Minutes <= 0)
    {
        return;
    }

    CoreSimulation->runMinutes(Minutes);
    RebuildGuidIndex();
    OnCoreRuntimeStateChanged.Broadcast();
}

FLLCoreWorldObservation ULLCoreBridgeSubsystem::GetWorldObservation() const
{
    FLLCoreWorldObservation Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const lifelens::World& CoreWorld = CoreSimulation->world();
    Result.SimulationMinute = static_cast<int64>(CoreWorld.minute);
    Result.TotalResidents = SafeCount(CoreWorld.characters.size());

    for (const lifelens::Character& Character : CoreWorld.characters)
    {
        if (Character.alive)
        {
            ++Result.LivingResidents;
        }
        else
        {
            ++Result.DeceasedResidents;
        }
    }

    return Result;
}

TArray<FLLCoreResidentObservation> ULLCoreBridgeSubsystem::GetResidentObservations() const
{
    TArray<FLLCoreResidentObservation> Result;
    if (!CoreSimulation)
    {
        return Result;
    }

    const auto& CoreCharacters = CoreSimulation->world().characters;
    Result.Reserve(SafeCount(CoreCharacters.size()));

    for (const lifelens::Character& Character : CoreCharacters)
    {
        FLLCoreResidentObservation Observation;
        if (BuildResidentObservation(static_cast<uint64>(Character.id), Observation))
        {
            Result.Add(MoveTemp(Observation));
        }
    }

    return Result;
}

bool ULLCoreBridgeSubsystem::GetResidentObservation(
    FGuid ResidentId,
    FLLCoreResidentObservation& OutObservation) const
{
    if (!CoreSimulation || !ResidentId.IsValid())
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    return BuildResidentObservation(*CoreCharacterId, OutObservation);
}

void ULLCoreBridgeSubsystem::ResetRuntime()
{
    delete CoreSimulation;
    CoreSimulation = nullptr;
    ActiveSeed = 0;
    CoreToGuid.Reset();
    GuidToCore.Reset();
    RecentEvents.Reset();
}

void ULLCoreBridgeSubsystem::RebuildGuidIndex()
{
    CoreToGuid.Reset();
    GuidToCore.Reset();

    if (!CoreSimulation)
    {
        return;
    }

    for (const lifelens::Character& Character : CoreSimulation->world().characters)
    {
        const uint64 CoreCharacterId = static_cast<uint64>(Character.id);
        const FGuid ResidentId = MakeStableResidentGuid(CoreCharacterId);
        CoreToGuid.Add(CoreCharacterId, ResidentId);
        GuidToCore.Add(ResidentId, CoreCharacterId);
    }
}

void ULLCoreBridgeSubsystem::PushCoreEvent(const FString& Line)
{
    RecentEvents.Add(Line);
    while (RecentEvents.Num() > MaxRecentEvents)
    {
        RecentEvents.RemoveAt(0, 1, EAllowShrinking::No);
    }
}

FGuid ULLCoreBridgeSubsystem::MakeStableResidentGuid(uint64 CoreCharacterId) const
{
    const uint64 Seed = CoreSimulation
        ? static_cast<uint64>(CoreSimulation->world().seed)
        : static_cast<uint64>(static_cast<uint32>(ActiveSeed));

    const uint64 First = MixStable64(Seed ^ CoreCharacterId ^ 0x4C6946654C656E73ull);
    const uint64 Second = MixStable64(
        (Seed << 1) ^ (CoreCharacterId * 0x9E3779B97F4A7C15ull) ^ 0x4F62736572766572ull);

    FGuid Result(
        static_cast<uint32>(First >> 32),
        static_cast<uint32>(First),
        static_cast<uint32>(Second >> 32),
        static_cast<uint32>(Second));

    if (!Result.IsValid())
    {
        Result.D = 1u;
    }
    return Result;
}

bool ULLCoreBridgeSubsystem::BuildResidentObservation(
    uint64 CoreCharacterId,
    FLLCoreResidentObservation& OutObservation) const
{
    if (!CoreSimulation)
    {
        return false;
    }

    const lifelens::CharacterId CharacterId = static_cast<lifelens::CharacterId>(CoreCharacterId);
    const lifelens::Character* CoreCharacter =
        lifelens::findObservedCharacter(CoreSimulation->world(), CharacterId);
    if (!CoreCharacter)
    {
        return false;
    }

    const lifelens::ResidentObservation CoreObservation =
        CoreSimulation->observeResident(CharacterId);
    const lifelens::EmotionObservation CoreEmotion =
        lifelens::makeEmotionObservation(*CoreCharacter);

    OutObservation = FLLCoreResidentObservation{};
    OutObservation.ResidentId = MakeStableResidentGuid(CoreCharacterId);
    OutObservation.DisplayName = UTF8_TO_TCHAR(CoreObservation.name.c_str());
    OutObservation.bAlive = CoreCharacter->alive;

    OutObservation.Needs.Hunger = CoreScalar(CoreObservation.needs.hunger);
    OutObservation.Needs.Thirst = CoreScalar(CoreObservation.needs.thirst);
    OutObservation.Needs.Sleep = CoreScalar(CoreObservation.needs.sleep);
    OutObservation.Needs.Bladder = CoreScalar(CoreObservation.needs.bladder);
    OutObservation.Needs.Hygiene = CoreScalar(CoreObservation.needs.hygiene);

    OutObservation.Emotion.Joy = CoreScalar(CoreEmotion.joy);
    OutObservation.Emotion.Sadness = CoreScalar(CoreEmotion.sadness);
    OutObservation.Emotion.Anger = CoreScalar(CoreEmotion.anger);
    OutObservation.Emotion.Fear = CoreScalar(CoreEmotion.fear);
    OutObservation.Emotion.Embarrassment = CoreScalar(CoreEmotion.embarrassment);
    OutObservation.Emotion.Pride = CoreScalar(CoreEmotion.pride);
    OutObservation.Emotion.Jealousy = CoreScalar(CoreEmotion.jealousy);
    OutObservation.Emotion.Affection = CoreScalar(CoreEmotion.affection);
    OutObservation.Emotion.Anxiety = CoreScalar(CoreEmotion.anxiety);
    OutObservation.Emotion.Relief = CoreScalar(CoreEmotion.relief);
    OutObservation.Emotion.Grief = CoreScalar(CoreEmotion.grief);
    OutObservation.Emotion.Valence = CoreScalar(CoreEmotion.valence);
    OutObservation.Emotion.Arousal = CoreScalar(CoreEmotion.arousal);
    OutObservation.Emotion.Intensity = CoreScalar(CoreEmotion.intensity);

    switch (CoreObservation.activityKind)
    {
        case lifelens::ObservedActivityKind::Physical:
            OutObservation.ActivityKind = ELLCoreObservedActivityKind::Physical;
            break;
        case lifelens::ObservedActivityKind::Social:
            OutObservation.ActivityKind = ELLCoreObservedActivityKind::Social;
            break;
        case lifelens::ObservedActivityKind::Idle:
        default:
            OutObservation.ActivityKind = ELLCoreObservedActivityKind::Idle;
            break;
    }

    OutObservation.ActivityLabel = UTF8_TO_TCHAR(CoreObservation.activityLabel.c_str());
    OutObservation.ActivityTargetName = UTF8_TO_TCHAR(CoreObservation.activityTargetName.c_str());
    if (CoreObservation.activityTargetId != 0)
    {
        OutObservation.ActivityTargetResidentId =
            MakeStableResidentGuid(static_cast<uint64>(CoreObservation.activityTargetId));
    }

    OutObservation.Relationships.Reserve(SafeCount(CoreObservation.relationships.size()));
    for (const lifelens::RelationshipObservation& CoreRelationship : CoreObservation.relationships)
    {
        FLLCoreRelationshipSnapshot Relationship;
        Relationship.TargetResidentId =
            MakeStableResidentGuid(static_cast<uint64>(CoreRelationship.targetId));
        Relationship.TargetName = UTF8_TO_TCHAR(CoreRelationship.targetName.c_str());
        Relationship.Affection = CoreScalar(CoreRelationship.affection);
        Relationship.Trust = CoreScalar(CoreRelationship.trust);
        Relationship.Respect = CoreScalar(CoreRelationship.respect);
        Relationship.Comfort = CoreScalar(CoreRelationship.comfort);
        Relationship.Familiarity = CoreScalar(CoreRelationship.familiarity);
        Relationship.Attraction = CoreScalar(CoreRelationship.attraction);
        Relationship.RomanticInterest = CoreScalar(CoreRelationship.romanticInterest);
        Relationship.SexualAttraction = CoreScalar(CoreRelationship.sexualAttraction);
        Relationship.Commitment = CoreScalar(CoreRelationship.commitment);
        Relationship.Conflict = CoreScalar(CoreRelationship.conflict);
        Relationship.Jealousy = CoreScalar(CoreRelationship.jealousy);
        Relationship.Fear = CoreScalar(CoreRelationship.fear);
        Relationship.Grudge = CoreScalar(CoreRelationship.grudge);
        Relationship.SocialBond = CoreScalar(CoreRelationship.socialBond);
        Relationship.RomancePotential = CoreScalar(CoreRelationship.romancePotential);
        OutObservation.Relationships.Add(MoveTemp(Relationship));
    }

    OutObservation.MemoryCount = SafeCount(CoreCharacter->memory.entries.size());
    OutObservation.BeliefCount = SafeCount(CoreCharacter->beliefs.beliefs.size());
    return true;
}
