#include "lifelens/WebClientBridge.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

#include "lifelens/ContinuousEcology.h"
#include "lifelens/ContinuousTerrain.h"
#include "lifelens/Hydrology.h"
#include "lifelens/SimulationClimate.h"
#include "lifelens/TraitsPreferences.h"

namespace lifelens {
namespace {

std::string escapeJson(const std::string& value)
{
    std::ostringstream out;
    for (const unsigned char c : value) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (c < 0x20) {
                    out << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c)
                        << std::dec << std::setw(0);
                } else {
                    out << static_cast<char>(c);
                }
                break;
        }
    }
    return out.str();
}

std::uint64_t parseUnsigned64(
    const std::string& text,
    std::uint64_t fallback)
{
    if (text.empty()) return fallback;

    std::uint64_t value = 0;
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 10);
    if (result.ec != std::errc{} || result.ptr != end) {
        return fallback;
    }
    return value;
}

void appendDouble(std::ostringstream& out, double value)
{
    out << std::fixed << std::setprecision(6) << value;
}

const char* activityKindName(ObservedActivityKind kind)
{
    switch (kind) {
        case ObservedActivityKind::Idle: return "Idle";
        case ObservedActivityKind::Physical: return "Physical";
        case ObservedActivityKind::Social: return "Social";
    }
    return "Idle";
}

const char* presentationActionKindName(PresentationActionKind kind)
{
    switch(kind){
        case PresentationActionKind::Physical: return "Physical";
        case PresentationActionKind::Social: return "Social";
        case PresentationActionKind::Civilization: return "Civilization";
        case PresentationActionKind::Parenting: return "Parenting";
        case PresentationActionKind::KnowledgeTeaching: return "KnowledgeTeaching";
        case PresentationActionKind::None:
        default: return "None";
    }
}

const char* presentationActionPhaseName(PresentationActionPhase phase)
{
    switch(phase){
        case PresentationActionPhase::Moving: return "Moving";
        case PresentationActionPhase::Interacting: return "Interacting";
        case PresentationActionPhase::Idle:
        default: return "Idle";
    }
}

const char* objectKindName(ObjectKind kind)
{
    switch(kind){
        case ObjectKind::Bed: return "Bed";
        case ObjectKind::Toilet: return "Toilet";
        case ObjectKind::Sink: return "Sink";
        case ObjectKind::Fridge: return "Fridge";
        case ObjectKind::Chair: return "Chair";
        case ObjectKind::Table: return "Table";
        case ObjectKind::Sofa: return "Sofa";
    }
    return "Unknown";
}

const char* parentingActionName(ParentingAction action)
{
    switch(action){
        case ParentingAction::Feed: return "Feed";
        case ParentingAction::PutToSleep: return "PutToSleep";
        case ParentingAction::Bathe: return "Bathe";
        case ParentingAction::ToiletAssist: return "ToiletAssist";
        case ParentingAction::Hold: return "Hold";
        case ParentingAction::Play: return "Play";
        case ParentingAction::Educate: return "Educate";
        case ParentingAction::Discipline: return "Discipline";
        case ParentingAction::Comfort: return "Comfort";
        case ParentingAction::HealthCare: return "HealthCare";
    }
    return "Comfort";
}

const char* memorySourceName(MemorySource source)
{
    switch (source) {
        case MemorySource::DirectWitness: return "DirectWitness";
        case MemorySource::ToldByOther: return "ToldByOther";
        case MemorySource::Inferred: return "Inferred";
    }
    return "Inferred";
}

const char* precipitationTypeName(PrecipitationType type)
{
    switch (type) {
        case PrecipitationType::None: return "None";
        case PrecipitationType::Rain: return "Rain";
        case PrecipitationType::Snow: return "Snow";
    }
    return "None";
}

const char* weatherSummaryName(WeatherSummary summary)
{
    switch (summary) {
        case WeatherSummary::Clear: return "Clear";
        case WeatherSummary::Cloudy: return "Cloudy";
        case WeatherSummary::Rain: return "Rain";
        case WeatherSummary::Snow: return "Snow";
        case WeatherSummary::Fog: return "Fog";
        case WeatherSummary::Storm: return "Storm";
        case WeatherSummary::Heat: return "Heat";
        case WeatherSummary::Cold: return "Cold";
    }
    return "Clear";
}

