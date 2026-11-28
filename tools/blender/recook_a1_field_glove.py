"""Export the editable A1 field-glove scene and its sparse equipment-contact actions.

Run with Blender 5.1.0 in background mode, passing the source .blend as the
loaded scene and the output .glb after --. The JSON file beside this script
records the artist's glove-joint corrections in the named local phalanx axes.
"""
import json
from pathlib import Path
import struct
import sys

import bpy
from mathutils import Matrix, Quaternion

def field_contacts():
    return json.loads(Path(__file__).with_name("a1_field_finger_contacts.json").read_text(encoding="utf-8"))["clips"]

def cook_contact_masks(path):
    """Write sparse static masks, including deliberately unchanged proximal joints.

    glTF rotation samples contain the node's absolute local bind orientation,
    followed by the authored local correction. The client removes the bind
    before applying the correction over the playing action.
    """
    data = path.read_bytes()
    json_size = struct.unpack_from("<I", data, 12)[0]
    doc = json.loads(data[20 : 20 + json_size])
    binary = bytearray(data[28 + json_size :])
    nodes = {n["name"]: (i, n) for i, n in enumerate(doc["nodes"])}
    names = field_contacts()
    doc["animations"] = [a for a in doc["animations"] if a.get("name") not in names]

    def accessor(values, kind, count):
        while len(binary) % 4:
            binary.append(0)
        offset = len(binary)
        binary.extend(struct.pack("<" + "f" * len(values), *values))
        view = len(doc["bufferViews"])
        doc["bufferViews"].append(
            {"buffer": 0, "byteOffset": offset, "byteLength": len(binary) - offset}
        )
        obj = {"bufferView": view, "componentType": 5126, "count": count, "type": kind}
        if kind == "SCALAR":
            obj.update(min=[min(values)], max=[max(values)])
        doc["accessors"].append(obj)
        return len(doc["accessors"]) - 1

    time = accessor((0.0, 1 / 24.0), "SCALAR", 2)
    for label, digits in names.items():
        samplers = []
        channels = []
        for name, matrix in digits.items():
            index, node = nodes[name]
            x, y, z, w = node.get("rotation", (0, 0, 0, 1))
            result = (
                Quaternion((w, x, y, z)) @ Matrix(matrix).to_quaternion()
            ).normalized()
            values = [result.x, result.y, result.z, result.w] * 2
            output = accessor(values, "VEC4", 2)
            channels.append(
                {
                    "sampler": len(samplers),
                    "target": {"node": index, "path": "rotation"},
                }
            )
            samplers.append(
                {"input": time, "output": output, "interpolation": "LINEAR"}
            )
        doc["animations"].append(
            {"name": label, "channels": channels, "samplers": samplers}
        )
    doc["buffers"][0]["byteLength"] = len(binary)
    header = json.dumps(doc, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
    header += b" " * (-len(header) % 4)
    binary += b"\0" * (-len(binary) % 4)
    path.write_bytes(
        struct.pack("<4sII", b"glTF", 2, 28 + len(header) + len(binary))
        + struct.pack("<I4s", len(header), b"JSON")
        + header
        + struct.pack("<I4s", len(binary), b"BIN\0")
        + binary
    )



if __name__ == "__main__":
    args = sys.argv[sys.argv.index("--") + 1:]
    if len(args) != 1:
        raise ValueError("Pass the destination GLB after --")
    output = Path(args[0]).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    controls = field_contacts()
    source = bpy.context.scene
    if len([o for o in source.objects if o.type == "ARMATURE"]) != 1:
        raise ValueError("Expected the authored field-glove armature")
    if any(name not in bpy.data.actions for name in controls):
        raise ValueError("The source is missing a field contact action")
    bpy.ops.export_scene.gltf(
        filepath=str(output), export_format="GLB", export_yup=True, export_skins=True,
        export_animations=True, export_animation_mode="ACTIONS",
        export_optimize_animation_keep_anim_armature=False)
    cook_contact_masks(output)
