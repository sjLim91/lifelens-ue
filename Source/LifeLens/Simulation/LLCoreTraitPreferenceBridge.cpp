#include "Simulation/LLCoreBridgeSubsystem.h"

#include "lifelens/ObserverReadModel.h"
#include "lifelens/Simulation.h"
#include "lifelens/TraitsPreferences.h"

bool ULLCoreBridgeSubsystem::GetResidentTraitPreferenceObservation(
    FGuid ResidentId,
    FLLCoreTraitPreferenceObservation& OutObservation) const
{
    OutObservation = FLLCoreTraitPreferenceObservation{};
    if (!CoreSimulation)
    {
        return false;
    }

    const uint64* CoreCharacterId = GuidToCore.Find(ResidentId);
    if (!CoreCharacterId)
    {
        return false;
    }

    const lifelens::Character* Character = lifelens::findObservedCharacter(
        CoreSimulation->world(),
        static_cast<lifelens::CharacterId>(*CoreCharacterId));
    if (!Character)
    {
        return false;
    }

    const lifelens::TraitProfile Traits =
        lifelens::deriveTraitProfile(Character->personality, Character->genetics);
    const lifelens::PreferenceProfile Preferences =
        lifelens::derivePreferenceProfile(Character->personality, Character->genetics);

    OutObservation.ResidentId = ResidentId;
    OutObservation.Traits.Resilience = static_cast<float>(Traits.resilience);
    OutObservation.Traits.Creativity = static_cast<float>(Traits.creativity);
    OutObservation.Traits.Discipline = static_cast<float>(Traits.discipline);
    OutObservation.Traits.Compassion = static_cast<float>(Traits.compassion);
    OutObservation.Traits.Adaptability = static_cast<float>(Traits.adaptability);
    OutObservation.Traits.Boldness = static_cast<float>(Traits.boldness);
    OutObservation.Traits.Perseverance = static_cast<float>(Traits.perseverance);
    OutObservation.Traits.Resourcefulness = static_cast<float>(Traits.resourcefulness);

    OutObservation.Preferences.Socializing = static_cast<float>(Preferences.socializing);
    OutObservation.Preferences.Solitude = static_cast<float>(Preferences.solitude);
    OutObservation.Preferences.Exploration = static_cast<float>(Preferences.exploration);
    OutObservation.Preferences.Crafting = static_cast<float>(Preferences.crafting);
    OutObservation.Preferences.Gathering = static_cast<float>(Preferences.gathering);
    OutObservation.Preferences.Comfort = static_cast<float>(Preferences.comfort);
    OutObservation.Preferences.Novelty = static_cast<float>(Preferences.novelty);
    OutObservation.Preferences.Order = static_cast<float>(Preferences.order);
    return true;
}
