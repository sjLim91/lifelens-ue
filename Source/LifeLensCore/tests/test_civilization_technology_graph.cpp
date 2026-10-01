#include <array>
#include <iostream>

#include "lifelens/CivilizationDecision.h"
#include "lifelens/CivilizationKnowledgeTransmission.h"

using namespace lifelens;

#define CHECK(expr) do { \
    if(!(expr)) { \
        std::cerr << "CHECK failed: " #expr << " at line " << __LINE__ << '\n'; \
        return 1; \
    } \
} while(false)

static ExperimentKind experimentForTechnology(TechnologyId technology)
{
    switch(technology){
        case TechnologyId::ChippedStoneTool: return ExperimentKind::HaftSharpFlake;
        case TechnologyId::DugSanitationPit: return ExperimentKind::DigSanitationPit;
        case TechnologyId::DiggingStick: return ExperimentKind::ShapeDiggingStick;
        case TechnologyId::StoneHammer: return ExperimentKind::HaftStoneHammer;
        case TechnologyId::CopperSmelting: return ExperimentKind::SmeltCopperOre;
        case TechnologyId::Cultivation: return ExperimentKind::CultivatePlantFood;
        case TechnologyId::TinSmelting: return ExperimentKind::SmeltTinOre;
        case TechnologyId::BronzeAlloying: return ExperimentKind::AlloyBronze;
        case TechnologyId::BronzeAxe: return ExperimentKind::CastBronzeAxe;
        case TechnologyId::BronzePick: return ExperimentKind::CastBronzePick;
        default: return ExperimentKind::StrikeStone;
    }
}

static KnowledgeState knowledgeWithGraphPrerequisites(
    TechnologyId technology,
    TechnologyId omitted=TechnologyId::None)
{
    KnowledgeState knowledge;
    for(const auto& edge:TechnologyPrerequisiteRegistry){
        if(edge.technology!=technology || edge.prerequisite==omitted) continue;
        const TechniqueId technique=techniqueForTechnology(edge.prerequisite);
        if(technique!=TechniqueId::None){
            knowledge.learn(technique,edge.minimumKnowledge,0.95);
        }
    }
    return knowledge;
}

