"""LifeLens native Blender creature authoring. No paid models, APIs or downloaded art.

Build editable skinned meshes + a real deforming walk cycle, export native .blend
and GLB for offline review. These assets are NOT substituted into the observer
until separate rig/animation and mobile visual validation is complete.

blender -b -t 2 --python Tools/blender/author_creatures.py -- OUTDIR
"""
from __future__ import annotations

import math
import pathlib
import sys
import bpy
from mathutils import Vector

SEGMENTS = 20
RINGS = 12


def volume(label, center, size):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=SEGMENTS, ring_count=RINGS,
                                         location=center)
    ob = bpy.context.object
    ob.name = label
    ob.scale = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return ob


def capsule(label, a, b, rx, ry=None):
    center = (Vector(a) + Vector(b)) * 0.5
    length = (Vector(a) - Vector(b)).length
    ob = volume(label, center, (rx, ry or rx, length * .5 + rx * .6))
    ob.rotation_mode = 'QUATERNION'
    ob.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(Vector(b) - Vector(a))
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return ob


def shader(name, color, roughness=0.84):
    m = bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1.0)
    m.use_nodes = True
    p = m.node_tree.nodes.get('Principled BSDF')
    p.inputs['Base Color'].default_value = (*color, 1.0)
    p.inputs['Roughness'].default_value = roughness
    return m


def colored_surface(name, roughness=0.9):
    m = shader(name, (0.8, .55, .3), roughness)
    nodes = m.node_tree.nodes
    color = nodes.new('ShaderNodeVertexColor')
    color.layer_name = 'Col'
    m.node_tree.links.new(color.outputs['Color'],
                          nodes.get('Principled BSDF').inputs['Base Color'])
    return m


def mesh_union(objs, label, voxel=.025):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    ob = bpy.context.object
    # Joined primitives inherit the first primitive's local origin. Rebase
    # vertices to armature/world-rest coordinates BEFORE skin weights and
    # species pigment sampling: otherwise head vertices bind near the pelvis
    # and tiger ventral pigment incorrectly blankets almost the whole body.
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    ob.name = label
    mod = ob.modifiers.new('Seamless organic surface', 'REMESH')
    mod.mode = 'VOXEL'
    mod.voxel_size = voxel
    bpy.ops.object.modifier_apply(modifier=mod.name)
    smooth = ob.modifiers.new('Relax voxel surface', 'SMOOTH')
    smooth.factor = 1.15
    smooth.iterations = 2
    bpy.ops.object.modifier_apply(modifier=smooth.name)
    for poly in ob.data.polygons:
        poly.use_smooth = True
    return ob


def make_armature(name, bone_specs):
    data = bpy.data.armatures.new(name + 'Data')
    rig = bpy.data.objects.new(name, data)
    bpy.context.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode='EDIT')
    for n, head, tail, parent in bone_specs:
        b = data.edit_bones.new(n)
        b.head = head
        b.tail = tail
        if parent is not None:
            b.parent = data.edit_bones[parent]
    bpy.ops.object.mode_set(mode='OBJECT')
    return rig


def segment_distance(p, a, b):
    a, b = Vector(a), Vector(b)
    d = b - a
    t = max(0.0, min(1.0, (p - a).dot(d) / max(d.length_squared, 1e-9)))
    return (p - (a + t * d)).length


def attach_weights(ob, rig, bone_specs):
    # Bone envelopes analytically recomputed from rest coordinates after weld;
    # each vertex receives the four nearest deformation bones, no rigid
    # transform-only proxy. Blender exports JOINTS_0 / WEIGHTS_0.
    groups = {n: ob.vertex_groups.new(name=n) for n, _, _, _ in bone_specs}
    segments = [(n, a, b) for n, a, b, _ in bone_specs]
    for vert in ob.data.vertices:
        p = vert.co
        scores = sorted(((1.0 / (segment_distance(p, a, b) + .035) ** 4, n)
                        for n, a, b in segments), reverse=True)[:4]
        total = sum(v for v, _ in scores)
        for score, n in scores:
            groups[n].add([vert.index], score / total, 'REPLACE')
    mod = ob.modifiers.new('LifeLens skinning', 'ARMATURE')
    mod.object = rig
    ob.parent = rig


def tint_mesh(ob, species, female=False):
    attr = ob.data.vertex_colors.new(name='Col')
    for poly in ob.data.polygons:
        for li in poly.loop_indices:
            p = ob.data.vertices[ob.data.loops[li].vertex_index].co
            if species == 'human':
                tone = (0.77, .51, .37) if female else (.70, .45, .32)
                varied = 0.98 + 0.025 * math.sin(17*p.x+6*p.z)
                rgb = [min(1., t*varied) for t in tone]
            else:
                # Model-space stripe bands follow the barrel, not pixel-space
                # fake overlays. Cream underbelly, ochre flanks, dark tiger bands.
                under = p.z < .37
                band = math.sin(p.y*23.0+3.5*math.sin(8*p.z+5*p.x))
                stripe = not under and band > .69 and -.9 < p.y < 1.0
                rgb = ([.17, .10, .075] if stripe else
                       [.90, .79, .60] if under else [.91, .44, .105])
            attr.data[li].color = (*rgb, 1.0)
    ob.data.materials.append(colored_surface(species.capitalize() + ' vertex PBR'))


