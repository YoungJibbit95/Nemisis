"""Non-destructive Blender developer authoring rehearsal on the real Project rig."""

import bpy, json, sys, math, hashlib, argparse
from pathlib import Path
from mathutils import Matrix, Vector, Quaternion
from mathutils.bvhtree import BVHTree

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--profile", choices=("ready", "armed-idle"), default="ready")
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
    if args.profile == "armed-idle" and side == "R":
        # Place the heel toward the measured rear of the same pistol-grip side.
        # This keeps the fingers around the handle instead of at the magazine.
        # The actual surface remains within the existing 20mm firing region.
        start.z -= 0.015
        # Keep the actual thumb-root flesh below the receiver, still on the
        # measured pistol-grip surface and inside that same physical region.
        start.y -= 0.0095
    if args.profile == "armed-idle" and side == "L":
        # Move the bottom-surface support a little toward its real fingers;
        # the unchanged 10mm handguard-region check bounds this calibration.
        start.x -= 0.0045
    direction = Vector((1, 0, 0) if side == "R" else (0, 1, 0))
    hit, normal, triangle, distance = weapon_tree.ray_cast(
        (C @ start.to_4d()).to_3d(), C.to_3x3() @ direction, 0.2
    )
    require(hit is not None, f"{side} physical M4 grip ray missed actual geometry")
    point = (CI @ hit.to_4d()).to_3d()
    axes = (
        ([Vector((0, 1, 0)), Vector((-1, 0, 0)), Vector((0, 0, 1))]
         if args.profile == "armed-idle"
         else [Vector((0, 0, 1)), Vector((-1, 0, 0)), Vector((0, -1, 0))])
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


def twist(name, degrees, axis="Z"):
    pose = rig.pose.bones[name]
    m = bone_world(name)
    pivot = m.translation.copy()
    change = (
        Matrix.Translation(pivot)
        @ Matrix.Rotation(math.radians(degrees), 4, axis)
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


def finger_surface(tree, start, direction):
    hit, normal, triangle, _ = tree.ray_cast(
        (C @ Vector((*start, 1))).to_3d(), C.to_3x3() @ Vector(direction), 0.25
    )
    require(hit is not None, f"Finger target ray missed actual geometry: {start}")
    return (CI @ hit.to_4d()).to_3d(), (CI.to_3x3() @ normal).normalized(), triangle


def author_finger_contact(side, finger, point, normal, triangle, world_weapon):
    # Bake positive flexion around each measured source hinge. No free chain
    # pole/roll: skin proximity alone previously hid hyperextended knuckles.
    bones = [rig.pose.bones["DEF-" + finger + "." + n + "." + side] for n in ("01", "02", "03")]
    for bone in bones:
        bone.rotation_mode = "QUATERNION"
        bone.rotation_quaternion = Quaternion((1, 0, 0, 0))
    bpy.context.view_layer.update()
    approach = C.to_3x3() @ world_weapon.to_3x3() @ contacts[side]["matrix"].to_3x3() @ Vector((0, -1 if side == "R" else 1, 0))
    hinges = []
    for bone in bones:
        m = bone_world(bone.name)
        direction = (rig.matrix_world.to_3x3() @ (bone.tail - bone.head)).normalized()
        axis = direction.cross(approach).normalized()
        hinges.append((m.to_3x3().inverted() @ axis).normalized())
    abduction_axis = (bone_world(bones[0].name).to_3x3().inverted() @ approach).normalized()
    opposition_world = C.to_3x3() @ world_weapon.to_3x3() @ contacts[side]["matrix"].to_3x3() @ Vector((0, 0, 1))
    opposition_axis = (bone_world(bones[0].name).to_3x3().inverted() @ opposition_world).normalized()
    selected = []
    for obj in character_objects:
        if obj.type != "MESH" or not obj.find_armature():
            continue
        ids = [v.index for v in obj.data.vertices
               if sum(g.weight for g in v.groups
                      if obj.vertex_groups[g.group].name.startswith("DEF-" + finger + ".")
                      and obj.vertex_groups[g.group].name.endswith("." + side)) > .5]
        if ids:
            selected.append((obj, ids))
    deps = bpy.context.evaluated_depsgraph_get()
    world_to_weapon = C @ world_weapon.inverted() @ CI
    target_tree = trigger_tree if side == "R" and finger == "f_index" else weapon_tree
    surface = (C @ point.to_4d()).to_3d()
    maximum = (90, 95, 85, 60, 60) if finger == "thumb" else (110, 110, 85, 20, 0)

    def fit(values):
        for i, bone in enumerate(bones):
            q = Quaternion(hinges[i], math.radians(values[i]))
            if i == 0:
                q = Quaternion(abduction_axis, math.radians(values[3])) @ q
                q = Quaternion(opposition_axis, math.radians(values[4])) @ q
            bone.rotation_quaternion = q
        bpy.context.view_layer.update()
        tip = rig.matrix_world @ bones[2].tail
        signed, patch, intended = [], [], []
        for obj, ids in selected:
            ev = obj.evaluated_get(deps)
            mesh = ev.to_mesh()
            for index in ids:
                p = ev.matrix_world @ mesh.vertices[index].co
                local = world_to_weapon @ p
                hit, n, tri, distance = weapon_tree.find_nearest(local)
                signed.append((local - hit).dot(n))
                if (p - tip).length < .025:
                    patch.append(target_tree.find_nearest(local)[3])
                    intended.append((local - surface).length)
            ev.to_mesh_clear()
        if not patch:
            return (1000, 1, 1, 1)
        penetration, gap, region = max(0, -min(signed)), min(patch), min(intended)
        score = (max(0, penetration - .0007) * 100 + max(0, gap - .0015) * 40 +
                 max(0, region - .020) + penetration * .1 + gap * .1 + abs(values[3]) * .000001)
        # Prefer a distributed curl instead of hiding all the bend in the
        # terminal joint. Contact/clearance remain the stronger constraints.
        if finger != "thumb":
            score += abs(values[2] - values[1] * .65) * .00002
        return score, penetration, gap, region

    best = None
    from itertools import product
    grids = ((0, 30, 60, 90), (0, 30, 60, 95), (0, 30, 60, 85), (-45, 0, 45), (-60, 0, 60)) if finger == "thumb" else (
        (0, 30, 60, 90, 110), (0, 35, 70, 110), (0, 30, 60, 85), (-20, 0, 20), (0,))
    for values in product(*grids):
        measured = fit(values)
        if best is None or measured[0] < best[0][0]:
            best = (measured, values)
    for step in (10, 5, 2.5):
        for iteration in range(3):
            changed = False
            for axis in range(5):
                for sign in (-1, 1):
                    values = list(best[1])
                    values[axis] = max(-maximum[axis] if axis >= 3 else 0,
                                       min(maximum[axis], values[axis] + sign * step))
                    measured = fit(values)
                    if measured[0] < best[0][0]:
                        best = (measured, tuple(values))
                        changed = True
            if not changed:
                break
    measured = fit(best[1])
    print("FINGER_SOURCE_HINGE", side, finger, best[1], measured, flush=True)
    require(measured[1] < .001 and measured[2] < .002,
            f"Anatomical finger skin contact invalid: {side}/{finger}: {measured}")
    return {"surface_point": list(point), "normal": list(normal), "triangle": triangle,
            "curl_degrees": list(best[1][:3]), "mcp_abduction_degrees": best[1][3],
            "thumb_opposition_degrees": best[1][4],
            "blender_hinge_axes_local": [list(a) for a in hinges],
            "gltf_hinge_axes_local": [list((mapping[b.name].to_3x3().inverted() @ a).normalized())
                                      for b, a in zip(bones, hinges)],
            "gltf_mcp_abduction_axis_local": list((mapping[bones[0].name].to_3x3().inverted() @ abduction_axis).normalized()),
            "gltf_thumb_opposition_axis_local": list((mapping[bones[0].name].to_3x3().inverted() @ opposition_axis).normalized()),
            "max_signed_skin_penetration_m": measured[1], "distal_surface_distance_m": measured[2]}


def finger_nearest(tree, probe):
    hit, normal, triangle, _ = tree.find_nearest((C @ Vector((*probe, 1))).to_3d())
    require(hit is not None, "Physical finger target mesh is empty")
    return (CI @ hit.to_4d()).to_3d(), (CI.to_3x3() @ normal).normalized(), triangle


print("AUTHOR_READY", flush=True)
require("spine_fk.003" in rig.pose.bones, "Actual chest control absent")
# Restore the anatomical right mount after correcting the camera's reflected
# +Z-depth view. A proper camera facing gameplay +Z has screen-right at world
# -X; the character's actual .R shoulder is on that side too. The W1 positive-X
# placement compensated for the old camera and crossed the firing arm. The
# original unarmed Sprint -> Idle ancestors still turn relative to the eye;
# measured tick 216 crossed the eye by .0554923m at X=-.11,Z=.21. Shift five
# centimetres right and four rearward, then solve both real arm chains again.
# The rearward component avoids relying on a larger lateral extension of .L.
# At Y=1.54 the real firing hand had 446 horizontally visible vertices but none
# vertically visible at the unchanged 74-degree FOV. Raise the complete authored
# weapon seven centimetres; contacts and both arm chains are solved together.
twist("spine_fk.003", -40)
retarget_spines()
if "head" in rig.pose.bones:
    twist("head", 40)
if args.profile == "armed-idle":
    # These are real clavicle ancestors, not offsets of the visible arms. The
    # previous raised reference put the stock above the shoulder and the firing
    # hand only 4-14cm ahead of the anatomical eye in Vulkan. Protract the actual
    # shoulders to permit a lower, farther-forward receiver at metre scale.
    twist("ORG-shoulder.R", 67)
    twist("ORG-shoulder.L", -30)
    twist("ORG-shoulder.R", 35, "Y")
    twist("ORG-shoulder.L", -10, "Y")
    world_weapon = Matrix.Translation(Vector((-0.13, 1.555, 0.295)))
else:
    world_weapon = Matrix.Translation(Vector((-0.16, 1.61, 0.17)))
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
        if args.profile == "armed-idle" and side == "R" and finger == "f_index":
            angles = [50, 55, 30]
        if finger == "thumb":
            angles = [25, 35, 25]
        for number, angle in zip(["01", "02", "03"], angles):
            name = "DEF-" + finger + "." + number + "." + side
            if name not in rig.pose.bones:
                continue
            pose = rig.pose.bones[name]
            if args.profile == "armed-idle":
                # Rigify source axes were measured in the extended finger:
                # +local X is anatomical flexion; thumbs use signed local Z.
                # Recomputing direction.cross(palmNormal) after each parent
                # curl reverses its sign beyond 90 degrees and hyperextends.
                axis = Vector((0, 0, -1 if side == "R" else 1)) if finger == "thumb" else Vector((1, 0, 0))
                pose.rotation_mode = "QUATERNION"
                pose.rotation_quaternion = Quaternion(axis, math.radians(angle))
                bpy.context.view_layer.update()
                continue
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
finger_goals = {}
if args.profile == "armed-idle":
    base_tree = bvh([o for o in weapon_objects if o.name == "Base"])
    trigger_tree = bvh([o for o in weapon_objects if o.name == "Trigger"])
    for side in ("R", "L"):
        for finger in ("f_index", "f_middle", "f_ring", "f_pinky", "thumb"):
            first = (CI @ bone_world("DEF-" + finger + ".01." + side)).translation
            local = (world_weapon.inverted() @ first.to_4d()).to_3d()
            if side == "R" and finger == "f_index":
                p, n, tri = finger_nearest(trigger_tree, (0.1, .027, .054))
            elif side == "R" and finger == "thumb":
                # The firing thumb lies on the same outside face as its palm;
                # the other fingers wrap onto the opposite handle face.
                p, n, tri = finger_surface(base_tree, (-.1, .035, .015), (1, 0, 0))
            elif side == "R":
                front, _, _ = finger_surface(base_tree, (0, local.y, .085), (0, 0, -1))
                p, n, tri = finger_surface(base_tree, (.1, local.y, front.z - .004), (-1, 0, 0))
            else:
                height = .095 if finger == "f_pinky" else .10
                start = (-.04, .11, .36) if finger == "thumb" else (-.12, height, local.z)
                direction = (-1, 0, 0) if finger == "thumb" else (1, 0, 0)
                p, n, tri = (finger_nearest(weapon_tree, start) if finger == "thumb"
                             else finger_surface(weapon_tree, start, direction))
            finger_goals[side + "/" + finger] = author_finger_contact(side, finger, p, n, tri, world_weapon)
posed, checks = pose_data()
mount_parent = "ORG-spine.003"
require(mount_parent in rest, "Measured common shoulder ancestor is absent")
mount_parent_world = CI @ bone_world(mount_parent) @ mapping[mount_parent]
mount_local = mount_parent_world.inverted() @ world_weapon
result = {
    "status": ("Blender-authored Project M4 Armed Idle; artist/GPU acceptance pending" if args.profile == "armed-idle"
               else "Blender-authored developer Rifle Ready; finger wrap and artist/GPU acceptance pending"),
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
if args.profile == "armed-idle":
    result["clip_name"] = "Project M4 Armed Idle"
    result["finger_surface_goals"] = finger_goals
    result["acceptance"] = {"visual": False, "finger_wrap": False, "armed_locomotion": False}
stem = "project_m4_armed_idle" if args.profile == "armed-idle" else "rifle_ready"
# Place the actual M4 geometry at the same authored transform for Blender review.
weapon_instance = C @ world_weapon @ CI
for obj in weapon_objects:
    if obj.parent not in weapon_objects:
        obj.matrix_world = weapon_instance @ obj.matrix_world
bpy.context.view_layer.update()
if args.profile == "armed-idle":
    # Inspect evaluated source-weighted flesh rather than trusting bone tips.
    # A signed nearest-triangle check is an offline clearance diagnostic, not
    # a GPU/artist acceptance or a general collision solver for open meshes.
    actual_weapon = bvh(weapon_objects)
    actual_trigger = bvh([o for o in weapon_objects if o.name == "Trigger"])
    deps = bpy.context.evaluated_depsgraph_get()
    finger_skin = {}
    for side in ("R", "L"):
        for finger in ("f_index", "f_middle", "f_ring", "f_pinky", "thumb"):
            tip = rig.matrix_world @ rig.pose.bones["DEF-" + finger + ".03." + side].tail
            sample, patch, trigger_patch = [], [], []
            for obj in character_objects:
                if obj.type != "MESH" or not obj.find_armature():
                    continue
                ev = obj.evaluated_get(deps)
                mesh = ev.to_mesh()
                for v in mesh.vertices:
                    weight = sum(g.weight for g in v.groups
                                 if obj.vertex_groups[g.group].name.startswith("DEF-" + finger + ".")
                                 and obj.vertex_groups[g.group].name.endswith("." + side))
                    if weight <= .5:
                        continue
                    p = ev.matrix_world @ v.co
                    hit, n, tri, distance = actual_weapon.find_nearest(p)
                    sample.append((p - hit).dot(n))
                    if (p - tip).length < .025:
                        patch.append((distance, v.index))
                        if side == "R" and finger == "f_index":
                            trigger_patch.append(actual_trigger.find_nearest(p)[3])
                ev.to_mesh_clear()
            require(sample and patch, "Missing actual weighted finger skin: " + side + finger)
            distance, vertex = min(patch)
            penetration = max(0, -min(sample))
            require(distance < .002, "Finger skin floats off actual weapon: " + side + finger)
            require(penetration < .001, f"Finger skin penetrates actual weapon: {side}/{finger}: {penetration}m")
            finger_skin[side + "/" + finger] = {
                "weighted_vertices": len(sample), "distal_patch_vertices": len(patch),
                "closest_distal_source_vertex": vertex, "distal_surface_distance_m": distance,
                "max_signed_penetration_m": penetration,
            }
            if trigger_patch:
                require(min(trigger_patch) < .002, "Actual right index skin misses the physical Trigger mesh")
                finger_skin[side + "/" + finger]["actual_trigger_surface_distance_m"] = min(trigger_patch)
    actual_skin = bvh([o for o in character_objects if o.type == "MESH" and o.find_armature()])
    rear = []
    stock = next(o for o in weapon_objects if o.name == "Stock")
    ev = stock.evaluated_get(deps)
    mesh = ev.to_mesh()
    for v in mesh.vertices:
        p = ev.matrix_world @ v.co
        local = world_weapon.inverted() @ CI @ p
        if local.z >= -.10:
            continue
        hit, n, tri, distance = actual_skin.find_nearest(p)
        rear.append((distance, (p - hit).dot(n)))
    ev.to_mesh_clear()
    require(rear, "Actual rear Stock geometry absent")
    stock_distance = min(d for d, s in rear)
    stock_penetration = max(0, -min(s for d, s in rear))
    require(stock_distance < .005 and stock_penetration < .001,
            f"Actual rear stock/skin contact invalid: gap={stock_distance}, penetration={stock_penetration}")
    result["skin_geometry_checks"] = {
        "finger_skin": finger_skin,
        "rear_stock_vertices": len(rear), "rear_stock_surface_distance_m": stock_distance,
        "rear_stock_max_signed_penetration_m": stock_penetration,
        "scope": "Offline evaluated source skin; visual/occlusion quality remains unaccepted",
    }
(OUT / (stem + "_pose.json")).write_text(json.dumps(result, indent=2), encoding="utf-8", newline="\n")
blend_name = "project_m4_armed_idle.blend" if args.profile == "armed-idle" else "project_rifle_ready.blend"
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / blend_name))
print(
    "READY_AUTHORING_PASS",
    json.dumps({"rotation_tracks": len(posed), "checks": checks, "arms": results}),
    flush=True,
)
