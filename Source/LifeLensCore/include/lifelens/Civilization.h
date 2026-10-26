#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <utility>
#include <string>
#include <vector>
#include "Ids.h"
#include "SmartObject.h"

namespace lifelens {

using ResourceNodeId = std::uint64_t;
using StorageId = std::uint64_t;

enum class MaterialKind {
    Unknown,
    Stone,
    Flint,
    Wood,
    Fiber,
    Clay,
    Water,
    PlantFood,
    Bone,
    Hide,
    CopperOre,
    TinOre,
    IronOre,
    Charcoal,
    CopperMetal,
    // Appended for C1-F; persisted ordinals above remain stable.
    TinMetal,
    Bronze
};

struct MaterialProperties {
    double hardness=0.0;
    double sharpnessPotential=0.0;
    double brittleness=0.0;
    double flexibility=0.0;
    double combustibility=0.0;
    double heatResistance=0.0;
    double nutrition=0.0;
};

inline MaterialProperties materialProperties(MaterialKind kind)
{
    switch(kind){
        case MaterialKind::Stone: return {0.76,0.48,0.52,0.02,0.00,0.88,0.00};
        case MaterialKind::Flint: return {0.82,0.94,0.86,0.01,0.00,0.90,0.00};
        case MaterialKind::Wood: return {0.34,0.18,0.20,0.42,0.88,0.24,0.00};
        case MaterialKind::Fiber: return {0.08,0.02,0.08,0.92,0.72,0.08,0.00};
        case MaterialKind::Clay: return {0.12,0.01,0.18,0.66,0.00,0.34,0.00};
        case MaterialKind::Water: return {0.00,0.00,0.00,1.00,0.00,0.04,0.00};
        case MaterialKind::PlantFood: return {0.04,0.00,0.08,0.36,0.54,0.06,0.72};
        case MaterialKind::Bone: return {0.58,0.42,0.44,0.08,0.08,0.54,0.00};
        case MaterialKind::Hide: return {0.10,0.01,0.05,0.88,0.30,0.12,0.00};
        case MaterialKind::CopperOre: return {0.72,0.12,0.22,0.02,0.00,0.82,0.00};
        case MaterialKind::TinOre: return {0.62,0.10,0.26,0.02,0.00,0.76,0.00};
        case MaterialKind::IronOre: return {0.86,0.08,0.20,0.01,0.00,0.94,0.00};
        case MaterialKind::Charcoal: return {0.18,0.02,0.64,0.02,0.98,0.52,0.00};
        case MaterialKind::CopperMetal: return {0.56,0.18,0.08,0.42,0.00,0.70,0.00};
        case MaterialKind::TinMetal: return {0.42,0.12,0.06,0.55,0.00,0.58,0.00};
        case MaterialKind::Bronze: return {0.78,0.62,0.12,0.28,0.00,0.82,0.00};
        case MaterialKind::Unknown:
        default: return {};
    }
}

enum class ItemKind {
    RawMaterial,
    SharpFlake,
    StoneCuttingTool,
    Cordage,
    SimpleContainer,
    FuelBundle,
    DiggingStick,
    StoneHammer,
    // C1-F metal tools. Appended to preserve persisted item ordinals.
    BronzeAxe,
    BronzePick,
    // C5 durable proto-record medium. Appended to preserve persisted ordinals.
    RecordTablet,
    PrimitiveWrap // A3 persisted item: append-only
};

enum class ToolCapability {
    None,
    Cut,
    Chop,
    Dig,
    Strike,
    Carry,
    Heat
};

inline ToolCapability itemCapability(ItemKind kind)
{
    switch(kind){
        case ItemKind::SharpFlake: return ToolCapability::Cut;
        case ItemKind::StoneCuttingTool: return ToolCapability::Chop;
        case ItemKind::DiggingStick: return ToolCapability::Dig;
        case ItemKind::StoneHammer: return ToolCapability::Strike;
        case ItemKind::BronzeAxe: return ToolCapability::Chop;
        case ItemKind::BronzePick: return ToolCapability::Strike;
        case ItemKind::SimpleContainer: return ToolCapability::Carry;
        case ItemKind::FuelBundle: return ToolCapability::Heat;
        default: return ToolCapability::None;
    }
}

struct ItemStack {
    ItemKind kind=ItemKind::RawMaterial;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    double quality=0.5;
    double durability=1.0;
};

class Inventory {
public:
    const std::vector<ItemStack>& stacks() const { return stacks_; }

    int count(ItemKind kind,MaterialKind material,bool anyMaterial=false) const
    {
        int total=0;
        for(const auto& stack:stacks_){
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            total+=stack.quantity;
        }
        return total;
    }

    void add(ItemStack item)
    {
        if(item.quantity<=0) return;
        item.quality=clamp01(item.quality);
        item.durability=clamp01(item.durability);
        for(auto& stack:stacks_){
            if(stack.kind==item.kind && stack.material==item.material
               && near(stack.quality,item.quality) && near(stack.durability,item.durability)){
                stack.quantity+=item.quantity;
                return;
            }
        }
        stacks_.push_back(item);
    }

