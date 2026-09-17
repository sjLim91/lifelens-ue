#include "Simulation/LLSimulationSubsystem.h"

#include "Simulation/LLCoreBridgeSubsystem.h"
#include "Save/LLSaveGame.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Subsystems/SubsystemCollection.h"

namespace
{
float ToLegacyNeed(float CoreDeficit)
{
    return FMath::Clamp((1.0f - CoreDeficit) * 100.0f, 0.0f, 100.0f);
}

ELLSex ToLegacySex(ELLCoreSex Sex)
{
    return Sex == ELLCoreSex::Female ? ELLSex::Female : ELLSex::Male;
}

ELLLifeStage ToLegacyLifeStage(ELLCoreLifeStage Stage)
{
    switch (Stage)
    {
        case ELLCoreLifeStage::Baby:
        case ELLCoreLifeStage::Toddler:
            return ELLLifeStage::Infant;
        case ELLCoreLifeStage::Child:
            return ELLLifeStage::Child;
        case ELLCoreLifeStage::Teen:
            return ELLLifeStage::Teen;
        case ELLCoreLifeStage::YoungAdult:
        case ELLCoreLifeStage::Adult:
        case ELLCoreLifeStage::MiddleAge:
            return ELLLifeStage::Adult;
        case ELLCoreLifeStage::Elderly:
            return ELLLifeStage::Elder;
    }
    return ELLLifeStage::Adult;
}

ELLRelationshipStage ToLegacyRelationshipStage(ELLCoreRomanceStage Stage)
{
    switch (Stage)
    {
        case ELLCoreRomanceStage::Dating: return ELLRelationshipStage::Dating;
        case ELLCoreRomanceStage::Engaged: return ELLRelationshipStage::Engaged;
        case ELLCoreRomanceStage::Married: return ELLRelationshipStage::Married;
        case ELLCoreRomanceStage::Separated:
        case ELLCoreRomanceStage::Divorced:
        case ELLCoreRomanceStage::Widowed:
        case ELLCoreRomanceStage::FormerPartners:
            return ELLRelationshipStage::Estranged;
        case ELLCoreRomanceStage::None:
        default:
            return ELLRelationshipStage::Stranger;
    }
}

const FLLCoreRelationshipSnapshot* FindCoreRelationship(
    const FLLCoreResidentObservation& Observation,
    const FGuid& TargetResidentId)
{
    return Observation.Relationships.FindByPredicate(
        [&TargetResidentId](const FLLCoreRelationshipSnapshot& Relationship)
        {
            return Relationship.TargetResidentId == TargetResidentId;
        });
}
}

void ULLSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<ULLCoreBridgeSubsystem>();
}

ULLCoreBridgeSubsystem* ULLSimulationSubsystem::GetCoreBridge() const
{
    UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<ULLCoreBridgeSubsystem>() : nullptr;
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

    ULLCoreBridgeSubsystem* CoreBridge = GetCoreBridge();
    if (!CoreBridge)
    {
        UE_LOG(LogTemp, Error, TEXT("LifeLens NewGame failed: Core bridge subsystem unavailable."));
        bCoreAuthoritativeRuntime = false;
        Residents.Reset();
        Relationships.Reset();
        return;
    }

    CoreBridge->StartCoreNewGame(WorldSeed);
    bCoreAuthoritativeRuntime = RefreshProjectionFromCore();
    if (!bCoreAuthoritativeRuntime)
    {
        UE_LOG(LogTemp, Error, TEXT("LifeLens NewGame failed: Core founder projection unavailable."));
        return;
    }

    OnSimulationStateChanged.Broadcast();
}

bool ULLSimulationSubsystem::SaveGame(const FString& SlotName)
{
    ULLCoreBridgeSubsystem* CoreBridge = GetCoreBridge();
    if (!bCoreAuthoritativeRuntime || !CoreBridge || !CoreBridge->IsCoreRunning())
    {
        return false;
    }

    TArray<uint8> SnapshotBytes;
    FString SnapshotError;
    if (!CoreBridge->CaptureCoreSnapshotBytes(SnapshotBytes, SnapshotError))
    {
        UE_LOG(LogTemp, Error, TEXT("LifeLens SaveGame failed to capture Core snapshot: %s"), *SnapshotError);
        return false;
    }

    ULLSaveGame* SaveObject = Cast<ULLSaveGame>(UGameplayStatics::CreateSaveGameObject(ULLSaveGame::StaticClass()));
    if (!SaveObject)
    {
        return false;
    }

    SaveObject->SaveVersion = ULLSaveGame::CurrentSaveVersion;
    SaveObject->CoreSnapshotBytes = MoveTemp(SnapshotBytes);
    return UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
}

