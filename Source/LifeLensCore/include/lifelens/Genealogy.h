#pragma once

#include <algorithm>
#include <cstddef>
#include <queue>
#include <unordered_set>
#include <vector>

#include "Ids.h"

namespace lifelens {

enum class KinshipType {
    Unrelated,
    Self,
    Parent,
    Child,
    Sibling,
    HalfSibling,
    Spouse,
    Grandparent,
    Grandchild,
    InLaw
};

inline bool isRomanceProhibitedKinship(KinshipType type)
{
    switch(type){
        case KinshipType::Self:
        case KinshipType::Parent:
        case KinshipType::Child:
        case KinshipType::Sibling:
        case KinshipType::HalfSibling:
        case KinshipType::Grandparent:
        case KinshipType::Grandchild:
            return true;
        case KinshipType::Unrelated:
        case KinshipType::Spouse:
        case KinshipType::InLaw:
            return false;
    }
    return true;
}

struct GenealogyNode {
    CharacterId characterId=0;
    std::vector<CharacterId> parents;
    std::vector<CharacterId> children;
    std::vector<CharacterId> spouses;
};

inline bool genealogyContains(const std::vector<CharacterId>& ids,CharacterId id)
{
    return std::find(ids.begin(),ids.end(),id)!=ids.end();
}

inline void genealogyAddUnique(std::vector<CharacterId>& ids,CharacterId id)
{
    if(id!=0 && !genealogyContains(ids,id)) ids.push_back(id);
}

class GenealogyBook {
public:
    GenealogyNode& getOrCreate(CharacterId id)
    {
        for(auto& node:nodes_) if(node.characterId==id) return node;
        nodes_.push_back(GenealogyNode{});
        nodes_.back().characterId=id;
        return nodes_.back();
    }

    const GenealogyNode* find(CharacterId id) const
    {
        for(const auto& node:nodes_) if(node.characterId==id) return &node;
        return nullptr;
    }

    bool canRegisterBirth(CharacterId child,CharacterId parentA,CharacterId parentB) const
    {
        if(child==0 || parentA==0 || parentB==0 || child==parentA || child==parentB || parentA==parentB) return false;
        const GenealogyNode* existing=find(child);
        if(existing==nullptr || existing->parents.empty()) return true;
        return existing->parents.size()==2 &&
               genealogyContains(existing->parents,parentA) &&
               genealogyContains(existing->parents,parentB);
    }

    bool registerBirth(CharacterId child,CharacterId parentA,CharacterId parentB)
    {
        if(!canRegisterBirth(child,parentA,parentB)) return false;

        // Ensure every node exists first. Creating a later node can reallocate the
        // vector, so references/pointers are acquired only after all growth is done.
        getOrCreate(child);
        getOrCreate(parentA);
        getOrCreate(parentB);

        GenealogyNode* childNode=findMutable(child);
        GenealogyNode* a=findMutable(parentA);
        GenealogyNode* b=findMutable(parentB);
        if(childNode==nullptr || a==nullptr || b==nullptr) return false;

        genealogyAddUnique(childNode->parents,parentA);
        genealogyAddUnique(childNode->parents,parentB);
        genealogyAddUnique(a->children,child);
        genealogyAddUnique(b->children,child);
        return true;
    }

    bool linkSpouses(CharacterId a,CharacterId b)
    {
        if(a==0 || b==0 || a==b) return false;

        getOrCreate(a);
        getOrCreate(b);
        GenealogyNode* first=findMutable(a);
        GenealogyNode* second=findMutable(b);
        if(first==nullptr || second==nullptr) return false;

        genealogyAddUnique(first->spouses,b);
        genealogyAddUnique(second->spouses,a);
        return true;
    }

    bool unlinkSpouses(CharacterId a,CharacterId b)
    {
        GenealogyNode* first=findMutable(a);
        GenealogyNode* second=findMutable(b);
        if(first==nullptr || second==nullptr) return false;
        const auto eraseId=[](std::vector<CharacterId>& ids,CharacterId id){
            const auto old=ids.size();
            ids.erase(std::remove(ids.begin(),ids.end(),id),ids.end());
            return ids.size()!=old;
        };
        const bool one=eraseId(first->spouses,b);
        const bool two=eraseId(second->spouses,a);
        return one || two;
    }