    bool remove(ItemKind kind,MaterialKind material,int quantity,bool anyMaterial=false)
    {
        if(quantity<=0) return true;
        if(count(kind,material,anyMaterial)<quantity) return false;
        int remaining=quantity;
        for(auto& stack:stacks_){
            if(remaining<=0) break;
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            const int take=std::min(stack.quantity,remaining);
            stack.quantity-=take;
            remaining-=take;
        }
        stacks_.erase(std::remove_if(stacks_.begin(),stacks_.end(),[](const ItemStack& s){return s.quantity<=0;}),stacks_.end());
        return remaining==0;
    }

    bool removeExactOne(const ItemStack& item)
    {
        for(auto it=stacks_.begin();it!=stacks_.end();++it){
            if(it->quantity<=0 || it->kind!=item.kind || it->material!=item.material) continue;
            if(!near(it->quality,item.quality) || !near(it->durability,item.durability)) continue;
            --it->quantity;
            if(it->quantity<=0) stacks_.erase(it);
            return true;
        }
        return false;
    }

    bool transferTo(Inventory& target,ItemKind kind,MaterialKind material,int quantity,bool anyMaterial=false)
    {
        if(quantity<=0) return true;
        if(count(kind,material,anyMaterial)<quantity) return false;

        int remaining=quantity;
        std::vector<ItemStack> moved;
        for(const auto& stack:stacks_){
            if(remaining<=0) break;
            if(stack.kind!=kind) continue;
            if(!anyMaterial && stack.material!=material) continue;
            const int take=std::min(stack.quantity,remaining);
            ItemStack part=stack;
            part.quantity=take;
            moved.push_back(part);
            remaining-=take;
        }
        if(remaining!=0) return false;
        if(!remove(kind,material,quantity,anyMaterial)) return false;
        for(const auto& stack:moved) target.add(stack);
        return true;
    }

    int agePlantFoodOneDay(
        double freshnessLoss,
        double spoilThreshold=0.08)
    {
        const double loss=std::max(0.0,freshnessLoss);
        const double threshold=clamp01(spoilThreshold);
        int spoiled=0;
        for(auto& stack:stacks_){
            if(stack.kind!=ItemKind::RawMaterial
               || stack.material!=MaterialKind::PlantFood
               || stack.quantity<=0) continue;
            stack.quality=clamp01(stack.quality-loss);
            if(stack.quality<=threshold){
                spoiled+=stack.quantity;
                stack.quantity=0;
            }
        }
        stacks_.erase(
            std::remove_if(
                stacks_.begin(),stacks_.end(),
                [](const ItemStack& stack){ return stack.quantity<=0; }),
            stacks_.end());
        return spoiled;
    }

    double averagePlantFoodFreshness() const
    {
        int units=0;
        double weighted=0.0;
        for(const auto& stack:stacks_){
            if(stack.kind!=ItemKind::RawMaterial
               || stack.material!=MaterialKind::PlantFood
               || stack.quantity<=0) continue;
            units+=stack.quantity;
            weighted+=stack.quality*static_cast<double>(stack.quantity);
        }
        return units>0 ? clamp01(weighted/static_cast<double>(units)) : 0.0;
    }

private:
    static double clamp01(double value){ return std::max(0.0,std::min(1.0,value)); }
    static bool near(double a,double b){ return a>b ? a-b<1e-9 : b-a<1e-9; }
    std::vector<ItemStack> stacks_;
};

inline int simpleContainerCount(const Inventory& inventory)
{
    return inventory.count(
        ItemKind::SimpleContainer,
        MaterialKind::Unknown,
        true);
}

inline int rawWaterUnitCount(const Inventory& inventory)
{
    return inventory.count(
        ItemKind::RawMaterial,
        MaterialKind::Water);
}

inline int portableWaterCount(const Inventory& inventory)
{
    // Each Water unit represents one filled SimpleContainer. Pairing is
    // inferred from counts so existing snapshot/item enums remain stable.
    return std::min(
        rawWaterUnitCount(inventory),
        simpleContainerCount(inventory));
}

inline int emptySimpleContainerCount(const Inventory& inventory)
{
    return std::max(
        0,
        simpleContainerCount(inventory)-rawWaterUnitCount(inventory));
}

inline bool consumePortableWater(Inventory& inventory,int quantity=1)
{
    if(quantity<=0) return true;
    if(portableWaterCount(inventory)<quantity) return false;
    // The container remains in inventory and becomes empty.
    return inventory.remove(
        ItemKind::RawMaterial,
        MaterialKind::Water,
        quantity);
}

inline bool transferPortableWater(
    Inventory& source,
    Inventory& target,
    int quantity)
{
    if(quantity<=0) return true;
    if(portableWaterCount(source)<quantity) return false;
    if(source.count(
            ItemKind::SimpleContainer,
            MaterialKind::Unknown,
            true)<quantity) return false;

    // Preflight both stacks before mutating either inventory. Water and its
    // physical container always move together.
    if(!source.transferTo(
            target,
            ItemKind::SimpleContainer,
            MaterialKind::Unknown,
            quantity,
            true)){
        return false;
    }
    if(!source.transferTo(
            target,
            ItemKind::RawMaterial,
            MaterialKind::Water,
            quantity)){
        // This should be unreachable after the preflight above. Roll the
        // container back defensively to preserve atomic water authority.
        target.transferTo(
            source,
            ItemKind::SimpleContainer,
            MaterialKind::Unknown,
            quantity,
            true);
        return false;
    }
    return true;
}

// Persistent identities within a wood node; no simulation RNG is consumed.
struct NaturalTreeState {
    int ordinal=0;
    int bornMinute=0;
    int remainingWood=0;
    int capacity=0;
    int felledMinute=-1;
    int generation=0;
};

struct ResourceNode {
    ResourceNodeId id=0;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    int maxQuantity=0;
    bool renewable=false;
    int regenerationPerDay=0;
    GridPos pos{};

