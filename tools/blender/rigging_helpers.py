"""Shared Blender rigging and coordinate helpers for Nemisis procedural assets."""

from __future__ import annotations

import math
from collections.abc import Callable
from typing import Any

import bpy


def ensure_asset_root(asset_id: str) -> bpy.types.Object:
    root = bpy.data.objects.get(asset_id)
    if root is not None and root.type == "EMPTY":
        return root

    root = bpy.data.objects.new(asset_id, None)
    root.empty_display_type = "PLAIN_AXES"
    root.empty_display_size = 0.18
    bpy.context.collection.objects.link(root)
    for obj in list(bpy.context.scene.objects):
        if obj == root or obj.parent is not None:
            continue
        world = obj.matrix_world.copy()
        obj.parent = root
        obj.matrix_world = world
    return root


def normalize_asset_root(
    asset_id: str,
    *,
    origin_socket: str | None = None,
    rotate_positive_y_to_negative_y: bool = False,
) -> bpy.types.Object:
    """Place the asset root at zero and optionally rotate authored +Y to Blender -Y."""

    root = ensure_asset_root(asset_id)
    if origin_socket:
        socket = bpy.data.objects.get(origin_socket)
        if socket is None:
            raise ValueError(f"Missing origin socket {origin_socket!r} for {asset_id}")
        bpy.context.view_layer.update()
        offset = root.matrix_world.inverted() @ socket.matrix_world.translation
        for child in list(root.children):
            child.location -= offset

    root.location = (0.0, 0.0, 0.0)
    root.rotation_euler = (0.0, 0.0, math.pi if rotate_positive_y_to_negative_y else 0.0)
    root.scale = (1.0, 1.0, 1.0)
    bpy.context.view_layer.update()
    return root


def _infer_deform_bone(name: str, available: set[str]) -> str:
    lowered = name.lower()
    side = ".R" if lowered.endswith("_r") or ".r" in lowered else ".L" if lowered.endswith("_l") or ".l" in lowered else ""

    candidates: list[str] = []
    if "upper_arm" in lowered or "shoulder_pad" in lowered:
        candidates.append(f"upper_arm{side}")
    elif any(token in lowered for token in ("forearm", "lower_arm", "wrist")):
        candidates.append(f"lower_arm{side}")
    elif "hand" in lowered:
        candidates.append(f"hand{side}")
    elif "thigh" in lowered:
        candidates.append(f"thigh{side}")
    elif any(token in lowered for token in ("shin", "lower_leg", "knee")):
        candidates.extend((f"calf{side}", f"lower_leg{side}"))
    elif "boot" in lowered or "foot" in lowered:
        candidates.append(f"foot{side}")
    elif "pelvis" in lowered:
        candidates.append("pelvis")
    elif any(token in lowered for token in ("torso", "chest", "shoulder_bar", "back_pack", "pack_")):
        candidates.append("spine")
    elif "neck" in lowered:
        candidates.append("neck")
    elif any(token in lowered for token in ("head", "helmet", "visor", "brow")):
        candidates.append("head")
    candidates.extend(("root", next(iter(available))))
    return next(candidate for candidate in candidates if candidate in available)


def bind_character_meshes(
    armature: bpy.types.Object,
    *,
    include: Callable[[bpy.types.Object], bool] | None = None,
) -> list[str]:
    """Rigid-weight procedural mesh parts to their nearest semantic bone."""

    available = set(armature.data.bones.keys())
    bound: list[str] = []
    for obj in bpy.context.scene.objects:
        if obj.type != "MESH" or obj.name.startswith(("col_", "hitbox_")):
            continue
        if include is not None and not include(obj):
            continue

        bone_name = _infer_deform_bone(obj.name, available)
        for group in list(obj.vertex_groups):
            obj.vertex_groups.remove(group)
        group = obj.vertex_groups.new(name=bone_name)
        group.add([vertex.index for vertex in obj.data.vertices], 1.0, "REPLACE")

        for modifier in list(obj.modifiers):
            if modifier.type == "ARMATURE":
                obj.modifiers.remove(modifier)
        modifier = obj.modifiers.new(name="NemisisArmature", type="ARMATURE")
        modifier.object = armature
        modifier.use_vertex_groups = True
        world = obj.matrix_world.copy()
        obj.parent = armature
        obj.parent_type = "OBJECT"
        obj.matrix_world = world
        bound.append(obj.name)
    bpy.context.view_layer.update()
    return sorted(bound)


def parent_socket_to_bone(socket_name: str, armature: bpy.types.Object, bone_name: str) -> None:
    socket = bpy.data.objects.get(socket_name)
    if socket is None or bone_name not in armature.data.bones:
        return
    world = socket.matrix_world.copy()
    socket.parent = armature
    socket.parent_type = "BONE"
    socket.parent_bone = bone_name
    socket.matrix_world = world


def _reset_pose(armature: bpy.types.Object) -> None:
    for pose_bone in armature.pose.bones:
        pose_bone.rotation_mode = "XYZ"
        pose_bone.location = (0.0, 0.0, 0.0)
        pose_bone.rotation_euler = (0.0, 0.0, 0.0)
        pose_bone.scale = (1.0, 1.0, 1.0)


