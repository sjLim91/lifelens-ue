#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "Civilization.h"
#include "CivilizationKnowledgeTransmission.h"
#include "ObserverReadModel.h"
#include "World.h"

namespace lifelens {

enum class CivilizationKnowledgeSource {
    Unknown,
    SelfDiscovery,
    DirectWitness,
    Teaching
};

struct CivilizationItemObservation {
    ItemKind item=ItemKind::RawMaterial;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    double quality=0.0;
    double durability=0.0;
};

struct CivilizationTechniqueObservation {
    TechniqueId technique=TechniqueId::None;
    KnowledgeLevel level=KnowledgeLevel::Unknown;
    double confidence=0.0;
    int successfulUses=0;

    bool hasProvenance=false;
    SocialFactId factId=0;
    CharacterId originResidentId=0;
    CharacterId immediateSourceId=0;
    CivilizationKnowledgeSource source=CivilizationKnowledgeSource::Unknown;
    int learnedMinute=-1;
    std::size_t hopCount=0;
};

struct ResidentCivilizationObservation {
    CharacterId residentId=0;
    int totalInventoryUnits=0;
    double gatheringSkill=0.0;
    double craftingSkill=0.0;
    double learningSkill=0.0;
    std::size_t knownTechniqueCount=0;
    std::size_t reproducibleTechniqueCount=0;
    int latestKnowledgeMinute=-1;
    TechniqueId latestTechnique=TechniqueId::None;
    std::vector<CivilizationItemObservation> inventory;
    std::vector<CivilizationTechniqueObservation> techniques;
};

struct CivilizationResourceObservation {
    ResourceNodeId id=0;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    int maxQuantity=0;
    bool renewable=false;
    int regenerationPerDay=0;
};

struct CivilizationStorageObservation {
    StorageId id=0;
    int totalUnits=0;
    std::vector<CivilizationItemObservation> inventory;
};

struct CivilizationFacilityRequirementObservation {
    MaterialKind material=MaterialKind::Unknown;
    int required=0;
    int delivered=0;
};

struct CivilizationFacilityObservation {
    FacilityId id=0;
    FacilityKind kind=FacilityKind::PrimitiveStorage;
    FacilityState state=FacilityState::Planned;
    GridPos pos{};
    CharacterId initiatedBy=0;
    CharacterId lastWorkedBy=0;
    int startedMinute=-1;
    int completedMinute=-1;
    double constructionWork=0.0;
    double requiredWork=0.0;
    double workProgress=0.0;
    double durability=1.0;
    bool active=false;
    StorageId linkedStorage=0;
    int requiredMaterialUnits=0;
    int deliveredMaterialUnits=0;
    std::vector<CivilizationFacilityRequirementObservation> requirements;
};

struct CivilizationDiscoveryObservation {
    SocialFactId factId=0;
    TechniqueId technique=TechniqueId::None;
    CharacterId discovererId=0;
    std::string discovererName;
    int minute=0;
    std::size_t recipientCount=0;
    std::size_t livingKnowerCount=0;
};

struct CivilizationWorldObservation {
    int minute=0;
    std::size_t resourceNodeCount=0;
    std::size_t depletedResourceNodeCount=0;
    int totalResourceUnits=0;
    std::size_t storageSiteCount=0;
    int totalStoredUnits=0;

    std::size_t facilityCount=0;
    std::size_t plannedFacilityCount=0;
    std::size_t underConstructionFacilityCount=0;
    std::size_t operationalFacilityCount=0;

    std::size_t techniqueFactCount=0;
    std::size_t transmissionReceiptCount=0;
    std::size_t uniqueKnownTechniqueTypes=0;
    std::size_t uniqueReproducibleTechniqueTypes=0;
    std::size_t knownTechniqueOwners=0;
    std::size_t reproducibleTechniqueOwners=0;