    std::vector<NaturalTreeState> trees;
    int ecologyMinute=0;
    double flower01=0.0;
    double fruit01=0.0;

    void ensureTrees()
    {
        if(material!=MaterialKind::Wood || !trees.empty() || maxQuantity<=0) return;
        int stock=quantity;
        for(int i=0;i<8;++i){
            const int capacity=maxQuantity/8+(i<maxQuantity%8 ? 1 : 0);
            const int remaining=std::min(stock,capacity);
            stock-=remaining;
            const int years=8+static_cast<int>((id^(static_cast<std::uint64_t>(i+1)*0x9e3779b97f4a7c15ULL))%48);
            // Initial birth dates belong to the world epoch, not first observation.
            trees.push_back({i,-years*365*1440,remaining,capacity,-1,0});
        }
    }

    int harvest(int requested,Inventory& destination,double efficiency=1.0)
    {
        if(requested<=0 || quantity<=0 || efficiency<=0.0) return 0;
        const int capacity=std::max(1,static_cast<int>(static_cast<double>(requested)*std::max(0.1,efficiency)));
        const int taken=std::min(quantity,capacity);
        ensureTrees();
        int left=taken;
        for(auto& tree:trees){
            const int cut=std::min(left,tree.remainingWood);
            tree.remainingWood-=cut; left-=cut;
            if(cut>0 && tree.remainingWood==0) tree.felledMinute=ecologyMinute;
            if(left==0) break;
        }
        quantity-=taken;
        destination.add({ItemKind::RawMaterial,material,taken,0.5,1.0});
        return taken;
    }

    void regenerateDay()
    {
        if(!renewable || regenerationPerDay<=0 || maxQuantity<=quantity) return;
        ensureTrees();
        int growth=std::min(maxQuantity-quantity,regenerationPerDay);
        quantity+=growth;
        for(auto& tree:trees){
            const int added=std::min(growth,tree.capacity-tree.remainingWood);
            if(added>0 && tree.remainingWood==0){
                tree.bornMinute=ecologyMinute; ++tree.generation;
            }
            tree.remainingWood+=added; growth-=added;
            if(growth==0) break;
        }
    }
};

struct StorageSite {
    StorageId id=0;
    Inventory inventory;
    GridPos pos{};
};

enum class TechniqueId {
    None,
    SharpFlake,
    ChippedStoneTool,
    FireMaking,
    FiberCordage,
    SimpleContainer,
    DesignatedSanitationArea,
    DugSanitationPit,
    PrimitiveStorage,
    DiggingStick,
    StoneHammer,
    CopperSmelting,
    // Appended to preserve every persisted primitive-technique ordinal.
    Cultivation,
    TinSmelting,
    BronzeAlloying,
    BronzeAxe,
    BronzePick,
    PrimitiveWrap
};

enum class KnowledgeLevel : int {
    Unknown=0,
    Observed=1,
    Hypothesized=2,
    Understood=3,
    Reproducible=4,
    Practiced=5,
    Mastered=6
};

struct TechniqueKnowledge {
    TechniqueId technique=TechniqueId::None;
    KnowledgeLevel level=KnowledgeLevel::Unknown;
    double confidence=0.0;
    int successfulUses=0;
};

class KnowledgeState {
public:
    KnowledgeLevel level(TechniqueId technique) const
    {
        const TechniqueKnowledge* record=find(technique);
        return record ? record->level : KnowledgeLevel::Unknown;
    }

    double confidence(TechniqueId technique) const
    {
        const TechniqueKnowledge* record=find(technique);
        return record ? record->confidence : 0.0;
    }

    bool knowsAtLeast(TechniqueId technique,KnowledgeLevel minimum) const
    {
        return static_cast<int>(level(technique))>=static_cast<int>(minimum);
    }

    void learn(TechniqueId technique,KnowledgeLevel newLevel,double newConfidence)
    {
        if(technique==TechniqueId::None) return;
        TechniqueKnowledge* record=findMutable(technique);
        if(!record){
            records_.push_back({technique,newLevel,clamp01(newConfidence),0});
            return;
        }
        if(static_cast<int>(newLevel)>static_cast<int>(record->level)) record->level=newLevel;
        record->confidence=std::max(record->confidence,clamp01(newConfidence));
    }

    void recordSuccessfulUse(TechniqueId technique)
    {
        TechniqueKnowledge* record=findMutable(technique);
        if(!record) return;
        ++record->successfulUses;
        if(record->successfulUses>=3 && record->level==KnowledgeLevel::Reproducible) record->level=KnowledgeLevel::Practiced;
        if(record->successfulUses>=12 && record->level==KnowledgeLevel::Practiced) record->level=KnowledgeLevel::Mastered;
        record->confidence=clamp01(record->confidence+0.03);
    }

