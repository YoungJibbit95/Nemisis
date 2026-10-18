"""Append reviewable bone-landmark palm frames to a separate Project arms GLB.

No vertex/weight/bind/clip edits, grip inference from bounding boxes, new solver,
or runtime registration. The original seven sockets remain unchanged. Artists
must approve normal sign, contact depth, grip approach and finger articulation.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import struct

from asset_readiness_check import gltf_node_local_matrix, mat4_multiply, mat4_transform_origin

ROOT = Path(__file__).resolve().parents[2]
SPEC = ROOT / "assets/metadata/project_palm_frame_proposal.json"


def require(value, message):
    if not value:
        raise ValueError(message)


def sha256_file(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(65536), b""):
            result.update(block)
    return result.hexdigest()


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def unit(v):
    length = math.sqrt(dot(v, v))
    require(math.isfinite(length) and length > 1e-6, "Degenerate palm landmarks")
    return [x / length for x in v]


def cross(a, b):
    return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]


def make_proposal(candidate_dir):
    recipe_path = ROOT / "assets/metadata/project_fps_candidate_regions.json"
    recipe = json.loads(recipe_path.read_text(encoding="utf-8"))
    spec = json.loads(SPEC.read_text(encoding="utf-8"))
    require(spec["source_sha256"] == recipe["source"]["sha256"], "Palm proposal/source recipe mismatch")
    for item in (recipe["source"], recipe["original_source"]):
        require(sha256_file(ROOT / item["path"]) == item["sha256"], "Source hash changed; review palm landmarks")
    report = json.loads((candidate_dir / "report.json").read_text(encoding="utf-8"))
    source_path = candidate_dir / "arms.glb"
    require(report["recipe_sha256"] == sha256_file(recipe_path) and
            report["variants"]["arms"]["sha256"] == sha256_file(source_path), "Stale/modified arms candidate")
    data = source_path.read_bytes()
    json_length = struct.unpack_from("<I", data, 12)[0]
    doc = json.loads(data[20:20 + json_length])
    # Preserve the entire original BIN chunk (including header/padding) exactly.
    bin_chunk = data[20 + json_length:]
    original = copy.deepcopy(doc)
    nodes = doc["nodes"]
    parents = {child: i for i, node in enumerate(nodes) for child in node.get("children", [])}
    names = {node.get("name"): i for i, node in enumerate(nodes)}
    frames = []
    identity = [[float(i == j) for j in range(4)] for i in range(4)]
    for frame in spec["frames"]:
        hand = names[frame["hand"]]

        def relative(node):
            if node == hand:
                return identity
            require(node in parents, "Palm landmark is not a descendant of the deforming hand")
            return mat4_multiply(relative(parents[node]), gltf_node_local_matrix(nodes[node]))

        index, middle, pinky = [mat4_transform_origin(relative(names[frame[key]])) for key in ("index", "middle", "pinky")]
        origin = [(index[i] + pinky[i]) * 0.25 for i in range(3)]
        z = unit(middle)
        across = [index[i] - pinky[i] for i in range(3)]
        x = unit([across[i] - z[i] * dot(across, z) for i in range(3)])
        y = unit(cross(z, x))
        matrix = [*x, 0.0, *y, 0.0, *z, 0.0, *origin, 1.0]
        require(frame["socket"] not in names, "Palm socket already exists")
        nodes[hand].setdefault("children", []).append(len(nodes))
        nodes.append({"name": frame["socket"], "matrix": matrix})
        frames.append({**frame, "translation_hand_local_m": origin,
                       "x_hand_local": x, "y_hand_local": y, "z_hand_local": z})
    for key, value in original.items():
        if key != "nodes":
            require(doc[key] == value, f"Source {key} was modified")
    for before, after in zip(original["nodes"], nodes):
        require({k: v for k, v in before.items() if k != "children"} ==
                {k: v for k, v in after.items() if k != "children"} and
                after.get("children", [])[:len(before.get("children", []))] == before.get("children", []),
                "Original node TRS/hierarchy changed")
    header = json.dumps(doc, separators=(",", ":"), ensure_ascii=True).encode()
    header += b" " * (-len(header) % 4)
    result = struct.pack("<III", 0x46546C67, 2, 20 + len(header) + len(bin_chunk)) + struct.pack("<II", len(header), 0x4E4F534A) + header + bin_chunk
    proposal = {"status": "anatomical landmark proposal; NOT approved weapon-contact frames",
                "source_arms_sha256": sha256_file(source_path), "proposal_sha256": hashlib.sha256(result).hexdigest(),
                "preserved_bin_sha256": hashlib.sha256(bin_chunk).hexdigest(), "frames": frames,
                "artist_decisions": ["Confirm palmar normal sign and approach rotation for each hand.",
                                     "Set contact depth on palm surface; author finger wrap and trigger pose.",
                                     "Approve hip/ADS/reload poses on the actual M4 geometry."]}
    return {"arms_palm_proposal.glb": result,
            "palm_proposal_report.json": (json.dumps(proposal, indent=2) + "\n").encode()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-dir", type=Path, required=True)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for name, data in make_proposal(args.candidate_dir).items():
        path = args.candidate_dir / name
        if args.check:
            require(path.read_bytes() == data, f"Stale proposal: {path}")
        else:
            path.write_bytes(data)
    print("Project palm proposal verified; no runtime/IK activation or artist acceptance")


if __name__ == "__main__":
    main()
