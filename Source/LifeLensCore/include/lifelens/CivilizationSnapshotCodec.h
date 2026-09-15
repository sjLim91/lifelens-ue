#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <unordered_set>
#include <vector>

#include "Civilization.h"
#include "World.h"

namespace lifelens {

constexpr char CivilizationSnapshotExtensionMagic[]={'L','L','C','I','V','0','0','1'};
constexpr std::uint32_t CivilizationSnapshotExtensionVersion=1;

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
        && static_cast<int>(value)<=static_cast<int>(TechniqueId::DesignatedSanitationArea);
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

    // Snapshot restore is the only place that must reinstate the exact private
    // practice counter. The object is non-const; all() intentionally remains a
    // read-only gameplay API, so persistence performs this narrowly-scoped
    // canonical-state restore without exposing a general mutation surface.
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
}

template<typename ReaderT>
bool readCivilizationResourceNode(ReaderT& r,ResourceNode& node)
{
    return r.u64(node.id)
        && r.enumeration(node.material)
        && r.i32(node.quantity)
        && r.i32(node.maxQuantity)
        && r.boolean(node.renewable)
        && r.i32(node.regenerationPerDay)
        && node.id!=0
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
}

template<typename ReaderT>
bool readCivilizationStorageSite(ReaderT& r,StorageSite& storage)
{
    return r.u64(storage.id)
        && storage.id!=0
        && readCivilizationInventory(r,storage.inventory);
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
}

template<typename ReaderT>
bool readCivilizationSnapshotExtension(ReaderT& r,World& world)
{
    char magic[sizeof(CivilizationSnapshotExtensionMagic)]{};
    std::uint32_t version=0;
    if(!r.raw(magic,sizeof(magic))
       || std::memcmp(magic,CivilizationSnapshotExtensionMagic,sizeof(magic))!=0
       || !r.u32(version)
       || version!=CivilizationSnapshotExtensionVersion) return false;

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
        if(!readCivilizationResourceNode(r,node) || !resourceIds.insert(node.id).second) return false;
        resources.push_back(node);
    }

    std::uint32_t storageCount=0;
    if(!r.count(storageCount)) return false;
    std::vector<StorageSite> storages;
    storages.reserve(storageCount);
    std::unordered_set<StorageId> storageIds;
    for(std::uint32_t i=0;i<storageCount;++i){
        StorageSite storage;
        if(!readCivilizationStorageSite(r,storage) || !storageIds.insert(storage.id).second) return false;
        storages.push_back(std::move(storage));
    }

    world.resourceNodes=std::move(resources);
    world.storageSites=std::move(storages);
    return true;
}

} // namespace lifelens