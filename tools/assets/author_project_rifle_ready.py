"""Non-destructive Blender developer authoring rehearsal on the real Project rig."""

import bpy, json, sys, math, hashlib, argparse
from pathlib import Path
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args(sys.argv[sys.argv.index("--") + 1 :])
ROOT = Path(__file__).resolve().parents[2]
OUT = args.output
OUT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / "tools/assets"))
from asset_readiness_check import gltf_node_local_matrix
import struct


def document(path):
    data = path.read_bytes()
    n = struct.unpack_from("<I", data, 12)[0]
    return json.loads(data[20 : 20 + n])


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


for obj in list(bpy.data.objects):
    bpy.data.objects.remove(obj, do_unlink=True)
source = args.source
doc = document(source)
nodes = doc["nodes"]
names = {n.get("name"): i for i, n in enumerate(nodes)}
parents = {c: i for i, n in enumerate(nodes) for c in n.get("children", [])}
world_cache = {}


def original_world(i):
    if i not in world_cache:
        local = Matrix(gltf_node_local_matrix(nodes[i]))
        world_cache[i] = original_world(parents[i]) @ local if i in parents else local
    return world_cache[i]


C = Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))
CI = C.inverted()
print("IMPORT_PROJECT", flush=True)
bpy.ops.import_scene.gltf(filepath=str(source))
rigs = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
require(len(rigs) == 1, "Expected one logical authoring armature")
rig = rigs[0]
rig.animation_data_clear()
for p in rig.pose.bones:
    p.matrix_basis = Matrix.Identity(4)
bpy.context.view_layer.update()
rest = {p.name: (rig.matrix_world @ p.matrix).copy() for p in rig.pose.bones}
mapping = {name: rest[name].inverted() @ C @ original_world(names[name]) for name in rest}


def bvh(objects):
    deps = bpy.context.evaluated_depsgraph_get()
    vertices = []
    faces = []
    for obj in objects:
        if obj.type != "MESH":
            continue
        evaluated = obj.evaluated_get(deps)
        mesh = evaluated.to_mesh()
        base = len(vertices)
        vertices.extend(evaluated.matrix_world @ v.co for v in mesh.vertices)
        faces.extend(tuple(base + i for i in p.vertices) for p in mesh.polygons)
        evaluated.to_mesh_clear()
    return BVHTree.FromPolygons(vertices, faces, all_triangles=False)


character_objects = list(bpy.context.scene.objects)
skin_tree = bvh(character_objects)
palms = {}
for side in ["R", "L"]:
    hand = original_world(names["DEF-hand." + side])
    points = [
        (hand.inverted() @ original_world(names["DEF-" + finger + ".01." + side])).translation
        for finger in ["f_index", "f_middle", "f_pinky"]
    ]
    index, middle, pinky = points
    origin = (index + pinky) * 0.25
    z = middle.normalized()
    x = (index - pinky - z * (index - pinky).dot(z)).normalized()
    y = z.cross(x).normalized()
    palm = Matrix.Identity(4)
    for j, axis in enumerate([x, y, z]):
        for i in range(3):
            palm[i][j] = axis[i]
    palm.translation = origin
    position = (C @ hand @ palm).translation
    direction = (C @ hand @ palm).to_3x3() @ Vector((0, -1 if side == "R" else 1, 0))
    direction.normalize()
    hit, normal, triangle, distance = skin_tree.ray_cast(position, direction, 0.06)
    require(hit is not None, f"{side} anatomical palm ray did not meet real skin")
    require(distance < 0.04, f"{side} anatomical palm surface too far from bone landmark")
    palm.translation = (hand.inverted() @ CI @ hit.to_4d()).to_3d()
    palms[side] = {
        "matrix": palm,
        "surface_distance_m": distance,
        "surface_normal_dot": normal.dot(direction),
        "triangle": triangle,
        "source_bone": "DEF-hand." + side,
    }

