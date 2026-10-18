"""Offline, topology-locked Project FPS candidate; no runtime asset replacement.

Python standard library only. The reviewed recipe contains explicit triangle
ordinals, never a positional/weight classifier executed by the game. Original
node indices/TRS, selected accessor elements, inverse binds and animation keys
are retained without a Blender import/export round trip.
"""

import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct

from asset_readiness_check import gltf_node_local_matrix, mat4_multiply, mat4_transform_origin


ROOT = Path(__file__).resolve().parents[2]
RECIPE = ROOT / "assets/metadata/project_fps_candidate_regions.json"
COMPONENT = {5121: ("B", 1), 5123: ("H", 2), 5125: ("I", 4), 5126: ("f", 4)}
WIDTH = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def load_source(recipe):
    for item in (recipe["source"], recipe["original_source"]):
        require(digest((ROOT / item["path"]).read_bytes()) == item["sha256"],
                f"Source hash changed: {item['path']}; review topology/regions before recooking")
    data = (ROOT / recipe["source"]["path"]).read_bytes()
    require(struct.unpack_from("<III", data) == (0x46546C67, 2, len(data)), "Expected glTF 2 GLB")
    size, kind = struct.unpack_from("<II", data, 12)
    require(kind == 0x4E4F534A, "Missing JSON chunk")
    doc = json.loads(data[20:20 + size])
    length, kind = struct.unpack_from("<II", data, 20 + size)
    require(kind == 0x004E4942, "Missing BIN chunk")
    binary = data[28 + size:28 + size + length]
    require(len(doc["meshes"]) == len(doc["skins"]) == len(doc["buffers"]) == 1,
            "Only the audited one-mesh/one-skin embedded GLB is supported")
    require("uri" not in doc["buffers"][0] and not doc.get("extensionsRequired"),
            "External buffers/required extensions need a new audit")
    require(len(doc["nodes"]) == recipe["node_count"], "Node topology changed")
    require(len(doc["skins"][0]["joints"]) == recipe["skin_joint_count"], "Skin topology changed")
    return doc, binary


def elements(doc, binary, index):
    a = doc["accessors"][index]
    require("sparse" not in a and "extensions" not in a, "Unsupported accessor encoding")
    _, component_size = COMPONENT[a["componentType"]]
    size = component_size * WIDTH[a["type"]]
    view = doc["bufferViews"][a["bufferView"]]
    require(view["buffer"] == 0, "Expected embedded buffer")
    start = view.get("byteOffset", 0) + a.get("byteOffset", 0)
    stride = view.get("byteStride", size)
    require(stride >= size and start + max(0, a["count"] - 1) * stride + size <= len(binary),
            "Accessor out of bounds")
    return [binary[start + i * stride:start + i * stride + size] for i in range(a["count"])]


def values(doc, binary, index):
    a = doc["accessors"][index]
    fmt = "<" + COMPONENT[a["componentType"]][0] * WIDTH[a["type"]]
    return [struct.unpack(fmt, x) for x in elements(doc, binary, index)]


def ordinals(ranges, maximum):
    result = []
    for first, last in ranges:  # inclusive, sorted, non-overlapping
        require(0 <= first <= last < maximum and (not result or first > result[-1]),
                "Invalid or overlapping explicit triangle ranges")
        result.extend(range(first, last + 1))
    return result


class Writer:
    def __init__(self, source):
        self.doc = copy.deepcopy(source)
        self.doc["accessors"] = []
        self.doc["bufferViews"] = []
        self.binary = bytearray()

    def accessor(self, original, rows, bounds=False):
        while len(self.binary) % 4:
            self.binary.append(0)
        raw = b"".join(rows)
        view = {"buffer": 0, "byteOffset": len(self.binary), "byteLength": len(raw)}
        self.doc["bufferViews"].append(view)
        self.binary.extend(raw)
        a = {k: v for k, v in original.items() if k not in ("bufferView", "byteOffset", "min", "max")}
        a.update(bufferView=len(self.doc["bufferViews"]) - 1, count=len(rows))
        if bounds:
            fmt = "<" + COMPONENT[a["componentType"]][0] * WIDTH[a["type"]]
            columns = list(zip(*(struct.unpack(fmt, row) for row in rows)))
            a["min"] = [min(c) for c in columns]
            a["max"] = [max(c) for c in columns]
        self.doc["accessors"].append(a)
        return len(self.doc["accessors"]) - 1

    def glb(self):
        self.doc["buffers"] = [{"byteLength": len(self.binary)}]
        header = json.dumps(self.doc, ensure_ascii=True, separators=(",", ":")).encode()
        header += b" " * (-len(header) % 4)
        binary = bytes(self.binary) + b"\0" * (-len(self.binary) % 4)
        return (struct.pack("<III", 0x46546C67, 2, 28 + len(header) + len(binary)) +
                struct.pack("<II", len(header), 0x4E4F534A) + header +
                struct.pack("<II", len(binary), 0x004E4942) + binary)


