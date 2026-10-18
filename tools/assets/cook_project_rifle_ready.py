"""Append a Blender-authored developer Ready variant without recooking old data.

Original geometry, weights, skin palettes, inverse binds and clip accessors stay
byte-identical. Only new rotation keys and explicitly named contact markers are
appended. Blender is needed for authoring, not for reproducible CI/runtime cook.
"""

import argparse
import hashlib
import json
import math
from pathlib import Path
import struct

from asset_readiness_check import gltf_node_local_matrix, mat4_multiply, mat4_transform_origin

ROOT = Path(__file__).resolve().parents[2]
POSE = ROOT / "assets/metadata/project_rifle_ready_pose.json"
WEAPON = "wpn_project_rifle_m4a1_ready_preview"


def require(value, message):
    if not value:
        raise ValueError(message)


def read(path):
    data = path.read_bytes()
    size = struct.unpack_from("<I", data, 12)[0]
    return json.loads(data[20 : 20 + size]), bytearray(data[28 + size :])


def encode(doc, binary):
    doc["buffers"][0]["byteLength"] = len(binary)
    header = json.dumps(doc, separators=(",", ":")).encode()
    header += b" " * (-len(header) % 4)
    binary += b"\0" * (-len(binary) % 4)
    return (
        struct.pack("<III", 0x46546C67, 2, 28 + len(header) + len(binary))
        + struct.pack("<II", len(header), 0x4E4F534A)
        + header
        + struct.pack("<II", len(binary), 0x004E4942)
        + binary
    )


def matrix_rows(rows):
    require(len(rows) == 4 and all(len(r) == 4 for r in rows), "Expected affine 4x4 matrix")
    require(all(math.isfinite(x) for r in rows for x in r), "Nonfinite authoring matrix")
    require(rows[3] == [0, 0, 0, 1], "Expected affine last row")
    return [rows[r][c] for c in range(4) for r in range(4)]


def marker(doc, name, rows, parent=None):
    require(not any(n.get("name") == name for n in doc["nodes"]), "Duplicate authored socket")
    index = len(doc["nodes"])
    doc["nodes"].append({"name": name, "matrix": matrix_rows(rows)})
    if parent is None:
        doc["scenes"][doc.get("scene", 0)]["nodes"].append(index)
    else:
        doc["nodes"][parent].setdefault("children", []).append(index)


def validate_physical_contacts(pose, weapon_doc):
    """Reject exchanged anatomical/physical ownership before writing a GLB.

    The original M4 anchors are measured against its pistol handle/handguard;
    the import tests independently retain those actual geometry contracts.
    A name or camera-right X coordinate alone is not the ownership oracle.
    """
    nodes = weapon_doc["nodes"]
    parents = {child: i for i, node in enumerate(nodes) for child in node.get("children", [])}

    def world(i):
        local = gltf_node_local_matrix(nodes[i])
        return mat4_multiply(world(parents[i]), local) if i in parents else local

    anchors = {}
    for side, tolerance in (("R", .02), ("L", .01)):
        require(pose["palms"][side]["source_bone"] == "DEF-hand." + side,
                "Physical palm source must retain its actual anatomical deform hand")
        for rows in (pose["palms"][side]["matrix"], pose["weapon_contact_frames"][side]["matrix"]):
            matrix_rows(rows)
            a, b, c = ([rows[r][column] for r in range(3)] for column in range(3))
            determinant = (a[0] * (b[1] * c[2] - b[2] * c[1]) -
                           b[0] * (a[1] * c[2] - a[2] * c[1]) +
                           c[0] * (a[1] * b[2] - a[2] * b[1]))
            require(abs(determinant - 1) < 1e-5 and
                    all(abs(sum(x * y for x, y in zip(u, v)) - float(i == j)) < 1e-5
                        for i, u in enumerate((a, b, c)) for j, v in enumerate((a, b, c))),
                    "Physical palm/contact frame must be a proper rotation, never reflection/negative scale")
        matches = [i for i, node in enumerate(nodes) if node.get("name") == "socket_grip_" + side.lower()]
        require(len(matches) == 1, "Expected one actual measured weapon grip")
        anchor = mat4_transform_origin(world(matches[0]))
        contact = [row[3] for row in pose["weapon_contact_frames"][side]["matrix"][:3]]
        require(sum((x - y) ** 2 for x, y in zip(contact, anchor)) < tolerance * tolerance,
                "Anatomical " + side + " contact is not on its physical pistol/support grip")
        anchors[side] = contact
    require(anchors["L"][2] > anchors["R"][2] + .15,
            "Left support must remain forward of right firing contact along physical weapon +Z")