weapon = ROOT / "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"
before = set(bpy.context.scene.objects)
print("IMPORT_M4", flush=True)
bpy.ops.import_scene.gltf(filepath=str(weapon))
weapon_objects = [o for o in bpy.context.scene.objects if o not in before]
weapon_tree = bvh(weapon_objects)
weapon_doc = document(weapon)
wn = weapon_doc["nodes"]
wp = {c: i for i, n in enumerate(wn) for c in n.get("children", [])}


def weapon_world(i):
    local = Matrix(gltf_node_local_matrix(wn[i]))
    return weapon_world(wp[i]) @ local if i in wp else local


grips = {
    n["name"]: weapon_world(i).translation
    for i, n in enumerate(wn)
    if n.get("name") in ["socket_grip_r", "socket_grip_l"]
}
contacts = {}
for side in ["R", "L"]:
    grip = grips["socket_grip_" + side.lower()]
    start = grip + Vector((-0.10, 0, 0) if side == "R" else (0, -0.10, 0))
    direction = Vector((1, 0, 0) if side == "R" else (0, 1, 0))
    hit, normal, triangle, distance = weapon_tree.ray_cast(
        (C @ start.to_4d()).to_3d(), C.to_3x3() @ direction, 0.2
    )
    require(hit is not None, f"{side} physical M4 grip ray missed actual geometry")
    point = (CI @ hit.to_4d()).to_3d()
    axes = (
        [Vector((0, 0, 1)), Vector((-1, 0, 0)), Vector((0, -1, 0))]
        if side == "R"
        else [Vector((0, 0, 1)), Vector((0, 1, 0)), Vector((-1, 0, 0))]
    )
    frame = Matrix.Identity(4)
    for j, axis in enumerate(axes):
        for i in range(3):
            frame[i][j] = axis[i]
    frame.translation = point
    contacts[side] = {
        "matrix": frame,
        "triangle": triangle,
        "normal": list(CI.to_3x3() @ normal),
        "grip_calibration": list(point - grip),
    }


def empty(name, matrix):
    obj = bpy.data.objects.new(name, None)
    bpy.context.collection.objects.link(obj)
    obj.matrix_world = matrix
    return obj


def bone_world(name):
    return rig.matrix_world @ rig.pose.bones[name].matrix


def twist(name, degrees):
    pose = rig.pose.bones[name]
    m = bone_world(name)
    pivot = m.translation.copy()
    change = (
        Matrix.Translation(pivot)
        @ Matrix.Rotation(math.radians(degrees), 4, "Z")
        @ Matrix.Translation(-pivot)
    )
    pose.matrix = rig.matrix_world.inverted() @ change @ m
    bpy.context.view_layer.update()


def retarget_spines():
    for suffix in [".003", ".004", ".005", ".006"]:
        deform = "DEF-spine" + suffix
        control = "ORG-spine" + suffix
        if deform not in rest or control not in rest:
            continue
        require(
            (rest[deform].translation - rest[control].translation).length < 0.002,
            f"{deform} does not match its measured source reference",
        )
        desired = bone_world(control) @ rest[control].inverted() @ rest[deform]
        rig.pose.bones[deform].matrix = rig.matrix_world.inverted() @ desired
        bpy.context.view_layer.update()


def pole_alignment(side, pole):
    s = bone_world("DEF-upper_arm." + side).translation
    e = bone_world("DEF-forearm." + side).translation
    w = bone_world("DEF-hand." + side).translation
    direction = (w - s).normalized()
    actual = (e - s) - direction * (e - s).dot(direction)
    desired = (pole - s) - direction * (pole - s).dot(direction)
    return actual.normalized(), desired.normalized(), direction