    const std::vector<TechniqueKnowledge>& all() const { return records_; }

private:
    static double clamp01(double value){ return std::max(0.0,std::min(1.0,value)); }
    const TechniqueKnowledge* find(TechniqueId technique) const
    {
        for(const auto& record:records_) if(record.technique==technique) return &record;
        return nullptr;
    }
    TechniqueKnowledge* findMutable(TechniqueId technique)
    {
        for(auto& record:records_) if(record.technique==technique) return &record;
        return nullptr;
    }
    std::vector<TechniqueKnowledge> records_;
};

enum class ExperimentKind {
    StrikeStone,
    HaftSharpFlake,
    FrictionWood,
    TwistFiber,
    ShapeClay,
    DesignateSanitationArea,
    DigSanitationPit,
    OrganizeStockpile,
    ShapeDiggingStick,
    HaftStoneHammer,
    SmeltCopperOre,
    CultivatePlantFood,
    SmeltTinOre,
    AlloyBronze,
    CastBronzeAxe,
    CastBronzePick,
    WeavePrimitiveWrap,
    // Generic, physical-material-driven join experiment. Unlike Technique
    // discoveries this does not pre-name a recipe or grant knowledge.
    ComposeMaterials
};

enum class CivilizationEventType {
    Gathered,
    Stored,
    ExperimentFailed,
    Discovered,
    Crafted,
    // Appended to preserve the numeric values of existing persisted/debug
    // event kinds. Retrieval moves already-stored authority back to a resident.
    Retrieved,
    // Appended for Core-authored frontier search. The event is emitted only
    // after a resident physically reaches an unexplored chunk and materializes it.
    Explored,
    // Appended for C3-E. A successful experiment becomes Rediscovered only
    // when the technique has older historical evidence and no other living
    // resident currently retains that knowledge.
    Rediscovered,
    Hunted,
    HuntFailed
};

struct CivilizationEvent {
    CivilizationEventType type=CivilizationEventType::Gathered;
    CharacterId actor=0;
    TechniqueId technique=TechniqueId::None;
    MaterialKind material=MaterialKind::Unknown;
    ItemKind item=ItemKind::RawMaterial;
    int quantity=0;
};

struct ExperimentContext {
    std::uint64_t worldSeed=1;
    CharacterId actor=0;
    std::uint64_t attemptIndex=0;
    ExperimentKind kind=ExperimentKind::StrikeStone;
    MaterialKind material=MaterialKind::Unknown;
    double learningSkill=0.5;
    double curiosity=0.5;
    double patience=0.5;
    bool sanitationProblemRecognized=false;
    bool sanitationSiteAvailable=false;
    bool sanitationPitCandidateAvailable=false;
    bool storageProblemRecognized=false;
    bool smeltingOpportunityAvailable=false;
    bool cultivationOpportunityAvailable=false;
};

struct ExperimentResult {
    bool attempted=false;
    bool success=false;
    double roll=1.0;
    double successChance=0.0;
    TechniqueId discovered=TechniqueId::None;
    bool producedItem=false;
    ItemStack output{};
    CivilizationEvent event{};
};

inline double civilizationRoll(const ExperimentContext& context);

// One resident's genuine material-composition trial. Unlike a fixed recipe,
// head/handle/binding are found among ACTUAL carried raw materials and chosen
// from Core physical properties. The result is an existing functional tool
// family with measured quality/durability; a novel named technology is NOT
// automatically discovered and no new saved enum is needed for the artifact.
struct ComposedMaterialDesign {
    MaterialKind head=MaterialKind::Unknown;
    MaterialKind handle=MaterialKind::Unknown;
    MaterialKind binding=MaterialKind::Unknown;
    ItemKind resultingTool=ItemKind::RawMaterial;
    double testedPerformance=0.0;
    double testedDurability=0.0;
};

inline std::optional<ComposedMaterialDesign> proposeMaterialComposition(
    const Inventory& inventory)
{
    std::optional<ComposedMaterialDesign> best;
    for(const auto& head:inventory.stacks()){
        if(head.kind!=ItemKind::RawMaterial ||head.quantity<=0) continue;
        const auto h=materialProperties(head.material);
        if(h.hardness<0.45 ||h.sharpnessPotential<0.18) continue;
        for(const auto& handle:inventory.stacks()){
            if(handle.kind!=ItemKind::RawMaterial ||handle.quantity<=0
               ||head.material==handle.material) continue;
            const auto grip=materialProperties(handle.material);
            if(grip.flexibility<0.25 ||grip.hardness<0.20) continue;
            for(const auto& binder:inventory.stacks()){
                if(binder.kind!=ItemKind::RawMaterial ||binder.quantity<=0
                   ||binder.material==head.material
                   ||binder.material==handle.material) continue;
                const auto join=materialProperties(binder.material);
                if(join.flexibility<0.75 ||join.hardness>0.25) continue;
                const double sharpness=std::clamp(
                    h.sharpnessPotential*(0.58+0.42*h.hardness)
                    *(0.55+0.45*join.flexibility),0.0,1.0);
                const double impact=std::clamp(
                    h.hardness*(0.65+0.35*grip.hardness)
                    *(1.0-0.35*h.brittleness),0.0,1.0);
                ComposedMaterialDesign candidate;
                candidate.head=head.material;
                candidate.handle=handle.material;
                candidate.binding=binder.material;
                candidate.resultingTool=sharpness>=impact
                    ?ItemKind::StoneCuttingTool:ItemKind::StoneHammer;
                candidate.testedPerformance=std::max(sharpness,impact);
                candidate.testedDurability=std::clamp(
                    0.35+0.32*join.flexibility+0.20*grip.flexibility
                    -0.18*h.brittleness,0.10,1.0);
                // Stable, resource-grounded tie-break; no RNG in a proposal.
                if(!best ||candidate.testedPerformance>best->testedPerformance
                   ||(candidate.testedPerformance==best->testedPerformance
                      &&static_cast<int>(candidate.head)<
                         static_cast<int>(best->head)))
                    best=candidate;
            }
        }
    }
    return best;
}

// Mere possession of an old tool does not prohibit invention. Only an
// already-owned tool with equal-or-better tested performance AND durability
// makes this physical material hypothesis redundant for this resident.
inline bool materialCompositionWorthTesting(
    const Inventory& inventory,const ComposedMaterialDesign& design)
{
    for(const auto& stack:inventory.stacks())
        if(stack.quantity>0 &&stack.kind==design.resultingTool
           &&stack.material==design.head
           &&stack.quality>=design.testedPerformance-1e-9
           &&stack.durability>=design.testedDurability-1e-9)
            return false;
    return true;
}

inline ExperimentResult attemptMaterialComposition(
    const ExperimentContext& context,Inventory& inventory)
{
    ExperimentResult result;
    result.event.actor=context.actor;
    result.event.material=context.material;
    result.event.technique=TechniqueId::None;
    if(context.actor==0 ||context.kind!=ExperimentKind::ComposeMaterials)
        return result;
    const auto choice=proposeMaterialComposition(inventory);
    if(!choice ||choice->head!=context.material) return result;
    Inventory consumed=inventory;
    if(!consumed.remove(ItemKind::RawMaterial,choice->head,1)
       ||!consumed.remove(ItemKind::RawMaterial,choice->handle,1)
       ||!consumed.remove(ItemKind::RawMaterial,choice->binding,1))
        return result;
    // A real failed trial destroys its consumed inputs; a successful one
    // produces ONE physical object. The World RNG state is untouched.
    inventory=std::move(consumed);
    result.attempted=true;
    result.roll=civilizationRoll(context);
    result.successChance=std::clamp(
        0.12+0.38*choice->testedPerformance
        +0.20*std::clamp(context.learningSkill,0.0,1.0)
        +0.14*std::clamp(context.curiosity,0.0,1.0),0.0,0.85);
    if(result.roll>=result.successChance){
        result.event.type=CivilizationEventType::ExperimentFailed;
        return result;
    }
    result.success=true;
    result.producedItem=true;
    result.output={choice->resultingTool,choice->head,1,
        choice->testedPerformance,choice->testedDurability};
    inventory.add(result.output);
    result.event.type=CivilizationEventType::Crafted;
    result.event.item=result.output.kind;
    result.event.quantity=1;
    return result;
}

struct Ingredient {
    ItemKind kind=ItemKind::RawMaterial;
    MaterialKind material=MaterialKind::Unknown;
    int quantity=0;
    bool anyMaterial=false;
};

struct TechniqueRecipe {
    TechniqueId technique=TechniqueId::None;
    std::vector<Ingredient> inputs;
    bool producesItem=false;
    ItemKind outputKind=ItemKind::RawMaterial;
    MaterialKind outputMaterial=MaterialKind::Unknown;
    int outputQuantity=0;
};

inline TechniqueRecipe techniqueRecipe(TechniqueId technique)
{
    switch(technique){
        case TechniqueId::SharpFlake:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Flint,2,false}},true,ItemKind::SharpFlake,MaterialKind::Flint,1};
        case TechniqueId::ChippedStoneTool:
            return {technique,{{ItemKind::SharpFlake,MaterialKind::Unknown,1,true},{ItemKind::RawMaterial,MaterialKind::Wood,1,false}},true,ItemKind::StoneCuttingTool,MaterialKind::Flint,1};
        case TechniqueId::FireMaking:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Wood,2,false}},false,ItemKind::FuelBundle,MaterialKind::Wood,0};
        case TechniqueId::FiberCordage:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Fiber,2,false}},true,ItemKind::Cordage,MaterialKind::Fiber,1};
        case TechniqueId::SimpleContainer:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Clay,3,false}},true,ItemKind::SimpleContainer,MaterialKind::Clay,1};
        case TechniqueId::DiggingStick:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Wood,2,false}},true,ItemKind::DiggingStick,MaterialKind::Wood,1};
        case TechniqueId::StoneHammer:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Stone,2,false},{ItemKind::RawMaterial,MaterialKind::Wood,1,false},{ItemKind::Cordage,MaterialKind::Fiber,1,false}},true,ItemKind::StoneHammer,MaterialKind::Stone,1};
        case TechniqueId::CopperSmelting:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::CopperOre,1,false},{ItemKind::RawMaterial,MaterialKind::Charcoal,1,false}},true,ItemKind::RawMaterial,MaterialKind::CopperMetal,1};
        case TechniqueId::TinSmelting:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::TinOre,1,false},{ItemKind::RawMaterial,MaterialKind::Charcoal,1,false}},true,ItemKind::RawMaterial,MaterialKind::TinMetal,1};
        case TechniqueId::BronzeAlloying:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::CopperMetal,2,false},{ItemKind::RawMaterial,MaterialKind::TinMetal,1,false},{ItemKind::RawMaterial,MaterialKind::Charcoal,1,false}},true,ItemKind::RawMaterial,MaterialKind::Bronze,3};
        case TechniqueId::BronzeAxe:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Bronze,2,false},{ItemKind::RawMaterial,MaterialKind::Wood,1,false},{ItemKind::Cordage,MaterialKind::Fiber,1,false}},true,ItemKind::BronzeAxe,MaterialKind::Bronze,1};
        case TechniqueId::BronzePick:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Bronze,2,false},{ItemKind::RawMaterial,MaterialKind::Wood,1,false},{ItemKind::Cordage,MaterialKind::Fiber,1,false}},true,ItemKind::BronzePick,MaterialKind::Bronze,1};
        case TechniqueId::PrimitiveWrap:
            return {technique,{{ItemKind::RawMaterial,MaterialKind::Fiber,2,false},
                {ItemKind::Cordage,MaterialKind::Fiber,1,false}},true,
                ItemKind::PrimitiveWrap,MaterialKind::Fiber,1};
        case TechniqueId::Cultivation:
            // Experimentation consumes one edible plant unit as seed stock.
            // Production itself belongs to CultivatedPlot authority.
            return {technique,{{ItemKind::RawMaterial,MaterialKind::PlantFood,1,false}},false,ItemKind::RawMaterial,MaterialKind::Unknown,0};
        case TechniqueId::DesignatedSanitationArea:
        case TechniqueId::DugSanitationPit:
        case TechniqueId::PrimitiveStorage:
            return {technique,{},false,ItemKind::RawMaterial,MaterialKind::Unknown,0};
        case TechniqueId::None:
        default:
            return {};
    }
}