const char* facilityKindName(FacilityKind kind)
{
    switch (kind) {
        case FacilityKind::PrimitiveStorage: return "PrimitiveStorage";
        case FacilityKind::FirePit: return "FirePit";
        case FacilityKind::WorkSurface: return "WorkSurface";
        case FacilityKind::SleepingPlace: return "SleepingPlace";
        case FacilityKind::Shelter: return "Shelter";
        case FacilityKind::Furnace: return "Furnace";
    }
    return "PrimitiveStorage";
}

const char* facilityStateName(FacilityState state)
{
    switch (state) {
        case FacilityState::Planned: return "Planned";
        case FacilityState::UnderConstruction: return "UnderConstruction";
        case FacilityState::Operational: return "Operational";
        case FacilityState::Ruined: return "Ruined";
    }
    return "Planned";
}

const char* sanitationSiteKindName(PrimitiveSanitationSiteKind kind)
{
    switch (kind) {
        case PrimitiveSanitationSiteKind::DesignatedArea: return "DesignatedArea";
        case PrimitiveSanitationSiteKind::DugPit: return "DugPit";
    }
    return "DesignatedArea";
}

const char* environmentalResidueKindName(EnvironmentalResidueKind kind)
{
    switch (kind) {
        case EnvironmentalResidueKind::HumanWaste: return "HumanWaste";
    }
    return "HumanWaste";
}

} // namespace

WebClientBridge::WebClientBridge() = default;

bool WebClientBridge::newGame(
    const std::string& worldSeedText,
    const std::string& populationSeedText,
    WorldGenerationVersion generationVersion)
{
    worldSeed_ = normalizeWorldSeed(
        parseUnsigned64(worldSeedText, 1));
    populationSeed_ = populationSeedText.empty()
        ? 0
        : static_cast<PopulationSeed>(
            parseUnsigned64(populationSeedText, 0));
    generationVersion_ =
        normalizeWorldGenerationVersion(generationVersion);

    simulation_ = std::make_unique<Simulation>(
        worldSeed_,
        populationSeed_,
        generationVersion_);
    simulation_->setupNewGame();
    return true;
}

void WebClientBridge::runMinutes(int minutes)
{
    if (!simulation_ || minutes <= 0) return;
    simulation_->runMinutes(minutes);
}

std::string WebClientBridge::worldOverviewJson() const
{
    if (!simulation_) return "{\"available\":false}";

    const WorldOverviewObservation overview =
        simulation_->observeWorldOverview();
    const World& world = simulation_->world();
    const WorldGenesisIdentity identity = world.genesisIdentity();

    struct MajorLifeEventItem {
        const Character* character = nullptr;
        const LifeHistoryEntry* event = nullptr;
    };

    std::vector<MajorLifeEventItem> majorEvents;
    for (const Character& character : world.characters) {
        for (const LifeHistoryEntry& event : character.lifeHistory) {
            if (!isMajorObserverLifeEvent(event.type)) continue;
            majorEvents.push_back(MajorLifeEventItem{&character, &event});
        }
    }
    std::sort(
        majorEvents.begin(),
        majorEvents.end(),
        [](const MajorLifeEventItem& a, const MajorLifeEventItem& b) {
            if (a.event->minute != b.event->minute) {
                return a.event->minute > b.event->minute;
            }
            return a.character->id < b.character->id;
        });
    if (majorEvents.size() > 16) majorEvents.resize(16);

    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"worldSeed\":\"" << identity.worldSeed << "\",";
    out << "\"populationSeed\":\"" << identity.populationSeed << "\",";
    out << "\"generationVersion\":" << identity.generationVersion << ",";
    out << "\"minute\":" << overview.minute << ",";
    out << "\"totalResidents\":" << overview.totalResidents << ",";
    out << "\"livingResidents\":" << overview.livingResidents << ",";
    out << "\"deceasedResidents\":" << overview.deceasedResidents << ",";
    out << "\"households\":" << overview.households << ",";
    out << "\"activeCouples\":" << overview.activeCouples << ",";
    out << "\"activePregnancies\":" << overview.activePregnancies << ",";
    out << "\"majorLifeEvents\":" << overview.majorLifeEvents << ",";
    out << "\"majorLifeEventItems\":[";
    for (std::size_t i = 0; i < majorEvents.size(); ++i) {
        if (i != 0) out << ",";
        const MajorLifeEventItem& item = majorEvents[i];
        out << "{";
        out << "\"type\":\"" << lifeEventName(item.event->type) << "\",";
        out << "\"minute\":" << item.event->minute << ",";
        out << "\"residentId\":\"" << item.character->id << "\",";
        out << "\"residentName\":\"" << escapeJson(item.character->name) << "\",";
        out << "\"value\":" << item.event->value << ",";
        out << "\"related\":[";
        for (std::size_t relatedIndex = 0;
             relatedIndex < item.event->relatedCharacters.size();
             ++relatedIndex) {
            if (relatedIndex != 0) out << ",";
            const CharacterId relatedId =
                item.event->relatedCharacters[relatedIndex];
            const Character* related =
                findObservedCharacter(world, relatedId);
            out << "{";
            out << "\"id\":\"" << relatedId << "\",";
            out << "\"name\":\""
                << escapeJson(related ? related->name : std::string{}) << "\"";
            out << "}";
        }
        out << "]";
        out << "}";
    }
    out << "]";
    out << "}";
    return out.str();
}