def cook_variant(source, binary, recipe, name):
    writer = Writer(source)
    skin = source["skins"][0]
    palette = recipe["variants"][name]["skin_slots"]
    require(palette == sorted(set(palette)), "Palette must be unique and source ordered")
    remap = {slot: i for i, slot in enumerate(palette)}
    parents = {child: i for i, node in enumerate(source["nodes"]) for child in node.get("children", [])}
    needed = set()

    def keep_ancestors(node):
        while node not in needed:
            needed.add(node)
            if node not in parents:
                break
            node = parents[node]

    for i, node in enumerate(source["nodes"]):
        if node.get("name", "").startswith("socket_"):
            keep_ancestors(i)
    primitives, mapping, stats = [], bytearray(), []
    for p, region in zip(source["meshes"][0]["primitives"], recipe["primitives"], strict=True):
        require(p.get("mode", 4) == 4 and not p.get("targets") and not p.get("extensions"),
                "Only audited non-morph triangle primitives supported")
        indices = [v[0] for v in values(source, binary, p["indices"])]
        require(len(indices) == region["triangle_count"] * 3 and
                source["accessors"][p["attributes"]["POSITION"]]["count"] == region["vertex_count"] and
                source["materials"][p["material"]]["name"] == region["material"], "Primitive topology changed")
        triangles = ordinals(region[name], region["triangle_count"])
        if not triangles:
            continue
        selected_indices = [indices[3 * t + corner] for t in triangles for corner in range(3)]
        selected_vertices = sorted(set(selected_indices))
        vertex_remap = {v: i for i, v in enumerate(selected_vertices)}
        primitive = copy.deepcopy(p)
        primitive["attributes"] = {}
        joints = values(source, binary, p["attributes"]["JOINTS_0"])
        weights = values(source, binary, p["attributes"]["WEIGHTS_0"])
        require(source["accessors"][p["attributes"]["WEIGHTS_0"]]["componentType"] == 5126,
                "Weight format changed")
        for v in selected_vertices:
            for slot, weight in zip(joints[v], weights[v], strict=True):
                if weight > 0:
                    keep_ancestors(skin["joints"][slot])
        for semantic, accessor in p["attributes"].items():
            rows = elements(source, binary, accessor)
            require(len(rows) == region["vertex_count"], "Attribute count mismatch")
            selected = [rows[v] for v in selected_vertices]
            if semantic == "JOINTS_0":
                a = source["accessors"][accessor]
                fmt = "<" + COMPONENT[a["componentType"]][0] * 4
                selected = [struct.pack(fmt, *(remap[slot] if weight > 0 else 0
                            for slot, weight in zip(joints[v], weights[v], strict=True))) for v in selected_vertices]
            primitive["attributes"][semantic] = writer.accessor(source["accessors"][accessor], selected,
                                                                 bounds=semantic == "POSITION")
        primitive["indices"] = writer.accessor({"componentType": 5125, "type": "SCALAR"},
                      [struct.pack("<I", vertex_remap[v]) for v in selected_indices])
        primitives.append(primitive)
        # Simple little-endian oracle map read by the independent engine contract test.
        mapping.extend(struct.pack("<III", region["primitive"], len(selected_vertices), len(triangles)))
        mapping.extend(struct.pack("<" + "I" * len(selected_vertices), *selected_vertices))
        mapping.extend(struct.pack("<" + "I" * len(triangles), *triangles))
        stats.append({"source_primitive": region["primitive"], "material": region["material"],
                      "vertices": len(selected_vertices), "triangles": len(triangles)})
    actual_palette = [i for i, node in enumerate(skin["joints"]) if node in needed]
    require(actual_palette == palette, f"{name}: retained skin/ancestor/socket closure changed")
    writer.doc["meshes"][0]["primitives"] = primitives
    writer.doc["skins"][0]["joints"] = [skin["joints"][i] for i in palette]
    ibm = skin["inverseBindMatrices"]
    rows = elements(source, binary, ibm)
    writer.doc["skins"][0]["inverseBindMatrices"] = writer.accessor(source["accessors"][ibm], [rows[i] for i in palette])
    for a, original in zip(writer.doc["animations"], source["animations"], strict=True):
        a["channels"], a["samplers"] = [], []
        samplers = {}
        for channel in original["channels"]:
            if channel["target"]["node"] not in needed:
                continue
            old_sampler = channel["sampler"]
            if old_sampler not in samplers:
                sampler = copy.deepcopy(original["samplers"][old_sampler])
                for field in ("input", "output"):
                    index = sampler[field]
                    sampler[field] = writer.accessor(source["accessors"][index], elements(source, binary, index),
                                                     bounds=field == "input")
                samplers[old_sampler] = len(a["samplers"])
                a["samplers"].append(sampler)
            c = copy.deepcopy(channel)
            c["sampler"] = samplers[old_sampler]
            a["channels"].append(c)
    # Shared external images are byte copies, generated once for both variants.
    textures = {}
    for i, image in enumerate(writer.doc.get("images", [])):
        view = source["bufferViews"][image.pop("bufferView")]
        ext = {"image/png": "png", "image/jpeg": "jpg"}[image["mimeType"]]
        path = f"textures/{i:02d}.{ext}"
        image["uri"] = path
        start = view.get("byteOffset", 0)
        textures[path] = binary[start:start + view["byteLength"]]
    require(writer.doc["nodes"] == source["nodes"], "Authored node hierarchy/TRS must remain identical")
    result = writer.glb()
    report = {"file": f"{name}.glb", "sha256": digest(result), "bytes": len(result),
              "skin_joints": len(palette), "pose_nodes_including_helpers": len(needed) - 7,
              "vertices": sum(s["vertices"] for s in stats),
              "triangles": sum(s["triangles"] for s in stats), "primitives": stats,
              "animation_channels": [len(a["channels"]) for a in writer.doc["animations"]]}
    return {f"{name}.glb": result, f"{name}.map": struct.pack("<I", len(stats)) + mapping, **textures}, report