def arm_constraint(side, world_weapon):
    local_contact = palms[side]["matrix"]
    desired_hand = world_weapon @ contacts[side]["matrix"] @ local_contact.inverted()
    desired_bone = C @ desired_hand @ mapping["DEF-hand." + side].inverted()
    target = empty("Ready_wrist_" + side, desired_bone)
    pole_runtime = Vector((-0.42, 1.12, -0.07) if side == "R" else (0.42, 1.12, 0.22))
    pole = empty("Ready_pole_" + side, Matrix.Translation((C @ pole_runtime.to_4d()).to_3d()))
    for name in [
        "DEF-upper_arm." + side,
        "DEF-upper_arm." + side + ".001",
        "DEF-forearm." + side,
        "DEF-forearm." + side + ".001",
    ]:
        p = rig.pose.bones[name]
        p.ik_stretch = 0
        if name.endswith(".001"):
            p.lock_ik_x = p.lock_ik_y = p.lock_ik_z = True
    ik = rig.pose.bones["DEF-forearm." + side + ".001"].constraints.new("IK")
    ik.target = target
    ik.pole_target = pole
    ik.chain_count = 4
    ik.use_tail = True
    ik.use_stretch = False
    wrist = rig.pose.bones["DEF-hand." + side].constraints.new("COPY_ROTATION")
    wrist.target = target
    wrist.owner_space = "WORLD"
    wrist.target_space = "WORLD"
    bpy.context.view_layer.update()
    actual, wanted, axis = pole_alignment(side, pole.matrix_world.translation)
    delta = math.atan2(axis.dot(actual.cross(wanted)), max(-1, min(1, actual.dot(wanted))))
    best = (actual.dot(wanted), ik.pole_angle)
    for angle in [delta, -delta, delta + math.pi, -delta + math.pi]:
        ik.pole_angle = angle
        bpy.context.view_layer.update()
        a, b, _ = pole_alignment(side, pole.matrix_world.translation)
        if a.dot(b) > best[0]:
            best = (a.dot(b), angle)
    ik.pole_angle = best[1]
    bpy.context.view_layer.update()
    residual = (bone_world("DEF-hand." + side).translation - desired_bone.translation).length
    require(residual < 0.001, f"{side} Blender wrist residual {residual:.6f}m")
    return {"wrist_error_m": residual, "pole_alignment": best[0], "pole_angle": best[1]}


def pose_data():
    posed = {name: CI @ bone_world(name) @ mapping[name] for name in rest}
    output = {}
    max_affine = 0
    max_translation = 0
    max_scale = 0
    for name, global_pose in posed.items():
        node = names[name]
        parent = parents.get(node)
        parent_name = nodes[parent].get("name") if parent is not None else None
        parent_pose = (
            posed[parent_name]
            if parent_name in posed
            else original_world(parent) if parent is not None else Matrix.Identity(4)
        )
        local = parent_pose.inverted() @ global_pose
        t, q, s = local.decompose()
        reconstructed = Matrix.Translation(t) @ q.to_matrix().to_4x4() @ Matrix.Diagonal((*s, 1))
        affine = max(abs(local[r][c] - reconstructed[r][c]) for r in range(4) for c in range(4))
        max_affine = max(max_affine, affine)
        require(affine < 0.00002, f"{name} cannot retain local affine TRS ({affine})")
        original = Matrix(gltf_node_local_matrix(nodes[node]))
        ot, oq, os = original.decompose()
        td = (t - ot).length
        sd = (s - os).length
        max_translation = max(max_translation, td)
        max_scale = max(max_scale, sd)
        require(td < 0.00002, f"{name} bone translation changed by {td}m")
        require(sd < 0.00002, f"{name} scale changed by {sd}")
        # Include unchanged upper-body rotations as well: an unarmed shoulder
        # helper must not leak through a mask of only visibly changed joints.
        cursor, upper = node, False
        while True:
            if cursor in (names["spine_fk.003"], names["DEF-spine.003"]):
                upper = True
                break
            if cursor not in parents:
                break
            cursor = parents[cursor]
        if (
            upper
            or min(sum((a - b) ** 2 for a, b in zip(q, oq)), sum((a + b) ** 2 for a, b in zip(q, oq))) > 1e-8
        ):
            output[name] = {"rotation_xyzw": [q.x, q.y, q.z, q.w]}
    return output, {
        "max_affine_reconstruction": max_affine,
        "max_local_translation_delta_m": max_translation,
        "max_local_scale_delta": max_scale,
    }


