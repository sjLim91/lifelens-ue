#include "lifelens/WebClientBridge.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

#include "lifelens/ContinuousEcology.h"
#include "lifelens/ContinuousTerrain.h"
#include "lifelens/CivilizationProgression.h"
#include "lifelens/Hydrology.h"
#include "lifelens/HumanTraceReadModel.h"
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

void appendHealthJson(std::ostringstream& out,const HealthState& health)
{
    out << "{";
    out << "\"stage\":\"" << healthStageName(healthStage(health)) << "\",";
    out << "\"functionalCapacity\":"; appendDouble(out,healthFunctionalCapacity01(health)); out << ",";
    out << "\"pathogenLoad\":"; appendDouble(out,health.pathogenLoad); out << ",";
    out << "\"illnessSeverity\":"; appendDouble(out,health.illnessSeverity); out << ",";
    out << "\"immunity\":"; appendDouble(out,health.immunity01); out << ",";
    out << "\"injurySeverity\":"; appendDouble(out,health.injurySeverity); out << ",";
    out << "\"environmentalStress\":"; appendDouble(out,health.environmentalStress01); out << ",";
    out << "\"careKnowledge\":"; appendDouble(out,health.careKnowledge01); out << ",";
    out << "\"infectionEpisodes\":" << health.infectionEpisodes << ",";
    out << "\"recoveryEpisodes\":" << health.recoveryEpisodes << ",";
    out << "\"accidentEpisodes\":" << health.accidentEpisodes << ",";
    out << "\"lastExposureMinute\":" << health.lastExposureMinute << ",";
    out << "\"lastIllnessMinute\":" << health.lastIllnessMinute << ",";
    out << "\"lastRecoveryMinute\":" << health.lastRecoveryMinute << ",";
    out << "\"lastAccidentMinute\":" << health.lastAccidentMinute;
    out << "}";
}

const char* traceMaterialName(MaterialKind kind)
{
    switch (kind) {
        case MaterialKind::Stone: return "Stone";
        case MaterialKind::Flint: return "Flint";
        case MaterialKind::Wood: return "Wood";
        case MaterialKind::Fiber: return "Fiber";
        case MaterialKind::Clay: return "Clay";
        case MaterialKind::Water: return "Water";
        case MaterialKind::PlantFood: return "PlantFood";
        case MaterialKind::Bone: return "Bone";
        case MaterialKind::Hide: return "Hide";
        case MaterialKind::CopperOre: return "CopperOre";
        case MaterialKind::TinOre: return "TinOre";
        case MaterialKind::IronOre: return "IronOre";
        case MaterialKind::Charcoal: return "Charcoal";
        case MaterialKind::CopperMetal: return "CopperMetal";
        case MaterialKind::TinMetal: return "TinMetal";
        case MaterialKind::Bronze: return "Bronze";
        default: return "Unknown";
    }
}

const char* traceFacilityName(FacilityKind kind)
{
    switch (kind) {
        case FacilityKind::PrimitiveStorage: return "PrimitiveStorage";
        case FacilityKind::FirePit: return "FirePit";
        case FacilityKind::WorkSurface: return "WorkSurface";
        case FacilityKind::SleepingPlace: return "SleepingPlace";
        case FacilityKind::Shelter: return "Shelter";
        case FacilityKind::Furnace: return "Furnace";
        case FacilityKind::CultivatedPlot: return "CultivatedPlot";
    }
    return "Unknown";
}

const char* traceFacilityStateName(FacilityState state)
{
    switch (state) {
        case FacilityState::Planned: return "Planned";
        case FacilityState::UnderConstruction: return "UnderConstruction";
        case FacilityState::Operational: return "Operational";
        case FacilityState::Ruined: return "Ruined";
    }
    return "Unknown";
}

