#include "lifelens/Simulation.h"
#include "lifelens/CivilizationSnapshotCodec.h"
#include "lifelens/WorldGenerationSnapshotCodec.h"

#include <unordered_set>

namespace lifelens {
namespace {

bool snapshotHasCharacter(const SimulationStateSnapshot& snapshot,CharacterId id)
{
    if(id==0) return false;
    for(const auto& character:snapshot.world.characters){
        if(character.id==id) return true;
    }
    return false;
}

bool validateSnapshot(const SimulationStateSnapshot& snapshot,std::string* error)
{
    const auto fail=[&](const char* message){
        if(error) *error=message;
        return false;
    };

    if(snapshot.version!=SimulationSnapshotVersion) return fail("unsupported snapshot version");
    if(!validSimulationRuleset(snapshot.ruleset)) return fail("snapshot contains invalid simulation ruleset");
    if(snapshot.world.seed==0) return fail("snapshot world seed must be nonzero");
    if(snapshot.world.minute<0) return fail("snapshot minute must be nonnegative");

    std::unordered_set<CharacterId> characterIds;
    for(const auto& character:snapshot.world.characters){
        if(character.id==0) return fail("snapshot contains zero character id");
        if(!characterIds.insert(character.id).second) return fail("snapshot contains duplicate character id");
        if(!validateIndividualCivilizationState(character.civilization,character.id))
            return fail("snapshot contains invalid character civilization state");
    }

    std::unordered_set<ResourceNodeId> resourceIds;
    for(const auto& node:snapshot.world.resourceNodes){
        if(node.id==0) return fail("snapshot contains zero resource node id");
        if(!resourceIds.insert(node.id).second) return fail("snapshot contains duplicate resource node id");
        if(!validMaterialKind(node.material) || node.material==MaterialKind::Unknown)
            return fail("snapshot contains invalid resource material");
        if(node.quantity<0 || node.maxQuantity<node.quantity || node.regenerationPerDay<0)
            return fail("snapshot contains invalid resource quantity");
    }

    std::unordered_set<StorageId> storageIds;
    for(const auto& storage:snapshot.world.storageSites){
        if(storage.id==0) return fail("snapshot contains zero storage id");
        if(!storageIds.insert(storage.id).second) return fail("snapshot contains duplicate storage id");
        if(!validateInventoryState(storage.inventory)) return fail("snapshot contains invalid storage inventory");
    }
    if(!validateWorldGenerationSnapshotState(snapshot.world))
        return fail("snapshot contains invalid world generation state");

    EnvironmentalResidueField validatedResidues;
    if(!validatedResidues.restoreState(snapshot.world.environmentalResidues.all()))
        return fail("snapshot contains invalid environmental residue state");
    for(const auto& residue:snapshot.world.environmentalResidues.all()){
        if(characterIds.count(residue.sourceCharacter)==0)
            return fail("environmental residue source character is missing");
        if(residue.createdMinute>snapshot.world.minute || residue.lastUpdatedMinute>snapshot.world.minute)
            return fail("environmental residue minute is in the future");
    }

    std::unordered_set<SanitationSiteId> sanitationSiteIds;
    int activeSanitationSites=0;
    for(const auto& site:snapshot.world.primitiveSanitationSites){
        if(!validPrimitiveSanitationSite(site))
            return fail("snapshot contains invalid primitive sanitation site");
        if(!sanitationSiteIds.insert(site.id).second)
            return fail("snapshot contains duplicate primitive sanitation site id");
        if(characterIds.count(site.establishedBy)==0)
            return fail("primitive sanitation site establisher is missing");
        if(site.establishedMinute>snapshot.world.minute)
            return fail("primitive sanitation site establishment minute is in the future");
        if(site.improvedBy!=0 && characterIds.count(site.improvedBy)==0)
            return fail("primitive sanitation site improver is missing");
        if(site.improvedMinute>snapshot.world.minute)
            return fail("primitive sanitation site improvement minute is in the future");
        if(site.active) ++activeSanitationSites;
    }
    if(activeSanitationSites>1)
        return fail("snapshot contains multiple active primitive sanitation sites");

    for(const auto& item:snapshot.runtime){
        if(characterIds.count(item.first)==0) return fail("runtime references missing character");
        if(item.second.actionIndex>item.second.plan.size()) return fail("runtime action index exceeds plan size");
        if(item.second.navigationRouteIndex>item.second.navigationRoute.size())
            return fail("runtime navigation route index exceeds route size");
        if(item.second.socialTarget!=0 && characterIds.count(item.second.socialTarget)==0)
            return fail("runtime social target is missing");

        const PendingContextAction& pending=item.second.pendingContext;
        const bool tokenPresent=pending.token!=0;
        const bool kindPresent=pending.kind!=ContextActionKind::None;
        if(tokenPresent!=kindPresent)
            return fail("runtime pending context token/kind mismatch");
        if(pending.active()){
            if(pending.issuedMinute<0 || pending.issuedMinute>snapshot.world.minute)
                return fail("runtime pending context minute is invalid");

            switch(pending.kind){
                case ContextActionKind::Social:
                    if(pending.social.target==0
                       || pending.social.target==item.first
                       || characterIds.count(pending.social.target)==0)
                        return fail("runtime pending social target is invalid");
                    break;
                case ContextActionKind::Parenting:
                    if(pending.parentingTarget==0
                       || characterIds.count(pending.parentingTarget)==0)
                        return fail("runtime pending parenting target is invalid");
                    break;
                case ContextActionKind::KnowledgeTeaching:
                    if(pending.knowledgeTeachingTarget==0
                       || characterIds.count(pending.knowledgeTeachingTarget)==0)
                        return fail("runtime pending teaching target is invalid");
                    break;
                case ContextActionKind::Civilization:
                case ContextActionKind::None:
                default:
                    break;
            }
        }
    }
    for(CharacterId id:characterIds){
        if(snapshot.runtime.find(id)==snapshot.runtime.end()) return fail("character is missing runtime state");
    }

    for(const auto& relationship:snapshot.relationships.all()){
        if(characterIds.count(relationship.from)==0 || characterIds.count(relationship.to)==0)
            return fail("relationship references missing character");
        if(relationship.from==relationship.to) return fail("self relationship is invalid");
    }

    for(const auto& node:snapshot.genealogy.all()){
        if(characterIds.count(node.characterId)==0) return fail("genealogy node references missing character");
        for(CharacterId id:node.parents) if(characterIds.count(id)==0) return fail("genealogy parent is missing");
        for(CharacterId id:node.children) if(characterIds.count(id)==0) return fail("genealogy child is missing");
        for(CharacterId id:node.spouses) if(characterIds.count(id)==0) return fail("genealogy spouse is missing");
    }

    for(const auto& pair:snapshot.romances.all()){
        if(characterIds.count(pair.first)==0 || characterIds.count(pair.second)==0)
            return fail("romance references missing character");
    }

    for(const auto& household:snapshot.households.all()){
        if(household.id==0) return fail("household id must be nonzero");
        for(const auto& member:household.members){
            if(characterIds.count(member.characterId)==0) return fail("household member is missing");
        }
    }

    for(const auto& pregnancy:snapshot.pregnancies.all()){
        if(characterIds.count(pregnancy.gestationalParent)==0 || characterIds.count(pregnancy.geneticPartner)==0)
            return fail("pregnancy references missing character");
    }

    for(const auto& birth:snapshot.births.all()){
        if(characterIds.count(birth.childId)==0 || characterIds.count(birth.parentA)==0 || characterIds.count(birth.parentB)==0)
            return fail("birth record references missing character");
    }

    SocialKnowledgeBook validatedKnowledge;
    if(!validatedKnowledge.restoreState(
        snapshot.socialKnowledge.facts(),snapshot.socialKnowledge.receipts()))
        return fail("snapshot contains invalid social knowledge state");

    for(const SocialFact& fact:snapshot.socialKnowledge.facts()){
        if(characterIds.count(fact.subject)==0) return fail("social fact subject is missing");
        if(fact.eventMinute<0 || fact.eventMinute>snapshot.world.minute)
            return fail("social fact minute is invalid");
    }
    for(const KnowledgeReceipt& receipt:snapshot.socialKnowledge.receipts()){
        if(characterIds.count(receipt.holder)==0 ||
           characterIds.count(receipt.originWitness)==0 ||
           characterIds.count(receipt.immediateSource)==0 ||
           characterIds.count(receipt.subject)==0)
            return fail("knowledge receipt references missing character");
        if(receipt.learnedMinute<0 || receipt.learnedMinute>snapshot.world.minute)
            return fail("knowledge receipt minute is invalid");
        for(CharacterId id:receipt.transmissionPath){
            if(characterIds.count(id)==0) return fail("knowledge path references missing character");
        }
    }

    return true;
}

} // namespace

SimulationStateSnapshot Simulation::captureSnapshot() const
{
    SimulationStateSnapshot snapshot;
    snapshot.version=SimulationSnapshotVersion;
    snapshot.ruleset=ruleset_;
    snapshot.world=world_;
    for(auto& character:snapshot.world.characters){
        if(character.civilization.character==0) character.civilization.character=character.id;
    }
    snapshot.relationships=relationships_;
    snapshot.genealogy=genealogy_;
    snapshot.romances=romances_;
    snapshot.households=households_;
    snapshot.pregnancies=pregnancies_;
    snapshot.births=births_;
    snapshot.socialKnowledge=socialKnowledge_;
    snapshot.logs=logs_;

    snapshot.runtime.reserve(runtime_.size());
    for(const auto& item:runtime_){
        const Runtime& source=item.second;
        SimulationRuntimeSnapshot target;
        target.goal=source.goal;
        target.plan=source.plan;
        target.actionIndex=source.actionIndex;
        target.pos=source.pos;
        target.announced=source.announced;
        target.lastGoal=source.lastGoal;
        target.repeatCount=source.repeatCount;
        target.consecutiveFailures=source.consecutiveFailures;
        target.penaltyUntilMinute=source.penaltyUntilMinute;
        target.socialCooldownUntilMinute=source.socialCooldownUntilMinute;
        target.socialActive=source.socialActive;
        target.socialIntent=source.socialIntent;
        target.socialTarget=source.socialTarget;
        target.pendingContext=source.pendingContext;
        target.navigationRoute=source.navigationRoute;
        target.navigationRouteIndex=source.navigationRouteIndex;
        target.navigationTarget=source.navigationTarget;
        target.navigationArrivalRadius=source.navigationArrivalRadius;
        target.navigationHasTarget=source.navigationHasTarget;
        target.navigationArrived=source.navigationArrived;
        target.navigationRouteFailed=source.navigationRouteFailed;
        snapshot.runtime.emplace(item.first,std::move(target));
    }
    return snapshot;
}

bool Simulation::restoreSnapshot(const SimulationStateSnapshot& snapshot,std::string* error)
{
    if(!validateSnapshot(snapshot,error)) return false;
    if(!sameSimulationRuleset(snapshot.ruleset,ruleset_)){
        if(error) *error="snapshot ruleset does not match simulation ruleset";
        return false;
    }

    std::unordered_map<CharacterId,Runtime> restoredRuntime;
    restoredRuntime.reserve(snapshot.runtime.size());
    for(const auto& item:snapshot.runtime){
        const SimulationRuntimeSnapshot& source=item.second;
        Runtime target;
        target.goal=source.goal;
        target.plan=source.plan;
        target.actionIndex=source.actionIndex;
        target.pos=source.pos;
        target.announced=source.announced;
        target.lastGoal=source.lastGoal;
        target.repeatCount=source.repeatCount;
        target.consecutiveFailures=source.consecutiveFailures;
        target.penaltyUntilMinute=source.penaltyUntilMinute;
        target.socialCooldownUntilMinute=source.socialCooldownUntilMinute;
        target.socialActive=source.socialActive;
        target.socialIntent=source.socialIntent;
        target.socialTarget=source.socialTarget;
        target.pendingContext=source.pendingContext;
        target.navigationRoute=source.navigationRoute;
        target.navigationRouteIndex=source.navigationRouteIndex;
        target.navigationTarget=source.navigationTarget;
        target.navigationArrivalRadius=source.navigationArrivalRadius;
        target.navigationHasTarget=source.navigationHasTarget;
        target.navigationArrived=source.navigationArrived;
        target.navigationRouteFailed=source.navigationRouteFailed;
        restoredRuntime.emplace(item.first,std::move(target));
    }

    world_=snapshot.world;
    relationships_=snapshot.relationships;
    genealogy_=snapshot.genealogy;
    romances_=snapshot.romances;
    households_=snapshot.households;
    pregnancies_=snapshot.pregnancies;
    births_=snapshot.births;
    socialKnowledge_=snapshot.socialKnowledge;
    runtime_=std::move(restoredRuntime);
    logs_=snapshot.logs;

    // Recent social communication is presentation provenance, not persisted
    // simulation truth. Clearing it prevents old speech bubbles / feed entries
    // from replaying after a load.
    clearRecentSocialEvents();

    if(error) error->clear();
    return true;
}

} // namespace lifelens