print("AUTHOR_READY", flush=True)
require("spine_fk.003" in rig.pose.bones, "Actual chest control absent")
twist("spine_fk.003", -40)
retarget_spines()
if "head" in rig.pose.bones:
    twist("head", 40)
world_weapon = Matrix.Translation(Vector((-0.11, 1.54, 0.21)))
results = {side: arm_constraint(side, world_weapon) for side in ["R", "L"]}

# Reproducible developer finger controls, baked offline. Each curl axis comes
# from the actual phalange direction and the measured palmar approach normal.
# These are authoring seeds, not an assertion of collision-free artist contact.
for side in ["R", "L"]:
    palm_world = world_weapon @ contacts[side]["matrix"]
    normal = C.to_3x3() @ palm_world.to_3x3() @ Vector((0, -1 if side == "R" else 1, 0))
    for finger in ["f_index", "f_middle", "f_ring", "f_pinky", "thumb"]:
        # Right index remains comparatively extended along the trigger guard.
        angles = [15, 20, 15] if side == "R" and finger == "f_index" else [50, 70, 50]
        if finger == "thumb":
            angles = [25, 35, 25]
        for number, angle in zip(["01", "02", "03"], angles):
            name = "DEF-" + finger + "." + number + "." + side
            if name not in rig.pose.bones:
                continue
            pose = rig.pose.bones[name]
            m = bone_world(name)
            direction = (rig.matrix_world.to_3x3() @ (pose.tail - pose.head)).normalized()
            axis = direction.cross(normal)
            if axis.length < 1e-6:
                continue
            pivot = m.translation.copy()
            change = (
                Matrix.Translation(pivot)
                @ Matrix.Rotation(math.radians(angle), 4, axis.normalized())
                @ Matrix.Translation(-pivot)
            )
            pose.matrix = rig.matrix_world.inverted() @ change @ m
            bpy.context.view_layer.update()
posed, checks = pose_data()
mount_parent = "ORG-spine.003"
require(mount_parent in rest, "Measured common shoulder ancestor is absent")
mount_parent_world = CI @ bone_world(mount_parent) @ mapping[mount_parent]
mount_local = mount_parent_world.inverted() @ world_weapon
result = {
    "status": "Blender-authored developer Rifle Ready; finger wrap and artist/GPU acceptance pending",
    "blender_version": bpy.app.version_string,
    "blender_build_hash": bpy.app.build_hash.decode(),
    "source_character_sha256": hashlib.sha256(
        (ROOT / "assets/processed/normalized/project_assets/chr_project_male1.glb").read_bytes()
    ).hexdigest(),
    "weapon_sha256": hashlib.sha256(weapon.read_bytes()).hexdigest(),
    "weapon_model_transform": [list(r) for r in world_weapon],
    "weapon_mount_parent": mount_parent,
    "weapon_mount_local_matrix": [list(r) for r in mount_local],
    "rotation_mask_roots": ["spine_fk.003", "DEF-spine.003"],
    "joint_pose": posed,
    "checks": checks,
    "arms": results,
    "palms": {
        side: {k: ([list(r) for r in v] if k == "matrix" else v) for k, v in p.items()}
        for side, p in palms.items()
    },
    "weapon_contact_frames": {
        side: {k: ([list(r) for r in v] if k == "matrix" else v) for k, v in p.items()}
        for side, p in contacts.items()
    },
}
(OUT / "rifle_ready_pose.json").write_text(json.dumps(result, indent=2))
# Place the actual M4 geometry at the same authored transform for Blender review.
weapon_instance = C @ world_weapon @ CI
for obj in weapon_objects:
    if obj.parent not in weapon_objects:
        obj.matrix_world = weapon_instance @ obj.matrix_world
bpy.context.view_layer.update()
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "project_rifle_ready.blend"))
print(
    "READY_AUTHORING_PASS",
    json.dumps({"rotation_tracks": len(posed), "checks": checks, "arms": results}),
    flush=True,
)
