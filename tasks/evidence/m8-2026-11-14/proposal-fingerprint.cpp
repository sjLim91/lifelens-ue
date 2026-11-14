#include <iostream>
#include <iomanip>
#include "lifelens/ArtifactInvention.h"
using namespace lifelens;
int main(){
 for(int i=0;i<192;++i){
  Character c;c.id=1;c.civilization.character=1;c.alive=true;c.lifeStage=LifeStage::Adult;
  c.personality.curiosity=0.9;c.needs={0.05,0.05,0.05,0.05,0.05};
  for(int m=1;m<=static_cast<int>(MaterialKind::Bronze);++m)for(int q=0;q<3;++q){
   const int quantity=(i*13+m*7+q)%5;
   if(quantity)c.civilization.inventory.add({ItemKind::RawMaterial,static_cast<MaterialKind>(m),quantity,0.3+0.1*q,1});
  }
  if(i%2)c.civilization.inventory.add({ItemKind::SimpleContainer,MaterialKind::Clay,2,0.5,1});
  c.civilization.knowledge.learn(TechniqueId::FireMaking,KnowledgeLevel::Reproducible,1);
  const auto initial=proposeArtifact(c);
  if(initial &&i%3==0){
   auto* t=rememberArtifactDesign(c,initial->design,480,1,1,ArtifactKnowledgeSource::Experiment);
   t->trials=i%7;t->successes=i%3;t->confidence=0.5;
  }
  if(initial &&i%5==0){
   PhysicalArtifact a;a.id=17;a.creator=1;a.createdMinute=480;a.design=initial->design;
   a.durability=measureArtifact(a.design).durability*(0.2+0.2*(i%4));
   c.civilization.inventory.artifacts.push_back(a);
  }
  const auto candidate=proposeArtifact(c);
  std::cout<<i<<' ';
  if(candidate)std::cout<<artifactDesignId(candidate->design)<<' '<<std::hexfloat<<candidate->score<<std::defaultfloat;
  else std::cout<<"none";
  std::cout<<'\n';
 }
}
