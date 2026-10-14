"""Topology-first procedural anatomy for the free LifeLens Blender pipeline.

Instead of attaching egg-shaped primitives for trunk/limbs, generate a
continuous loft with explicit anatomical cross-sections and curvature.
All generated vertices live in the bind-pose armature's local/world frame.
No web services, paid assets, stochastic samples, or scene-side effects.
"""
from __future__ import annotations

import math
import bpy
from mathutils import Vector

TAU=2*math.pi


def _catmull(a,b,c,d,t):
    # Cardinal spline with tension 0.5. No random variation and shared knot
    # tangents: produces continuous torso/limb contours.
    return .5*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t)


def interpolate_knots(knots,t):
    t=max(0.,min(1.,t))
    progress=t*(len(knots)-1)
    index=min(len(knots)-2,int(progress))
    f=progress-index
    a=knots[max(0,index-1)]
    b=knots[index]
    c=knots[index+1]
    d=knots[min(len(knots)-1,index+2)]
    return tuple(_catmull(a[k],b[k],c[k],d[k],f) for k in range(len(b)))


def _finish(name,verts,faces):
    mesh=bpy.data.meshes.new(name+'Topology')
    mesh.from_pydata(verts,[],faces)
    mesh.validate(clean_customdata=False)
    mesh.update()
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    for poly in mesh.polygons:
        poly.use_smooth=True
    assert len(verts)>100 and len(faces)>100,(name,len(verts),len(faces))
    return obj


def longitudinal_loft(name,knots,axis='z',steps=48,sides=32):
    """Four columns per knot: [axis, x center, other center, x radius, other radius].

    For a vertical human: axis z, "other" = y. For quadruped: axis y,
    "other" = z. Signed scallops along the shoulder are deliberately subtle.
    """
    assert axis in ('z','y')
    assert len(knots)>=4
    verts=[]
    faces=[]
    for j in range(steps+1):
        u=j/steps
        axial,cx,co,rx,ro=interpolate_knots(knots,u)
        rx=max(.012,rx); ro=max(.012,ro)
        for s in range(sides):
            angle=TAU*s/sides
            xx=cx+rx*math.cos(angle)
            oo=co+ro*math.sin(angle)
            if axis=='z':
                verts.append((xx,oo,axial))
            else:
                verts.append((xx,axial,oo))
    for j in range(steps):
        for s in range(sides):
            a=j*sides+s
            b=j*sides+(s+1)%sides
            c=(j+1)*sides+(s+1)%sides
            d=(j+1)*sides+s
            faces.append((a,b,c,d) if axis=='z' else (d,c,b,a))
    first=len(verts)
    verts.append(tuple(sum(verts[s][i] for s in range(sides))/sides for i in range(3)))
    last=len(verts)
    verts.append(tuple(sum(verts[steps*sides+s][i] for s in range(sides))/sides for i in range(3)))
    for s in range(sides):
        if axis=='z':
            faces.append((first,(s+1)%sides,s))
            faces.append((last,steps*sides+s,steps*sides+(s+1)%sides))
        else:
            faces.append((first,s,(s+1)%sides))
            faces.append((last,steps*sides+(s+1)%sides,steps*sides+s))
    return _finish(name,verts,faces)


