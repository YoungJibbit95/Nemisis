"""Cook one static, textured trim mesh for the Outer Facility."""
from pathlib import Path
import json
import math
import struct

root = Path(__file__).resolve().parents[2]
image = (root / 'assets/textures/environments/movement_accent_basecolor.png').read_bytes()
out = root / 'assets/generated/outer_facility/map_facility_trim_01.glb'
positions, normals, uvs, indices = [], [], [], []

def quad(points, normal):
    base = len(positions)
    positions.extend(points)
    normals.extend([normal] * 4)
    a=math.dist(points[0],points[1])/3.0
    b=math.dist(points[1],points[2])/3.0
    uvs.extend([(0,0), (a,0), (a,b), (0,b)])
    indices.extend([base,base+1,base+2,base,base+2,base+3])

# Reused texture covers both faces of all exterior and interior walls. The
# matching solid greybox walls remain the sole movement/shot collision source.
def z_wall(x0, x1, z0, z1):
    quad([(x1,0,z0),(x0,0,z0),(x0,3,z0),(x1,3,z0)], (0,0,-1))
    quad([(x0,0,z1),(x1,0,z1),(x1,3,z1),(x0,3,z1)], (0,0,1))
def x_wall(x0, x1, z0, z1):
    quad([(x0,0,z0),(x0,0,z1),(x0,3,z1),(x0,3,z0)], (-1,0,0))
    quad([(x1,0,z1),(x1,0,z0),(x1,3,z0),(x1,3,z1)], (1,0,0))
for x in (-21,-7): x_wall(x-.205,x+.205,-8,12)
for x in (10,24): x_wall(x-.205,x+.205,6,22)
for z in (-8,12):
    for x0,x1 in ((-21,-16),(-12,-7)): z_wall(x0,x1,z-.205,z+.205)
for z in (6,22):
    for x0,x1 in ((10,15),(19,24)): z_wall(x0,x1,z-.205,z+.205)
# Textured traversable strips guide players from the spawn to the court and
# along each building interior. These quads are 15 mm above the solid floor.
for x0,x1,z0,z1 in ((-2,2,-24,25),(-15.5,-12.5,-8,12),(15.5,18.5,6,22),
                    (-14,-2,-16,-13),(2,17,-1,2)):
    quad([(x0,.015,z0),(x0,.015,z1),(x1,.015,z1),(x1,.015,z0)], (0,1,0))

def packf(rows): return b''.join(struct.pack('<' + 'f'*len(r), *r) for r in rows)
parts=[packf(positions),packf(normals),packf(uvs),struct.pack('<'+'H'*len(indices),*indices),image]
views=[]; data=b''
for index,part in enumerate(parts):
    data+=b'\0'*(-len(data)%4); views.append({'buffer':0,'byteOffset':len(data),'byteLength':len(part)})
    if index<3: views[-1]['target']=34962
    if index==3: views[-1]['target']=34963
    data+=part
mins=[min(p[i] for p in positions) for i in range(3)]; maxs=[max(p[i] for p in positions) for i in range(3)]
doc={'asset':{'version':'2.0','generator':'Nemisis Outer Facility cook'},'scene':0,'scenes':[{'nodes':[0]}],
     'nodes':[{'name':'map_facility_trim_01','mesh':0}],
     'meshes':[{'name':'map_facility_trim_01_lod0','primitives':[{'attributes':{'POSITION':0,'NORMAL':1,'TEXCOORD_0':2},'indices':3,'material':0}]}],
     'materials':[{'name':'reused_movement_accent','pbrMetallicRoughness':{'baseColorTexture':{'index':0},'metallicFactor':0.18,'roughnessFactor':0.46}}],
     'textures':[{'source':0,'sampler':0}],'images':[{'bufferView':4,'mimeType':'image/png'}],
     'samplers':[{'magFilter':9729,'minFilter':9729,'wrapS':10497,'wrapT':10497}],
     'buffers':[{'byteLength':len(data)}],'bufferViews':views,
     'accessors':[{'bufferView':0,'componentType':5126,'count':len(positions),'type':'VEC3','min':mins,'max':maxs},
                  {'bufferView':1,'componentType':5126,'count':len(normals),'type':'VEC3'},
                  {'bufferView':2,'componentType':5126,'count':len(uvs),'type':'VEC2'},
                  {'bufferView':3,'componentType':5123,'count':len(indices),'type':'SCALAR'}]}
js=json.dumps(doc,separators=(',',':')).encode(); js+=b' '*(-len(js)%4); data+=b'\0'*(-len(data)%4)
out.parent.mkdir(parents=True,exist_ok=True)
out.write_bytes(struct.pack('<III',0x46546c67,2,12+8+len(js)+8+len(data))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(data),0x004e4942)+data)
print(out, len(indices)//3, 'triangles',out.stat().st_size,'bytes')
