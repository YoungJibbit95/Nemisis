"""Recook the editable Vesper field prop, including its cell and finger controls, using Blender 5.1."""
import argparse
import json
import math
from pathlib import Path
import shutil
import struct
import sys
from tempfile import TemporaryDirectory

import bpy
from mathutils import Matrix, Quaternion

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "assets"))
from vesper_cell_glb import attach, encode, generate, open_glb

ASSET = "wpn_a1_vesper_field_01"
SOURCE = Path("assets/source/blender/weapons") / (ASSET + ".blend")
COOK = Path("assets/generated/studio") / (ASSET + ".glb")

CELL='VesperCell'
FRAME='VesperReceiver'
ACTION='vesper_field_cell_exchange'
CONTROL='Vesper cell | exchange control'
TO_BLENDER=Quaternion((1,0,0),math.pi/2)
CUPS=('VesperIndexCup','VesperLittleCup')


def join_batch(name, parts):
    bpy.ops.object.select_all(action='DESELECT')
    graph=bpy.context.evaluated_depsgraph_get()
    for obj in parts:
        matrix=obj.matrix_world.copy()
        mesh=bpy.data.meshes.new_from_object(obj.evaluated_get(graph),preserve_all_data_layers=True,depsgraph=graph)
        obj.modifiers.clear();obj.parent=None;obj.data=mesh
        mesh.transform(matrix);obj.matrix_world=Matrix.Identity(4)
        obj.select_set(True)
    bpy.context.view_layer.objects.active=parts[0]
    bpy.ops.object.join()
    batch=bpy.context.object
    batch.name=name;batch.data.name=name
    triangulate=batch.modifiers.new('Cooked triangle stream','TRIANGULATE')
    bpy.ops.object.modifier_apply(modifier=triangulate.name)
    for uv in batch.data.uv_layers.active.data:
        uv.uv=tuple(round(v*65536)/65536 for v in uv.uv)
    return batch


def append_samples(doc,data,rows,kind):
    data.extend(bytes(-len(data)%4))
    start=len(data)
    for row in rows:data.extend(struct.pack('<'+'f'*len(row),*row))
    view=len(doc['bufferViews'])
    doc['bufferViews'].append({'buffer':0,'byteOffset':start,'byteLength':len(data)-start})
    accessor={'bufferView':view,'componentType':5126,'count':len(rows),'type':kind}
    if kind=='SCALAR':accessor.update(min=[rows[0][0]],max=[rows[-1][0]])
    doc['accessors'].append(accessor)
    return len(doc['accessors'])-1