inline bool hasIngredients(const Inventory& inventory,const std::vector<Ingredient>& inputs)
{
    for(const auto& input:inputs){
        if(input.quantity<=0) continue;
        if(inventory.count(input.kind,input.material,input.anyMaterial)<input.quantity) return false;
    }
    return true;
}

inline bool consumeIngredients(Inventory& inventory,const std::vector<Ingredient>& inputs)
{
    if(!hasIngredients(inventory,inputs)) return false;
    for(const auto& input:inputs){
        if(input.quantity<=0) continue;
        if(!inventory.remove(input.kind,input.material,input.quantity,input.anyMaterial)) return false;
    }
    return true;
}

inline TechniqueId experimentTechnique(ExperimentKind kind)
{
    switch(kind){
        case ExperimentKind::StrikeStone: return TechniqueId::SharpFlake;
        case ExperimentKind::HaftSharpFlake: return TechniqueId::ChippedStoneTool;
        case ExperimentKind::FrictionWood: return TechniqueId::FireMaking;
        case ExperimentKind::TwistFiber: return TechniqueId::FiberCordage;
        case ExperimentKind::ShapeClay: return TechniqueId::SimpleContainer;
        case ExperimentKind::DesignateSanitationArea: return TechniqueId::DesignatedSanitationArea;
        case ExperimentKind::DigSanitationPit: return TechniqueId::DugSanitationPit;
        case ExperimentKind::OrganizeStockpile: return TechniqueId::PrimitiveStorage;
        case ExperimentKind::ShapeDiggingStick: return TechniqueId::DiggingStick;
        case ExperimentKind::HaftStoneHammer: return TechniqueId::StoneHammer;
        case ExperimentKind::SmeltCopperOre: return TechniqueId::CopperSmelting;
        case ExperimentKind::CultivatePlantFood: return TechniqueId::Cultivation;
        case ExperimentKind::SmeltTinOre: return TechniqueId::TinSmelting;
        case ExperimentKind::AlloyBronze: return TechniqueId::BronzeAlloying;
        case ExperimentKind::CastBronzeAxe: return TechniqueId::BronzeAxe;
        case ExperimentKind::CastBronzePick: return TechniqueId::BronzePick;
        case ExperimentKind::WeavePrimitiveWrap: return TechniqueId::PrimitiveWrap;
        default: return TechniqueId::None;
    }
}