def _insert_pose_key(armature: bpy.types.Object, frame: int, transforms: dict[str, dict[str, tuple[float, float, float]]]) -> None:
    _reset_pose(armature)
    for bone_name, values in transforms.items():
        pose_bone = armature.pose.bones.get(bone_name)
        if pose_bone is None:
            continue
        if "location" in values:
            pose_bone.location = values["location"]
        if "rotation" in values:
            pose_bone.rotation_euler = values["rotation"]
    for pose_bone in armature.pose.bones:
        pose_bone.keyframe_insert("location", frame=frame, group=pose_bone.name)
        pose_bone.keyframe_insert("rotation_euler", frame=frame, group=pose_bone.name)


def _new_action(
    asset_id: str,
    armature: bpy.types.Object,
    name: str,
    frames: list[tuple[int, dict[str, dict[str, tuple[float, float, float]]]]],
) -> bpy.types.Action:
    action_name = f"{asset_id}_{name}"
    previous = bpy.data.actions.get(action_name)
    if previous is not None:
        bpy.data.actions.remove(previous)
    action = bpy.data.actions.new(action_name)
    action.use_fake_user = True
    action["nemisis_asset_id"] = asset_id
    action["nemisis_clip_role"] = name
    armature.animation_data_create()
    armature.animation_data.action = action
    for frame, transforms in frames:
        _insert_pose_key(armature, frame, transforms)
    armature.animation_data.action = None
    return action


def create_character_actions(asset_id: str, armature: bpy.types.Object, *, first_person: bool) -> list[str]:
    """Author small, loopable validation clips that export as independent glTF animations."""

    for action in list(bpy.data.actions):
        if action.get("nemisis_asset_id") == asset_id:
            bpy.data.actions.remove(action)

    idle = [
        (1, {}),
        (20, {"root": {"location": (0.0, 0.0, 0.008)}, "spine": {"rotation": (0.012, 0.0, 0.0)}}),
        (40, {}),
    ]
    aim = [
        (1, {}),
        (8, {
            "upper_arm.R": {"rotation": (0.0, 0.0, -0.08)},
            "upper_arm.L": {"rotation": (0.0, 0.0, 0.08)},
            "lower_arm.R": {"rotation": (-0.04, 0.0, 0.0)},
            "lower_arm.L": {"rotation": (-0.04, 0.0, 0.0)},
        }),
        (16, {}),
    ]
    clips = [_new_action(asset_id, armature, "idle", idle), _new_action(asset_id, armature, "aim", aim)]

    if not first_person and "thigh.R" in armature.pose.bones:
        walk = [
            (1, {
                "thigh.R": {"rotation": (0.42, 0.0, 0.0)},
                "thigh.L": {"rotation": (-0.42, 0.0, 0.0)},
                "upper_arm.R": {"rotation": (-0.22, 0.0, 0.0)},
                "upper_arm.L": {"rotation": (0.22, 0.0, 0.0)},
            }),
            (13, {
                "root": {"location": (0.0, 0.0, 0.025)},
                "thigh.R": {"rotation": (-0.42, 0.0, 0.0)},
                "thigh.L": {"rotation": (0.42, 0.0, 0.0)},
                "upper_arm.R": {"rotation": (0.22, 0.0, 0.0)},
                "upper_arm.L": {"rotation": (-0.22, 0.0, 0.0)},
            }),
            (25, {
                "thigh.R": {"rotation": (0.42, 0.0, 0.0)},
                "thigh.L": {"rotation": (-0.42, 0.0, 0.0)},
                "upper_arm.R": {"rotation": (-0.22, 0.0, 0.0)},
                "upper_arm.L": {"rotation": (0.22, 0.0, 0.0)},
            }),
        ]
        clips.append(_new_action(asset_id, armature, "walk", walk))

    _reset_pose(armature)
    bpy.context.scene.frame_start = 1
    bpy.context.scene.frame_end = 40
    return [action.name for action in clips]


def rig_procedural_character(asset_id: str, armature: bpy.types.Object, *, first_person: bool) -> dict[str, Any]:
    bound = bind_character_meshes(armature, include=lambda obj: obj.name.startswith(asset_id))
    clips = create_character_actions(asset_id, armature, first_person=first_person)

    socket_bones = {
        "socket_root": "root",
        "socket_camera": "root" if first_person else "head",
        "socket_weapon_root": "hand.R",
        "socket_hand_r": "hand.R",
        "socket_hand_l": "hand.L",
        "socket_head": "head",
        "socket_backpack": "spine",
        "socket_vfx": "hand.R" if first_person else "spine",
    }
    for socket_name, bone_name in socket_bones.items():
        parent_socket_to_bone(socket_name, armature, bone_name)

    return {
        "skin": armature.name,
        "skinned_meshes": bound,
        "animation_clips": clips,
        "animation_clip_roles": {
            clip: clip.rsplit("_", 1)[-1] for clip in clips
        },
        "rig_type": "deforming_armature",
    }


def scene_rig_metadata(asset_id: str) -> dict[str, Any]:
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    if not armatures:
        return {}
    armature = armatures[0]
    skinned = sorted(
        obj.name
        for obj in bpy.context.scene.objects
        if obj.type == "MESH"
        and any(modifier.type == "ARMATURE" and modifier.object == armature for modifier in obj.modifiers)
    )
    clips = sorted(action.name for action in bpy.data.actions if action.get("nemisis_asset_id") == asset_id)
    return {
        "skin": armature.name,
        "skinned_meshes": skinned,
        "animation_clips": clips,
        "animation_clip_roles": {clip: clip.rsplit("_", 1)[-1] for clip in clips},
        "rig_type": "deforming_armature",
    }