def append_ready(doc, binary, pose):
    names = {n.get("name"): i for i, n in enumerate(doc["nodes"])}

    def accessor(kind, rows):
        while len(binary) % 4:
            binary.append(0)
        offset = len(binary)
        for row in rows:
            binary.extend(struct.pack("<" + "f" * len(row), *row))
        view = len(doc["bufferViews"])
        doc["bufferViews"].append({"buffer": 0, "byteOffset": offset, "byteLength": len(binary) - offset})
        index = len(doc["accessors"])
        item = {"bufferView": view, "componentType": 5126, "count": len(rows), "type": kind}
        if kind == "SCALAR":
            item.update(min=[min(r[0] for r in rows)], max=[max(r[0] for r in rows)])
        doc["accessors"].append(item)
        return index

    clip = {"name": "Developer Rifle Ready Idle", "channels": [], "samplers": []}
    time = accessor("SCALAR", [(0,), (1,)])
    for name, authored in sorted(pose["joint_pose"].items()):
        require(name in names, f"Authored joint absent: {name}")
        q = authored["rotation_xyzw"]
        require(
            len(q) == 4 and all(math.isfinite(x) for x in q) and abs(sum(x * x for x in q) - 1) < 1e-5,
            f"Invalid rotation: {name}",
        )
        output = accessor("VEC4", [q, q])
        clip["channels"].append(
            {"sampler": len(clip["samplers"]), "target": {"node": names[name], "path": "rotation"}}
        )
        clip["samplers"].append({"input": time, "output": output, "interpolation": "LINEAR"})
    doc.setdefault("animations", []).append(clip)
    for side in ("R", "L"):
        marker(doc, "socket_palm_" + side.lower(), pose["palms"][side]["matrix"], names["DEF-hand." + side])
    require(pose["weapon_mount_parent"] in names, "Authored chest mount parent is missing")
    marker(doc, "socket_rifle_ready", pose["weapon_mount_local_matrix"], names[pose["weapon_mount_parent"]])


def cook(candidate):
    pose = json.loads(POSE.read_text(encoding="utf-8"))
    require(
        all(pose["weapon_model_transform"][r][c] == float(r == c) for r in range(3) for c in range(3)),
        "Ready mount currently requires authored identity rotation",
    )
    character = ROOT / "assets/processed/normalized/project_assets/chr_project_male1.glb"
    weapon = ROOT / "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"
    for path, key in ((character, "source_character_sha256"), (weapon, "weapon_sha256")):
        require(hashlib.sha256(path.read_bytes()).hexdigest() == pose[key], "Authored source hash changed")
    validate_physical_contacts(pose, read(weapon)[0])
    outputs = {}
    manifest = json.loads((candidate / "preview_assets.json").read_text())
    report = json.loads((candidate / "report.json").read_text())
    for name in ("arms", "body"):
        require(
            hashlib.sha256((candidate / (name + "_runtime.glb")).read_bytes()).hexdigest()
            == report["runtime_player_frames"][name]["sha256"],
            "Stale/modified runtime candidate",
        )
        doc, binary = read(candidate / (name + "_runtime.glb"))
        append_ready(doc, binary, pose)
        outputs[name + "_ready.glb"] = encode(doc, binary)
        asset_id = f"chr_project_male1_fp_{name}_preview"
        asset = next(a for a in manifest["assets"] if a["id"] == asset_id)
        asset["cooked"] = f"project-fps-candidate/{name}_ready.glb"
        metadata = json.loads((candidate / (name + "_runtime.metadata.json")).read_text())
        metadata.update(
            export=asset["cooked"], status="developer Ready; finger contact and GPU acceptance pending"
        )
        metadata["sockets"] += ["socket_palm_r", "socket_palm_l", "socket_rifle_ready"]
        outputs[name + "_ready.metadata.json"] = (json.dumps(metadata, indent=2) + "\n").encode()
    doc, binary = read(weapon)
    for side in ("R", "L"):
        marker(doc, "socket_palm_contact_" + side.lower(), pose["weapon_contact_frames"][side]["matrix"])
    outputs["rifle_ready.glb"] = encode(doc, binary)
    cooked = "project-fps-candidate/rifle_ready.glb"
    manifest["assets"].append(
        {
            "id": WEAPON,
            "kind": "mesh",
            "source": str(weapon.relative_to(ROOT)).replace("\\", "/"),
            "cooked": cooked,
            "streamable": False,
        }
    )
    metadata = {
        "id": WEAPON,
        "source": manifest["assets"][-1]["source"],
        "export": cooked,
        "category": "weapon",
        "scale_meters": True,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "sockets": ["socket_muzzle", "socket_palm_contact_r", "socket_palm_contact_l"],
        "collision": "none",
        "license": "original_project_asset",
        "generated_by": "tools/assets/cook_project_rifle_ready.py",
    }
    outputs["rifle_ready.metadata.json"] = (json.dumps(metadata, indent=2) + "\n").encode()
    outputs["ready_assets.json"] = (json.dumps(manifest, indent=2) + "\n").encode()
    return outputs


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-dir", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for name, data in cook(args.candidate_dir).items():
        path = args.candidate_dir / name
        if args.check:
            require(path.read_bytes() == data, f"Stale Ready variant: {path}")
        else:
            path.write_bytes(data)
    print(
        "Developer Rifle Ready variant: old geometry/palettes/clip accessors preserved; visual acceptance pending"
    )