inline std::uint64_t civilizationMix(std::uint64_t value)
{
    value+=0x9e3779b97f4a7c15ULL;
    value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
    value=(value^(value>>27))*0x94d049bb133111ebULL;
    return value^(value>>31);
}

inline double civilizationRoll(const ExperimentContext& context)
{
    std::uint64_t value=context.worldSeed ? context.worldSeed : 1;
    value=civilizationMix(value^context.actor);
    value=civilizationMix(value^(static_cast<std::uint64_t>(context.kind)+1ULL)*0x9e3779b97f4a7c15ULL);
    value=civilizationMix(value^(static_cast<std::uint64_t>(context.material)+1ULL)*0xbf58476d1ce4e5b9ULL);
    value=civilizationMix(value^context.attemptIndex*0x94d049bb133111ebULL);
    const std::uint64_t mantissa=value>>11;
    return static_cast<double>(mantissa)*(1.0/9007199254740992.0);
}

inline double experimentBaseChance(ExperimentKind kind,MaterialKind material)
{
    switch(kind){
        case ExperimentKind::StrikeStone:
            if(material==MaterialKind::Flint) return 0.42;
            if(material==MaterialKind::Stone) return 0.24;
            return 0.0;
        case ExperimentKind::HaftSharpFlake: return material==MaterialKind::Wood ? 0.28 : 0.0;
        case ExperimentKind::FrictionWood: return material==MaterialKind::Wood ? 0.15 : 0.0;
        case ExperimentKind::TwistFiber: return material==MaterialKind::Fiber ? 0.34 : 0.0;
        case ExperimentKind::WeavePrimitiveWrap: return material==MaterialKind::Fiber ? 0.28 : 0.0;
        case ExperimentKind::ShapeClay: return material==MaterialKind::Clay ? 0.30 : 0.0;
        case ExperimentKind::ShapeDiggingStick: return material==MaterialKind::Wood ? 0.29 : 0.0;
        case ExperimentKind::HaftStoneHammer: return material==MaterialKind::Stone ? 0.24 : 0.0;
        case ExperimentKind::SmeltCopperOre: return material==MaterialKind::CopperOre ? 0.18 : 0.0;
        case ExperimentKind::SmeltTinOre: return material==MaterialKind::TinOre ? 0.20 : 0.0;
        case ExperimentKind::AlloyBronze: return material==MaterialKind::Bronze ? 0.16 : 0.0;
        case ExperimentKind::CastBronzeAxe: return material==MaterialKind::Bronze ? 0.26 : 0.0;
        case ExperimentKind::CastBronzePick: return material==MaterialKind::Bronze ? 0.24 : 0.0;
        case ExperimentKind::CultivatePlantFood: return material==MaterialKind::PlantFood ? 0.22 : 0.0;
        case ExperimentKind::DesignateSanitationArea: return material==MaterialKind::Unknown ? 0.32 : 0.0;
        case ExperimentKind::DigSanitationPit: return material==MaterialKind::Unknown ? 0.28 : 0.0;
        case ExperimentKind::OrganizeStockpile: return material==MaterialKind::Unknown ? 0.34 : 0.0;
        default: return 0.0;
    }
}

