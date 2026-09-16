#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <vector>

#include "Civilization.h"
#include "Facility.h"
#include "World.h"

namespace lifelens {

constexpr char CivilizationSnapshotExtensionMagic[]={'L','L','C','I','V','0','0','1'};
constexpr std::uint32_t CivilizationSnapshotExtensionVersion=3;
constexpr std::uint32_t CivilizationSnapshotExtensionSpatialVersion=2;
constexpr std::uint32_t CivilizationSnapshotExtensionLegacyVersion=1;

inline bool validCivilizationUnit(double value)
{
    return value>=0.0 && value<=1.0;
}

inline bool validMaterialKind(MaterialKind value)
{
    return static_cast<int>(value)>=static_cast<int>(MaterialKind::Unknown)
        && static_cast<int>(value)<=static_cast<int>(MaterialKind::Charcoal);
}

inline bool validItemKind(ItemKind value)
{
    return static_cast<int>(value)>=static_cast<int>(ItemKind::RawMaterial)
        && static_cast<int>(value)<=static_cast<int>(ItemKind::FuelBundle);
}

inline bool validTechniqueId(TechniqueId value)
{
    return static_cast<int>(value)>=static_cast<int>(TechniqueId::None)
        && static_cast<int>(value)<=static_cast<int>(TechniqueId::DugSanitationPit);
}

inline bool validKnowledgeLevel(KnowledgeLevel value)
{
    return static_cast<int>(value)>=static_cast<int>(KnowledgeLevel::Unknown)
        && static_cast<int>(value)<=static_cast<int>(KnowledgeLevel::Mastered);
}

inline bool validateInventoryState(const Inventory& inventory)
{
    for(const ItemStack& stack:inventory.stacks()){
        if(!validItemKind(stack.kind) || !validMaterialKind(stack.material)) return false;
        if(stack.quantity<=0) return false;
        if(!validCivilizationUnit(stack.quality) || !validCivilizationUnit(stack.durability)) return false;
    }
    return true;
}

inline bool validateKnowledgeState(const KnowledgeState& knowledge)
{
    std::unordered_set<int> techniques;
    for(const TechniqueKnowledge& record:knowledge.all()){
        if(record.technique==TechniqueId::None || !validTechniqueId(record.technique)) return false;
        if(!validKnowledgeLevel(record.level) || record.level==KnowledgeLevel::Unknown) return false;
        if(!validCivilizationUnit(record.confidence) || record.successfulUses<0) return false;
        if(!techniques.insert(static_cast<int>(record.technique)).second) return false;
    }
    return true;
}

inline bool validateIndividualCivilizationState(
    const IndividualCivilizationState& state,
    CharacterId expectedCharacter)
{
    return state.character==expectedCharacter
        && expectedCharacter!=0
        && validCivilizationUnit(state.gatheringSkill)
        && validCivilizationUnit(state.craftingSkill)
        && validCivilizationUnit(state.learningSkill)
        && validateInventoryState(state.inventory)
        && validateKnowledgeState(state.knowledge);
}

inline void initializeLegacyCivilizationState(World& world)
{
    for(Character& character:world.characters){
        IndividualCivilizationState state;
        state.character=character.id;
        state.gatheringSkill=std::max(0.18,std::min(0.90,
            0.30+character.personality.patience*0.24+character.personality.adaptability*0.22+
            character.personality.conscientiousness*0.14));
        state.craftingSkill=std::max(0.16,std::min(0.90,
            0.24+character.personality.openness*0.22+character.personality.patience*0.22+
            character.personality.conscientiousness*0.18));
        state.learningSkill=std::max(0.18,std::min(0.94,
            0.24+character.genetics.learningPotential*0.24+character.personality.curiosity*0.22+
            character.personality.openness*0.18));
        character.civilization=std::move(state);
    }
    world.facilities.clear();
    // World(seed) already creates the deterministic natural environment.
}

template<typename WriterT>
void writeCivilizationItemStack(WriterT& w,const ItemStack& stack)
{
    w.enumeration(stack.kind);
    w.enumeration(stack.material);
    w.i32(stack.quantity);
    w.real(stack.quality);
    w.real(stack.durability);
}

template<typename ReaderT>
bool readCivilizationItemStack(ReaderT& r,ItemStack& stack)
{
    return r.enumeration(stack.kind)
        && r.enumeration(stack.material)
        && r.i32(stack.quantity)
        && r.real(stack.quality)
        && r.real(stack.durability)
        && validItemKind(stack.kind)
        && validMaterialKind(stack.material)
        && stack.quantity>0
        && validCivilizationUnit(stack.quality)
        && validCivilizationUnit(stack.durability);
}

template<typename WriterT>
void writeCivilizationInventory(WriterT& w,const Inventory& inventory)
{
    w.u32(static_cast<std::uint32_t>(inventory.stacks().size()));
    for(const ItemStack& stack:inventory.stacks()) writeCivilizationItemStack(w,stack);
}

template<typename ReaderT>
bool readCivilizationInventory(ReaderT& r,Inventory& inventory)
{
    std::uint32_t count=0;
    if(!r.count(count)) return false;
    Inventory result;
    for(std::uint32_t i=0;i<count;++i){
        ItemStack stack;
        if(!readCivilizationItemStack(r,stack)) return false;
        result.add(stack);
    }
    inventory=std::move(result);
    return true;
}

template<typename WriterT>
void writeTechniqueKnowledgeRecord(WriterT& w,const TechniqueKnowledge& record)
{
    w.enumeration(record.technique);
    w.enumeration(record.level);
    w.real(record.confidence);
    w.i32(record.successfulUses);
}

template<typename ReaderT>
bool readTechniqueKnowledgeRecord(ReaderT& r,TechniqueKnowledge& record)
{
    return r.enumeration(record.technique)
        && r.enumeration(record.level)
        && r.real(record.confidence)
        && r.i32(record.successfulUses)
        && record.technique!=TechniqueId::None
        && validTechniqueId(record.technique)
        && record.level!=KnowledgeLevel::Unknown
        && validKnowledgeLevel(record.level)
        && validCivilizationUnit(record.confidence)
        && record.successfulUses>=0;
}

template<typename WriterT>
void writeCivilizationKnowledge(WriterT& w,const KnowledgeState& knowledge)
{
    w.u32(static_cast<std::uint32_t>(knowledge.all().size()));
    for(const TechniqueKnowledge& record:knowledge.all()) writeTechniqueKnowledgeRecord(w,record);
}

template<typename ReaderT>
bool readCivilizationKnowledge(ReaderT& r,KnowledgeState& knowledge)
{
    std::uint32_t count=0;
    if(!r.count(count)) return false;
    std::vector<TechniqueKnowledge> records;
    records.reserve(count);
    std::unordered_set<int> seen;
    for(std::uint32_t i=0;i<count;++i){
        TechniqueKnowledge record;
        if(!readTechniqueKnowledgeRecord(r,record)) return false;
        if(!seen.insert(static_cast<int>(record.technique)).second) return false;
        records.push_back(record);
    }

    auto& target=const_cast<std::vector<TechniqueKnowledge>&>(knowledge.all());
    target=std::move(records);
    return true;
}

template<typename WriterT>
void writeIndividualCivilizationState(WriterT& w,const IndividualCivilizationState& state)
{
    w.u64(state.character);
    writeCivilizationInventory(w,state.inventory);
    writeCivilizationKnowledge(w,state.knowledge);
    w.real(state.gatheringSkill);
    w.real(state.craftingSkill);
    w.real(state.learningSkill);
}

template<typename ReaderT>
bool readIndividualCivilizationState(ReaderT& r,IndividualCivilizationState& state)
{
    return r.u64(state.character)
        && readCivilizationInventory(r,state.inventory)
        && readCivilizationKnowledge(r,state.knowledge)
        && r.real(state.gatheringSkill)
        && r.real(state.craftingSkill)
        && r.real(state.learningSkill)
        && state.character!=0
        && validCivilizationUnit(state.gatheringSkill)
        && validCivilizationUnit(state.craftingSkill)
        && validCivilizationUnit(state.learningSkill);
}

template<typename WriterT>
void writeCivilizationResourceNode(WriterT& w,const ResourceNode& node)
{
    w.u64(node.id);
    w.enumeration(node.material);
    w.i32(node.quantity);
    w.i32(node.maxQuantity);
    w.boolean(node.renewable);
    w.i32(node.regenerationPerDay);
    w.i32(node.pos.x);
    w.i32(node.pos.y);
}

template<typename ReaderT>
bool readCivilizationResourceNode(
    ReaderT& r,
    ResourceNode& node,
    std::uint32_t version=CivilizationSnapshotExtensionVersion)
{
    if(!r.u64(node.id)
       || !r.enumeration(node.material)
       || !r.i32(node.quantity)
       || !r.i32(node.maxQuantity)
       || !r.boolean(node.renewable)
       || !r.i32(node.regenerationPerDay)) return false;
    if(version>=CivilizationSnapshotExtensionSpatialVersion
       && (!r.i32(node.pos.x) || !r.i32(node.pos.y))) return false;
    return node.id!=0
        && validMaterialKind(node.material)
        && node.material!=MaterialKind::Unknown
        && node.quantity>=0
        && node.maxQuantity>=node.quantity
        && node.regenerationPerDay>=0;
}

template<typename WriterT>
void writeCivilizationStorageSite(WriterT& w,const StorageSite& storage)
{
    w.u64(storage.id);
    writeCivilizationInventory(w,storage.inventory);
    w.i32(storage.pos.x);
    w.i32(storage.pos.y);
}

template<typename ReaderT>
bool readCivilizationStorageSite(
    ReaderT& r,
    StorageSite& storage,
    std::uint32_t version=CivilizationSnapshotExtensionVersion)
{
    if(!r.u64(storage.id) || storage.id==0 || !readCivilizationInventory(r,storage.inventory)) return false;
    if(version>=CivilizationSnapshotExtensionSpatialVersion
       && (!r.i32(storage.pos.x) || !r.i32(storage.pos.y))) return false;
    return true;
}

template<typename WriterT>
void writeFacilityRequirement(WriterT& w,const FacilityMaterialRequirement& requirement)
{
    w.enumeration(requirement.material);
    w.i32(requirement.required);
    w.i32(requirement.delivered);
}

template<typename ReaderT>
bool readFacilityRequirement(ReaderT& r,FacilityMaterialRequirement& requirement)
{
    return r.enumeration(requirement.material)
        && r.i32(requirement.required)
        && r.i32(requirement.delivered)
        && validMaterialKind(requirement.material)
        && requirement.material!=MaterialKind::Unknown
        && requirement.required>0
        && requirement.delivered>=0
        && requirement.delivered<=requirement.required;
}

template<typename WriterT>
void writeConstructedFacility(WriterT& w,const ConstructedFacility& facility)
{
    w.u64(facility.id);
    w.enumeration(facility.kind);
    w.enumeration(facility.state);
    w.i32(facility.pos.x);
    w.i32(facility.pos.y);
    w.u64(facility.initiatedBy);
    w.u64(facility.lastWorkedBy);
    w.i32(facility.startedMinute);
    w.i32(facility.completedMinute);
    w.real(facility.constructionWork);
    w.real(facility.requiredWork);
    w.real(facility.durability);
    w.boolean(facility.active);
    w.u64(facility.linkedStorage);
    w.u32(static_cast<std::uint32_t>(facility.requirements.size()));
    for(const auto& requirement:facility.requirements) writeFacilityRequirement(w,requirement);
}

template<typename ReaderT>
bool readConstructedFacility(ReaderT& r,ConstructedFacility& facility)
{
    if(!r.u64(facility.id)
       || !r.enumeration(facility.kind)
       || !r.enumeration(facility.state)
       || !r.i32(facility.pos.x)
       || !r.i32(facility.pos.y)
       || !r.u64(facility.initiatedBy)
       || !r.u64(facility.lastWorkedBy)
       || !r.i32(facility.startedMinute)
       || !r.i32(facility.completedMinute)
       || !r.real(facility.constructionWork)
       || !r.real(facility.requiredWork)
       || !r.real(facility.durability)
       || !r.boolean(facility.active)
       || !r.u64(facility.linkedStorage)) return false;

    std::uint32_t requirementCount=0;
    if(!r.count(requirementCount) || requirementCount==0) return false;
    facility.requirements.clear();
    facility.requirements.reserve(requirementCount);
    for(std::uint32_t i=0;i<requirementCount;++i){
        FacilityMaterialRequirement requirement;
        if(!readFacilityRequirement(r,requirement)) return false;
        facility.requirements.push_back(requirement);
    }
    return validConstructedFacility(facility);
}

template<typename WriterT>
void writeCivilizationSnapshotExtension(WriterT& w,const World& world)
{
    w.raw(CivilizationSnapshotExtensionMagic,sizeof(CivilizationSnapshotExtensionMagic));
    w.u32(CivilizationSnapshotExtensionVersion);

    w.u32(static_cast<std::uint32_t>(world.characters.size()));
    for(const Character& character:world.characters){
        writeIndividualCivilizationState(w,character.civilization);
    }

    w.u32(static_cast<std::uint32_t>(world.resourceNodes.size()));
    for(const ResourceNode& node:world.resourceNodes) writeCivilizationResourceNode(w,node);

    w.u32(static_cast<std::uint32_t>(world.storageSites.size()));
    for(const StorageSite& storage:world.storageSites) writeCivilizationStorageSite(w,storage);

    w.u32(static_cast<std::uint32_t>(world.facilities.size()));
    for(const ConstructedFacility& facility:world.facilities) writeConstructedFacility(w,facility);
}

template<typename ReaderT>
bool readCivilizationSnapshotExtension(
    ReaderT& r,
    World& world,
    std::uint32_t* outVersion=nullptr)
{
    char magic[sizeof(CivilizationSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(magic,CivilizationSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(version)
       || (version!=CivilizationSnapshotExtensionLegacyVersion
           && version!=CivilizationSnapshotExtensionSpatialVersion
           && version!=CivilizationSnapshotExtensionVersion)) return false;
    if(outVersion) *outVersion=version;

    std::uint32_t characterCount=0;
    if(!r.count(characterCount) || characterCount!=world.characters.size()) return false;
    std::unordered_set<CharacterId> restoredCharacters;
    for(std::uint32_t i=0;i<characterCount;++i){
        IndividualCivilizationState state;
        if(!readIndividualCivilizationState(r,state)) return false;
        if(!restoredCharacters.insert(state.character).second) return false;
        auto it=std::find_if(world.characters.begin(),world.characters.end(),[&](const Character& c){return c.id==state.character;});
        if(it==world.characters.end()) return false;
        it->civilization=std::move(state);
    }

    std::uint32_t resourceCount=0;
    if(!r.count(resourceCount)) return false;
    std::vector<ResourceNode> resources;
    resources.reserve(resourceCount);
    std::unordered_set<ResourceNodeId> resourceIds;
    for(std::uint32_t i=0;i<resourceCount;++i){
        ResourceNode node;
        if(!readCivilizationResourceNode(r,node,version) || !resourceIds.insert(node.id).second) return false;
        resources.push_back(node);
    }

    std::uint32_t storageCount=0;
    if(!r.count(storageCount)) return false;
    std::vector<StorageSite> storages;
    storages.reserve(storageCount);
    std::unordered_set<StorageId> storageIds;
    for(std::uint32_t i=0;i<storageCount;++i){
        StorageSite storage;
        if(!readCivilizationStorageSite(r,storage,version) || !storageIds.insert(storage.id).second) return false;
        storages.push_back(std::move(storage));
    }

    std::vector<ConstructedFacility> facilities;
    if(version>=3){
        std::uint32_t facilityCount=0;
        if(!r.count(facilityCount)) return false;
        facilities.reserve(facilityCount);
        std::unordered_set<FacilityId> facilityIds;
        for(std::uint32_t i=0;i<facilityCount;++i){
            ConstructedFacility facility;
            if(!readConstructedFacility(r,facility)
               || !facilityIds.insert(facility.id).second) return false;
            if(facility.linkedStorage!=0 && storageIds.find(facility.linkedStorage)==storageIds.end())
                return false;
            facilities.push_back(std::move(facility));
        }
    }

    world.resourceNodes=std::move(resources);
    world.storageSites=std::move(storages);
    world.facilities=std::move(facilities);
    return true;
}

} // namespace lifelens