    std::size_t sharedParentCount(CharacterId a,CharacterId b) const
    {
        const GenealogyNode* first=find(a);
        const GenealogyNode* second=find(b);
        if(first==nullptr || second==nullptr) return 0;
        std::size_t count=0;
        for(CharacterId parent:first->parents){
            if(genealogyContains(second->parents,parent)) ++count;
        }
        return count;
    }

    bool isDirectParent(CharacterId parent,CharacterId child) const
    {
        const GenealogyNode* childNode=find(child);
        return childNode!=nullptr && genealogyContains(childNode->parents,parent);
    }

    bool isSiblingLike(CharacterId a,CharacterId b) const
    {
        return a!=b && sharedParentCount(a,b)>0;
    }

    KinshipType relationBetween(CharacterId from,CharacterId to) const
    {
        if(from==0 || to==0) return KinshipType::Unrelated;
        if(from==to) return KinshipType::Self;
        if(isDirectParent(from,to)) return KinshipType::Parent;
        if(isDirectParent(to,from)) return KinshipType::Child;

        const GenealogyNode* fromNode=find(from);
        const GenealogyNode* toNode=find(to);
        if(fromNode!=nullptr && genealogyContains(fromNode->spouses,to)) return KinshipType::Spouse;

        const std::size_t shared=sharedParentCount(from,to);
        if(shared>=2) return KinshipType::Sibling;
        if(shared==1) return KinshipType::HalfSibling;

        if(toNode!=nullptr){
            for(CharacterId parent:toNode->parents){
                if(isDirectParent(from,parent)) return KinshipType::Grandparent;
            }
        }
        if(fromNode!=nullptr){
            for(CharacterId parent:fromNode->parents){
                if(isDirectParent(to,parent)) return KinshipType::Grandchild;
            }
        }

        if(isInLaw(from,to)) return KinshipType::InLaw;
        return KinshipType::Unrelated;
    }

    std::vector<CharacterId> ancestors(CharacterId id,int maxDepth=8) const
    {
        return traverse(id,maxDepth,true);
    }

    std::vector<CharacterId> descendants(CharacterId id,int maxDepth=8) const
    {
        return traverse(id,maxDepth,false);
    }

    const std::vector<GenealogyNode>& all() const { return nodes_; }

private:
    GenealogyNode* findMutable(CharacterId id)
    {
        for(auto& node:nodes_) if(node.characterId==id) return &node;
        return nullptr;
    }

    bool isInLaw(CharacterId a,CharacterId b) const
    {
        const GenealogyNode* first=find(a);
        const GenealogyNode* second=find(b);
        if(first==nullptr || second==nullptr) return false;

        for(CharacterId spouse:first->spouses){
            if(isDirectParent(spouse,b) || isDirectParent(b,spouse) || isSiblingLike(spouse,b)) return true;
        }
        for(CharacterId spouse:second->spouses){
            if(isDirectParent(spouse,a) || isDirectParent(a,spouse) || isSiblingLike(spouse,a)) return true;
        }
        return false;
    }

    std::vector<CharacterId> traverse(CharacterId id,int maxDepth,bool upward) const
    {
        std::vector<CharacterId> result;
        if(id==0 || maxDepth<=0) return result;
        std::queue<std::pair<CharacterId,int>> pending;
        std::unordered_set<CharacterId> seen;
        pending.push({id,0});
        seen.insert(id);
        while(!pending.empty()){
            const auto current=pending.front();
            pending.pop();
            if(current.second>=maxDepth) continue;
            const GenealogyNode* node=find(current.first);
            if(node==nullptr) continue;
            const auto& next=upward ? node->parents : node->children;
            for(CharacterId relative:next){
                if(seen.insert(relative).second){
                    result.push_back(relative);
                    pending.push({relative,current.second+1});
                }
            }
        }
        return result;
    }

    std::vector<GenealogyNode> nodes_;
};

} // namespace lifelens