int main()
{
    constexpr int TechnologySlots=17;
    std::array<bool,TechnologySlots> registered{};
    for(const auto& definition:TechnologyRegistry){
        const int raw=static_cast<int>(definition.id);
        CHECK(raw>0 && raw<TechnologySlots);
        CHECK(!registered[raw]);
        registered[raw]=true;
        CHECK(definition.legacyTechnique!=TechniqueId::None);
        CHECK(definition.primaryCapability!=CapabilityId::None);
        CHECK(technologyIdForTechnique(definition.legacyTechnique)==definition.id);
        CHECK(techniqueForTechnology(definition.id)==definition.legacyTechnique);
    }
    for(int raw=1;raw<TechnologySlots;++raw) CHECK(registered[raw]);

    bool reach[TechnologySlots][TechnologySlots]{};
    for(const auto& edge:TechnologyPrerequisiteRegistry){
        const int technology=static_cast<int>(edge.technology);
        const int prerequisite=static_cast<int>(edge.prerequisite);
        CHECK(technology>0 && technology<TechnologySlots);
        CHECK(prerequisite>0 && prerequisite<TechnologySlots);
        CHECK(technology!=prerequisite);
        CHECK(edge.minimumKnowledge>=KnowledgeLevel::Observed);
        reach[technology][prerequisite]=true;
    }
    for(int k=1;k<TechnologySlots;++k){
        for(int i=1;i<TechnologySlots;++i){
            for(int j=1;j<TechnologySlots;++j){
                reach[i][j]=reach[i][j] || (reach[i][k] && reach[k][j]);
            }
        }
    }
    for(int raw=1;raw<TechnologySlots;++raw) CHECK(!reach[raw][raw]);

    CHECK(technologyHasPrerequisite(
        TechnologyId::CopperSmelting,TechnologyId::FireMaking));
    CHECK(technologyHasPrerequisite(
        TechnologyId::CopperSmelting,TechnologyId::StoneHammer));
    CHECK(technologyHasPrerequisite(
        TechnologyId::CopperSmelting,TechnologyId::SimpleContainer));
    CHECK(technologyHasPrerequisite(
        TechnologyId::TinSmelting,TechnologyId::CopperSmelting));
    CHECK(technologyHasPrerequisite(
        TechnologyId::BronzeAlloying,TechnologyId::TinSmelting));
    CHECK(technologyHasPrerequisite(
        TechnologyId::BronzeAxe,TechnologyId::BronzeAlloying));
    CHECK(technologyHasPrerequisite(
        TechnologyId::BronzePick,TechnologyId::StoneHammer));

    constexpr std::array<TechnologyId,10> gated={{
        TechnologyId::ChippedStoneTool,
        TechnologyId::DugSanitationPit,
        TechnologyId::DiggingStick,
        TechnologyId::StoneHammer,
        TechnologyId::CopperSmelting,
        TechnologyId::Cultivation,
        TechnologyId::TinSmelting,
        TechnologyId::BronzeAlloying,
        TechnologyId::BronzeAxe,
        TechnologyId::BronzePick
    }};

    for(const TechnologyId technology:gated){
        KnowledgeState complete=knowledgeWithGraphPrerequisites(technology);
        CHECK(technologyKnowledgePrerequisitesSatisfied(complete,technology));

        ExperimentContext context;
        context.kind=experimentForTechnology(technology);
        context.smeltingOpportunityAvailable=true;
        context.cultivationOpportunityAvailable=true;
        context.sanitationProblemRecognized=true;
        context.sanitationSiteAvailable=true;
        context.sanitationPitCandidateAvailable=true;
        context.storageProblemRecognized=true;
        CHECK(experimentPrerequisitesMet(context,complete));

        TechnologyId omitted=TechnologyId::None;
        for(const auto& edge:TechnologyPrerequisiteRegistry){
            if(edge.technology==technology){
                omitted=edge.prerequisite;
                break;
            }
        }
        CHECK(omitted!=TechnologyId::None);
        KnowledgeState incomplete=
            knowledgeWithGraphPrerequisites(technology,omitted);
        CHECK(!technologyKnowledgePrerequisitesSatisfied(
            incomplete,technology));
        CHECK(!experimentPrerequisitesMet(context,incomplete));
    }

    Character learner;
    learner.id=1;
    learner.civilization.character=1;
    learner.civilization.knowledge=
        knowledgeWithGraphPrerequisites(TechnologyId::BronzeAxe);
    CHECK(techniquePrerequisiteContextSatisfied(
        learner,TechniqueId::BronzeAxe));
    learner.civilization.knowledge=knowledgeWithGraphPrerequisites(
        TechnologyId::BronzeAxe,TechnologyId::FiberCordage);
    CHECK(!techniquePrerequisiteContextSatisfied(
        learner,TechniqueId::BronzeAxe));

    CHECK(technologyTransformationEffectCount(
        TechnologyId::PrimitiveStorage)==1);
    CHECK(technologyTransformationEffectCount(
        TechnologyId::Cultivation)==1);
    CHECK(technologyTransformationEffectCount(
        TechnologyId::BronzeAxe)==1);
    CHECK(technologyTransformationEffectWeight(
        TechnologyId::BronzeAxe,
        CivilizationTransformationId::AdvancedTooling)==0.50);
    const double metallurgyWeight=
        technologyTransformationEffectWeight(
            TechnologyId::CopperSmelting,
            CivilizationTransformationId::MetallurgicalProduction)
        +technologyTransformationEffectWeight(
            TechnologyId::TinSmelting,
            CivilizationTransformationId::MetallurgicalProduction)
        +technologyTransformationEffectWeight(
            TechnologyId::BronzeAlloying,
            CivilizationTransformationId::MetallurgicalProduction);
    CHECK(metallurgyWeight>0.999 && metallurgyWeight<1.001);

    World world(777777);
    world.characters.clear();
    Character bronzeLearner;
    bronzeLearner.id=7;
    bronzeLearner.alive=true;
    bronzeLearner.civilization.character=7;
    bronzeLearner.civilization.knowledge=
        knowledgeWithGraphPrerequisites(TechnologyId::BronzeAxe);
    bronzeLearner.civilization.knowledge.learn(
        TechniqueId::BronzeAxe,KnowledgeLevel::Observed,0.60);
    world.characters.push_back(bronzeLearner);

    const CivilizationTechnologyStatus status=
        observeTechnologyStatus(
            world,world.characters[0],TechnologyId::BronzeAxe);
    CHECK(status.discovered);
    CHECK(status.prerequisiteCount==3);
    CHECK(status.satisfiedPrerequisiteCount==3);
    CHECK(status.prerequisitesSatisfied);
    CHECK(status.transformationEffectCount==1);

    std::cout << "civilization technology prerequisite/effect graph passed\n";
    return 0;
}