def human(female):
    w = .90 if female else 1.06
    hip = 1.11 if female else .96
    parts = [
        volume('pelvic girdle', (0,0,.94), (.22*hip,.145,.175)),
        volume('abdomen', (0,-.015,1.16), (.20,.14,.23)),
        volume('thorax', (0,0,1.39), (.265*w,.167,.225)),
        volume('cranium', (0,0,1.68), (.108,.099,.138)),
        volume('face & cheeks',(0,-.063,1.615),(.097,.068,.095)),
        volume('mandible',(0,-.047,1.562),(.079,.069,.046)),
        volume('nose bridge',(0,-.14,1.63),(.021,.031,.055)),
        volume('nose tip',(0,-.165,1.612),(.028,.026,.019)),
        capsule('neck',(0,0,1.47),(0,0,1.60),.071,.066),
    ]
    for side in (-1,1):
        s=side
        parts += [
            volume('deltoid',(.28*s*w,.012,1.43),(.115,.12,.118)),
            capsule('biceps',(.29*s*w,0,1.4),(.37*s*w,-.015,1.20),.084),
            capsule('forearm',(.37*s*w,-.015,1.20),(.40*s*w,-.03,1.015),.062),
            volume('palm',(.408*s*w,-.032,.97),(.064,.037,.082)),
            capsule('thigh',(.135*s*hip,0,.88),(.14*s*hip,-.022,.50),.119),
            capsule('calf',(.14*s*hip,-.022,.49),(.135*s*hip,-.035,.115),.073),
            volume('foot',(.135*s*hip,-.12,.072),(.099,.187,.064)),
            volume('ear',(s*.116,0,1.65),(.024,.032,.048)),
        ]
        for d in range(4):
            x=(.37+d*.025)*s*w
            parts.append(capsule('independent fingers',(x,-.03,.93),(x,-.044,.84),.014))
        for d in range(4):
            x=(.080+d*.035)*s*hip
            parts.append(volume('toes',(x,-.254,.050),(.019,.035,.022)))
    if female:
        for s in (-1,1):
            parts.append(volume('pectoral anatomy',(s*.118,-.136,1.38),(.102,.075,.092)))
    else:
        for s in (-1,1):
            parts.append(volume('pectoralis major',(s*.122,-.134,1.40),(.128,.051,.092)))
    body=mesh_union(parts,'LifeLensSeamlessHuman',.019)
    bones=[
        ('pelvis',(0,0,.93),(0,0,1.05),None),
        ('spine_02',(0,0,1.05),(0,0,1.47),'pelvis'),
        ('neck_01',(0,0,1.47),(0,0,1.61),'spine_02'),
        ('Head',(0,0,1.61),(0,0,1.77),'neck_01'),
    ]
    for s,suffix in [(-1,'r'),(1,'l')]:
        bones+= [
           ('clavicle_'+suffix,(0,0,1.45),(.27*s*w,0,1.44),'spine_02'),
           ('upperarm_'+suffix,(.27*s*w,0,1.44),(.37*s*w,0,1.20),'clavicle_'+suffix),
           ('lowerarm_'+suffix,(.37*s*w,0,1.20),(.40*s*w,0,1.01),'upperarm_'+suffix),
           ('hand_'+suffix,(.40*s*w,0,1.01),(.40*s*w,0,.85),'lowerarm_'+suffix),
           ('thigh_'+suffix,(s*.135*hip,0,.91),(s*.14*hip,0,.49),'pelvis'),
           ('calf_'+suffix,(s*.14*hip,0,.49),(s*.135*hip,-.035,.12),'thigh_'+suffix),
           ('foot_'+suffix,(s*.135*hip,-.035,.12),(s*.135*hip,-.23,.06),'calf_'+suffix),
        ]
    return body, make_armature('LifeLensHumanoidRig',bones), bones


