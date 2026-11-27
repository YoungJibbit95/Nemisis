"""Packed glTF accessors and per-mesh, source-bound Vesper index recipes."""
import ctypes as c
import hashlib
import json
import math
import struct
import zlib

COMPONENTS = {5121: "B", 5123: "H", 5125: "I", 5126: "f"}
WIDTH = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}


def unpack_accessor(doc, binary, index):
    accessor = doc["accessors"][index]
    view = doc["bufferViews"][accessor["bufferView"]]
    if view.get("buffer", 0) != 0 or "sparse" in accessor:
        raise ValueError("Vesper accessors must use the primary binary buffer")
    components = WIDTH[accessor["type"]]
    decode = struct.Struct("<" + COMPONENTS[accessor["componentType"]] * components)
    start = view.get("byteOffset", 0) + accessor.get("byteOffset", 0)
    stride = view.get("byteStride", decode.size)
    return [decode.unpack_from(binary, start + i * stride) for i in range(accessor["count"])]


def open_glb(path):
    data = path.read_bytes()
    json_size = struct.unpack_from("<I", data, 12)[0]
    return json.loads(data[20:20 + json_size]), data[28 + json_size:]


LEVELS = ((.30, .005), (.12, .010))

def static_primitive(doc, mesh_index=None):
    """The recipe is scoped to one indexed, rigid triangle batch."""
    if (mesh_index is None and len(doc["meshes"]) != 1) or len(doc["meshes"][mesh_index or 0]["primitives"]) != 1:
        raise ValueError("Expected a single batched mesh with one material primitive")
    primitive = doc["meshes"][mesh_index or 0]["primitives"][0]
    if doc.get("skins") or doc.get("animations") or primitive.get("targets"):
        raise ValueError("Deforming assets require silhouette and pose validation")
    if primitive.get("mode", 4) != 4 or "indices" not in primitive:
        raise ValueError("Expected indexed triangles")
    return primitive


def encode(doc, binary):
    doc["buffers"][0]["byteLength"] = len(binary)
    header = json.dumps(doc, separators=(",", ":")).encode()
    header += b" " * (-len(header) % 4)
    binary += b"\0" * (-len(binary) % 4)
    return (struct.pack("<4sIIII", b"glTF", 2, 28+len(header)+len(binary), len(header), 0x4e4f534a)
            + header + struct.pack("<II", len(binary), 0x004e4942) + binary)


def generate(source, recipe, library, levels=LEVELS, mesh_index=None):
    import numpy as np
    doc, data = open_glb(source)
    primitive = static_primitive(doc,mesh_index)
    attrs = primitive["attributes"]
    positions, normals, uv = [np.array(unpack_accessor(doc, data, attrs[key]), dtype="<f4")
                              for key in ("POSITION", "NORMAL", "TEXCOORD_0")]
    if len(positions) > 65536:
        raise ValueError("Static prop recipes support up to 65,536 near vertices")
    indices = np.array(unpack_accessor(doc, data, primitive["indices"]),dtype="<u4").ravel()
    attributes = np.column_stack((normals, uv)).astype("<f4")
    weights = np.array([.06,.06,.06,.20,.20],dtype="<f4")
    lib = c.CDLL(str(library.resolve()))
    simplify = lib.meshopt_simplifyWithAttributes
    ptr, size = c.c_void_p, c.c_size_t
    simplify.restype = size
    simplify.argtypes = [ptr,ptr,size,ptr,size,size,ptr,size,ptr,size,ptr,size,
                         c.c_float,c.c_uint,c.POINTER(c.c_float)]
    streams = []
    if (len(levels) != 2 or not 0 < levels[1][0] < levels[0][0] < 1
            or any(not math.isfinite(error) or error < 0 for _, error in levels)):
        raise ValueError("Expected decreasing medium and far fractions with finite nonnegative errors")
    for fraction, error_limit in levels:
        result = np.empty_like(indices)
        error = c.c_float()
        # The atlas has splits at adjacent planes. Measure normal/UV costs
        # during permissive seam collapse and retain the original vertex buffer.
        count = simplify(result.ctypes.data,indices.ctypes.data,len(indices),positions.ctypes.data,
                         len(positions),12,attributes.ctypes.data,20,weights.ctypes.data,5,None,
                         3*int(len(indices)*fraction/3),error_limit,1 << 5,c.byref(error))
        print(f"{count//3} triangles, reported relative error {error.value:.6f}")
        streams.append(result[:count].astype("<u2").tobytes())
    payload = (hashlib.sha256(source.read_bytes()).digest()
               + struct.pack("<II",*(len(stream)//2 for stream in streams)) + b"".join(streams))
    recipe.write_bytes(zlib.compress(payload,9))


def attach(source, destination, recipe, mesh_index=None):
    payload = zlib.decompress(recipe.read_bytes())
    if len(payload)<40 or hashlib.sha256(source.read_bytes()).digest()!=payload[:32]:
        raise ValueError("Index recipe does not match this near mesh; regenerate and review both tiers")
    counts = struct.unpack_from("<II",payload,32)
    doc, data = open_glb(source)
    data = bytearray(data)
    primitive = static_primitive(doc,mesh_index)
    if "novacoreIndexLods" in primitive.get("extras", {}):
        raise ValueError("Expected a near-only input")
    previous = doc["accessors"][primitive["indices"]]["count"]
    vertices = doc["accessors"][primitive["attributes"]["POSITION"]]["count"]
    cursor = 40
    levels = []
    for count in counts:
        indices = payload[cursor:cursor+2*count]
        if (not 0<count<previous or count%3 or len(indices)!=2*count
                or any(index>=vertices for index, in struct.iter_unpack("<H",indices))):
            raise ValueError("Invalid static prop index recipe")
        data.extend(b"\0" * (-len(data)%4))
        view = len(doc["bufferViews"])
        doc["bufferViews"].append({"buffer":0,"byteOffset":len(data),"byteLength":len(indices),"target":34963})
        data.extend(indices)
        levels.append(len(doc["accessors"]))
        doc["accessors"].append({"bufferView":view,"componentType":5123,"count":count,"type":"SCALAR"})
        cursor += len(indices)
        previous = count
    if cursor!=len(payload):
        raise ValueError("Unexpected trailing indices")
    primitive.setdefault("extras",{})["novacoreIndexLods"] = levels
    destination.write_bytes(encode(doc,data))
    return [count//3 for count in counts]