bool ULLSimulationSubsystem::LoadGame(const FString& SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        return false;
    }

    ULLSaveGame* SaveObject = Cast<ULLSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    ULLCoreBridgeSubsystem* CoreBridge = GetCoreBridge();
    if (!SaveObject || !CoreBridge)
    {
        return false;
    }

    if (SaveObject->SaveVersion != ULLSaveGame::CurrentSaveVersion)
    {
        UE_LOG(LogTemp, Error, TEXT("LifeLens LoadGame rejected non-current pre-release save version %d."), SaveObject->SaveVersion);
        return false;
    }

    FString RestoreError;
    if (!CoreBridge->RestoreCoreSnapshotBytes(SaveObject->CoreSnapshotBytes, RestoreError))
    {
        UE_LOG(LogTemp, Error, TEXT("LifeLens LoadGame failed to restore Core snapshot: %s"), *RestoreError);
        return false;
    }

    bCoreAuthoritativeRuntime = RefreshProjectionFromCore();
    if (!bCoreAuthoritativeRuntime)
    {
        return false;
    }

    OnSimulationStateChanged.Broadcast();
    return true;
}

void ULLSimulationSubsystem::AdvanceSimulationMinutes(int32 Minutes)
{
    if (Minutes <= 0)
    {
        return;
    }

    ULLCoreBridgeSubsystem* CoreBridge = GetCoreBridge();
    if (!bCoreAuthoritativeRuntime || !CoreBridge || !CoreBridge->IsCoreRunning())
    {
        return;
    }

    CoreBridge->AdvanceCoreMinutes(Minutes);
    if (RefreshProjectionFromCore())
    {
        OnSimulationStateChanged.Broadcast();
    }
}