def curving_limb(name,knots,steps=32,sides=16):
    """(x,y,z, width, depth) anatomical spline around centerline.

    Local ring orientation follows the limb rather than global primitive
    ellipsoids. Continuous taper makes knees, ankles, hocks and paws legible.
    """
    assert len(knots)>=4
    verts=[];faces=[]
    for j in range(steps+1):
        u=j/steps
        cx,cy,cz,rx,ry=interpolate_knots(knots,u)
        left=Vector(interpolate_knots(knots,max(0,u-.005))[:3])
        right=Vector(interpolate_knots(knots,min(1,u+.005))[:3])
        tangent=(right-left).normalized()
        if tangent.length<.01:
            tangent=Vector((0,0,1))
        xdir=Vector((1,0,0))
        xdir=(xdir-tangent*xdir.dot(tangent)).normalized()
        if xdir.length<.001:
            xdir=Vector((0,1,0))
        ydir=tangent.cross(xdir).normalized()
        for k in range(sides):
            theta=TAU*k/sides
            point=Vector((cx,cy,cz))+xdir*max(.006,rx)*math.cos(theta)+ydir*max(.006,ry)*math.sin(theta)
            verts.append(tuple(point))
    for j in range(steps):
        for k in range(sides):
            a=j*sides+k;b=j*sides+(k+1)%sides
            c=(j+1)*sides+(k+1)%sides;d=(j+1)*sides+k
            faces.append((a,b,c,d))
    verts.append(tuple(interpolate_knots(knots,0)[:3]))
    start=len(verts)-1
    verts.append(tuple(interpolate_knots(knots,1)[:3]))
    end=len(verts)-1
    for s in range(sides):
        faces.append((start,(s+1)%sides,s))
        faces.append((end,steps*sides+s,steps*sides+(s+1)%sides))
    return _finish(name,verts,faces)


def human_torso(female):
    shoulder=.86 if female else 1.05
    hip=1.16 if female else .96
    return longitudinal_loft('Anatomical rib cage waist pelvis',[
      (.78,0,-.005,.035,.075),
      (.84,0,-.005,.21*hip,.145),
      (.93,0,0,.220*hip,.15),
      (1.05,0,-.005,.17*hip,.145),
      (1.18,0,-.014,.168,.128),
      (1.29,0,0,.204*shoulder,.145),
      (1.40,0,.008,.240*shoulder,.155),
      (1.47,0,.008,.235*shoulder,.147),
      (1.52,0,0,.11,.10)
    ],axis='z',steps=70,sides=36)


def tiger_trunk():
    return longitudinal_loft('Tiger spine ribcage shoulder and flank',[
      (-.95,0,.81,.11,.13),
      (-.78,0,.83,.285,.286),
      (-.59,0,.85,.342,.327),
      (-.40,0,.81,.34,.32),
      (-.12,0,.80,.30,.31),
      (.17,0,.85,.32,.337),
      (.41,0,.90,.367,.352),
      (.58,0,.91,.346,.341),
      (.73,0,.94,.285,.295),
      (.91,0,1.01,.195,.216),
    ],axis='y',steps=72,sides=40)


def tiger_skull():
    """Build a feline wedge, not a round teddy-bear cranium.

    Broad zygomatic arches taper sharply into a low bridge; the raised
    supraorbital brows and shallow orbital sockets are formed on the
    anatomical mesh BEFORE voxel fusion and skinning.
    """
    skull=longitudinal_loft('Tiger feline skull and cranial bridge',[
      (.75,0,1.047,.070,.078),
      (.86,0,1.10,.170,.161),
      (1.00,0,1.143,.230,.186),
      (1.13,0,1.146,.253,.174),
      (1.23,0,1.138,.247,.159),
      (1.31,0,1.116,.191,.127),
      (1.38,0,1.078,.136,.088),
      (1.46,0,1.049,.093,.059),
      (1.52,0,1.04,.045,.035),
      (1.55,0,1.04,.014,.018),
    ],axis='y',steps=70,sides=44)
    for vertex in skull.data.vertices:
        x,y,z=vertex.co
        ax=abs(x)
        # Pronounced upper cheek/temporal arch but no spherical muzzle.
        cheek=math.exp(-((y-1.205)/.125)**2-((z-1.072)/.103)**2)
        vertex.co.x += math.copysign(.027*cheek,x) if ax>.012 else 0.
        # Forehead glabella is lower than the brow's two outer ridges.
        if z>1.19 and 1.06<y<1.37:
            brow=math.exp(-((ax-.125)/.060)**2-((y-1.235)/.095)**2)
            center=math.exp(-(x/.070)**2-((y-1.27)/.11)**2)
            vertex.co.z+=.016*brow-.017*center
    skull.data.update()
    skull['lifelens_feline_head']='cranial bridge + cheek arches + eye socket ridges'
    return skull



