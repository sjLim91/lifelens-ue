"""Local EGL shader check; no browser, Core, or world generation involved."""
import ctypes as c, pathlib, re, json, argparse
root=pathlib.Path(__file__).resolve().parents[3]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--three-chunks', type=pathlib.Path, default=root/'web/node_modules/three/src/renderers/shaders/ShaderChunk')
parser.add_argument('--output', type=pathlib.Path)
args=parser.parse_args()
egl=c.CDLL('libEGL.so.1')
egl.eglGetProcAddress.restype=c.c_void_p
def ep(name, result, *args):
    fn=getattr(egl,name);fn.restype=result;fn.argtypes=list(args);return fn
def gp(name, result, *args):
    return c.CFUNCTYPE(result,*args)(egl.eglGetProcAddress(name.encode()))
getdisplay=gp('eglGetPlatformDisplayEXT',c.c_void_p,c.c_uint,c.c_void_p,c.POINTER(c.c_int))
display=getdisplay(0x31DD,None,None)
a,b=c.c_int(),c.c_int()
assert ep('eglInitialize',c.c_uint,c.c_void_p,c.POINTER(c.c_int),c.POINTER(c.c_int))(display,c.byref(a),c.byref(b))
assert ep('eglBindAPI',c.c_uint,c.c_uint)(0x30A0)
attrs=(c.c_int*13)(0x3033,1,0x3040,4,0x3024,8,0x3023,8,0x3022,8,0x3021,8,0x3038)
config=c.c_void_p();count=c.c_int()
assert ep('eglChooseConfig',c.c_uint,c.c_void_p,c.POINTER(c.c_int),c.POINTER(c.c_void_p),c.c_int,c.POINTER(c.c_int))(display,attrs,c.byref(config),1,c.byref(count)) and count.value
context=ep('eglCreateContext',c.c_void_p,c.c_void_p,c.c_void_p,c.c_void_p,c.POINTER(c.c_int))(display,config,None,(c.c_int*3)(0x3098,2,0x3038))
surface=ep('eglCreatePbufferSurface',c.c_void_p,c.c_void_p,c.c_void_p,c.POINTER(c.c_int))(display,config,(c.c_int*5)(0x3057,8,0x3056,8,0x3038))
assert ep('eglMakeCurrent',c.c_uint,c.c_void_p,c.c_void_p,c.c_void_p,c.c_void_p)(display,surface,surface,context)
getstr=gp('glGetString',c.c_char_p,c.c_uint)
print('renderer:',getstr(0x1F01).decode())
chunk=lambda name: (args.three_chunks/f'{name}.glsl.js').read_text().split('`')[1]
source=(root/'web/src/render/water-surface-material.ts').read_text()
vertex=re.search(r'vertexShader: `([\s\S]*?)`',source)[1]
fragment=re.search(r'fragmentShader: `([\s\S]*?)`',source)[1]
def expand(s):return re.sub(r'#include <([^>]+)>',lambda m:chunk(m[1]),s)
def shader(kind,source):
    sh=gp('glCreateShader',c.c_uint,c.c_uint)(kind)
    s=c.c_char_p(source.encode())
    gp('glShaderSource',None,c.c_uint,c.c_int,c.POINTER(c.c_char_p),c.c_void_p)(sh,1,c.byref(s),None)
    gp('glCompileShader',None,c.c_uint)(sh)
    ok=c.c_int();gp('glGetShaderiv',None,c.c_uint,c.c_uint,c.POINTER(c.c_int))(sh,0x8B81,c.byref(ok))
    if not ok.value:
        log=c.create_string_buffer(12000);gp('glGetShaderInfoLog',None,c.c_uint,c.c_int,c.c_void_p,c.c_char_p)(sh,len(log),None,log);raise RuntimeError(log.value.decode())
    return sh