    std::vector<CivilizationResourceObservation> resources;
    std::vector<CivilizationStorageObservation> storages;
    std::vector<CivilizationFacilityObservation> facilities;
    std::vector<CivilizationDiscoveryObservation> recentDiscoveries;
};

inline TechniqueId techniqueFromCivilizationFact(const SocialFact& fact)
{
    // The old sanitation-only boundary remains a covered subset after adding
    // PrimitiveStorage. Structural compatibility markers:
    // raw<=static_cast<int>(TechniqueId::DugSanitationPit)
    for(int raw=static_cast<int>(TechniqueId::SharpFlake);
        raw<=static_cast<int>(TechniqueId::PrimitiveStorage);++raw){
        const TechniqueId candidate=static_cast<TechniqueId>(raw);
        if(factRepresentsTechnique(fact,candidate)) return candidate;
    }
    return TechniqueId::None;
}

inline CivilizationKnowledgeSource civilizationKnowledgeSource(
    const SocialFact& fact,
    const KnowledgeReceipt& receipt)
{
    if(receipt.source==MemorySource::ToldByOther) return CivilizationKnowledgeSource::Teaching;
    if(receipt.source==MemorySource::DirectWitness){
        return fact.subject==receipt.holder
            ? CivilizationKnowledgeSource::SelfDiscovery
            : CivilizationKnowledgeSource::DirectWitness;
    }
    return CivilizationKnowledgeSource::Unknown;
}

inline const KnowledgeReceipt* earliestTechniqueReceipt(
    const SocialKnowledgeBook& socialKnowledge,
    CharacterId holder,
    TechniqueId technique,
    const SocialFact** outFact=nullptr)
{
    const KnowledgeReceipt* best=nullptr;
    const SocialFact* bestFact=nullptr;
    for(const KnowledgeReceipt& receipt:socialKnowledge.receipts()){
        if(receipt.holder!=holder) continue;
        const SocialFact* fact=socialKnowledge.findFact(receipt.factId);
        if(fact==nullptr || !factRepresentsTechnique(*fact,technique)) continue;
        if(best==nullptr || receipt.learnedMinute<best->learnedMinute ||
           (receipt.learnedMinute==best->learnedMinute && receipt.factId<best->factId)){
            best=&receipt;
            bestFact=fact;
        }
    }
    if(outFact!=nullptr) *outFact=bestFact;
    return best;
}

inline CivilizationItemObservation makeCivilizationItemObservation(const ItemStack& stack)
{
    return {stack.kind,stack.material,stack.quantity,stack.quality,stack.durability};
}

inline void sortCivilizationItems(std::vector<CivilizationItemObservation>& items)
{
    std::sort(items.begin(),items.end(),[](const auto& a,const auto& b){
        if(a.item!=b.item) return static_cast<int>(a.item)<static_cast<int>(b.item);
        if(a.material!=b.material) return static_cast<int>(a.material)<static_cast<int>(b.material);
        if(a.quality!=b.quality) return a.quality>b.quality;
        return a.durability>b.durability;
    });
}

inline ResidentCivilizationObservation buildResidentCivilizationObservation(
    const World& world,
    const SocialKnowledgeBook& socialKnowledge,
    const Character& character)
{
    ResidentCivilizationObservation dto;
    dto.residentId=character.id;
    dto.gatheringSkill=character.civilization.gatheringSkill;
    dto.craftingSkill=character.civilization.craftingSkill;
    dto.learningSkill=character.civilization.learningSkill;

    for(const ItemStack& stack:character.civilization.inventory.stacks()){
        if(stack.quantity<=0) continue;
        dto.totalInventoryUnits+=stack.quantity;
        dto.inventory.push_back(makeCivilizationItemObservation(stack));
    }
    sortCivilizationItems(dto.inventory);

    for(const TechniqueKnowledge& knowledge:character.civilization.knowledge.all()){
        if(knowledge.technique==TechniqueId::None || knowledge.level==KnowledgeLevel::Unknown) continue;

        CivilizationTechniqueObservation observed;
        observed.technique=knowledge.technique;
        observed.level=knowledge.level;
        observed.confidence=knowledge.confidence;
        observed.successfulUses=knowledge.successfulUses;

        const SocialFact* fact=nullptr;
        const KnowledgeReceipt* receipt=earliestTechniqueReceipt(
            socialKnowledge,character.id,knowledge.technique,&fact);
        if(receipt!=nullptr && fact!=nullptr){
            observed.hasProvenance=true;
            observed.factId=fact->id;
            observed.originResidentId=fact->subject;
            observed.immediateSourceId=receipt->immediateSource;
            observed.source=civilizationKnowledgeSource(*fact,*receipt);
            observed.learnedMinute=receipt->learnedMinute;
            observed.hopCount=receipt->hopCount();
        }

        ++dto.knownTechniqueCount;
        if(static_cast<int>(knowledge.level)>=static_cast<int>(KnowledgeLevel::Reproducible)){
            ++dto.reproducibleTechniqueCount;
        }
        dto.techniques.push_back(observed);
    }

    std::sort(dto.techniques.begin(),dto.techniques.end(),[](const auto& a,const auto& b){
        return static_cast<int>(a.technique)<static_cast<int>(b.technique);
    });

    for(const KnowledgeReceipt& receipt:socialKnowledge.receipts()){
        if(receipt.holder!=character.id) continue;
        const SocialFact* fact=socialKnowledge.findFact(receipt.factId);
        if(fact==nullptr) continue;
        const TechniqueId technique=techniqueFromCivilizationFact(*fact);
        if(technique==TechniqueId::None) continue;
        if(receipt.learnedMinute>dto.latestKnowledgeMinute ||
           (receipt.learnedMinute==dto.latestKnowledgeMinute &&
            static_cast<int>(technique)<static_cast<int>(dto.latestTechnique))){
            dto.latestKnowledgeMinute=receipt.learnedMinute;
            dto.latestTechnique=technique;
        }
    }

    return dto;
}

inline CivilizationStorageObservation makeCivilizationStorageObservation(const StorageSite& storage)
{
    CivilizationStorageObservation dto;
    dto.id=storage.id;
    for(const ItemStack& stack:storage.inventory.stacks()){
        if(stack.quantity<=0) continue;
        dto.totalUnits+=stack.quantity;
        dto.inventory.push_back(makeCivilizationItemObservation(stack));
    }
    sortCivilizationItems(dto.inventory);
    return dto;
}

inline CivilizationFacilityObservation makeCivilizationFacilityObservation(
    const ConstructedFacility& facility)
{
    CivilizationFacilityObservation dto;
    dto.id=facility.id;
    dto.kind=facility.kind;
    dto.state=facility.state;
    dto.pos=facility.pos;
    dto.initiatedBy=facility.initiatedBy;
    dto.lastWorkedBy=facility.lastWorkedBy;
    dto.startedMinute=facility.startedMinute;
    dto.completedMinute=facility.completedMinute;
    dto.constructionWork=facility.constructionWork;
    dto.requiredWork=facility.requiredWork;
    dto.workProgress=facility.requiredWork>0.0
        ? std::max(0.0,std::min(1.0,facility.constructionWork/facility.requiredWork))
        : 0.0;
    dto.durability=facility.durability;
    dto.active=facility.active;
    dto.linkedStorage=facility.linkedStorage;
    dto.requirements.reserve(facility.requirements.size());
    for(const auto& requirement:facility.requirements){
        CivilizationFacilityRequirementObservation observed;
        observed.material=requirement.material;
        observed.required=requirement.required;
        observed.delivered=requirement.delivered;
        dto.requiredMaterialUnits+=std::max(0,requirement.required);
        dto.deliveredMaterialUnits+=std::max(0,requirement.delivered);
        dto.requirements.push_back(observed);
    }
    return dto;
}

inline std::size_t livingTechniqueKnowerCount(
    const World& world,
    TechniqueId technique,
    KnowledgeLevel minimum=KnowledgeLevel::Observed)
{
    std::size_t count=0;
    for(const Character& character:world.characters){
        if(character.alive && character.civilization.knowledge.knowsAtLeast(technique,minimum)) ++count;
    }
    return count;
}

inline CivilizationWorldObservation buildCivilizationWorldObservation(
    const World& world,
    const SocialKnowledgeBook& socialKnowledge,
    std::size_t maxRecentDiscoveries=32)
{
    CivilizationWorldObservation dto;
    dto.minute=world.minute;

    for(const ResourceNode& node:world.resourceNodes){
        CivilizationResourceObservation observed;
        observed.id=node.id;
        observed.material=node.material;
        observed.quantity=node.quantity;
        observed.maxQuantity=node.maxQuantity;
        observed.renewable=node.renewable;
        observed.regenerationPerDay=node.regenerationPerDay;
        dto.resources.push_back(observed);
        ++dto.resourceNodeCount;
        dto.totalResourceUnits+=std::max(0,node.quantity);
        if(node.quantity<=0) ++dto.depletedResourceNodeCount;
    }
    std::sort(dto.resources.begin(),dto.resources.end(),[](const auto& a,const auto& b){return a.id<b.id;});

    for(const StorageSite& storage:world.storageSites){
        CivilizationStorageObservation observed=makeCivilizationStorageObservation(storage);
        dto.totalStoredUnits+=observed.totalUnits;
        dto.storages.push_back(std::move(observed));
        ++dto.storageSiteCount;
    }
    std::sort(dto.storages.begin(),dto.storages.end(),[](const auto& a,const auto& b){return a.id<b.id;});

    for(const ConstructedFacility& facility:world.facilities){
        dto.facilities.push_back(makeCivilizationFacilityObservation(facility));
        ++dto.facilityCount;
        switch(facility.state){
            case FacilityState::Planned: ++dto.plannedFacilityCount; break;
            case FacilityState::UnderConstruction: ++dto.underConstructionFacilityCount; break;
            case FacilityState::Operational: ++dto.operationalFacilityCount; break;
            case FacilityState::Ruined: break;
        }
    }
    std::sort(dto.facilities.begin(),dto.facilities.end(),[](const auto& a,const auto& b){return a.id<b.id;});

    // Legacy sanitation slot subset is still contained in this expanded count:
    // static_cast<std::size_t>(TechniqueId::DugSanitationPit)+1
    constexpr std::size_t TechniqueSlots=static_cast<std::size_t>(TechniqueId::PrimitiveStorage)+1;
    std::array<bool,TechniqueSlots> knownTypes{};
    std::array<bool,TechniqueSlots> reproducibleTypes{};
    for(const Character& character:world.characters){
        if(!character.alive) continue;
        for(const TechniqueKnowledge& knowledge:character.civilization.knowledge.all()){
            const std::size_t index=static_cast<std::size_t>(knowledge.technique);
            if(index==0 || index>=TechniqueSlots || knowledge.level==KnowledgeLevel::Unknown) continue;
            knownTypes[index]=true;
            ++dto.knownTechniqueOwners;
            if(static_cast<int>(knowledge.level)>=static_cast<int>(KnowledgeLevel::Reproducible)){
                reproducibleTypes[index]=true;
                ++dto.reproducibleTechniqueOwners;
            }
        }
    }
    for(std::size_t i=1;i<TechniqueSlots;++i){
        if(knownTypes[i]) ++dto.uniqueKnownTechniqueTypes;
        if(reproducibleTypes[i]) ++dto.uniqueReproducibleTechniqueTypes;
    }

    for(const SocialFact& fact:socialKnowledge.facts()){
        const TechniqueId technique=techniqueFromCivilizationFact(fact);
        if(technique==TechniqueId::None) continue;
        ++dto.techniqueFactCount;

        if(fact.importance<0.90) continue;

        CivilizationDiscoveryObservation discovery;
        discovery.factId=fact.id;
        discovery.technique=technique;
        discovery.discovererId=fact.subject;
        discovery.minute=fact.eventMinute;
        if(const Character* character=findObservedCharacter(world,fact.subject)){
            discovery.discovererName=character->name;
        }
        for(const KnowledgeReceipt& receipt:socialKnowledge.receipts()){
            if(receipt.factId==fact.id) ++discovery.recipientCount;
        }
        discovery.livingKnowerCount=livingTechniqueKnowerCount(world,technique);
        dto.recentDiscoveries.push_back(std::move(discovery));
    }

    for(const KnowledgeReceipt& receipt:socialKnowledge.receipts()){
        const SocialFact* fact=socialKnowledge.findFact(receipt.factId);
        if(fact!=nullptr && techniqueFromCivilizationFact(*fact)!=TechniqueId::None){
            ++dto.transmissionReceiptCount;
        }
    }

    std::sort(dto.recentDiscoveries.begin(),dto.recentDiscoveries.end(),[](const auto& a,const auto& b){
        if(a.minute!=b.minute) return a.minute>b.minute;
        return a.factId>b.factId;
    });
    if(dto.recentDiscoveries.size()>maxRecentDiscoveries){
        dto.recentDiscoveries.resize(maxRecentDiscoveries);
    }

    return dto;
}

} // namespace lifelens