inline double clampCivilization01(double value)
{
    return std::max(0.0,std::min(1.0,value));
}

inline TechniqueRecipe experimentRecipe(const ExperimentContext& context)
{
    TechniqueRecipe recipe=techniqueRecipe(experimentTechnique(context.kind));
    if(context.kind==ExperimentKind::StrikeStone && context.material==MaterialKind::Stone){
        recipe.inputs={{ItemKind::RawMaterial,MaterialKind::Stone,2,false}};
        recipe.outputMaterial=MaterialKind::Stone;
    }
    if(context.kind==ExperimentKind::HaftSharpFlake){
        recipe.inputs={{ItemKind::SharpFlake,MaterialKind::Unknown,1,true},{ItemKind::RawMaterial,MaterialKind::Wood,1,false}};
    }
    return recipe;
}

inline bool experimentPrerequisitesMet(const ExperimentContext& context,const KnowledgeState& knowledge)
{
    if(context.kind==ExperimentKind::HaftSharpFlake){
        return knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::ShapeDiggingStick){
        return knowledge.knowsAtLeast(TechniqueId::SharpFlake,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::WeavePrimitiveWrap){
        // More complex woven structures require demonstrated, practiced
        // cordage work rather than a free technology timer.
        return knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Practiced);
    }
    if(context.kind==ExperimentKind::HaftStoneHammer){
        return knowledge.knowsAtLeast(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::SmeltCopperOre){
        return context.smeltingOpportunityAvailable
            && knowledge.knowsAtLeast(TechniqueId::FireMaking,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::SimpleContainer,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::SmeltTinOre){
        return context.smeltingOpportunityAvailable
            && knowledge.knowsAtLeast(TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::AlloyBronze){
        return context.smeltingOpportunityAvailable
            && knowledge.knowsAtLeast(TechniqueId::CopperSmelting,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::TinSmelting,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::CastBronzeAxe){
        return knowledge.knowsAtLeast(TechniqueId::BronzeAlloying,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::ChippedStoneTool,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::CastBronzePick){
        return knowledge.knowsAtLeast(TechniqueId::BronzeAlloying,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::StoneHammer,KnowledgeLevel::Reproducible)
            && knowledge.knowsAtLeast(TechniqueId::FiberCordage,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::CultivatePlantFood){
        return context.cultivationOpportunityAvailable
            && knowledge.knowsAtLeast(
                TechniqueId::DiggingStick,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::DesignateSanitationArea){
        return context.sanitationProblemRecognized && context.sanitationSiteAvailable;
    }
    if(context.kind==ExperimentKind::DigSanitationPit){
        return context.sanitationPitCandidateAvailable
            && knowledge.knowsAtLeast(
                TechniqueId::DesignatedSanitationArea,KnowledgeLevel::Reproducible);
    }
    if(context.kind==ExperimentKind::OrganizeStockpile){
        return context.storageProblemRecognized;
    }
    return true;
}

inline ExperimentResult attemptExperiment(
    const ExperimentContext& context,
    Inventory& inventory,
    KnowledgeState& knowledge)
{
    ExperimentResult result;
    result.discovered=experimentTechnique(context.kind);
    result.event.actor=context.actor;
    result.event.technique=result.discovered;
    result.event.material=context.material;

    if(context.actor==0 || result.discovered==TechniqueId::None) return result;
    if(!experimentPrerequisitesMet(context,knowledge)) return result;

    TechniqueRecipe recipe=experimentRecipe(context);
    if(!hasIngredients(inventory,recipe.inputs)) return result;

    const double base=experimentBaseChance(context.kind,context.material);
    if(base<=0.0) return result;

    result.attempted=true;
    result.roll=civilizationRoll(context);
    result.successChance=clampCivilization01(
        base
        +clampCivilization01(context.learningSkill)*0.22
        +clampCivilization01(context.curiosity)*0.12
        +clampCivilization01(context.patience)*0.08);

    consumeIngredients(inventory,recipe.inputs);

    if(result.roll>=result.successChance){
        knowledge.learn(result.discovered,KnowledgeLevel::Hypothesized,0.30);
        result.event.type=CivilizationEventType::ExperimentFailed;
        return result;
    }

    result.success=true;
    knowledge.learn(result.discovered,KnowledgeLevel::Reproducible,0.72+0.20*(1.0-result.roll));
    knowledge.recordSuccessfulUse(result.discovered);
    result.event.type=CivilizationEventType::Discovered;

    if(recipe.producesItem && recipe.outputQuantity>0){
        result.producedItem=true;
        result.output={recipe.outputKind,recipe.outputMaterial,recipe.outputQuantity,0.45+0.35*context.learningSkill,1.0};
        inventory.add(result.output);
        result.event.item=result.output.kind;
        result.event.quantity=result.output.quantity;
    }
    return result;
}

struct CraftResult {
    bool success=false;
    ItemStack output{};
    CivilizationEvent event{};
};

inline CraftResult reproduceTechnique(
    CharacterId actor,
    TechniqueId technique,
    Inventory& inventory,
    KnowledgeState& knowledge,
    double skill=0.5)
{
    CraftResult result;
    result.event.actor=actor;
    result.event.technique=technique;
    if(actor==0 || !knowledge.knowsAtLeast(technique,KnowledgeLevel::Reproducible)) return result;

    TechniqueRecipe recipe=techniqueRecipe(technique);
    if(recipe.technique==TechniqueId::None || !hasIngredients(inventory,recipe.inputs)) return result;
    if(!consumeIngredients(inventory,recipe.inputs)) return result;

    result.success=true;
    result.event.type=CivilizationEventType::Crafted;
    knowledge.recordSuccessfulUse(technique);
    if(recipe.producesItem && recipe.outputQuantity>0){
        result.output={recipe.outputKind,recipe.outputMaterial,recipe.outputQuantity,0.50+0.35*clampCivilization01(skill),1.0};
        inventory.add(result.output);
        result.event.item=result.output.kind;
        result.event.material=result.output.material;
        result.event.quantity=result.output.quantity;
    }
    return result;
}

struct IndividualCivilizationState {
    CharacterId character=0;
    Inventory inventory;
    KnowledgeState knowledge;
    double gatheringSkill=0.5;
    double craftingSkill=0.5;
    double learningSkill=0.5;
};

inline CivilizationEvent gatherResource(
    IndividualCivilizationState& individual,
    ResourceNode& node,
    int requested)
{
    CivilizationEvent event;
    event.type=CivilizationEventType::Gathered;
    event.actor=individual.character;
    event.material=node.material;
    event.item=ItemKind::RawMaterial;
    const double efficiency=0.75+0.5*clampCivilization01(individual.gatheringSkill);

    if(node.material==MaterialKind::Water){
        const int emptyContainers=emptySimpleContainerCount(individual.inventory);
        if(requested<=0 || node.quantity<=0 || emptyContainers<=0) return event;

        const int skillCapacity=std::max(
            1,
            static_cast<int>(
                static_cast<double>(requested)*std::max(0.1,efficiency)));
        const int taken=std::min({
            node.quantity,
            skillCapacity,
            emptyContainers
        });
        if(taken<=0) return event;

        node.quantity-=taken;
        individual.inventory.add({
            ItemKind::RawMaterial,
            MaterialKind::Water,
            taken,
            0.5,
            1.0
        });
        event.quantity=taken;
        return event;
    }

    event.quantity=node.harvest(
        requested,
        individual.inventory,
        efficiency);
    return event;
}

inline CivilizationEvent storeItems(
    IndividualCivilizationState& individual,
    StorageSite& storage,
    ItemKind kind,
    MaterialKind material,
    int quantity,
    bool anyMaterial=false)
{
    CivilizationEvent event;
    event.type=CivilizationEventType::Stored;
    event.actor=individual.character;
    event.item=kind;
    event.material=material;
    const bool moved=
        kind==ItemKind::RawMaterial && material==MaterialKind::Water
            ? transferPortableWater(
                individual.inventory,
                storage.inventory,
                quantity)
            : individual.inventory.transferTo(
                storage.inventory,
                kind,
                material,
                quantity,
                anyMaterial);
    if(moved) event.quantity=quantity;
    return event;
}

inline CivilizationEvent retrieveItems(
    IndividualCivilizationState& individual,
    StorageSite& storage,
    ItemKind kind,
    MaterialKind material,
    int quantity,
    bool anyMaterial=false)
{
    CivilizationEvent event;
    event.type=CivilizationEventType::Retrieved;
    event.actor=individual.character;
    event.item=kind;
    event.material=material;
    const bool moved=
        kind==ItemKind::RawMaterial && material==MaterialKind::Water
            ? transferPortableWater(
                storage.inventory,
                individual.inventory,
                quantity)
            : storage.inventory.transferTo(
                individual.inventory,
                kind,
                material,
                quantity,
                anyMaterial);
    if(moved){
        event.quantity=quantity;
    }
    return event;
}

} // namespace lifelens