std::string WebClientBridge::residentsJson() const
{
    if (!simulation_) return "{\"available\":false,\"residents\":[]}";

    const std::vector<ResidentObservation> residents =
        simulation_->observeAllResidents();
    const World& world = simulation_->world();

    std::ostringstream out;
    out << "{\"available\":true,\"residents\":[";
    bool first = true;
    for (const ResidentObservation& resident : residents) {
        if (!first) out << ",";
        first = false;

        GridPos position{};
        const bool hasPosition =
            simulation_->runtimePosition(resident.id, position);
        const Character* character =
            findObservedCharacter(world, resident.id);
        const FamilyObservation family =
            simulation_->observeFamily(resident.id);
        const ResidentPresentationObservation presentation =
            simulation_->observeResidentPresentation(resident.id);

        const TraitProfile traits = character
            ? deriveTraitProfile(character->personality, character->genetics)
            : TraitProfile{};
        const PreferenceProfile preferences = character
            ? derivePreferenceProfile(character->personality, character->genetics)
            : PreferenceProfile{};

        std::vector<RelationshipObservation> relationships =
            resident.relationships;
        std::sort(
            relationships.begin(),
            relationships.end(),
            [](const RelationshipObservation& a, const RelationshipObservation& b) {
                const double aScore = std::max({
                    a.socialBond,
                    a.romancePotential,
                    a.conflict,
                    a.grudge,
                    a.fear});
                const double bScore = std::max({
                    b.socialBond,
                    b.romancePotential,
                    b.conflict,
                    b.grudge,
                    b.fear});
                return aScore > bScore;
            });
        if (relationships.size() > 12) relationships.resize(12);

        std::vector<const MemoryRecord*> memories;
        if (character) {
            memories = character->memory.recallAbove(world.minute, 0.0);
            if (memories.size() > 8) memories.resize(8);
        }

        std::vector<const BeliefRecord*> beliefs;
        if (character) {
            beliefs.reserve(character->beliefs.beliefs.size());
            for (const BeliefRecord& belief : character->beliefs.beliefs) {
                beliefs.push_back(&belief);
            }
            std::sort(
                beliefs.begin(),
                beliefs.end(),
                [](const BeliefRecord* a, const BeliefRecord* b) {
                    return a->confidence > b->confidence;
                });
            if (beliefs.size() > 8) beliefs.resize(8);
        }

        const int ageYears = character && character->hasBirthMinute
            ? ageYearsFromMinutes(character->birthMinute, world.minute)
            : 0;

        out << "{";
        out << "\"id\":\"" << resident.id << "\",";
        out << "\"name\":\"" << escapeJson(resident.name) << "\",";
        out << "\"sex\":\"" << sexName(resident.sex) << "\",";
        out << "\"alive\":" << (character && character->alive ? "true" : "false") << ",";
        out << "\"lifeStage\":\""
            << (character ? lifeStageName(character->lifeStage) : "Unknown") << "\",";
        out << "\"ageYears\":" << ageYears << ",";

        out << "\"activityKind\":\""
            << activityKindName(resident.activityKind) << "\",";
        out << "\"activityLabel\":\""
            << escapeJson(resident.activityLabel) << "\",";
        out << "\"physicalGoal\":\""
            << goalName(resident.physicalGoal) << "\",";
        out << "\"socialIntent\":\""
            << socialIntentName(resident.socialIntent) << "\",";
        out << "\"activityTargetId\":\""
            << resident.activityTargetId << "\",";
        out << "\"activityTargetName\":\""
            << escapeJson(resident.activityTargetName) << "\",";

        out << "\"presentation\":{";
        out << "\"active\":" << (presentation.active ? "true" : "false") << ",";
        out << "\"kind\":\"" << presentationActionKindName(presentation.kind) << "\",";
        out << "\"phase\":\"" << presentationActionPhaseName(presentation.phase) << "\",";
        out << "\"physicalGoal\":\"" << goalName(presentation.physicalGoal) << "\",";
        out << "\"socialIntent\":\"" << socialIntentName(presentation.socialIntent) << "\",";
        out << "\"civilizationIntent\":\"" << civilizationIntentName(presentation.civilizationIntent) << "\",";
        out << "\"parentingAction\":\"" << parentingActionName(presentation.parentingAction) << "\",";
        out << "\"targetResidentId\":\"" << presentation.targetResidentId << "\",";
        out << "\"hasTargetGrid\":" << (presentation.hasTargetGrid ? "true" : "false") << ",";
        out << "\"targetGridX\":" << presentation.targetGrid.x << ",";
        out << "\"targetGridY\":" << presentation.targetGrid.y << ",";
        out << "\"hasObjectTarget\":" << (presentation.hasObjectTarget ? "true" : "false") << ",";
        out << "\"objectId\":\"" << presentation.objectId << "\",";
        out << "\"objectKind\":\"" << objectKindName(presentation.objectKind) << "\",";
        out << "\"emergencyFallback\":" << (presentation.emergencyFallback ? "true" : "false") << ",";
        out << "\"designatedSanitationSite\":" << (presentation.designatedSanitationSite ? "true" : "false") << ",";
        out << "\"sanitationSiteId\":\"" << presentation.sanitationSiteId << "\",";
        out << "\"contextActionToken\":\"" << presentation.contextActionToken << "\",";
        out << "\"durationTicks\":" << presentation.durationTicks;
        out << "},";

        out << "\"emotion\":{";
        if (character) {
            out << "\"joy\":"; appendDouble(out, character->emotion.joy); out << ",";
            out << "\"sadness\":"; appendDouble(out, character->emotion.sadness); out << ",";
            out << "\"anger\":"; appendDouble(out, character->emotion.anger); out << ",";
            out << "\"fear\":"; appendDouble(out, character->emotion.fear); out << ",";
            out << "\"embarrassment\":"; appendDouble(out, character->emotion.embarrassment); out << ",";
            out << "\"pride\":"; appendDouble(out, character->emotion.pride); out << ",";
            out << "\"jealousy\":"; appendDouble(out, character->emotion.jealousy); out << ",";
            out << "\"affection\":"; appendDouble(out, character->emotion.affection); out << ",";
            out << "\"anxiety\":"; appendDouble(out, character->emotion.anxiety); out << ",";
            out << "\"relief\":"; appendDouble(out, character->emotion.relief); out << ",";
            out << "\"grief\":"; appendDouble(out, character->emotion.grief); out << ",";
        }
        out << "\"valence\":"; appendDouble(out, resident.emotionValence); out << ",";
        out << "\"arousal\":"; appendDouble(out, resident.emotionArousal); out << ",";
        out << "\"intensity\":"; appendDouble(out, resident.emotionIntensity);
        out << "},";

        out << "\"needs\":{";
        out << "\"hunger\":"; appendDouble(out, resident.needs.hunger); out << ",";
        out << "\"thirst\":"; appendDouble(out, resident.needs.thirst); out << ",";
        out << "\"sleep\":"; appendDouble(out, resident.needs.sleep); out << ",";
        out << "\"bladder\":"; appendDouble(out, resident.needs.bladder); out << ",";
        out << "\"hygiene\":"; appendDouble(out, resident.needs.hygiene);
        out << "},";

        out << "\"personality\":{";
        if (character) {
            out << "\"introversion\":"; appendDouble(out, character->personality.introversion); out << ",";
            out << "\"conscientiousness\":"; appendDouble(out, character->personality.conscientiousness); out << ",";
            out << "\"openness\":"; appendDouble(out, character->personality.openness); out << ",";
            out << "\"agreeableness\":"; appendDouble(out, character->personality.agreeableness); out << ",";
            out << "\"emotionalStability\":"; appendDouble(out, character->personality.emotionalStability); out << ",";
            out << "\"empathy\":"; appendDouble(out, character->personality.empathy); out << ",";
            out << "\"impulsiveness\":"; appendDouble(out, character->personality.impulsiveness); out << ",";
            out << "\"riskTolerance\":"; appendDouble(out, character->personality.riskTolerance); out << ",";
            out << "\"ambition\":"; appendDouble(out, character->personality.ambition); out << ",";
            out << "\"patience\":"; appendDouble(out, character->personality.patience); out << ",";
            out << "\"sociability\":"; appendDouble(out, character->personality.sociability); out << ",";
            out << "\"curiosity\":"; appendDouble(out, character->personality.curiosity); out << ",";
            out << "\"orderliness\":"; appendDouble(out, character->personality.orderliness); out << ",";
            out << "\"adaptability\":"; appendDouble(out, character->personality.adaptability);
        }
        out << "},";

        out << "\"traits\":{";
        out << "\"resilience\":"; appendDouble(out, traits.resilience); out << ",";
        out << "\"creativity\":"; appendDouble(out, traits.creativity); out << ",";
        out << "\"discipline\":"; appendDouble(out, traits.discipline); out << ",";
        out << "\"compassion\":"; appendDouble(out, traits.compassion); out << ",";
        out << "\"adaptability\":"; appendDouble(out, traits.adaptability); out << ",";
        out << "\"boldness\":"; appendDouble(out, traits.boldness); out << ",";
        out << "\"perseverance\":"; appendDouble(out, traits.perseverance); out << ",";
        out << "\"resourcefulness\":"; appendDouble(out, traits.resourcefulness);
        out << "},";

        out << "\"preferences\":{";
        out << "\"socializing\":"; appendDouble(out, preferences.socializing); out << ",";
        out << "\"solitude\":"; appendDouble(out, preferences.solitude); out << ",";
        out << "\"exploration\":"; appendDouble(out, preferences.exploration); out << ",";
        out << "\"crafting\":"; appendDouble(out, preferences.crafting); out << ",";
        out << "\"gathering\":"; appendDouble(out, preferences.gathering); out << ",";
        out << "\"comfort\":"; appendDouble(out, preferences.comfort); out << ",";
        out << "\"novelty\":"; appendDouble(out, preferences.novelty); out << ",";
        out << "\"order\":"; appendDouble(out, preferences.order);
        out << "},";

        out << "\"relationships\":[";
        for (std::size_t i = 0; i < relationships.size(); ++i) {
            if (i != 0) out << ",";
            const RelationshipObservation& relation = relationships[i];
            out << "{";
            out << "\"targetId\":\"" << relation.targetId << "\",";
            out << "\"targetName\":\"" << escapeJson(relation.targetName) << "\",";
            out << "\"affection\":"; appendDouble(out, relation.affection); out << ",";
            out << "\"trust\":"; appendDouble(out, relation.trust); out << ",";
            out << "\"respect\":"; appendDouble(out, relation.respect); out << ",";
            out << "\"comfort\":"; appendDouble(out, relation.comfort); out << ",";
            out << "\"familiarity\":"; appendDouble(out, relation.familiarity); out << ",";
            out << "\"attraction\":"; appendDouble(out, relation.attraction); out << ",";
            out << "\"romanticInterest\":"; appendDouble(out, relation.romanticInterest); out << ",";
            out << "\"sexualAttraction\":"; appendDouble(out, relation.sexualAttraction); out << ",";
            out << "\"commitment\":"; appendDouble(out, relation.commitment); out << ",";
            out << "\"conflict\":"; appendDouble(out, relation.conflict); out << ",";
            out << "\"jealousy\":"; appendDouble(out, relation.jealousy); out << ",";
            out << "\"fear\":"; appendDouble(out, relation.fear); out << ",";
            out << "\"grudge\":"; appendDouble(out, relation.grudge); out << ",";
            out << "\"socialBond\":"; appendDouble(out, relation.socialBond); out << ",";
            out << "\"romancePotential\":"; appendDouble(out, relation.romancePotential);
            out << "}";
        }
        out << "],";

        const auto appendFamilyMembers = [&](const std::vector<FamilyMemberObservation>& members) {
            out << "[";
            for (std::size_t i = 0; i < members.size(); ++i) {
                if (i != 0) out << ",";
                const FamilyMemberObservation& member = members[i];
                out << "{";
                out << "\"id\":\"" << member.id << "\",";
                out << "\"name\":\"" << escapeJson(member.name) << "\",";
                out << "\"alive\":" << (member.alive ? "true" : "false") << ",";
                out << "\"lifeStage\":\"" << lifeStageName(member.lifeStage) << "\"";
                out << "}";
            }
            out << "]";
        };

        out << "\"family\":{";
        out << "\"householdId\":\"" << family.householdId << "\",";
        out << "\"hasActivePartner\":" << (family.hasActivePartner ? "true" : "false") << ",";
        out << "\"partnerId\":\"" << family.partnerId << "\",";
        out << "\"partnerName\":\"" << escapeJson(family.partnerName) << "\",";
        out << "\"partnerStage\":\"" << romanceStageName(family.partnerStage) << "\",";
        out << "\"cohabitingWithPartner\":" << (family.cohabitingWithPartner ? "true" : "false") << ",";
        out << "\"expectingChild\":" << (family.expectingChild ? "true" : "false") << ",";
        out << "\"pregnancyPartnerId\":\"" << family.pregnancyPartnerId << "\",";
        out << "\"pregnancyPartnerName\":\"" << escapeJson(family.pregnancyPartnerName) << "\",";
        out << "\"parents\":"; appendFamilyMembers(family.parents); out << ",";
        out << "\"children\":"; appendFamilyMembers(family.children); out << ",";
        out << "\"siblings\":"; appendFamilyMembers(family.siblings);
        out << "},";

        out << "\"memories\":[";
        for (std::size_t i = 0; i < memories.size(); ++i) {
            if (i != 0) out << ",";
            const MemoryRecord& memory = *memories[i];
            out << "{";
            out << "\"who\":\"" << memory.who << "\",";
            out << "\"sourceCharacter\":\"" << memory.sourceCharacter << "\",";
            out << "\"what\":\"" << escapeJson(memory.what) << "\",";
            out << "\"where\":\"" << escapeJson(memory.where) << "\",";
            out << "\"minute\":" << memory.minute << ",";
            out << "\"emotionValence\":"; appendDouble(out, memory.emotionValence); out << ",";
            out << "\"emotionIntensity\":"; appendDouble(out, memory.emotionIntensity); out << ",";
            out << "\"importance\":"; appendDouble(out, memory.importance); out << ",";
            out << "\"confidence\":"; appendDouble(out, memory.confidence); out << ",";
            out << "\"effectiveConfidence\":"; appendDouble(out, memory.effectiveConfidence(world.minute)); out << ",";
            out << "\"witnessed\":" << (memory.witnessed ? "true" : "false") << ",";
            out << "\"source\":\"" << memorySourceName(memory.source) << "\"";
            out << "}";
        }
        out << "],";

        out << "\"beliefs\":[";
        for (std::size_t i = 0; i < beliefs.size(); ++i) {
            if (i != 0) out << ",";
            const BeliefRecord& belief = *beliefs[i];
            out << "{";
            out << "\"subject\":\"" << belief.subject << "\",";
            out << "\"proposition\":\"" << escapeJson(belief.proposition) << "\",";
            out << "\"stance\":"; appendDouble(out, belief.stance); out << ",";
            out << "\"confidence\":"; appendDouble(out, belief.confidence); out << ",";
            out << "\"lastUpdatedMinute\":" << belief.lastUpdatedMinute;
            out << "}";
        }
        out << "],";

        out << "\"hasPosition\":" << (hasPosition ? "true" : "false");
        if (hasPosition) {
            out << ",\"gridX\":" << position.x;
            out << ",\"gridY\":" << position.y;
        }
        out << "}";
    }
    out << "]}";
    return out.str();
}