def tiger_pinna(side):
    """Triangular rounded feline ear emerging from actual skull outline.

    Ear bases are wide/flat toward the face and narrow to a soft outer tip,
    avoiding the spherical 'teddy bear ears' of Q3-Q12.
    """
    s=side
    return curving_limb('Tiger tapered anatomical pinna',[
      (s*.166,1.076,1.282,.068,.049),
      (s*.178,1.082,1.325,.070,.043),
      (s*.194,1.082,1.372,.058,.034),
      (s*.206,1.077,1.409,.038,.027),
      (s*.211,1.071,1.432,.018,.019),
      (s*.210,1.067,1.440,.007,.010),
    ],steps=30,sides=20)


def tiger_foreleg(s,y,front):
    x=s
    if front:
        pts=[
          (x*.28,y-.04,.95,.105,.130),
          (x*.315,y,.76,.145,.142),
          (x*.316,y+.023,.53,.113,.127),
          (x*.300,y-.05,.35,.082,.09),
          (x*.282,y+.02,.185,.091,.10),
          (x*.282,y+.105,.132,.092,.102),
          (x*.282,y+.198,.103,.092,.095),
          (x*.282,y+.269,.090,.024,.026),
        ]
    else:
        pts=[
          (x*.27,y-.03,1.00,.11,.14),
          (x*.315,y-.08,.75,.157,.145),
          (x*.35,y+.10,.48,.124,.125),
          (x*.295,y-.05,.30,.070,.073),
          (x*.289,y-.09,.176,.082,.087),
          (x*.29,y+.032,.123,.096,.105),
          (x*.29,y+.15,.103,.087,.09),
          (x*.29,y+.242,.090,.023,.028),
        ]
    return curving_limb('Tiger forelimb' if front else 'Tiger hind hock',pts,steps=48,sides=24)


def tiger_tail():
    # Flexible silhouette, no round sausage-segment seams.
    return curving_limb('Tiger long tapered caudal appendage',[
      (0,-.78,.87,.11,.11),
      (-.010,-.98,.81,.11,.105),
      (-.027,-1.18,.69,.100,.102),
      (.008,-1.42,.61,.087,.085),
      (.050,-1.63,.67,.071,.072),
      (.09,-1.82,.78,.063,.062),
      (.065,-1.99,.89,.052,.052),
      (.047,-2.12,.93,.027,.027),
      (.045,-2.15,.93,.006,.006)
    ],steps=72,sides=16)


def human_arm(side,shoulder):
    s=side
    return curving_limb('Human deltoid elbow forearm wrist',[
      (s*.230*shoulder,-.001,1.465,.057,.065),
      (s*.276*shoulder,.008,1.40,.084,.092),
      (s*.31*shoulder,.001,1.31,.082,.078),
      (s*.352*shoulder,-.018,1.21,.069,.069),
      (s*.365*shoulder,-.023,1.165,.065,.071),
      (s*.395*shoulder,-.030,1.063,.057,.050),
      (s*.399*shoulder,-.032,1.015,.045,.041),
      (s*.403*shoulder,-.033,.994,.032,.03)
    ],steps=45,sides=20)


def human_leg(side,hip):
    s=side
    return curving_limb('Human hip quadriceps calf Achilles',[
      (s*.135*hip,0,.955,.099,.11),
      (s*.138*hip,-.002,.87,.133,.130),
      (s*.140*hip,-.014,.69,.11,.111),
      (s*.142*hip,-.019,.50,.075,.078),
      (s*.141*hip,-.018,.44,.074,.074),
      (s*.140*hip,-.031,.315,.088,.093),
      (s*.136*hip,-.030,.17,.062,.068),
      (s*.134*hip,-.041,.10,.050,.054)
    ],steps=48,sides=22)
