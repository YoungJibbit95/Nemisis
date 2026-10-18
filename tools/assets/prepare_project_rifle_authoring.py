"""Prepare a temporary real-rig Blender source; production GLBs stay unchanged."""

import json, struct, sys, copy, argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--candidate-dir", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
repo = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(repo / "tools/assets"))
from cook_project_fps_candidate import Writer, elements, values

root = args.candidate_dir


def read(path):
    data = path.read_bytes()
    size = struct.unpack_from("<I", data, 12)[0]
    return json.loads(data[20 : 20 + size]), data[28 + size :]


arms, ab = read(root / "arms_runtime.glb")
body, bb = read(root / "body_runtime.glb")
union = list(arms["skins"][0]["joints"])
union.extend(j for j in body["skins"][0]["joints"] if j not in union)
writer = Writer(arms)
primitives = []
ibms = {}
for source, binary in [(arms, ab), (body, bb)]:
    palette = source["skins"][0]["joints"]
    remap = {i: union.index(j) for i, j in enumerate(palette)}
    for j, row in zip(palette, elements(source, binary, source["skins"][0]["inverseBindMatrices"])):
        if j in ibms:
            assert ibms[j] == row
        ibms[j] = row
    for old in source["meshes"][0]["primitives"]:
        p = copy.deepcopy(old)
        p["attributes"] = {}
        p.pop("material", None)
        for attr, index in old["attributes"].items():
            rows = elements(source, binary, index)
            if attr == "JOINTS_0":
                rows = [struct.pack("<4H", *(remap[j] for j in v)) for v in values(source, binary, index)]
            p["attributes"][attr] = writer.accessor(
                source["accessors"][index], rows, bounds=attr == "POSITION"
            )
        p["indices"] = writer.accessor(
            source["accessors"][old["indices"]], elements(source, binary, old["indices"])
        )
        primitives.append(p)
writer.doc["meshes"][0]["primitives"] = primitives
writer.doc["skins"][0]["joints"] = union
index = arms["skins"][0]["inverseBindMatrices"]
writer.doc["skins"][0]["inverseBindMatrices"] = writer.accessor(
    arms["accessors"][index], [ibms[j] for j in union]
)
data = writer.glb()
size = struct.unpack_from("<I", data, 12)[0]
doc = json.loads(data[20 : 20 + size])
nodes = doc["nodes"]
parents = {c: i for i, n in enumerate(nodes) for c in n.get("children", [])}
keep = set()


def ancestors(i):
    while i not in keep:
        keep.add(i)
        if i not in parents:
            break
        i = parents[i]


for skin in doc["skins"]:
    for i in skin["joints"]:
        ancestors(i)
for i, n in enumerate(nodes):
    if "mesh" in n or n.get("name", "").startswith("socket_"):
        ancestors(i)
mapping = {old: new for new, old in enumerate(sorted(keep))}
doc["nodes"] = [nodes[i] for i in sorted(keep)]
for n in doc["nodes"]:
    if "children" in n:
        n["children"] = [mapping[c] for c in n["children"] if c in keep]
for s in doc["scenes"]:
    s["nodes"] = [mapping[i] for i in s["nodes"] if i in keep]
for s in doc["skins"]:
    s["joints"] = [mapping[i] for i in s["joints"]]
    if "skeleton" in s:
        s["skeleton"] = mapping[s["skeleton"]]
for m in doc["meshes"]:
    for p in m["primitives"]:
        p.pop("material", None)
for k in ["animations", "images", "textures", "materials", "samplers"]:
    doc.pop(k, None)
header = json.dumps(doc, separators=(",", ":")).encode()
header += b" " * (-len(header) % 4)
tail = data[20 + size :]
out = (
    struct.pack("<III", 0x46546C67, 2, 20 + len(header) + len(tail))
    + struct.pack("<II", len(header), 0x4E4F534A)
    + header
    + tail
)
target = args.output
target.parent.mkdir(parents=True, exist_ok=True)
target.write_bytes(out)
print(
    "Temporary authoring union", len(union), "skin joints;", len(keep), "nodes; production assets unchanged"
)
