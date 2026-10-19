"""Review additional licensed originals. Never touch runtime assets."""
import importlib.util
import json
from pathlib import Path
import shutil
spec=importlib.util.spec_from_file_location('sources','Tools/research/review_distributed_wildlife.py')
s=importlib.util.module_from_spec(spec);spec.loader.exec_module(s)
selected={
 'Wolf':['cdeff35b7a2541759a561bc921b8e01e','a2832b845e4e4edd9d439342cf4fd590','a330f31c57134b8fb59c93a033b6268f'],
 'Deer':['5d6c6d1918b64f75b76f321e8bc7a0ee','01f33919650c421e91666908db8c421e','4cff1e63f4e941198b3d56300b3393b9'],
 'Hare':['619f8da783b34797871a5b1b2f1f697d','948938cd237d49a9868662213fa05543','e7213589744d436b9d96e2dbb31198a5'],
 'FreshwaterFish':['80220d404cce4c83b859d1fba209b608']}
config=json.loads(Path('Tools/assets/native-wildlife-selection.json').read_text())
config['selected']=selected
s.ROOT=Path('docs/asset-reviews/wildlife-quality-candidates')
s.CONFIG=Path('/tmp/wildlife-quality-selection.json');s.CONFIG.write_text(json.dumps(config))
s.main()
available=[]
for species,uids in selected.items():
 for uid in uids:
  folder=s.ROOT/species/uid
  if (folder/'source.glb').is_file():
   available.append({'species':species,'uid':uid,'source':str(folder/'source.glb'),'review':str(folder/'review')})
(s.ROOT/'selection.json').write_text(json.dumps({'runtimeAdmission':False,'available':available},indent=2)+'\n')
print('AVAILABLE_QUALITY_CANDIDATES',len(available))