std::string WebClientBridge::worldPresentationJson() const
{
    if (!simulation_) {
        return "{\"available\":false,\"facilities\":[],\"sanitationSites\":[],\"residues\":[]}";
    }

    const CivilizationWorldObservation civilization =
        simulation_->observeCivilizationWorld(0);
    const EnvironmentObservation environment =
        simulation_->observeEnvironment(96);
    const World& world = simulation_->world();

    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"minute\":" << world.minute << ",";

    out << "\"facilities\":[";
    for (std::size_t i = 0; i < civilization.facilities.size(); ++i) {
        if (i != 0) out << ",";
        const CivilizationFacilityObservation& facility =
            civilization.facilities[i];
        out << "{";
        out << "\"id\":\"" << facility.id << "\",";
        out << "\"kind\":\"" << facilityKindName(facility.kind) << "\",";
        out << "\"state\":\"" << facilityStateName(facility.state) << "\",";
        out << "\"gridX\":" << facility.pos.x << ",";
        out << "\"gridY\":" << facility.pos.y << ",";
        out << "\"initiatedBy\":\"" << facility.initiatedBy << "\",";
        out << "\"lastWorkedBy\":\"" << facility.lastWorkedBy << "\",";
        out << "\"startedMinute\":" << facility.startedMinute << ",";
        out << "\"completedMinute\":" << facility.completedMinute << ",";
        out << "\"workProgress\":"; appendDouble(out, facility.workProgress); out << ",";
        out << "\"durability\":"; appendDouble(out, facility.durability); out << ",";
        out << "\"active\":" << (facility.active ? "true" : "false") << ",";
        out << "\"lit\":" << (facility.lit ? "true" : "false") << ",";
        out << "\"heatLevel\":"; appendDouble(out, facility.heatLevel); out << ",";
        out << "\"fuelUnits\":" << facility.fuelUnits << ",";
        out << "\"charcoalUnits\":" << facility.charcoalUnits << ",";
        out << "\"oreUnits\":" << facility.oreUnits << ",";
        out << "\"metalUnits\":" << facility.metalUnits;
        out << "}";
    }
    out << "],";

    out << "\"sanitationSites\":[";
    for (std::size_t i = 0; i < world.primitiveSanitationSites.size(); ++i) {
        if (i != 0) out << ",";
        const PrimitiveSanitationSite& site =
            world.primitiveSanitationSites[i];
        out << "{";
        out << "\"id\":\"" << site.id << "\",";
        out << "\"kind\":\"" << sanitationSiteKindName(site.kind) << "\",";
        out << "\"gridX\":" << site.pos.x << ",";
        out << "\"gridY\":" << site.pos.y << ",";
        out << "\"establishedBy\":\"" << site.establishedBy << "\",";
        out << "\"establishedMinute\":" << site.establishedMinute << ",";
        out << "\"active\":" << (site.active ? "true" : "false") << ",";
        out << "\"useCount\":" << site.useCount << ",";
        out << "\"improvementProgress\":";
        appendDouble(
            out,
            DugSanitationPitWorkRequired > 0.0
                ? std::max(0.0, std::min(
                    1.0,
                    site.improvementWork / DugSanitationPitWorkRequired))
                : 0.0);
        out << "}";
    }
    out << "],";

    out << "\"residues\":[";
    for (std::size_t i = 0; i < environment.residues.size(); ++i) {
        if (i != 0) out << ",";
        const EnvironmentalResidueObservation& residue =
            environment.residues[i];
        out << "{";
        out << "\"id\":\"" << residue.id << "\",";
        out << "\"kind\":\"" << environmentalResidueKindName(residue.kind) << "\",";
        out << "\"gridX\":" << residue.pos.x << ",";
        out << "\"gridY\":" << residue.pos.y << ",";
        out << "\"sourceCharacter\":\"" << residue.sourceCharacter << "\",";
        out << "\"ageMinutes\":" << residue.ageMinutes << ",";
        out << "\"amount\":"; appendDouble(out, residue.amount); out << ",";
        out << "\"intensity\":"; appendDouble(out, residue.intensity); out << ",";
        out << "\"radiusTiles\":" << residue.radiusTiles;
        out << "}";
    }
    out << "],";
    out << "\"aggregateWasteAmount\":"; appendDouble(out, environment.aggregateAmount); out << ",";
    out << "\"peakWasteIntensity\":"; appendDouble(out, environment.peakIntensity);
    out << "}";
    return out.str();
}

