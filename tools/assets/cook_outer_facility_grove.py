"""Bake six low-poly trees into two opaque primitives per reusable grove."""
from pathlib import Path
import json, math, struct

root=Path(__file__).resolve().parents[2]
out=root/'assets/generated/outer_facility/map_facility_grove_01.glb'
offsets=[(-2.5,-2),(0,-2.5),(2.5,-2),(-2.5,2),(0,2.5),(2.5,2)]
parts=[([],[]),([],[])]
def triangle(group,a,b,c):
    u=[b[i]-a[i] for i in range(3)]; v=[c[i]-a[i] for i in range(3)]
    n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]]
    length=math.sqrt(sum(x*x for x in n)); n=tuple(x/length for x in n)
    parts[group][0].extend((a,b,c)); parts[group][1].extend((n,n,n))
def ring(cx,cz,r,y,count=6):
    return [(cx+r*math.cos(2*math.pi*i/count),y,cz+r*math.sin(2*math.pi*i/count)) for i in range(count)]
for i,(x,z) in enumerate(offsets):
    bottom=ring(x,z,.18,0); top=ring(x,z,.18,2.2)
    for j in range(6):
        k=(j+1)%6
        triangle(0,bottom[j],top[j],top[k]); triangle(0,bottom[j],top[k],bottom[k])
    # Two stacked hexagonal crowns, one joined six-tree primitive.
    for y,r,h in ((1.8,1.5,1.9),(2.8,1.15,1.8)):
        skirt=ring(x,z,r,y); tip=(x,y+h+(i%3)*.08,z); center=(x,y,z)
        for j in range(6):
            k=(j+1)%6
            triangle(1,skirt[j],tip,skirt[k]); triangle(1,skirt[k],center,skirt[j])
data=b''; views=[]; accessors=[]; primitives=[]
for group,(positions,normals) in enumerate(parts):
    attrs={}
    for name,rows in [('POSITION',positions),('NORMAL',normals)]:
        data+=b'\0'*(-len(data)%4); payload=b''.join(struct.pack('<3f',*p) for p in rows)
        views.append({'buffer':0,'byteOffset':len(data),'byteLength':len(payload),'target':34962}); data+=payload
        accessor={'bufferView':len(views)-1,'componentType':5126,'count':len(rows),'type':'VEC3'}
        if name=='POSITION':
            accessor['min']=[min(p[i] for p in rows) for i in range(3)]
            accessor['max']=[max(p[i] for p in rows) for i in range(3)]
        accessors.append(accessor); attrs[name]=len(accessors)-1
    primitives.append({'attributes':attrs,'material':group})
doc={'asset':{'version':'2.0','generator':'Nemisis Outer Facility grove cook'},'scene':0,'scenes':[{'nodes':[0]}],
     'nodes':[{'name':'map_facility_grove_01','mesh':0}], 'meshes':[{'name':'map_facility_grove_01_lod0','primitives':primitives}],
     'materials':[{'name':'bark','pbrMetallicRoughness':{'baseColorFactor':[.25,.15,.08,1],'metallicFactor':0,'roughnessFactor':1}},
                  {'name':'foliage','pbrMetallicRoughness':{'baseColorFactor':[.12,.36,.16,1],'metallicFactor':0,'roughnessFactor':1}}],
     'buffers':[{'byteLength':len(data)}],'bufferViews':views,'accessors':accessors}
js=json.dumps(doc,separators=(',',':')).encode(); js+=b' '*(-len(js)%4); data+=b'\0'*(-len(data)%4)
out.parent.mkdir(parents=True,exist_ok=True)
out.write_bytes(struct.pack('<III',0x46546c67,2,12+8+len(js)+8+len(data))+struct.pack('<II',len(js),0x4e4f534a)+js+struct.pack('<II',len(data),0x004e4942)+data)
print(out, [len(p)//3 for p,n in parts], 'triangles', out.stat().st_size, 'bytes')