def runtime_player_frame(source, glb):
    """Canonicalize the existing subset once offline; retain its skin/keys/BIN.

    The legacy normalized file's toes face -Z. A common scene parent turns its
    real rig forward to +Z and puts the rig's horizontal root at the player pivot.
    The eye reference uses ORG-eye bones (not displaced eye control handles).
    """
    nodes = source["nodes"]
    names = {n.get("name"): i for i, n in enumerate(nodes)}
    parents = {c: i for i, n in enumerate(nodes) for c in n.get("children", [])}

    def world(i):
        local = gltf_node_local_matrix(nodes[i])
        return mat4_multiply(world(parents[i]), local) if i in parents else local

    root = mat4_transform_origin(world(names["rig"]))
    for side in ("L", "R"):
        foot = mat4_transform_origin(world(names[f"DEF-foot.{side}"]))
        toe = mat4_transform_origin(world(names[f"DEF-toe.{side}"]))
        require(toe[2] < foot[2] - 0.05, "Character forward changed; review canonical player frame")
    eyes = [mat4_transform_origin(world(names[f"ORG-eye.{s}"])) for s in ("L", "R")]
    eye = [(eyes[0][i] + eyes[1][i]) * 0.5 for i in range(3)]
    parent = parents[names["socket_camera"]]
    p = world(parent)
    # Invert the actual affine parent, preserving its scale rather than assuming
    # an orthonormal rotation. This is an offline attachment authoring operation.
    a, b, c = p[0][:3]; d, e, f = p[1][:3]; g, h, i = p[2][:3]
    det = a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g)
    require(abs(det) > 1e-9, "Singular eye attachment parent")
    inverse = [[e*i-f*h, c*h-b*i, b*f-c*e],
               [f*g-d*i, a*i-c*g, c*d-a*f],
               [d*h-e*g, b*g-a*h, a*e-b*d]]
    local_eye = [sum(row[k]*(eye[k]-p[k][3]) for k in range(3))/det for row in inverse]
    size = struct.unpack_from("<I", glb, 12)[0]
    doc = json.loads(glb[20:20+size])
    old_nodes = len(doc["nodes"])
    roots = doc["scenes"][doc.get("scene", 0)]["nodes"]
    doc["nodes"].append({"name": "project_fps_canonical_player_frame", "children": roots,
                         "rotation": [0, 1, 0, 0], "translation": [root[0], 0, root[2]]})
    doc["scenes"][doc.get("scene", 0)]["nodes"] = [old_nodes]
    doc["nodes"][parent].setdefault("children", []).append(len(doc["nodes"]))
    doc["nodes"].append({"name": "socket_fps_eye", "translation": local_eye})
    header = json.dumps(doc, ensure_ascii=True, separators=(",", ":")).encode()
    header += b" " * (-len(header) % 4)
    tail = glb[20+size:]
    result = struct.pack("<III", 0x46546C67, 2, 20+len(header)+len(tail)) + struct.pack("<II", len(header), 0x4E4F534A) + header + tail
    return result, {"root_translation": [root[0], 0, root[2]], "rotation_xyzw": [0, 1, 0, 0],
                    "eye_source_world": eye, "eye_parent": nodes[parent]["name"], "eye_parent_local": local_eye,
                    "source_forward": "-Z proven by both foot-to-toe vectors", "runtime_forward": "+Z",
                    "sha256": digest(result), "binary_chunk_unchanged": True}