std::string WebClientBridge::dynamicEnvironmentJson(
    int centerChunkX,
    int centerChunkY) const
{
    if (!simulation_) return "{\"available\":false}";

    const WorldGenesisIdentity identity =
        simulation_->world().genesisIdentity();
    const DynamicEnvironmentObservation environment =
        deriveDynamicEnvironment(
            identity,
            ChunkCoord{centerChunkX, centerChunkY},
            simulation_->world().minute);

    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"centerChunkX\":" << centerChunkX << ",";
    out << "\"centerChunkY\":" << centerChunkY << ",";
    out << "\"simulationMinute\":" << environment.simulationMinute << ",";
    out << "\"airTemperatureC\":"; appendDouble(out, environment.airTemperatureC); out << ",";
    out << "\"precipitationIntensity01\":"; appendDouble(out, environment.precipitationIntensity01); out << ",";
    out << "\"cloudCover01\":"; appendDouble(out, environment.cloudCover01); out << ",";
    out << "\"windIntensity01\":"; appendDouble(out, environment.windIntensity01); out << ",";
    out << "\"humidity01\":"; appendDouble(out, environment.humidity01); out << ",";
    out << "\"visibility01\":"; appendDouble(out, environment.visibility01); out << ",";
    out << "\"surfaceWetness01\":"; appendDouble(out, environment.surfaceWetness01); out << ",";
    out << "\"precipitationType\":\"" << precipitationTypeName(environment.precipitationType) << "\",";
    out << "\"summary\":\"" << weatherSummaryName(environment.summary) << "\"";
    out << "}";
    return out.str();
}