def tiger():
    parts=[
      volume('barrel',(0,0,.81),(.31,.73,.35)),
      volume('shoulder mantle',(0,.49,.90),(.38,.32,.38)),
      volume('hindquarters',(0,-.53,.85),(.35,.32,.34)),
      capsule('neck',(0,.58,.96),(0,.93,1.11),.25),
      volume('head',(0,1.10,1.13),(.245,.29,.23)),
      volume('muzzle',(0,1.38,1.035),(.18,.16,.113)),
      volume('nose',(0,1.54,1.053),(.082,.045,.054)),
      volume('chin',(0,1.40,.948),(.14,.132,.048)),
    ]
    for s in (-1,1):
        for forward in (True,False):
            y=.46 if forward else -.57
            parts += [
              capsule('anatomical upper leg',(s*.265,y,.82),(s*.29,y,.44),.125 if forward else .145),
              capsule('lower leg',(s*.29,y,.44),(s*.29,y+.07,.11),.091),
              volume('digitigrade paw',(s*.29,y+.15,.105),(.137,.20,.085)),
            ]
            for digit in (-1,0,1):
                parts.append(volume('toe',(s*.29+digit*.075,y+.31,.077),(.040,.074,.043)))
        parts.append(volume('ear',(s*.17,1.11,1.38),(.105,.082,.135)))
        parts.append(volume('cheek fluff',(s*.204,1.33,1.03),(.096,.14,.11)))
    for i in range(10):
        t=i/9
        parts.append(volume('tail segment',(0,-.79-t*.9,.83+.25*math.sin(math.pi*t)),
                            (.085-.040*t,.125,.085-.04*t)))
    body=mesh_union(parts,'LifeLensSeamlessTiger',.025)
    bones=[
      ('pelvis',(0,-.55,.83),(0,-.3,.86),None),
      ('spine',(0,-.3,.86),(0,.65,.90),'pelvis'),
      ('neck',(0,.65,.90),(0,.96,1.12),'spine'),
      ('head',(0,.96,1.12),(0,1.48,1.08),'neck'),
      ('tail_01',(0,-.73,.81),(0,-1.17,.99),'pelvis'),
      ('tail_02',(0,-1.17,.99),(0,-1.62,.89),'tail_01'),
    ]
    for s, suffix in [(-1,'r'),(1,'l')]:
        for kind,y,parent in [('front',.48,'spine'),('back',-.57,'pelvis')]:
            a=f'{kind}_{suffix}_upper'
            b=f'{kind}_{suffix}_lower'
            bones.extend([
                (a,(s*.265,y,.79),(s*.29,y,.41),parent),
                (b,(s*.29,y,.41),(s*.29,y+.18,.10),a),
            ])
    return body,make_armature('LifeLensTigerRig',bones),bones


def idle_and_walk(rig, creature):
    action=bpy.data.actions.new('LifeLensWalkCycle')
    rig.animation_data_create()
    rig.animation_data.action=action
    rig.rotation_mode='XYZ'
    for frame in (1,7,13,19,25):
        phase=math.pi*2*(frame-1)/24
        for pb in rig.pose.bones:
            pb.rotation_mode='XYZ'
            pb.rotation_euler=(0,0,0)
            side=-1 if pb.name.endswith('_r') else 1
            if creature=='human':
                if pb.name.startswith('thigh_'):
                    pb.rotation_euler.x=side*.27*math.sin(phase)
                elif pb.name.startswith('calf_'):
                    pb.rotation_euler.x=.21*max(0,math.cos(phase*side))
                elif pb.name.startswith('upperarm_'):
                    pb.rotation_euler.x=-side*.2*math.sin(phase)
            else:
                if pb.name.startswith('front_') or pb.name.startswith('back_'):
                    phase_shift=0 if pb.name.startswith('front_') else math.pi
                    pb.rotation_euler.x=side*.19*math.sin(phase+phase_shift)
                elif pb.name.startswith('tail_'):
                    pb.rotation_euler.z=.14*math.sin(phase+.45)
            pb.keyframe_insert(data_path='rotation_euler',frame=frame,group=pb.name)
    # Keep the Action linked to armature so glTF exports a real animation clip.
    bpy.context.scene.frame_start=1
    bpy.context.scene.frame_end=25
    bpy.context.scene.frame_set(1)
    return action


def author(which,out):
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)
    # Previous rig Actions are orphaned after object deletion. Remove them
    # before export so a male/tiger GLB never inherits a female skeleton clip.
    for stale in list(bpy.data.actions):
        bpy.data.actions.remove(stale, do_unlink=True)
    if which=='resident-female':
        skin, rig, bones=human(True)
        kind='human'
    elif which=='resident-male':
        skin, rig, bones=human(False)
        kind='human'
    else:
        skin, rig, bones=tiger()
        kind='tiger'
    attach_weights(skin,rig,bones)
    tint_mesh(skin,kind,which=='resident-female')
    idle_and_walk(rig,kind)
    skin['lifelens_asset_stage']='experimental_blender_rig'
    skin['lifelens_runtime_replacement']=False
    skin['lifelens_creature']=which
    assert len(skin.data.vertices)>1000
    assert len(rig.data.bones)>=16 if kind=='human' else len(rig.data.bones)>=10
    bpy.ops.wm.save_as_mainfile(filepath=str(out/f'{which}.blend'))
    bpy.ops.export_scene.gltf(filepath=str(out/f'{which}.glb'),
                              export_format='GLB',export_skins=True,
                              export_animations=True,export_colors=True)
    print('LIFELENS_AUTHORED',which,
          len(skin.data.vertices),'vertices',len(rig.data.bones),'bones')


def main():
    args=sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else []
    if len(args)!=1:
        raise SystemExit('Expected exactly one OUTDIR after --')
    out=pathlib.Path(args[0]).resolve()
    out.mkdir(parents=True,exist_ok=True)
    for which in ('resident-female','resident-male','tiger'):
        author(which,out)
    print('LIFELENS_CREATURE_AUTHORING_OK')


if __name__=='__main__':
    main()