def cook(recipe):
    source, binary = load_source(recipe)
    outputs, reports = {}, {}
    for name in ("arms", "body"):
        files, reports[name] = cook_variant(source, binary, recipe, name)
        outputs.update(files)
    assets = []
    player_frames = {}
    sockets = [node["name"] for node in source["nodes"] if node.get("name", "").startswith("socket_")]
    for name in ("arms", "body"):
        asset_id = f"chr_project_male1_fp_{name}_preview"
        runtime, player_frames[name] = runtime_player_frame(source, outputs[f"{name}.glb"])
        outputs[f"{name}_runtime.glb"] = runtime
        cooked = f"project-fps-candidate/{name}_runtime.glb"
        assets.append({"id": asset_id, "kind": "mesh", "source": recipe["source"]["path"],
                       "cooked": cooked, "tags": ["project_fps_preview"], "streamable": False})
        metadata = {"id": asset_id, "source": recipe["source"]["path"], "export": cooked,
                    "category": "character", "scale_meters": True, "runtime_up_axis": "Y",
                    "gameplay_forward_axis": "+Z", "sockets": sockets + ["socket_fps_eye"], "collision": "none",
                    "license": "original_project_asset", "generated_by": "tools/assets/cook_project_fps_candidate.py",
                    "status": "developer preview; seams and palm frames unapproved"}
        outputs[f"{name}_runtime.metadata.json"] = (json.dumps(metadata, indent=2) + "\n").encode()
    outputs["preview_assets.json"] = (json.dumps({"manifest": {"name": "project_fps_preview"},
                                                 "assets": assets}, indent=2) + "\n").encode()
    report = {"source": recipe["source"], "recipe_sha256": digest(RECIPE.read_bytes()),
              "status": "technical candidate; not visually accepted or registered for gameplay",
              "variants": reports, "runtime_player_frames": player_frames, "artist_review": recipe["artist_review"]}
    outputs["report.json"] = (json.dumps(report, indent=2) + "\n").encode()
    return outputs, report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "build/project-fps-candidate")
    parser.add_argument("--check", action="store_true", help="Compare every generated byte without writing")
    args = parser.parse_args()
    recipe = json.loads(RECIPE.read_text(encoding="utf-8"))
    outputs, report = cook(recipe)  # validate everything before the first write
    for relative, data in outputs.items():
        path = args.output / relative
        require(path.resolve() not in ((ROOT / recipe[k]["path"]).resolve() for k in ("source", "original_source")),
                "Refusing to overwrite a source asset")
        if args.check:
            require(path.is_file() and path.read_bytes() == data, f"Missing/stale candidate: {path}")
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            if not path.is_file() or path.read_bytes() != data:
                path.write_bytes(data)
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