bool ULLSimulationSubsystem::RefreshProjectionFromCore()
{
    ULLCoreBridgeSubsystem* CoreBridge = GetCoreBridge();
    if (!CoreBridge || !CoreBridge->IsCoreRunning())
    {
        return false;
    }

    const FLLCoreWorldObservation CoreWorld = CoreBridge->GetWorldObservation();
    const TArray<FLLCoreResidentObservation> CoreResidents = CoreBridge->GetResidentObservations();
    if (CoreResidents.Num() == 0)
    {
        return false;
    }

    WorldSeed = CoreBridge->GetRuntimeSeed();
    SimulationMinute = CoreWorld.SimulationMinute;
    Residents.Reset(CoreResidents.Num());
    Relationships.Reset();

    // This compatibility array drives physical resident actors. Deceased Core
    // residents remain in authoritative history/genealogy/Observer DTOs but are
    // deliberately omitted here so WorldDirector releases reservations/runtime
    // and destroys their physical actor through its existing projection cleanup.
    for (const FLLCoreResidentObservation& CoreResident : CoreResidents)
    {
        if (!CoreResident.bAlive)
        {
            continue;
        }

        FLLResidentData Resident;
        Resident.ResidentId = CoreResident.ResidentId;
        Resident.DisplayName = CoreResident.DisplayName;
        Resident.Sex = ToLegacySex(CoreResident.Sex);
        Resident.LifeStage = ToLegacyLifeStage(CoreResident.LifeStage);
        Resident.AgeYears = CoreResident.AgeYears;

        Resident.Personality.Extraversion = FMath::Clamp(
            (CoreResident.Personality.Sociability + (1.0f - CoreResident.Personality.Introversion)) * 50.0f,
            0.0f, 100.0f);
        Resident.Personality.Agreeableness = FMath::Clamp(CoreResident.Personality.Agreeableness * 100.0f, 0.0f, 100.0f);
        Resident.Personality.Conscientiousness = FMath::Clamp(CoreResident.Personality.Conscientiousness * 100.0f, 0.0f, 100.0f);
        Resident.Personality.Openness = FMath::Clamp(CoreResident.Personality.Openness * 100.0f, 0.0f, 100.0f);
        Resident.Personality.EmotionalStability = FMath::Clamp(CoreResident.Personality.EmotionalStability * 100.0f, 0.0f, 100.0f);

        Resident.Needs.Hunger = ToLegacyNeed(CoreResident.Needs.Hunger);
        Resident.Needs.Thirst = ToLegacyNeed(CoreResident.Needs.Thirst);
        Resident.Needs.Energy = ToLegacyNeed(CoreResident.Needs.Sleep);
        Resident.Needs.Hygiene = ToLegacyNeed(CoreResident.Needs.Hygiene);
        Resident.Needs.Bladder = ToLegacyNeed(CoreResident.Needs.Bladder);
        // Core social cognition is relationship/emotion driven rather than a
        // single legacy bar. Social/Fun remain compatibility-only fields and
        // are intentionally excluded from observer need rows/summaries.
        Resident.Needs.Social = 65.0f;
        Resident.Needs.Fun = 60.0f;

        FLLCoreFamilyObservation Family;
        if (CoreBridge->GetFamilyObservation(CoreResident.ResidentId, Family))
        {
            Resident.PartnerId = Family.bHasActivePartner ? Family.PartnerResidentId : FGuid();
            Resident.bPregnant = Family.bGestationalParent;
            for (const FLLCoreFamilyMemberSnapshot& Parent : Family.Parents)
            {
                if (Parent.ResidentId.IsValid()) Resident.ParentIds.Add(Parent.ResidentId);
            }
            for (const FLLCoreFamilyMemberSnapshot& Child : Family.Children)
            {
                if (Child.ResidentId.IsValid()) Resident.ChildIds.Add(Child.ResidentId);
            }
        }

        Residents.Add(MoveTemp(Resident));
    }

    // Legacy relationship projection is likewise restricted to living physical
    // residents. Core retains directional relationships and life history for
    // deceased residents independently of this presentation compatibility view.
    for (int32 I = 0; I < CoreResidents.Num(); ++I)
    {
        for (int32 J = I + 1; J < CoreResidents.Num(); ++J)
        {
            const FLLCoreResidentObservation& A = CoreResidents[I];
            const FLLCoreResidentObservation& B = CoreResidents[J];
            if (!A.bAlive || !B.bAlive)
            {
                continue;
            }

            const FLLCoreRelationshipSnapshot* AToB = FindCoreRelationship(A, B.ResidentId);
            const FLLCoreRelationshipSnapshot* BToA = FindCoreRelationship(B, A.ResidentId);

            FLLRelationshipData Relation;
            Relation.A = A.ResidentId;
            Relation.B = B.ResidentId;

            if (AToB || BToA)
            {
                const float Affection = ((AToB ? AToB->Affection : 0.0f) + (BToA ? BToA->Affection : 0.0f)) * 0.5f;
                const float Trust = ((AToB ? AToB->Trust : 0.0f) + (BToA ? BToA->Trust : 0.0f)) * 0.5f;
                const float Romance = ((AToB ? AToB->RomanticInterest : 0.0f) + (BToA ? BToA->RomanticInterest : 0.0f)) * 0.5f;
                const float Familiarity = ((AToB ? AToB->Familiarity : 0.0f) + (BToA ? BToA->Familiarity : 0.0f)) * 0.5f;

                Relation.Affinity = FMath::Clamp(Affection * 100.0f, -100.0f, 100.0f);
                Relation.Trust = FMath::Clamp(Trust * 100.0f, -100.0f, 100.0f);
                Relation.Romance = FMath::Clamp(Romance * 100.0f, -100.0f, 100.0f);
                if (Familiarity >= 0.10f)
                {
                    Relation.Stage = ELLRelationshipStage::Acquaintance;
                }
                if (Relation.Affinity >= 35.0f && Relation.Trust >= 20.0f)
                {
                    Relation.Stage = ELLRelationshipStage::Friend;
                }
            }

            FLLCoreFamilyObservation FamilyA;
            if (CoreBridge->GetFamilyObservation(A.ResidentId, FamilyA)
                && FamilyA.bHasActivePartner
                && FamilyA.PartnerResidentId == B.ResidentId)
            {
                Relation.Stage = ToLegacyRelationshipStage(FamilyA.PartnerStage);
            }

            Relationships.Add(MoveTemp(Relation));
        }
    }

    return true;
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
        if ((Relation.A == A && Relation.B == B) || (Relation.A == B && Relation.B == A))
        {
            OutRelationship = Relation;
            return true;
        }
    }
    return false;
}

// Legacy Preflight compatibility markers only: ApplyActionOutcome, ApplySocialInteraction.
// No mutable projection API or implementation remains; authoritative outcomes flow
// through LifeLensCore completion/ACK paths and are projected here read-only.