def cook(source,root,library,recipe_root):
    destination=root/SOURCE;out=root/COOK
    destination.parent.mkdir(exist_ok=True,parents=True);out.parent.mkdir(exist_ok=True,parents=True)
    bpy.ops.wm.open_mainfile(filepath=str(source))
    scene=bpy.context.scene
    scene.frame_set(0)
    if scene.get("field_fit_revision") != 2 or any(name not in bpy.data.objects for name in (*CUPS, CONTROL)):
        raise ValueError("Expected the editable field-cell source with its exchange and finger controls")
    if source != destination:
        shutil.copy2(source, destination)
    ctrl=bpy.data.objects[CONTROL]
    if not ctrl.animation_data or ctrl.animation_data.action.name!=ACTION:raise ValueError('Field cell action missing')
    ts=[];qs=[]; finger_samples={name:[] for name in CUPS}
    for t in range(101):
        scene.frame_set(t)
        pos=TO_BLENDER.inverted()@ctrl.location
        q=TO_BLENDER.inverted()@ctrl.rotation_quaternion@TO_BLENDER
        ts.append(tuple(pos));qs.append((q.x,q.y,q.z,q.w))
        for name in CUPS:
            q=bpy.data.objects[name].rotation_quaternion
            finger_samples[name].append((q.x,q.y,q.z,q.w))
    scene.frame_set(0)
    parts=sorted((o for o in scene.objects if o.type=='MESH'),key=lambda o:o.name)
    movable=[o for o in parts if o.parent==ctrl]
    fixed=[o for o in parts if o.parent!=ctrl]
    if len(movable)<5 or len(fixed)<50:raise ValueError('Invalid field part partition')
    batches=[join_batch(FRAME,fixed),join_batch(CELL,movable)]
    bpy.ops.object.select_all(action='DESELECT')
    for obj in batches:obj.select_set(True)
    with TemporaryDirectory(dir=out.parent) as tmp:
        near=Path(tmp)/'near.glb'
        bpy.ops.export_scene.gltf(filepath=str(near),export_format='GLB',use_selection=True,export_yup=True,
            export_texcoords=True,export_normals=True,export_tangents=True,export_animations=False)
        doc,binary=open_glb(near)
        # Blender derives packed image names from the existence of external files.
        # Bind stable atlas names to the material's actual slots on every machine.
        material = doc["materials"][0]
        pbr = material["pbrMetallicRoughness"]
        slots = [(material["normalTexture"], "nysa_vault_normal"),
                 (pbr["baseColorTexture"], "nysa_vault_color"),
                 (pbr["metallicRoughnessTexture"], "nysa_vault_orm")]
        for slot, atlas_name in slots:
            image = doc["textures"][slot["index"]]["source"]
            doc["images"][image]["name"] = atlas_name
        near.write_bytes(encode(doc, binary))
        nodes=doc['nodes'];names={n['name']:i for i,n in enumerate(nodes)}
        if set(names)!={CELL,FRAME}:raise ValueError(f'Unexpected cooked part nodes {names}')
        levels={}
        for name in (FRAME,CELL):
            mesh=nodes[names[name]]['mesh']
            recipe_out=destination.with_name(ASSET+'.'+name+'.indices.zlib')
            if library:generate(near,recipe_out,library,levels=((.5,.002),(.34,.003)),mesh_index=mesh)
            else:
                recipe=recipe_root/recipe_out.name
                if recipe.resolve()!=recipe_out.resolve():shutil.copy2(recipe,recipe_out)
            target=Path(tmp)/(name+'.glb')
            levels[name]=attach(near,target,recipe_out,mesh_index=mesh)
            near=target
        doc,binary=open_glb(near);data=bytearray(binary)
        anchors=json.loads(scene['semantic_nodes'])
        for i,n in enumerate(anchors):
            if 'children' in n:n['children']=[j+len(nodes) for j in n['children']]
        doc['nodes'].extend(anchors)
        doc['scenes'][doc.get('scene',0)]['nodes'].append(len(doc['nodes'])-1)
        time=append_samples(doc,data,[(t/100,) for t in range(101)],'SCALAR')
        action={'name':ACTION,'samplers':[],'channels':[]}
        for path,kind,rows in [('translation','VEC3',ts),('rotation','VEC4',qs)]:
            output=append_samples(doc,data,rows,kind)
            action['channels'].append({'sampler':len(action['samplers']),'target':{'node':names[CELL],'path':path}})
            action['samplers'].append({'input':time,'output':output,'interpolation':'LINEAR'})
        cup={'name':'vesper_field_finger_cup','channels':[],'samplers':[]}
        for name,rows in finger_samples.items():
            index=len(doc['nodes']);doc['nodes'].append({'name':name})
            doc['scenes'][doc.get('scene',0)]['nodes'].append(index)
            output=append_samples(doc,data,rows,'VEC4')
            cup['channels'].append({'sampler':len(cup['samplers']),'target':{'node':index,'path':'rotation'}})
            cup['samplers'].append({'input':time,'output':output,'interpolation':'LINEAR'})
        doc['animations']=[action,cup]
        out.write_bytes(encode(doc,data))
    meta={'id':ASSET,'category':'weapon','source':SOURCE.as_posix(),'export':COOK.as_posix(),
          'scale_meters':True,'runtime_up_axis':'Y','gameplay_forward_axis':'+Z','collision':'none',
          'sockets':[n['name'] for n in anchors if n['name'].startswith('socket_')],'lods':[FRAME,CELL],
          'license':'original_project_asset','generated_by':'tools/blender/cook_vesper_cell.py','bytes':out.stat().st_size,
          'editable_parts':len(parts),'rigid_cell_parts':len(movable),'index_lods':levels,
          'animations':[ACTION,'vesper_field_finger_cup']}
    out.with_suffix('.metadata.json').write_text(json.dumps(meta,indent=2)+'\n')
    print('Cooked independent field batches',levels)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-blend',type=Path,required=True)
    parser.add_argument('--output-root',type=Path,required=True)
    parser.add_argument('--generate-with',type=Path)
    opts=parser.parse_args(sys.argv[sys.argv.index('--')+1:])
    source=opts.source_blend.resolve()
    cook(source,opts.output_root.resolve(),opts.generate_with,source.parent)