def program(fog,tone):
    defines=('#define USE_FOG\n' if fog else '')+('#define FOG_EXP2\n' if fog=='exp' else '')+('#define TONE_MAPPING\n' if tone else '')
    vs=shader(0x8B31,'precision highp float;\n'+defines+'attribute vec3 position; uniform mat4 modelMatrix, viewMatrix, projectionMatrix;\n'+expand(vertex))
    fs=shader(0x8B30,'precision highp float;\n'+defines+'uniform vec3 cameraPosition;\n'+chunk('tonemapping_pars_fragment')+chunk('colorspace_pars_fragment')+'\nvec3 toneMapping(vec3 color){ return ACESFilmicToneMapping(color); }\nvec4 linearToOutputTexel(vec4 value){return sRGBTransferOETF(value);}\n'+expand(fragment))
    p=gp('glCreateProgram',c.c_uint)()
    for sh in [vs,fs]:gp('glAttachShader',None,c.c_uint,c.c_uint)(p,sh)
    gp('glLinkProgram',None,c.c_uint)(p)
    ok=c.c_int();gp('glGetProgramiv',None,c.c_uint,c.c_uint,c.POINTER(c.c_int))(p,0x8B82,c.byref(ok));assert ok.value,'link failed'
    gp('glUseProgram',None,c.c_uint)(p)
    def loc(n):return gp('glGetUniformLocation',c.c_int,c.c_uint,c.c_char_p)(p,n.encode())
    def f(n,v):gp('glUniform1f',None,c.c_int,c.c_float)(loc(n),v)
    def v3(n,v):gp('glUniform3f',None,c.c_int,c.c_float,c.c_float,c.c_float)(loc(n),*v)
    identity=[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1]
    view=identity[:];view[14]=-1
    for n,v in [('modelMatrix',identity),('viewMatrix',view),('projectionMatrix',identity)]:
        gp('glUniformMatrix4fv',None,c.c_int,c.c_int,c.c_uint,c.POINTER(c.c_float))(loc(n),1,0,(c.c_float*16)(*v))
    for n,v in {'uTime':0,'uOpacity':.8,'uWaveScale':.58,'uFlowSpeed':1.25,'uWind':0,'uRain':0,'uDaylight':1,'toneMappingExposure':1,'fogDensity':0,'fogNear':0,'fogFar':100}.items():f(n,v)
    # Representative linear-light colors; material profiles themselves are checked by CI.
    for n,v in {'uDeepColor':(.02,.10,.15),'uShallowColor':(.08,.28,.4),'uSkyColor':(.35,.55,.6),'cameraPosition':(0,3,4),'fogColor':(.2,.3,.4)}.items():v3(n,v)
    vertices=(c.c_float*9)(-1,-1,0,3,-1,0,-1,3,0)
    pos=gp('glGetAttribLocation',c.c_int,c.c_uint,c.c_char_p)(p,b'position')
    gp('glEnableVertexAttribArray',None,c.c_uint)(pos)
    gp('glVertexAttribPointer',None,c.c_uint,c.c_int,c.c_uint,c.c_uint,c.c_int,c.c_void_p)(pos,3,0x1406,0,0,c.cast(vertices,c.c_void_p))
    def pixel():
        gp('glViewport',None,c.c_int,c.c_int,c.c_int,c.c_int)(0,0,8,8)
        gp('glDrawArrays',None,c.c_uint,c.c_int,c.c_int)(4,0,3)
        data=(c.c_ubyte*4)();gp('glReadPixels',None,c.c_int,c.c_int,c.c_int,c.c_int,c.c_uint,c.c_uint,c.c_void_p)(4,4,1,1,0x1908,0x1401,data)
        assert gp('glGetError',c.c_uint)()==0
        return list(data)
    day=pixel();f('uDaylight',0);night=pixel();assert sum(night[:3])<sum(day[:3]),(day,night)
    if fog:
        if fog=='exp':f('fogDensity',10)
        else:f('fogFar',.1)
        mist=pixel();assert all(abs(mist[i]-v)<=1 for i,v in enumerate([51,76,102])),mist
    else:mist=None
    gp('glDeleteProgram',None,c.c_uint)(p)
    for sh in [vs,fs]:gp('glDeleteShader',None,c.c_uint)(sh)
    return dict(fog=fog,toneMapping=tone,day=day,night=night,denseFog=mist)
results=[program(fog,tone) for fog in [False,'linear','exp'] for tone in [False,True]]
if args.output: args.output.write_text(json.dumps(results,indent=2)+'\n')
print(json.dumps(results))
print('PASS: six shader variants compile/link/render; night dims; linear/exp fog converges to scene fog color.')
