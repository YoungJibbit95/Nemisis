"""Check the two rigid game meshes, their source-bound tiers, and the vacated cell socket."""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np

from vesper_cell_glb import open_glb, unpack_accessor


def upward_hits(triangles, x, z):
    # Barycentric intersection with a vertical line through the receiver recess.
    a,b,c=(triangles[:,i] for i in range(3))
    d=b-a;e=c-a
    det=d[:,0]*e[:,2]-d[:,2]*e[:,0]
    inv=np.divide(1.,det,out=np.zeros_like(det),where=abs(det)>1e-14)
    dx=x-a[:,0];dz=z-a[:,2]
    u=(dx*e[:,2]-dz*e[:,0])*inv
    v=(d[:,0]*dz-d[:,2]*dx)*inv
    ids=(abs(det)>1e-14)&(u>=0)&(v>=0)&(u+v<=1.)
    return np.sort((a[:,1]+u*d[:,1]+v*e[:,1])[ids])


def check(path):
    doc,data=open_glb(path)
    nodes={n['name']:i for i,n in enumerate(doc['nodes'])}
    out={'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'parts':{}}
    positions={};triangles={}
    for name in ('VesperReceiver','VesperCell'):
        mesh=doc['meshes'][doc['nodes'][nodes[name]]['mesh']]
        assert len(mesh['primitives'])==1
        prim=mesh['primitives'][0]
        pos=np.array(unpack_accessor(doc,data,prim['attributes']['POSITION']))
        tiers=[np.array(unpack_accessor(doc,data,i)).reshape(-1,3) for i in [prim['indices'],*prim['extras']['novacoreIndexLods']]]
        counts=list(map(len,tiers));assert counts[0]>counts[1]>counts[2]>0
        assert len(tiers)==3 and all(t.min()>=0 and t.max()<len(pos) for t in tiers)
        positions[name]=pos;triangles[name]=tiers[0]
        out['parts'][name]={'vertices':len(pos),'triangles':counts}
    clip=next(a for a in doc['animations'] if a['name']=='vesper_field_cell_exchange')
    channels={}
    for ch in clip['channels']:
        assert ch['target']['node']==nodes['VesperCell']
        sampler=clip['samplers'][ch['sampler']]
        curve=np.array(unpack_accessor(doc,data,sampler['output']))
        times=np.array(unpack_accessor(doc,data,sampler['input'])).flatten()
        assert np.all(np.diff(times)>0) and times[0]==0 and times[-1]==1 and len(times)==101
        channels[ch['target']['path']]=curve
    move=channels['translation'];rot=channels['rotation']
    assert np.linalg.norm(move[[0,-1]],axis=1).max()<1e-6 and np.max(abs(rot[[0,-1]]-[0,0,0,1]))<1e-6
    cup=next(a for a in doc["animations"] if a["name"]=="vesper_field_finger_cup")
    assert len(cup["channels"])==2
    seen=set()
    for channel in cup["channels"]:
        target=channel["target"]
        seen.add(doc["nodes"][target["node"]]["name"])
        assert target["path"]=="rotation"
        sampler=cup["samplers"][channel["sampler"]]
        sample=np.array(unpack_accessor(doc,data,sampler["output"]))
        times=np.array(unpack_accessor(doc,data,sampler["input"])).flatten()
        assert len(times)==len(sample)==101 and np.all(np.diff(times)>0)
        assert np.max(abs(sample[[0,-1]]-[0,0,0,1]))<1e-6 and abs(sample[:,0]).max()>.05
    assert seen=={"VesperIndexCup","VesperLittleCup"}
    frame=60
    x,y,z,w=rot[frame]
    r=np.array([[1-2*(y*y+z*z),2*(x*y-z*w),2*(x*z+y*w)],
                [2*(x*y+z*w),1-2*(x*x+z*z),2*(y*z-x*w)],
                [2*(x*z-y*w),2*(y*z+x*w),1-2*(x*x+y*y)]])
    cell=positions['VesperCell']
    seated=cell[triangles['VesperCell']]
    detached=(cell@r.T+move[frame])[triangles['VesperCell']]
    body=positions['VesperReceiver'][triangles['VesperReceiver']]
    depths=[]
    for x,z in ((0,.163),(-.009,.153),(.009,.175)):
        closed=upward_hits(seated,x,z)
        clear=upward_hits(detached,x,z)
        back=upward_hits(body,x,z)
        assert len(closed)>0 and len(clear)==0 and len(back)>0,(x,z,closed,clear,back)
        depths.append(float(back[0]-closed[0]))
    out.update(action=clip['name'],pose_samples=len(move),socket_probe_rays=len(depths),
               clearance_to_socket_back_mm=[round(v*1000,2) for v in depths],
               peak_origin_travel_mm=round(np.linalg.norm(move,axis=1).max()*1000,2))
    return out


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('asset',type=Path)
    print(json.dumps(check(parser.parse_args().asset),indent=2))
