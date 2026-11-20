"""Cook the original Dev Range foliage cluster to a single Y-up GLB primitive."""
import json
import math
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'assets/generated/dev_range/prop_range_foliage_01.glb'
positions, normals = [], []

def triangle(a, b, c):
    u = [b[i] - a[i] for i in range(3)]
    v = [c[i] - a[i] for i in range(3)]
    n = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
    length = math.sqrt(sum(x*x for x in n))
    positions.extend((a, b, c)); normals.extend((tuple(x/length for x in n),)*3)

def leaf(cx, cz, angle, height, reach, width):
    dx, dz = math.cos(angle), math.sin(angle)
    sx, sz = -dz * width, dx * width
    base = (cx, 0.04, cz)
    mid = (cx + dx*reach*0.48, height*0.56, cz + dz*reach*0.48)
    tip = (cx + dx*reach, height, cz + dz*reach)
    left = (mid[0]+sx, mid[1], mid[2]+sz)
    right = (mid[0]-sx, mid[1], mid[2]-sz)
    ridge = (mid[0], mid[1]+0.07, mid[2])
    for a,b,c in ((base,left,ridge),(left,tip,ridge),(tip,right,ridge),(right,base,ridge)):
        triangle(a,b,c); triangle(c,b,a)

for cx,cz,height in ((-0.95,0.0,0.82),(0.0,0.18,1.05),(0.95,-0.08,0.72)):
    for i in range(7):
        leaf(cx,cz,i*2*math.pi/7 + cx*0.2,height*(0.75+0.05*(i%4)),0.38+0.035*(i%3),0.105)

packed_positions = b''.join(struct.pack('<3f', *p) for p in positions)
packed_normals = b''.join(struct.pack('<3f', *n) for n in normals)
binary = packed_positions + packed_normals
mins = [min(p[i] for p in positions) for i in range(3)]
maxs = [max(p[i] for p in positions) for i in range(3)]
doc = {'asset':{'version':'2.0','generator':'Nemisis foliage cook'},
       'scene':0,'scenes':[{'nodes':[0]}], 'nodes':[{'name':'prop_range_foliage_01','mesh':0}],
       'meshes':[{'name':'prop_range_foliage_01_lod0','primitives':[{'attributes':{'POSITION':0,'NORMAL':1},'material':0}]}],
       'materials':[{'name':'foliage_matte','pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1],'metallicFactor':0,'roughnessFactor':1}}],
       'buffers':[{'byteLength':len(binary)}],
       'bufferViews':[{'buffer':0,'byteOffset':0,'byteLength':len(packed_positions),'target':34962},
                      {'buffer':0,'byteOffset':len(packed_positions),'byteLength':len(packed_normals),'target':34962}],
       'accessors':[{'bufferView':0,'componentType':5126,'count':len(positions),'type':'VEC3','min':mins,'max':maxs},
                    {'bufferView':1,'componentType':5126,'count':len(normals),'type':'VEC3'}]}
js=json.dumps(doc,separators=(',',':')).encode(); js+=b' '*((-len(js))%4); binary+=b'\0'*((-len(binary))%4)
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_bytes(struct.pack('<III',0x46546c67,2,12+8+len(js)+8+len(binary))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(binary),0x004e4942)+binary)
print(f'{OUT}: {len(positions)//3} triangles, bounds {mins} .. {maxs}, {OUT.stat().st_size} bytes')