std::string WebClientBridge::terrainWindowJson(
    int centerChunkX,
    int centerChunkY,
    int radiusChunks) const
{
    if (!simulation_) return "{\"available\":false,\"chunks\":[]}";

    const WorldGenesisIdentity identity =
        simulation_->world().genesisIdentity();
    const int radius = std::max(0, std::min(radiusChunks, 16));

    std::ostringstream out;
    out << "{\"available\":true,";
    out << "\"centerChunkX\":" << centerChunkX << ",";
    out << "\"centerChunkY\":" << centerChunkY << ",";
    out << "\"radiusChunks\":" << radius << ",";
    out << "\"worldSeed\":\"" << identity.worldSeed << "\",";
    out << "\"chunks\":[";

    bool first = true;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            const ChunkCoord coord{
                centerChunkX + x,
                centerChunkY + y};
            const GridPos origin = chunkOriginGrid(coord);
            const GridPos center{
                origin.x + WorldChunkSpanGridCells / 2,
                origin.y + WorldChunkSpanGridCells / 2};

            const ContinuousTerrainSample terrain =
                deriveContinuousTerrainSample(identity, center);
            const HydrologyFacts water =
                deriveHydrologyFacts(identity, coord);
            const ContinuousEcologySample ecology =
                deriveContinuousEcologySample(identity, center);

            if (!first) out << ",";
            first = false;
            out << "{";
            out << "\"x\":" << coord.x << ",";
            out << "\"y\":" << coord.y << ",";
            out << "\"elevation01\":"; appendDouble(out, terrain.elevation01); out << ",";
            out << "\"gradientX\":"; appendDouble(out, terrain.gradientXPerGrid); out << ",";
            out << "\"gradientY\":"; appendDouble(out, terrain.gradientYPerGrid); out << ",";
            out << "\"waterKind\":\"" << surfaceWaterKindName(water.surfaceKind) << "\",";
            out << "\"salinity\":\"" << waterSalinityName(water.salinity) << "\",";
            out << "\"waterAvailability\":"; appendDouble(out, water.surfaceAvailability); out << ",";
            out << "\"biome\":\"" << continuousEcologyBiomeName(ecology.biome) << "\",";
            out << "\"moisture01\":"; appendDouble(out, ecology.moisture01); out << ",";
            out << "\"temperature01\":"; appendDouble(out, ecology.temperature01); out << ",";
            out << "\"forestCoverage01\":"; appendDouble(out, ecology.forestCoverage01); out << ",";
            out << "\"grassCoverage01\":"; appendDouble(out, ecology.grassCoverage01); out << ",";
            out << "\"shrubCoverage01\":"; appendDouble(out, ecology.shrubCoverage01); out << ",";
            out << "\"rockCoverage01\":"; appendDouble(out, ecology.rockCoverage01); out << ",";
            out << "\"wetlandCoverage01\":"; appendDouble(out, ecology.wetlandCoverage01);
            out << "}";
        }
    }

    out << "]}";
    return out.str();
}

} // namespace lifelens
