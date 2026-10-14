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
                under = p.z < .60 and abs(p.y) < .55 and abs(p.x) < .26
                band = math.sin(p.y*23.0+3.5*math.sin(8*p.z+5*p.x))
                stripe = not under and band > .69 and -.9 < p.y < 1.0
                rgb = ([.17, .10, .075] if stripe else
                       [.83, .73, .59] if under else [.78, .39, .155])
            attr.data[li].color = (*rgb, 1.0)
    ob.data.materials.append(colored_surface(species.capitalize() + ' vertex PBR'))


def human(female):
    from anatomical_lofts import human_torso, human_arm, human_leg
    shoulder = .86 if female else 1.05
    hip = 1.16 if female else .96
    # Q5 trunk, upper & lower limbs are authored continuous anatomical
    # cross-section lofts, not stacked spheres. This replaces the egg-collage.
    parts = [
        human_torso(female),
        volume('cranium', (0,0,1.68), (.115,.105,.137)),
        volume('facial midface',(0,-.059,1.622),(.098,.067,.092)),
        volume('jaw angle',(0,-.048,1.560),(.078,.070,.052)),
        volume('nasal bridge',(0,-.139,1.632),(.020,.026,.048)),
        volume('nasal tip',(0,-.162,1.610),(.025,.021,.015)),
        capsule('trapezius and neck',(0,0,1.472),(0,0,1.585),.069,.063),
    ]
    for s in (-1,1):
        parts.extend([
            human_arm(s,shoulder),
            human_leg(s,hip),
            volume('palm',(s*.403*shoulder,-.030,.960),(.050,.033,.064)),
            volume('forefoot',(s*.134*hip,-.115,.069),(.093,.170,.055)),
            volume('auricle',(s*.115,0,1.653),(.016,.027,.044)),
        ])
        for digit in range(4):
            x=(.37+digit*.025)*s*shoulder
            parts.append(capsule('individual finger',
                                  (x,-.032,.922),(x,-.046,.850),.011))
        for digit in range(4):
            x=(.080+digit*.035)*s*hip
            parts.append(volume('toe',(x,-.245,.052),(.016,.027,.017)))
    for s in (-1,1):
        if female:
            parts.append(volume('pectoral soft anatomy',
                                (s*.111,-.115,1.375),(.087,.071,.086)))
        else:
            parts.append(volume('pectoral muscle plane',
                                (s*.122,-.128,1.398),(.119,.039,.071)))
    body=mesh_union(parts,'LifeLensSeamlessHuman',.017)
    bones=[
        ('pelvis',(0,0,.93),(0,0,1.05),None),
        ('spine_02',(0,0,1.05),(0,0,1.47),'pelvis'),
        ('neck_01',(0,0,1.47),(0,0,1.61),'spine_02'),
        ('Head',(0,0,1.61),(0,0,1.77),'neck_01'),
    ]
    for s,suffix in [(-1,'r'),(1,'l')]:
        bones+= [
            ('clavicle_'+suffix,(0,0,1.45),(.27*s*shoulder,0,1.44),'spine_02'),
            ('upperarm_'+suffix,(.27*s*shoulder,0,1.44),(.37*s*shoulder,0,1.20),'clavicle_'+suffix),
            ('lowerarm_'+suffix,(.37*s*shoulder,0,1.20),(.40*s*shoulder,0,1.01),'upperarm_'+suffix),
            ('hand_'+suffix,(.40*s*shoulder,0,1.01),(.40*s*shoulder,0,.85),'lowerarm_'+suffix),
            ('thigh_'+suffix,(s*.135*hip,0,.91),(s*.14*hip,0,.49),'pelvis'),
            ('calf_'+suffix,(s*.14*hip,0,.49),(s*.135*hip,-.035,.12),'thigh_'+suffix),
            ('foot_'+suffix,(s*.135*hip,-.035,.12),(s*.135*hip,-.23,.06),'calf_'+suffix),
        ]
    return body, make_armature('LifeLensHumanoidRig',bones), bones


def tiger():
    from anatomical_lofts import tiger_trunk, tiger_skull, tiger_foreleg, tiger_tail
    parts=[
        tiger_trunk(),
        tiger_skull(),
        capsule('heavy tiger nape',(0,.65,.93),(0,.99,1.12),.248),
        volume('short lower jaw',(0,1.365,.993),(.133,.150,.061)),
        volume('upper muzzle',(0,1.452,1.063),(.145,.115,.077)),
    ]
    for s in (-1,1):
        parts.append(tiger_foreleg(s,.46,True))
        parts.append(tiger_foreleg(s,-.57,False))
        # Lobed cheek/zygomatic arch is blended with the face, not two
        # separate cartoon-white balls on the outside of the muzzle.
        parts.append(volume('zygomatic pad',(s*.153,1.28,1.088),
                            (.069,.10,.082)))
        parts.append(volume('tiger auricle',(s*.17,1.07,1.345),
                            (.059,.067,.065)))
        for y in (.46,-.57):
            for digit in (-1,0,1):
                parts.append(volume('flattened digit',
                     (s*.290+digit*.065,y+.31,.077),(.037,.050,.030)))
    parts.append(tiger_tail())
    body=mesh_union(parts,'LifeLensSeamlessTiger',.019)
    bones=[
        ('pelvis',(0,-.55,.83),(0,-.3,.86),None),
        ('spine',(0,-.3,.86),(0,.65,.90),'pelvis'),
        ('neck',(0,.65,.90),(0,.96,1.12),'spine'),
        ('head',(0,.96,1.12),(0,1.48,1.08),'neck'),
        ('tail_01',(0,-.78,.84),(0,-1.42,.65),'pelvis'),
        ('tail_02',(0,-1.42,.65),(0,-2.12,.93),'tail_01'),
    ]
    for s,suffix in [(-1,'r'),(1,'l')]:
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
    # Skin weights and coat pigmentation require world-rest-space vertices,
    # never positions relative to the first joined primitive's original origin.
    zmax=max(v.co.z for v in skin.data.vertices)
    zmin=min(v.co.z for v in skin.data.vertices)
    assert max(abs(v) for v in skin.location) < 1e-6
    assert zmax > (1.68 if kind=='human' else 1.34),(which,'incorrect mesh bind origin',zmax)
    assert zmin < .08,(which,'incorrect ground contact origin',zmin)
    attach_weights(skin,rig,bones)
    tint_mesh(skin,kind,which=='resident-female')
    # Eye, ear, hair, paw and claw topology is authored separately from the
    # body voxel loft, then fully skinned to the current creature's bones.
    sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
    from author_facial_details import create_creature_details
    detail_mesh=create_creature_details(which,rig)
    assert len(detail_mesh.data.vertices)>1000
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
