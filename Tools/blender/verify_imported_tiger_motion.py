"""Measure real deformed source vertices; flat-ground checks are not Observer/device QA."""
import json
import math
from pathlib import Path
import sys

import bpy


def main():
    out=Path(sys.argv[sys.argv.index('--')+1]).resolve()
    bpy.ops.wm.open_mainfile(filepath=str(out/'tiger-rigged.blend'))
    body=bpy.data.objects['LifeLensImportedTigerBody']
    rig=bpy.data.objects['LifeLensTigerRig']
    scene=bpy.context.scene
    groups={group.name:group.index for group in body.vertex_groups}
    legs=[('Fore.L.Paw',.25),('Fore.R.Paw',.75),('Hind.R.Paw',.5),('Hind.L.Paw',0)]
    regions={name:[v.index for v in body.data.vertices
                   if any(g.group==groups[name] and g.weight>.5 for g in v.groups)]
             for name,_ in legs}
    if any(len(indices)<8 for indices in regions.values()):raise ValueError('Paw skin region absent')
    def evaluate(frame):
        scene.frame_set(frame);bpy.context.view_layer.update()
        dg=bpy.context.evaluated_depsgraph_get()
        obj=body.evaluated_get(dg);mesh=obj.to_mesh()
        points=[obj.matrix_world@vertex.co for vertex in mesh.vertices]
        obj.to_mesh_clear()
        return points
    rig.animation_data.action=bpy.data.actions['tiger_idle']
    idle=evaluate(1)
    # Use the same low sole vertices through the full clip, not a moving proxy.
    soles={name:sorted(indices,key=lambda index:idle[index].z)[:max(8,len(indices)//12)]
           for name,indices in regions.items()}
    rig.animation_data.action=bpy.data.actions['tiger_walk']
    report={name:{'max_stance_ground_error':0.,'max_stance_slide':0.,'samples':0} for name,_ in legs}
    previous={};root_xy=[];displacement=0.
    for frame in range(1,38):
        points=evaluate(frame)
        t=(frame-1)/36
        root=rig.pose.bones['Root'].matrix.translation
        root_xy.append((root.x,root.y))
        displacement=max(displacement,max((a-b).length for a,b in zip(points,idle)))
        for name,offset in legs:
            phase=(t+offset)%1
            if not .04<phase<.58:
                previous.pop(name,None);continue
            selected=[points[index] for index in soles[name]]
            z=min(point.z for point in selected)
            x=sum(point.x for point in selected)/len(selected)
            y=sum(point.y for point in selected)/len(selected)-.7096774193548387*t
            entry=report[name]
            entry['max_stance_ground_error']=max(entry['max_stance_ground_error'],abs(z))
            if name in previous:
                last=previous[name]
                if phase>last[2]:
                    entry['max_stance_slide']=max(entry['max_stance_slide'],math.hypot(x-last[0],y-last[1]))
                    previous[name]=(last[0],last[1],phase)
                else:previous[name]=(x,y,phase)
            else:previous[name]=(x,y,phase)
            entry['samples']+=1
    root_motion=max(math.hypot(x-root_xy[0][0],y-root_xy[0][1]) for x,y in root_xy)
    passed=displacement>.01 and root_motion<1e-5 and all(
        entry['samples']>=8 and entry['max_stance_ground_error']<.035 and entry['max_stance_slide']<.035
        for entry in report.values())
    result={'status':'PASS_FLAT_GROUND_ONLY' if passed else 'FAIL',
            'vertex_deformation_max':displacement,'root_horizontal_motion_max':root_motion,
            'paw_metrics':report,'observer_validation':'NOT_RUN','android_performance':'NOT_MEASURED',
            'merge_allowed':False}
    (out/'tiger-motion-validation.json').write_text(json.dumps(result,indent=2)+'\n')
    print('TIGER_MOTION_VALIDATION',json.dumps(result))
    if not passed:raise ValueError('Flat-ground skin/contact gate failed; inspect actual deformation')


if __name__=='__main__':main()