void appendHumanTraces(std::ostringstream& out, const HumanTraceWindowObservation& traces)
{
    out << "{\"total\":" << traces.total << ",\"entries\":[";
    bool first = true;
    for (const auto& trace : traces.entries) {
        if (!first) out << ",";
        first = false;
        out << "{\"id\":\"" << trace.id << "\",\"gridX\":" << trace.pos.x
            << ",\"gridY\":" << trace.pos.y;
        if (trace.kind == HumanTraceKind::ResourceUse) {
            out << ",\"kind\":\"ResourceUse\",\"material\":\"" << traceMaterialName(trace.material)
                << "\",\"quantity\":" << trace.quantity
                << ",\"baselineQuantity\":" << trace.baselineQuantity
                << ",\"renewable\":" << (trace.renewable ? "true" : "false");
        } else if (trace.kind == HumanTraceKind::Residue) {
            out << ",\"kind\":\"Residue\",\"sourceResidentId\":\"" << trace.sourceCharacter << "\""
                << ",\"amount\":"; appendDouble(out, trace.amount);
            out << ",\"intensity\":"; appendDouble(out, trace.intensity);
            out << ",\"radiusTiles\":" << trace.radiusTiles;
        } else {
            out << ",\"kind\":\"Facility\",\"sourceResidentId\":\"" << trace.sourceCharacter << "\""
                << ",\"facilityKind\":\"" << traceFacilityName(trace.facilityKind)
                << "\",\"state\":\"" << traceFacilityStateName(trace.facilityState) << "\""
                << ",\"progress01\":"; appendDouble(out, trace.progress01);
            out << ",\"deliveredMaterialUnits\":" << trace.deliveredMaterialUnits
                << ",\"requiredMaterialUnits\":" << trace.requiredMaterialUnits
                << ",\"active\":" << (trace.active ? "true" : "false")
                << ",\"lit\":" << (trace.lit ? "true" : "false")
                << ",\"cropPlanted\":" << (trace.cropPlanted ? "true" : "false")
                << ",\"cropGrowth01\":"; appendDouble(out, trace.cropGrowth01);
            out << ",\"cropMoisture01\":"; appendDouble(out, trace.cropMoisture01);
            out << ",\"cropCare01\":"; appendDouble(out, trace.cropCare01);
            out << ",\"cropHarvestUnits\":" << trace.cropHarvestUnits;
        }
        out << "}";
    }
    out << "]}";
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

const char* kinshipName(KinshipType type)
{
    switch(type){
        case KinshipType::Self: return "Self";
        case KinshipType::Parent: return "Parent";
        case KinshipType::Child: return "Child";
        case KinshipType::Sibling: return "Sibling";
        case KinshipType::HalfSibling: return "HalfSibling";
        case KinshipType::Spouse: return "Spouse";
        case KinshipType::Grandparent: return "Grandparent";
        case KinshipType::Grandchild: return "Grandchild";
        case KinshipType::InLaw: return "InLaw";
        case KinshipType::Unrelated:
        default: return "Unrelated";
    }
}

const char* pregnancyStageName(PregnancyStage stage)
{
    switch(stage){
        case PregnancyStage::FirstTrimester: return "FirstTrimester";
        case PregnancyStage::SecondTrimester: return "SecondTrimester";
        case PregnancyStage::ThirdTrimester: return "ThirdTrimester";
        case PregnancyStage::Due: return "Due";
        case PregnancyStage::Completed: return "Completed";
    }
    return "Completed";
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

const char* itemKindName(ItemKind kind)
{
    switch(kind){
        case ItemKind::RawMaterial: return "RawMaterial";
        case ItemKind::SharpFlake: return "SharpFlake";
        case ItemKind::StoneCuttingTool: return "StoneCuttingTool";
        case ItemKind::Cordage: return "Cordage";
        case ItemKind::SimpleContainer: return "SimpleContainer";
        case ItemKind::FuelBundle: return "FuelBundle";
        case ItemKind::DiggingStick: return "DiggingStick";
        case ItemKind::StoneHammer: return "StoneHammer";
        case ItemKind::BronzeAxe: return "BronzeAxe";
        case ItemKind::BronzePick: return "BronzePick";
    }
    return "RawMaterial";
}

const char* techniqueIdName(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake: return "SharpFlake";
        case TechniqueId::ChippedStoneTool: return "ChippedStoneTool";
        case TechniqueId::FireMaking: return "FireMaking";
        case TechniqueId::FiberCordage: return "FiberCordage";
        case TechniqueId::SimpleContainer: return "SimpleContainer";
        case TechniqueId::DesignatedSanitationArea: return "DesignatedSanitationArea";
        case TechniqueId::DugSanitationPit: return "DugSanitationPit";
        case TechniqueId::PrimitiveStorage: return "PrimitiveStorage";
        case TechniqueId::DiggingStick: return "DiggingStick";
        case TechniqueId::StoneHammer: return "StoneHammer";
        case TechniqueId::CopperSmelting: return "CopperSmelting";
        case TechniqueId::Cultivation: return "Cultivation";
        case TechniqueId::TinSmelting: return "TinSmelting";
        case TechniqueId::BronzeAlloying: return "BronzeAlloying";
        case TechniqueId::BronzeAxe: return "BronzeAxe";
        case TechniqueId::BronzePick: return "BronzePick";
        case TechniqueId::None:
        default: return "None";
    }
}

const char* knowledgeLevelName(KnowledgeLevel level)
{
    switch(level){
        case KnowledgeLevel::Observed: return "Observed";
        case KnowledgeLevel::Hypothesized: return "Hypothesized";
        case KnowledgeLevel::Understood: return "Understood";
        case KnowledgeLevel::Reproducible: return "Reproducible";
        case KnowledgeLevel::Practiced: return "Practiced";
        case KnowledgeLevel::Mastered: return "Mastered";
        case KnowledgeLevel::Unknown:
        default: return "Unknown";
    }
}

const char* civilizationKnowledgeSourceName(CivilizationKnowledgeSource source)
{
    switch(source){
        case CivilizationKnowledgeSource::SelfDiscovery: return "SelfDiscovery";
        case CivilizationKnowledgeSource::DirectWitness: return "DirectWitness";
        case CivilizationKnowledgeSource::Teaching: return "Teaching";
        case CivilizationKnowledgeSource::Unknown:
        default: return "Unknown";
    }
}

const char* socialEventTypeName(SocialEventType type)
{
    switch(type){
        case SocialEventType::PositiveInteraction: return "PositiveInteraction";
        case SocialEventType::Help: return "Help";
        case SocialEventType::Comfort: return "Comfort";
        case SocialEventType::Conflict: return "Conflict";
        case SocialEventType::Betrayal: return "Betrayal";
        case SocialEventType::Rejection: return "Rejection";
        case SocialEventType::Apology: return "Apology";
        case SocialEventType::Intimacy: return "Intimacy";
        case SocialEventType::Commitment: return "Commitment";
    }
    return "PositiveInteraction";
}

const char* socialPresentationLevelName(SocialPresentationLevel level)
{
    switch(level){
        case SocialPresentationLevel::Everyday: return "Everyday";
        case SocialPresentationLevel::Meaningful: return "Meaningful";
        case SocialPresentationLevel::Important: return "Important";
    }
    return "Everyday";
}

const char* sanitationSiteKindName(PrimitiveSanitationSiteKind kind)
{
    switch(kind){
        case PrimitiveSanitationSiteKind::DesignatedArea: return "DesignatedArea";
        case PrimitiveSanitationSiteKind::DugPit: return "DugPit";
    }
    return "DesignatedArea";
}

const char* seasonSummaryName(SeasonSummary season)
{
    switch(season){
        case SeasonSummary::Spring: return "Spring";
        case SeasonSummary::Summer: return "Summer";
        case SeasonSummary::Autumn: return "Autumn";
        case SeasonSummary::Winter: return "Winter";
    }
    return "Spring";
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

void appendResidentPresentationJson(
    std::ostringstream& out,
    const ResidentPresentationObservation& presentation)
{
    out << "{";
    out << "\"active\":" << (presentation.active ? "true" : "false") << ",";
    out << "\"kind\":\"" << presentationActionKindName(presentation.kind) << "\",";
    out << "\"phase\":\"" << presentationActionPhaseName(presentation.phase) << "\",";
    out << "\"physicalGoal\":\"" << goalName(presentation.physicalGoal) << "\",";
    out << "\"socialIntent\":\"" << socialIntentName(presentation.socialIntent) << "\",";
    out << "\"civilizationIntent\":\"" << civilizationIntentName(presentation.civilizationIntent) << "\",";
    out << "\"civilizationMaterial\":\"" << materialName(presentation.civilizationMaterial) << "\",";
    out << "\"civilizationItem\":\"" << itemKindName(presentation.civilizationItem) << "\",";
    out << "\"civilizationTechnique\":\"" << techniqueIdName(presentation.civilizationTechnique) << "\",";
    out << "\"civilizationQuantity\":" << presentation.civilizationQuantity << ",";
    out << "\"civilizationResourceNode\":\"" << presentation.civilizationResourceNode << "\",";
    out << "\"civilizationStorage\":\"" << presentation.civilizationStorage << "\",";
    out << "\"facilityAction\":\"" << facilityBuildActionName(presentation.facilityAction) << "\",";
    out << "\"facilityId\":\"" << presentation.facilityId << "\",";
    out << "\"facilityKind\":\"" << traceFacilityName(presentation.facilityKind) << "\",";
    out << "\"parentingAction\":\"" << parentingActionName(presentation.parentingAction) << "\",";
    out << "\"knowledgeTeachingTechnique\":\"" << techniqueIdName(presentation.knowledgeTeachingTechnique) << "\",";
    out << "\"issuedMinute\":" << presentation.issuedMinute << ",";
    out << "\"targetResidentId\":\"" << presentation.targetResidentId << "\",";
    out << "\"hasTargetGrid\":" << (presentation.hasTargetGrid ? "true" : "false") << ",";
    out << "\"targetGridX\":" << presentation.targetGrid.x << ",";
    out << "\"targetGridY\":" << presentation.targetGrid.y << ",";
    out << "\"hasObjectTarget\":" << (presentation.hasObjectTarget ? "true" : "false") << ",";
    out << "\"objectId\":\"" << presentation.objectId << "\",";
    out << "\"objectKind\":\"" << objectKindName(presentation.objectKind) << "\",";
    out << "\"emergencyFallback\":" << (presentation.emergencyFallback ? "true" : "false") << ",";
    out << "\"directNaturalWaterSource\":" << (presentation.directNaturalWaterSource ? "true" : "false") << ",";
    out << "\"designatedSanitationSite\":" << (presentation.designatedSanitationSite ? "true" : "false") << ",";
    out << "\"sanitationSiteId\":\"" << presentation.sanitationSiteId << "\",";
    out << "\"contextActionToken\":\"" << presentation.contextActionToken << "\",";
    out << "\"durationTicks\":" << presentation.durationTicks;
    out << "}";
}

std::string civilizationWorldObservationJson(
    const CivilizationWorldObservation& world,
    const SocietyWorldObservation& society)
{
    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"minute\":" << world.minute << ",";
    out << "\"society\":{";
    out << "\"livingResidentCount\":" << society.livingResidentCount << ",";
    out << "\"specializedResidentCount\":" << society.specializedResidentCount << ",";
    out << "\"educatorCount\":" << society.educatorCount << ",";
    out << "\"producerCount\":" << society.producerCount << ",";
    out << "\"caregiverCount\":" << society.caregiverCount << ",";
    out << "\"storekeeperCount\":" << society.storekeeperCount << ",";
    out << "\"recentTeachingReceipts\":" << society.recentTeachingReceipts << ",";
    out << "\"exchangeFactCount\":" << society.exchangeFactCount << ",";
    out << "\"activeInstitutionCount\":" << society.activeInstitutionCount << ",";
    out << "\"recordStage\":\"" << collectiveRecordStageName(society.recordStage) << "\",";
    out << "\"residents\":[";
    for(std::size_t i=0;i<society.residents.size();++i){
        if(i!=0) out << ",";
        const ResidentSocietyStatus& resident=society.residents[i];
        out << "{";
        out << "\"residentId\":\"" << resident.residentId << "\",";
        out << "\"role\":\"" << societyRoleName(resident.role) << "\",";
        out << "\"roleStrength01\":"; appendDouble(out,resident.roleStrength01); out << ",";
        out << "\"practicedTechnologyCount\":" << resident.practicedTechnologyCount << ",";
        out << "\"successfulTechniqueUses\":" << resident.successfulTechniqueUses;
        out << "}";
    }
    out << "],";
    out << "\"demands\":[";
    for(std::size_t i=0;i<society.demands.size();++i){
        if(i!=0) out << ",";
        const SocietyDemandSignal& demand=society.demands[i];
        out << "{";
        out << "\"material\":\"" << traceMaterialName(demand.material) << "\",";
        out << "\"desiredUnits\":" << demand.desiredUnits << ",";
        out << "\"availableUnits\":" << demand.availableUnits << ",";
        out << "\"deficitUnits\":" << demand.deficitUnits << ",";
        out << "\"demand01\":"; appendDouble(out,demand.demand01);
        out << "}";
    }
    out << "],";
    out << "\"institutions\":[";
    for(std::size_t i=0;i<society.institutions.size();++i){
        if(i!=0) out << ",";
        const SocietyInstitutionStatus& institution=society.institutions[i];
        out << "{";
        out << "\"kind\":\"" << societyInstitutionKindName(institution.kind) << "\",";
        out << "\"active\":" << (institution.active ? "true" : "false") << ",";
        out << "\"strength01\":"; appendDouble(out,institution.strength01); out << ",";
        out << "\"evidenceCount\":" << institution.evidenceCount;
        out << "}";
    }
    out << "]},";

    out << "\"resourceNodeCount\":" << world.resourceNodeCount << ",";
    out << "\"depletedResourceNodeCount\":" << world.depletedResourceNodeCount << ",";
    out << "\"totalResourceUnits\":" << world.totalResourceUnits << ",";
    out << "\"storageSiteCount\":" << world.storageSiteCount << ",";
    out << "\"totalStoredUnits\":" << world.totalStoredUnits << ",";
    out << "\"facilityCount\":" << world.facilityCount << ",";
    out << "\"plannedFacilityCount\":" << world.plannedFacilityCount << ",";
    out << "\"underConstructionFacilityCount\":" << world.underConstructionFacilityCount << ",";
    out << "\"operationalFacilityCount\":" << world.operationalFacilityCount << ",";
    out << "\"techniqueFactCount\":" << world.techniqueFactCount << ",";
    out << "\"transmissionReceiptCount\":" << world.transmissionReceiptCount << ",";
    out << "\"uniqueKnownTechniqueTypes\":" << world.uniqueKnownTechniqueTypes << ",";
    out << "\"uniqueReproducibleTechniqueTypes\":" << world.uniqueReproducibleTechniqueTypes << ",";
    out << "\"knownTechniqueOwners\":" << world.knownTechniqueOwners << ",";
    out << "\"reproducibleTechniqueOwners\":" << world.reproducibleTechniqueOwners << ",";
    out << "\"commonTechnologyCount\":" << world.commonTechnologyCount << ",";
    out << "\"decliningTechnologyCount\":" << world.decliningTechnologyCount << ",";
    out << "\"lostTechnologyCount\":" << world.lostTechnologyCount << ",";
    out << "\"establishedTechnologyCount\":" << world.establishedTechnologyCount << ",";
    out << "\"contestedTechnologyCount\":" << world.contestedTechnologyCount << ",";
    out << "\"resistedTechnologyCount\":" << world.resistedTechnologyCount << ",";
    out << "\"activeTransformationCount\":" << world.activeTransformationCount << ",";

    out << "\"technologyPopulation\":[";
    for(std::size_t i=0;i<world.technologyPopulation.size();++i){
        if(i!=0) out << ",";
        const CivilizationTechnologyPopulationStatus& technology=
            world.technologyPopulation[i];
        out << "{";
        out << "\"technology\":\"" << technologyIdName(technology.technology) << "\",";
        out << "\"state\":\"" << technologyPopulationStateName(technology.state) << "\",";
        out << "\"livingKnowerCount\":" << technology.livingKnowerCount << ",";
        out << "\"reproducibleKnowerCount\":" << technology.reproducibleKnowerCount << ",";
        out << "\"operationalResidentCount\":" << technology.operationalResidentCount << ",";
        out << "\"adoptedResidentCount\":" << technology.adoptedResidentCount << ",";
        out << "\"evaluatingResidentCount\":" << technology.evaluatingResidentCount << ",";
        out << "\"adoptingResidentCount\":" << technology.adoptingResidentCount << ",";
        out << "\"resistantResidentCount\":" << technology.resistantResidentCount << ",";
        out << "\"successfulUseCount\":" << technology.successfulUseCount << ",";
        out << "\"diffusion01\":"; appendDouble(out,technology.diffusion01); out << ",";
        out << "\"adoptionRatio01\":"; appendDouble(out,technology.adoptionRatio01); out << ",";
        out << "\"meanAcceptance01\":"; appendDouble(out,technology.meanAcceptance01); out << ",";
        out << "\"adoptionState\":\"" << technologySocialAdoptionStateName(technology.adoptionState) << "\",";
        out << "\"historicallyKnown\":" << (technology.historicallyKnown ? "true" : "false") << ",";
        out << "\"historicalFactCount\":" << technology.historicalFactCount << ",";
        out << "\"firstEvidenceMinute\":" << technology.firstEvidenceMinute << ",";
        out << "\"latestEvidenceMinute\":" << technology.latestEvidenceMinute;
        out << "}";
    }
    out << "],";

    out << "\"transformations\":[";
    for(std::size_t i=0;i<world.transformations.size();++i){
        if(i!=0) out << ",";
        const CivilizationTransformationStatus& transformation=
            world.transformations[i];
        out << "{";
        out << "\"transformation\":\"" << civilizationTransformationName(transformation.transformation) << "\",";
        out << "\"active\":" << (transformation.active ? "true" : "false") << ",";
        out << "\"magnitude01\":"; appendDouble(out,transformation.magnitude01); out << ",";
        out << "\"evidenceCount\":" << transformation.evidenceCount << ",";
        out << "\"supportingTechnologyCount\":" << transformation.supportingTechnologyCount;
        out << "}";
    }
    out << "],";

    out << "\"resources\":[";
    for (std::size_t i = 0; i < world.resources.size(); ++i) {
        if (i != 0) out << ",";
        const CivilizationResourceObservation& resource = world.resources[i];
        out << "{";
        out << "\"id\":\"" << resource.id << "\",";
        out << "\"gridX\":" << resource.pos.x << ",";
        out << "\"gridY\":" << resource.pos.y << ",";
        out << "\"hasAccessGrid\":" << (resource.hasAccessPos ? "true" : "false") << ",";
        out << "\"accessGridX\":" << resource.accessPos.x << ",";
        out << "\"accessGridY\":" << resource.accessPos.y << ",";
        out << "\"material\":\"" << traceMaterialName(resource.material) << "\",";
        out << "\"quantity\":" << resource.quantity << ",";
        out << "\"maxQuantity\":" << resource.maxQuantity << ",";
        out << "\"renewable\":" << (resource.renewable ? "true" : "false") << ",";
        out << "\"regenerationPerDay\":" << resource.regenerationPerDay;
        out << "}";
    }
    out << "],";

    out << "\"storages\":[";
    for (std::size_t i = 0; i < world.storages.size(); ++i) {
        if (i != 0) out << ",";
        const CivilizationStorageObservation& storage = world.storages[i];
        out << "{";
        out << "\"id\":\"" << storage.id << "\",";
        out << "\"gridX\":" << storage.pos.x << ",";
        out << "\"gridY\":" << storage.pos.y << ",";
        out << "\"totalUnits\":" << storage.totalUnits << ",";
        out << "\"inventory\":[";
        for (std::size_t j = 0; j < storage.inventory.size(); ++j) {
            if (j != 0) out << ",";
            const CivilizationItemObservation& item = storage.inventory[j];
            out << "{";
            out << "\"item\":\"" << itemKindName(item.item) << "\",";
            out << "\"material\":\"" << traceMaterialName(item.material) << "\",";
            out << "\"quantity\":" << item.quantity << ",";
            out << "\"quality\":"; appendDouble(out, item.quality); out << ",";
            out << "\"durability\":"; appendDouble(out, item.durability);
            out << "}";
        }
        out << "]}";
    }
    out << "],";

    out << "\"facilities\":[";
    for (std::size_t i = 0; i < world.facilities.size(); ++i) {
        if (i != 0) out << ",";
        const CivilizationFacilityObservation& facility = world.facilities[i];
        out << "{";
        out << "\"id\":\"" << facility.id << "\",";
        out << "\"kind\":\"" << traceFacilityName(facility.kind) << "\",";
        out << "\"state\":\"" << traceFacilityStateName(facility.state) << "\",";
        out << "\"gridX\":" << facility.pos.x << ",";
        out << "\"gridY\":" << facility.pos.y << ",";
        out << "\"initiatedBy\":\"" << facility.initiatedBy << "\",";
        out << "\"lastWorkedBy\":\"" << facility.lastWorkedBy << "\",";
        out << "\"startedMinute\":" << facility.startedMinute << ",";
        out << "\"completedMinute\":" << facility.completedMinute << ",";
        out << "\"constructionWork\":"; appendDouble(out, facility.constructionWork); out << ",";
        out << "\"requiredWork\":"; appendDouble(out, facility.requiredWork); out << ",";
        out << "\"workProgress\":"; appendDouble(out, facility.workProgress); out << ",";
        out << "\"durability\":"; appendDouble(out, facility.durability); out << ",";
        out << "\"active\":" << (facility.active ? "true" : "false") << ",";
        out << "\"linkedStorage\":\"" << facility.linkedStorage << "\",";
        out << "\"requiredMaterialUnits\":" << facility.requiredMaterialUnits << ",";
        out << "\"deliveredMaterialUnits\":" << facility.deliveredMaterialUnits << ",";
        out << "\"fuelUnits\":" << facility.fuelUnits << ",";
        out << "\"charcoalUnits\":" << facility.charcoalUnits << ",";
        out << "\"oreUnits\":" << facility.oreUnits << ",";
        out << "\"metalUnits\":" << facility.metalUnits << ",";
        out << "\"furnaceChargeMaterial\":\"" << traceMaterialName(facility.furnaceChargeMaterial) << "\",";
        out << "\"furnaceOutputMaterial\":\"" << traceMaterialName(facility.furnaceOutputMaterial) << "\",";
        out << "\"furnaceOutputPerCharge\":" << facility.furnaceOutputPerCharge << ",";
        out << "\"heatLevel\":"; appendDouble(out, facility.heatLevel); out << ",";
        out << "\"lit\":" << (facility.lit ? "true" : "false") << ",";
        out << "\"burnMinutesRemaining\":" << facility.burnMinutesRemaining << ",";
        out << "\"lastFireMinute\":" << facility.lastFireMinute << ",";
        out << "\"cropPlanted\":" << (facility.cropPlanted ? "true" : "false") << ",";
        out << "\"cropPlantedMinute\":" << facility.cropPlantedMinute << ",";
        out << "\"cropGrowth01\":"; appendDouble(out, facility.cropGrowth01); out << ",";
        out << "\"cropMoisture01\":"; appendDouble(out, facility.cropMoisture01); out << ",";
        out << "\"cropCare01\":"; appendDouble(out, facility.cropCare01); out << ",";
        out << "\"cropHarvestUnits\":" << facility.cropHarvestUnits << ",";
        out << "\"lastCultivationMinute\":" << facility.lastCultivationMinute << ",";
        out << "\"requirements\":[";
        for (std::size_t j = 0; j < facility.requirements.size(); ++j) {
            if (j != 0) out << ",";
            const CivilizationFacilityRequirementObservation& requirement = facility.requirements[j];
            out << "{";
            out << "\"material\":\"" << traceMaterialName(requirement.material) << "\",";
            out << "\"required\":" << requirement.required << ",";
            out << "\"delivered\":" << requirement.delivered;
            out << "}";
        }
        out << "]}";
    }
    out << "],";

    out << "\"recentDiscoveries\":[";
    for (std::size_t i = 0; i < world.recentDiscoveries.size(); ++i) {
        if (i != 0) out << ",";
        const CivilizationDiscoveryObservation& discovery = world.recentDiscoveries[i];
        out << "{";
        out << "\"factId\":\"" << discovery.factId << "\",";
        out << "\"technique\":\"" << techniqueIdName(discovery.technique) << "\",";
        out << "\"discovererId\":\"" << discovery.discovererId << "\",";
        out << "\"discovererName\":\"" << escapeJson(discovery.discovererName) << "\",";
        out << "\"minute\":" << discovery.minute << ",";
        out << "\"rediscovery\":" << (discovery.rediscovery ? "true" : "false") << ",";
        out << "\"discoveryOrdinal\":" << discovery.discoveryOrdinal << ",";
        out << "\"recipientCount\":" << discovery.recipientCount << ",";
        out << "\"livingKnowerCount\":" << discovery.livingKnowerCount;
        out << "}";
    }
    out << "]";
    out << "}";
    return out.str();
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
    const WorldGenesisIdentity identity =
        simulation_->world().genesisIdentity();

    std::size_t exposedResidents=0;
    std::size_t illResidents=0;
    std::size_t injuredResidents=0;
    std::size_t criticalResidents=0;
    double immunityTotal=0.0;
    std::size_t healthPopulation=0;
    for(const Character& character:simulation_->world().characters){
        if(!character.alive) continue;
        ++healthPopulation;
        immunityTotal+=character.health.immunity01;
        switch(healthStage(character.health)){
            case HealthStage::Exposed: ++exposedResidents; break;
            case HealthStage::Ill:
            case HealthStage::Recovering: ++illResidents; break;
            case HealthStage::Injured: ++injuredResidents; break;
            case HealthStage::Critical: ++criticalResidents; break;
            case HealthStage::Well: break;
        }
    }

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
    out << "\"lifeStages\":{";
    out << "\"baby\":" << overview.lifeStages.baby << ",";
    out << "\"toddler\":" << overview.lifeStages.toddler << ",";
    out << "\"child\":" << overview.lifeStages.child << ",";
    out << "\"teen\":" << overview.lifeStages.teen << ",";
    out << "\"youngAdult\":" << overview.lifeStages.youngAdult << ",";
    out << "\"adult\":" << overview.lifeStages.adult << ",";
    out << "\"middleAge\":" << overview.lifeStages.middleAge << ",";
    out << "\"elderly\":" << overview.lifeStages.elderly;
    out << "},";
    out << "\"households\":" << overview.households << ",";
    out << "\"activeCouples\":" << overview.activeCouples << ",";
    out << "\"datingCouples\":" << overview.datingCouples << ",";
    out << "\"engagedCouples\":" << overview.engagedCouples << ",";
    out << "\"marriedCouples\":" << overview.marriedCouples << ",";
    out << "\"separatedCouples\":" << overview.separatedCouples << ",";
    out << "\"activePregnancies\":" << overview.activePregnancies << ",";
    out << "\"majorLifeEvents\":" << overview.majorLifeEvents << ",";
    out << "\"health\":{";
    out << "\"exposedResidents\":" << exposedResidents << ",";
    out << "\"illResidents\":" << illResidents << ",";
    out << "\"injuredResidents\":" << injuredResidents << ",";
    out << "\"criticalResidents\":" << criticalResidents << ",";
    out << "\"meanImmunity\":";
    appendDouble(
        out,
        healthPopulation>0
            ? immunityTotal/static_cast<double>(healthPopulation)
            : 0.0);
    out << "}";
    out << "}";
    return out.str();
}

std::string WebClientBridge::residentRuntimeJson() const
{
    if (!simulation_) return "{\"available\":false,\"residents\":[]}";

    const World& world = simulation_->world();
    std::ostringstream out;
    out << "{\"available\":true,\"residents\":[";

    bool first = true;
    for (const Character& character : world.characters) {
        if (!first) out << ",";
        first = false;

        const ResidentPresentationObservation presentation =
            simulation_->observeResidentPresentation(character.id);
        GridPos position{};
        const bool hasPosition =
            simulation_->runtimePosition(character.id, position);

        std::string activityKind = "Idle";
        Goal physicalGoal = Goal::Idle;
        SocialIntent socialIntent = SocialIntent::None;
        CharacterId activityTargetId = 0;
        std::string activityLabel = "Idle";
        std::string activityTargetName;

        const auto applyPresentationTarget = [&]() {
            activityTargetId = presentation.targetResidentId;
            if (const Character* target =
                    findObservedCharacter(world, activityTargetId)) {
                activityTargetName = target->name;
            }
        };

        if (presentation.active) {
            activityKind = presentationActionKindName(presentation.kind);
            switch (presentation.kind) {
                case PresentationActionKind::Physical:
                    physicalGoal = presentation.physicalGoal;
                    activityLabel = goalName(physicalGoal);
                    break;
                case PresentationActionKind::Social:
                    socialIntent = presentation.socialIntent;
                    activityLabel = socialIntentName(socialIntent);
                    applyPresentationTarget();
                    break;
                case PresentationActionKind::Civilization:
                    activityLabel =
                        civilizationIntentName(presentation.civilizationIntent);
                    break;
                case PresentationActionKind::Parenting:
                    activityLabel =
                        parentingActionName(presentation.parentingAction);
                    applyPresentationTarget();
                    break;
                case PresentationActionKind::KnowledgeTeaching:
                    activityLabel = "KnowledgeTeaching";
                    applyPresentationTarget();
                    break;
                case PresentationActionKind::None:
                default:
                    activityKind = "Idle";
                    activityLabel = "Idle";
                    break;
            }
        }

        const int ageYears = character.hasBirthMinute
            ? ageYearsFromMinutes(character.birthMinute, world.minute)
            : 0;

        out << "{";
        out << "\"id\":\"" << character.id << "\",";
        out << "\"name\":\"" << escapeJson(character.name) << "\",";
        out << "\"sex\":\"" << sexName(character.sex) << "\",";
        out << "\"alive\":" << (character.alive ? "true" : "false") << ",";
        out << "\"lifeStage\":\"" << lifeStageName(character.lifeStage) << "\",";
        out << "\"ageYears\":" << ageYears << ",";
        out << "\"hasBirthMinute\":" << (character.hasBirthMinute ? "true" : "false") << ",";
        out << "\"birthMinute\":" << character.birthMinute << ",";
        out << "\"deathMinute\":" << character.deathMinute << ",";
        out << "\"activityKind\":\"" << escapeJson(activityKind) << "\",";
        out << "\"activityLabel\":\"" << escapeJson(activityLabel) << "\",";
        out << "\"physicalGoal\":\"" << goalName(physicalGoal) << "\",";
        out << "\"socialIntent\":\"" << socialIntentName(socialIntent) << "\",";
        out << "\"activityTargetId\":\"" << activityTargetId << "\",";
        out << "\"activityTargetName\":\"" << escapeJson(activityTargetName) << "\",";
        out << "\"presentation\":";
        appendResidentPresentationJson(out, presentation);
        out << ",";

        out << "\"emotion\":{";
        out << "\"joy\":"; appendDouble(out, character.emotion.joy); out << ",";
        out << "\"sadness\":"; appendDouble(out, character.emotion.sadness); out << ",";
        out << "\"anger\":"; appendDouble(out, character.emotion.anger); out << ",";
        out << "\"fear\":"; appendDouble(out, character.emotion.fear); out << ",";
        out << "\"embarrassment\":"; appendDouble(out, character.emotion.embarrassment); out << ",";
        out << "\"pride\":"; appendDouble(out, character.emotion.pride); out << ",";
        out << "\"jealousy\":"; appendDouble(out, character.emotion.jealousy); out << ",";
        out << "\"affection\":"; appendDouble(out, character.emotion.affection); out << ",";
        out << "\"anxiety\":"; appendDouble(out, character.emotion.anxiety); out << ",";
        out << "\"relief\":"; appendDouble(out, character.emotion.relief); out << ",";
        out << "\"grief\":"; appendDouble(out, character.emotion.grief); out << ",";
        out << "\"valence\":"; appendDouble(out, character.emotion.valence); out << ",";
        out << "\"arousal\":"; appendDouble(out, character.emotion.arousal); out << ",";
        out << "\"intensity\":"; appendDouble(out, character.emotion.intensity());
        out << "},";

        out << "\"needs\":{";
        out << "\"hunger\":"; appendDouble(out, character.needs.hunger); out << ",";
        out << "\"thirst\":"; appendDouble(out, character.needs.thirst); out << ",";
        out << "\"sleep\":"; appendDouble(out, character.needs.sleep); out << ",";
        out << "\"bladder\":"; appendDouble(out, character.needs.bladder); out << ",";
        out << "\"hygiene\":"; appendDouble(out, character.needs.hygiene);
        out << "},";

        out << "\"health\":";
        appendHealthJson(out,character.health);
        out << ",";

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
        const ResidentCivilizationObservation civilization =
            simulation_->observeResidentCivilization(resident.id);
        const Household* household =
            simulation_->households().householdOf(resident.id);

        const PregnancyState* pregnancy = simulation_->pregnancies().activeFor(resident.id);
        bool pregnancyAsGestationalParent = pregnancy != nullptr;
        if (pregnancy == nullptr) {
            for (const PregnancyState& candidate : simulation_->pregnancies().all()) {
                if (candidate.active() && candidate.geneticPartner == resident.id) {
                    pregnancy = &candidate;
                    pregnancyAsGestationalParent = false;
                    break;
                }
            }
        }

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
        out << "\"hasBirthMinute\":" << (character && character->hasBirthMinute ? "true" : "false") << ",";
        out << "\"birthMinute\":" << (character ? character->birthMinute : 0) << ",";
        out << "\"deathMinute\":" << (character ? character->deathMinute : -1) << ",";

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

        out << "\"presentation\":";
        appendResidentPresentationJson(out, presentation);
        out << ",";

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

        out << "\"health\":";
        if(character) appendHealthJson(out,character->health);
        else appendHealthJson(out,HealthState{});
        out << ",";

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

        out << "\"genetics\":{";
        if (character) {
            out << "\"faceShape\":"; appendDouble(out, character->genetics.faceShape); out << ",";
            out << "\"eyePigment\":"; appendDouble(out, character->genetics.eyePigment); out << ",";
            out << "\"hairPigment\":"; appendDouble(out, character->genetics.hairPigment); out << ",";
            out << "\"skinTone\":"; appendDouble(out, character->genetics.skinTone); out << ",";
            out << "\"heightPotential\":"; appendDouble(out, character->genetics.heightPotential); out << ",";
            out << "\"buildPotential\":"; appendDouble(out, character->genetics.buildPotential); out << ",";
            out << "\"healthPotential\":"; appendDouble(out, character->genetics.healthPotential); out << ",";
            out << "\"learningPotential\":"; appendDouble(out, character->genetics.learningPotential); out << ",";
            out << "\"temperamentSensitivity\":"; appendDouble(out, character->genetics.temperamentSensitivity);
        }
        out << "},";

        out << "\"lifeCondition\":{";
        if (character) {
            out << "\"physicalHealth\":"; appendDouble(out, character->lifeCondition.physicalHealth); out << ",";
            out << "\"energyCapacity\":"; appendDouble(out, character->lifeCondition.energyCapacity); out << ",";
            out << "\"movementCapacity\":"; appendDouble(out, character->lifeCondition.movementCapacity); out << ",";
            out << "\"reproductivePotential\":"; appendDouble(out, character->lifeCondition.reproductivePotential); out << ",";
            out << "\"workCapacity\":"; appendDouble(out, character->lifeCondition.workCapacity); out << ",";
            out << "\"appearanceAgeFactor\":"; appendDouble(out, character->lifeCondition.appearanceAgeFactor); out << ",";
            out << "\"lifeGoalFamilyFocus\":"; appendDouble(out, character->lifeCondition.lifeGoalFamilyFocus); out << ",";
            out << "\"familyRoleSalience\":"; appendDouble(out, character->lifeCondition.familyRoleSalience);
        }
        out << "},";

        out << "\"development\":{";
        if (character) {
            out << "\"attachment\":"; appendDouble(out, character->development.attachment); out << ",";
            out << "\"confidence\":"; appendDouble(out, character->development.confidence); out << ",";
            out << "\"stress\":"; appendDouble(out, character->development.stress); out << ",";
            out << "\"socialSkill\":"; appendDouble(out, character->development.socialSkill); out << ",";
            out << "\"emotionalSecurity\":"; appendDouble(out, character->development.emotionalSecurity); out << ",";
            out << "\"disciplineInternalization\":"; appendDouble(out, character->development.disciplineInternalization); out << ",";
            out << "\"learningSupport\":"; appendDouble(out, character->development.learningSupport); out << ",";
            out << "\"health\":"; appendDouble(out, character->development.health);
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

        out << "\"civilization\":{";
        out << "\"totalInventoryUnits\":" << civilization.totalInventoryUnits << ",";
        out << "\"gatheringSkill\":"; appendDouble(out, civilization.gatheringSkill); out << ",";
        out << "\"craftingSkill\":"; appendDouble(out, civilization.craftingSkill); out << ",";
        out << "\"learningSkill\":"; appendDouble(out, civilization.learningSkill); out << ",";
        out << "\"knownTechniqueCount\":" << civilization.knownTechniqueCount << ",";
        out << "\"reproducibleTechniqueCount\":" << civilization.reproducibleTechniqueCount << ",";
        out << "\"availableCapabilityCount\":" << civilization.availableCapabilityCount << ",";
        out << "\"knownTechnologyCount\":" << civilization.knownTechnologyCount << ",";
        out << "\"operationalTechnologyCount\":" << civilization.operationalTechnologyCount << ",";
        out << "\"adoptedTechnologyCount\":" << civilization.adoptedTechnologyCount << ",";
        out << "\"latestKnowledgeMinute\":" << civilization.latestKnowledgeMinute << ",";
        out << "\"latestTechnique\":\"" << techniqueIdName(civilization.latestTechnique) << "\",";
        out << "\"inventory\":[";
        for (std::size_t i = 0; i < civilization.inventory.size(); ++i) {
            if (i != 0) out << ",";
            const CivilizationItemObservation& item = civilization.inventory[i];
            out << "{";
            out << "\"item\":\"" << itemKindName(item.item) << "\",";
            out << "\"material\":\"" << traceMaterialName(item.material) << "\",";
            out << "\"quantity\":" << item.quantity << ",";
            out << "\"quality\":"; appendDouble(out, item.quality); out << ",";
            out << "\"durability\":"; appendDouble(out, item.durability);
            out << "}";
        }
        out << "],";
        out << "\"techniques\":[";
        for (std::size_t i = 0; i < civilization.techniques.size(); ++i) {
            if (i != 0) out << ",";
            const CivilizationTechniqueObservation& technique = civilization.techniques[i];
            out << "{";
            out << "\"technique\":\"" << techniqueIdName(technique.technique) << "\",";
            out << "\"level\":\"" << knowledgeLevelName(technique.level) << "\",";
            out << "\"confidence\":"; appendDouble(out, technique.confidence); out << ",";
            out << "\"successfulUses\":" << technique.successfulUses << ",";
            out << "\"hasProvenance\":" << (technique.hasProvenance ? "true" : "false") << ",";
            out << "\"factId\":\"" << technique.factId << "\",";
            out << "\"originResidentId\":\"" << technique.originResidentId << "\",";
            out << "\"immediateSourceId\":\"" << technique.immediateSourceId << "\",";
            out << "\"source\":\"" << civilizationKnowledgeSourceName(technique.source) << "\",";
            out << "\"learnedMinute\":" << technique.learnedMinute << ",";
            out << "\"hopCount\":" << technique.hopCount;
            out << "}";
        }
        out << "],";
        out << "\"capabilities\":[";
        for (std::size_t i = 0; i < civilization.capabilities.size(); ++i) {
            if (i != 0) out << ",";
            const CivilizationCapabilityStatus& capability =
                civilization.capabilities[i];
            out << "{";
            out << "\"capability\":\"" << capabilityIdName(capability.capability) << "\",";
            out << "\"available\":" << (capability.available ? "true" : "false") << ",";
            out << "\"knownSupportingTechnologies\":" << capability.knownSupportingTechnologies << ",";
            out << "\"operationalSupportingTechnologies\":" << capability.operationalSupportingTechnologies;
            out << "}";
        }
        out << "],";
        out << "\"technologies\":[";
        for (std::size_t i = 0; i < civilization.technologies.size(); ++i) {
            if (i != 0) out << ",";
            const CivilizationTechnologyStatus& technology =
                civilization.technologies[i];
            out << "{";
            out << "\"technology\":\"" << technologyIdName(technology.technology) << "\",";
            out << "\"legacyTechnique\":\"" << techniqueIdName(technology.legacyTechnique) << "\",";
            out << "\"primaryCapability\":\"" << capabilityIdName(technology.primaryCapability) << "\",";
            out << "\"knowledgeLevel\":\"" << knowledgeLevelName(technology.knowledgeLevel) << "\",";
            out << "\"discovered\":" << (technology.discovered ? "true" : "false") << ",";
            out << "\"reproducible\":" << (technology.reproducible ? "true" : "false") << ",";
            out << "\"operational\":" << (technology.operational ? "true" : "false") << ",";
            out << "\"adopted\":" << (technology.adopted ? "true" : "false") << ",";
            out << "\"adoptionDisposition\":\"" << technologyAdoptionDispositionName(technology.adoptionDisposition) << "\",";
            out << "\"adoptionAcceptance01\":"; appendDouble(out,technology.adoptionAcceptance01); out << ",";
            out << "\"successfulUses\":" << technology.successfulUses << ",";
            out << "\"prerequisiteCount\":" << technology.prerequisiteCount << ",";
            out << "\"satisfiedPrerequisiteCount\":" << technology.satisfiedPrerequisiteCount << ",";
            out << "\"prerequisitesSatisfied\":" << (technology.prerequisitesSatisfied ? "true" : "false") << ",";
            out << "\"transformationEffectCount\":" << technology.transformationEffectCount;
            out << "}";
        }
        out << "]";
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
                out << "\"lifeStage\":\"" << lifeStageName(member.lifeStage) << "\",";
                out << "\"kinship\":\"" << kinshipName(member.kinship) << "\"";
                out << "}";
            }
            out << "]";
        };

        out << "\"family\":{";
        out << "\"subjectId\":\"" << family.subjectId << "\",";
        out << "\"householdId\":\"" << family.householdId << "\",";
        out << "\"hasRomanceHistory\":" << (family.hasRomanceHistory ? "true" : "false") << ",";
        out << "\"hasActivePartner\":" << (family.hasActivePartner ? "true" : "false") << ",";
        out << "\"partnerId\":\"" << family.partnerId << "\",";
        out << "\"partnerName\":\"" << escapeJson(family.partnerName) << "\",";
        out << "\"partnerStage\":\"" << romanceStageName(family.partnerStage) << "\",";
        out << "\"cohabitingWithPartner\":" << (family.cohabitingWithPartner ? "true" : "false") << ",";
        out << "\"isGestationalParent\":" << (family.isGestationalParent ? "true" : "false") << ",";
        out << "\"expectingChild\":" << (family.expectingChild ? "true" : "false") << ",";
        out << "\"pregnancyPartnerId\":\"" << family.pregnancyPartnerId << "\",";
        out << "\"pregnancyPartnerName\":\"" << escapeJson(family.pregnancyPartnerName) << "\",";
        out << "\"parents\":"; appendFamilyMembers(family.parents); out << ",";
        out << "\"children\":"; appendFamilyMembers(family.children); out << ",";
        out << "\"siblings\":"; appendFamilyMembers(family.siblings);
        out << "},";

        out << "\"household\":";
        if (household == nullptr) {
            out << "null";
        } else {
            out << "{";
            out << "\"id\":\"" << household->id << "\",";
            out << "\"homeObjectId\":\"" << household->home << "\",";
            out << "\"resources\":"; appendDouble(out, household->resources); out << ",";
            out << "\"sharedMoney\":"; appendDouble(out, household->sharedMoney); out << ",";
            out << "\"sharedObjectIds\":[";
            for (std::size_t i = 0; i < household->sharedObjects.size(); ++i) {
                if (i != 0) out << ",";
                out << "\"" << household->sharedObjects[i] << "\"";
            }
            out << "],";
            out << "\"members\":[";
            for (std::size_t i = 0; i < household->members.size(); ++i) {
                if (i != 0) out << ",";
                const HouseholdMember& member = household->members[i];
                const Character* memberCharacter =
                    findObservedCharacter(world, member.characterId);
                out << "{";
                out << "\"id\":\"" << member.characterId << "\",";
                out << "\"name\":\"" << escapeJson(
                    memberCharacter ? memberCharacter->name : std::string{}) << "\",";
                out << "\"contributionWeight\":"; appendDouble(out, member.contributionWeight); out << ",";
                out << "\"responsibilities\":{";
                out << "\"cooking\":"; appendDouble(out, member.responsibilities.cooking); out << ",";
                out << "\"cleaning\":"; appendDouble(out, member.responsibilities.cleaning); out << ",";
                out << "\"shopping\":"; appendDouble(out, member.responsibilities.shopping); out << ",";
                out << "\"maintenance\":"; appendDouble(out, member.responsibilities.maintenance); out << ",";
                out << "\"caregiving\":"; appendDouble(out, member.responsibilities.caregiving);
                out << "}";
                out << "}";
            }
            out << "]";
            out << "}";
        }
        out << ",";

        out << "\"pregnancy\":";
        if (pregnancy == nullptr) {
            out << "null";
        } else {
            out << "{";
            out << "\"role\":\"" << (pregnancyAsGestationalParent ? "GestationalParent" : "GeneticPartner") << "\",";
            out << "\"gestationalParentId\":\"" << pregnancy->gestationalParent << "\",";
            out << "\"geneticPartnerId\":\"" << pregnancy->geneticPartner << "\",";
            out << "\"stage\":\"" << pregnancyStageName(pregnancy->stage) << "\",";
            out << "\"conceptionMinute\":" << pregnancy->conceptionMinute << ",";
            out << "\"dueMinute\":" << pregnancy->dueMinute << ",";
            out << "\"lastUpdateMinute\":" << pregnancy->lastUpdateMinute << ",";
            out << "\"health\":"; appendDouble(out, pregnancy->health); out << ",";
            out << "\"fatigue\":"; appendDouble(out, pregnancy->fatigue); out << ",";
            out << "\"stress\":"; appendDouble(out, pregnancy->stress); out << ",";
            out << "\"nutrition\":"; appendDouble(out, pregnancy->nutrition);
            out << "}";
        }
        out << ",";

        out << "\"lifeHistory\":[";
        if (character) {
            const std::size_t historyCount = character->lifeHistory.size();
            const std::size_t firstHistory = historyCount > 16
                ? historyCount - 16
                : 0;
            bool firstHistoryEntry = true;
            for (std::size_t i = firstHistory; i < historyCount; ++i) {
                if (!firstHistoryEntry) out << ",";
                firstHistoryEntry = false;
                const LifeHistoryEntry& event = character->lifeHistory[i];
                out << "{";
                out << "\"type\":\"" << lifeEventName(event.type) << "\",";
                out << "\"minute\":" << event.minute << ",";
                out << "\"relatedCharacterIds\":[";
                for (std::size_t j = 0; j < event.relatedCharacters.size(); ++j) {
                    if (j != 0) out << ",";
                    out << "\"" << event.relatedCharacters[j] << "\"";
                }
                out << "],";
                out << "\"value\":" << event.value;
                out << "}";
            }
        }
        out << "],";

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
            out << "\"recallScore\":"; appendDouble(out, memory.recallScore(world.minute)); out << ",";
            out << "\"decayPerDay\":"; appendDouble(out, memory.decayPerDay); out << ",";
            out << "\"witnessed\":" << (memory.witnessed ? "true" : "false") << ",";
            out << "\"source\":\"" << memorySourceName(memory.source) << "\",";
            out << "\"tags\":[";
            for (std::size_t tagIndex = 0; tagIndex < memory.tags.size(); ++tagIndex) {
                if (tagIndex != 0) out << ",";
                out << "\"" << escapeJson(memory.tags[tagIndex]) << "\"";
            }
            out << "]";
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
            out << "\"supportWeight\":"; appendDouble(out, belief.supportWeight); out << ",";
            out << "\"contradictionWeight\":"; appendDouble(out, belief.contradictionWeight); out << ",";
            out << "\"supportCount\":" << belief.supportCount << ",";
            out << "\"contradictionCount\":" << belief.contradictionCount << ",";
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
    const SimulationCalendarObservation calendar =
        deriveSimulationCalendar(simulation_->world().minute);

    std::ostringstream out;
    out << "{";
    out << "\"available\":true,";
    out << "\"centerChunkX\":" << centerChunkX << ",";
    out << "\"centerChunkY\":" << centerChunkY << ",";
    out << "\"simulationMinute\":" << environment.simulationMinute << ",";
    out << "\"baselineTemperature01\":"; appendDouble(out, environment.baselineTemperature01); out << ",";
    out << "\"baselineMoisture01\":"; appendDouble(out, environment.baselineMoisture01); out << ",";
    out << "\"airTemperatureC\":"; appendDouble(out, environment.airTemperatureC); out << ",";
    out << "\"seasonalTemperatureModifierC\":"; appendDouble(out, environment.seasonalTemperatureModifierC); out << ",";
    out << "\"dailyTemperatureModifierC\":"; appendDouble(out, environment.dailyTemperatureModifierC); out << ",";
    out << "\"precipitationIntensity01\":"; appendDouble(out, environment.precipitationIntensity01); out << ",";
    out << "\"cloudCover01\":"; appendDouble(out, environment.cloudCover01); out << ",";
    out << "\"windIntensity01\":"; appendDouble(out, environment.windIntensity01); out << ",";
    out << "\"humidity01\":"; appendDouble(out, environment.humidity01); out << ",";
    out << "\"visibility01\":"; appendDouble(out, environment.visibility01); out << ",";
    out << "\"surfaceWetness01\":"; appendDouble(out, environment.surfaceWetness01); out << ",";
    out << "\"precipitationType\":\"" << precipitationTypeName(environment.precipitationType) << "\",";
    out << "\"summary\":\"" << weatherSummaryName(environment.summary) << "\",";
    out << "\"calendar\":{";
    out << "\"minuteOfDay\":" << calendar.minuteOfDay << ",";
    out << "\"hourOfDay\":" << calendar.hourOfDay << ",";
    out << "\"minuteOfHour\":" << calendar.minuteOfHour << ",";
    out << "\"dayIndex\":" << calendar.dayIndex << ",";
    out << "\"dayOfYear\":" << calendar.dayOfYear << ",";
    out << "\"yearIndex\":" << calendar.yearIndex << ",";
    out << "\"annualPhase\":"; appendDouble(out, calendar.annualPhase); out << ",";
    out << "\"season\":\"" << seasonSummaryName(calendar.season) << "\",";
    out << "\"isDay\":" << (calendar.isDay ? "true" : "false") << ",";
    out << "\"isNight\":" << (calendar.isNight ? "true" : "false") << ",";
    out << "\"daylight01\":"; appendDouble(out, calendar.daylight01);
    out << "}";
    out << "}";
    return out.str();
}

std::string WebClientBridge::recentSocialEventsJson(
    std::size_t maxEvents) const
{
    if (!simulation_) return "{\"available\":false,\"events\":[]}";

    const std::size_t bounded = std::min<std::size_t>(maxEvents, 64);
    const std::vector<SocialCommunicationObservation> events =
        simulation_->observeRecentSocialEvents(bounded);

    std::ostringstream out;
    out << "{\"available\":true,\"count\":" << events.size() << ",\"events\":[";
    for (std::size_t i = 0; i < events.size(); ++i) {
        if (i != 0) out << ",";
        const SocialCommunicationObservation& event = events[i];
        out << "{";
        out << "\"sequence\":\"" << event.sequence << "\",";
        out << "\"actorId\":\"" << event.actor << "\",";
        out << "\"targetId\":\"" << event.target << "\",";
        out << "\"type\":\"" << socialEventTypeName(event.type) << "\",";
        out << "\"intensity\":"; appendDouble(out, event.intensity); out << ",";
        out << "\"importance\":"; appendDouble(out, event.importance); out << ",";
        out << "\"minute\":" << event.minute << ",";
        out << "\"where\":\"" << escapeJson(event.where) << "\",";
        out << "\"presentationLevel\":\"" << socialPresentationLevelName(event.presentationLevel) << "\",";
        out << "\"successful\":" << (event.successful ? "true" : "false");
        out << "}";
    }
    out << "]}";
    return out.str();
}

std::string WebClientBridge::civilizationWorldJson(
    std::size_t maxRecentDiscoveries) const
{
    if (!simulation_) {
        return "{\"available\":false,\"technologyPopulation\":[],\"transformations\":[],\"resources\":[],\"storages\":[],\"facilities\":[],\"recentDiscoveries\":[]}";
    }

    return civilizationWorldObservationJson(
        simulation_->observeCivilizationWorld(
            std::min<std::size_t>(maxRecentDiscoveries, 64)),
        simulation_->observeSocietyWorld());
}

std::string WebClientBridge::civilizationWorldWindowJson(
    std::size_t maxRecentDiscoveries,
    int centerChunkX,
    int centerChunkY,
    int radiusChunks) const
{
    if (!simulation_) {
        return "{\"available\":false,\"technologyPopulation\":[],\"transformations\":[],\"resources\":[],\"storages\":[],\"facilities\":[],\"recentDiscoveries\":[]}";
    }

    const int radius=std::max(0,std::min(radiusChunks,16));
    return civilizationWorldObservationJson(
        simulation_->observeCivilizationWorldWindow(
            {centerChunkX,centerChunkY},
            radius,
            std::min<std::size_t>(maxRecentDiscoveries,64)),
        simulation_->observeSocietyWorld());
}

std::string WebClientBridge::worldObjectsJson() const
{
    if (!simulation_) {
        return "{\"available\":false,\"smartObjects\":[],\"sanitationSites\":[]}";
    }

    const World& world = simulation_->world();
    std::ostringstream out;
    out << "{\"available\":true,";

    out << "\"smartObjects\":[";
    for (std::size_t i = 0; i < world.objects.size(); ++i) {
        if (i != 0) out << ",";
        const SmartObject& object = world.objects[i];
        out << "{";
        out << "\"id\":\"" << object.id << "\",";
        out << "\"kind\":\"" << objectKindName(object.kind) << "\",";
        out << "\"gridX\":" << object.pos.x << ",";
        out << "\"gridY\":" << object.pos.y << ",";
        out << "\"reservedById\":\""
            << (object.reservedBy.has_value() ? *object.reservedBy : 0) << "\",";
        out << "\"useDurationTicks\":" << object.useDurationTicks << ",";
        out << "\"effectPerTick\":{";
        out << "\"hunger\":"; appendDouble(out, object.effectPerTick.hunger); out << ",";
        out << "\"thirst\":"; appendDouble(out, object.effectPerTick.thirst); out << ",";
        out << "\"sleep\":"; appendDouble(out, object.effectPerTick.sleep); out << ",";
        out << "\"bladder\":"; appendDouble(out, object.effectPerTick.bladder); out << ",";
        out << "\"hygiene\":"; appendDouble(out, object.effectPerTick.hygiene);
        out << "}";
        out << "}";
    }
    out << "],";

    out << "\"sanitationSites\":[";
    for (std::size_t i = 0; i < world.primitiveSanitationSites.size(); ++i) {
        if (i != 0) out << ",";
        const PrimitiveSanitationSite& site = world.primitiveSanitationSites[i];
        out << "{";
        out << "\"id\":\"" << site.id << "\",";
        out << "\"kind\":\"" << sanitationSiteKindName(site.kind) << "\",";
        out << "\"gridX\":" << site.pos.x << ",";
        out << "\"gridY\":" << site.pos.y << ",";
        out << "\"establishedBy\":\"" << site.establishedBy << "\",";
        out << "\"establishedMinute\":" << site.establishedMinute << ",";
        out << "\"active\":" << (site.active ? "true" : "false") << ",";
        out << "\"useCount\":" << site.useCount << ",";
        out << "\"improvementWork\":"; appendDouble(out, site.improvementWork); out << ",";
        out << "\"improvedBy\":\"" << site.improvedBy << "\",";
        out << "\"improvedMinute\":" << site.improvedMinute;
        out << "}";
    }
    out << "]";
    out << "}";
    return out.str();
}

std::string WebClientBridge::humanTracesWindowJson(
    int centerChunkX,
    int centerChunkY,
    int radiusChunks) const
{
    if (!simulation_) {
        return "{\"available\":false,\"humanTraces\":{\"total\":0,\"entries\":[]}}";
    }

    const int radius = std::max(0, std::min(radiusChunks, 16));
    std::ostringstream out;
    out << "{\"available\":true,";
    out << "\"centerChunkX\":" << centerChunkX << ",";
    out << "\"centerChunkY\":" << centerChunkY << ",";
    out << "\"radiusChunks\":" << radius << ",";
    out << "\"humanTraces\":";
    appendHumanTraces(out, buildHumanTraceWindowObservation(
        simulation_->world(), {centerChunkX, centerChunkY}, radius));
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
            out << "\"flowPotential\":"; appendDouble(out, water.flowPotential); out << ",";
            out << "\"drainageAccumulationPotential\":"; appendDouble(out, water.drainageAccumulationPotential); out << ",";
            out << "\"hasDownstream\":" << (water.hasDownstream ? "true" : "false") << ",";
            out << "\"downstreamChunkX\":" << water.downstream.x << ",";
            out << "\"downstreamChunkY\":" << water.downstream.y << ",";
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

    out << "],\"humanTraces\":";
    appendHumanTraces(out, buildHumanTraceWindowObservation(
        simulation_->world(), {centerChunkX, centerChunkY}, radius));
    out << "}";
    return out.str();
}

} // namespace lifelens
