"""Generate Nemisis dev primitives in Blender.

Run from the Nemisis repo root:
    blender --background --python tools/blender/make_dev_primitives.py

Optional:
    blender --background --python tools/blender/make_dev_primitives.py -- --only target
"""

from __future__ import annotations

import json
import math
import sys
from pathlib import Path

try:
    import bpy
    from mathutils import Vector
except ModuleNotFoundError as exc:
    raise SystemExit("Run this script with Blender, not plain Python.") from exc


REPO_ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = REPO_ROOT / "assets" / "source" / "blender"
EXPORT_ROOT = REPO_ROOT / "assets" / "export" / "gltf"


def ensure_dirs(*paths: Path) -> None:
    for path in paths:
        path.mkdir(parents=True, exist_ok=True)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    for data_block in (
        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.armatures,
        bpy.data.cameras,
        bpy.data.lights,
    ):
        for item in list(data_block):
            if item.users == 0:
                data_block.remove(item)


def material(name: str, color: tuple[float, float, float, float]) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = color
    if color[3] < 1.0:
        mat.blend_method = "BLEND"
        if hasattr(mat, "use_screen_refraction"):
            mat.use_screen_refraction = False
        if hasattr(mat, "show_transparent_back"):
            mat.show_transparent_back = True

    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        if "Base Color" in bsdf.inputs:
            bsdf.inputs["Base Color"].default_value = color
        if "Alpha" in bsdf.inputs:
            bsdf.inputs["Alpha"].default_value = color[3]
        if "Roughness" in bsdf.inputs:
            bsdf.inputs["Roughness"].default_value = 0.72
        if "Metallic" in bsdf.inputs:
            bsdf.inputs["Metallic"].default_value = 0.0
    return mat


def assign_material(obj: bpy.types.Object, mat: bpy.types.Material) -> bpy.types.Object:
    obj.data.materials.append(mat)
    return obj


def apply_object_transform(obj: bpy.types.Object) -> bpy.types.Object:
    for selected in bpy.context.selected_objects:
        selected.select_set(False)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    obj.select_set(False)
    return obj


def cube(
    name: str,
    location: tuple[float, float, float],
    scale: tuple[float, float, float],
    mat: bpy.types.Material,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    assign_material(obj, mat)
    return apply_object_transform(obj)


def cylinder(
    name: str,
    location: tuple[float, float, float],
    radius: float,
    depth: float,
    mat: bpy.types.Material,
    vertices: int = 32,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    assign_material(obj, mat)
    return apply_object_transform(obj)


def cylinder_between(
    name: str,
    start: tuple[float, float, float],
    end: tuple[float, float, float],
    radius: float,
    mat: bpy.types.Material,
    vertices: int = 16,
) -> bpy.types.Object:
    start_v = Vector(start)
    end_v = Vector(end)
    direction = end_v - start_v
    depth = direction.length
    if depth <= 0.0001:
        raise ValueError(f"Cannot create zero-length cylinder: {name}")

    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        location=(start_v + end_v) * 0.5,
    )
    obj = bpy.context.object
    obj.name = name
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    assign_material(obj, mat)
    return apply_object_transform(obj)


def sphere(
    name: str,
    location: tuple[float, float, float],
    scale: tuple[float, float, float],
    mat: bpy.types.Material,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(segments=32, ring_count=16, radius=1.0, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    assign_material(obj, mat)
    return apply_object_transform(obj)


def empty(name: str, location: tuple[float, float, float]) -> bpy.types.Object:
    bpy.ops.object.empty_add(type="PLAIN_AXES", location=location)
    obj = bpy.context.object
    obj.name = name
    obj.empty_display_size = 0.12
    return obj


def armature(
    name: str,
    bones: list[
        tuple[
            str,
            tuple[float, float, float],
            tuple[float, float, float],
            str | None,
        ]
    ],
) -> bpy.types.Object:
    bpy.ops.object.armature_add(enter_editmode=True, location=(0.0, 0.0, 0.0))
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_data"
    obj.data.display_type = "STICK"

    edit_bones = obj.data.edit_bones
    for bone in list(edit_bones):
        edit_bones.remove(bone)

    created = {}
    for bone_name, head, tail, parent_name in bones:
        bone = edit_bones.new(bone_name)
        bone.head = head
        bone.tail = tail
        if parent_name:
            bone.parent = created[parent_name]
            bone.use_connect = False
        created[bone_name] = bone

    bpy.ops.object.mode_set(mode="OBJECT")
    obj.show_in_front = True
    return obj


def write_metadata(
    asset_id: str,
    category: str,
    sockets: list[str],
    collision: str,
    lods: list[str],
) -> None:
    metadata = {
        "id": asset_id,
        "source": f"assets/source/blender/{category}/{asset_id}.blend",
        "export": f"assets/export/gltf/{category}/{asset_id}.glb",
        "category": category.rstrip("s"),
        "scale_meters": True,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "sockets": sockets,
        "collision": collision,
        "lods": lods,
        "license": "original_project_asset",
        "generated_by": "tools/blender/make_dev_primitives.py",
    }
    export_dir = EXPORT_ROOT / category
    ensure_dirs(export_dir)
    (export_dir / f"{asset_id}.metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")


def save_and_export(asset_id: str, category: str, sockets: list[str], collision: str, lods: list[str]) -> None:
    source_dir = SOURCE_ROOT / category
    export_dir = EXPORT_ROOT / category
    ensure_dirs(source_dir, export_dir)

    blend_path = source_dir / f"{asset_id}.blend"
    export_path = export_dir / f"{asset_id}.glb"
    if hasattr(bpy.context.preferences.filepaths, "save_version"):
        bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    bpy.ops.export_scene.gltf(filepath=str(export_path), export_format="GLB")
    write_metadata(asset_id, category, sockets, collision, lods)
    print(f"Wrote {blend_path.relative_to(REPO_ROOT)}")
    print(f"Wrote {export_path.relative_to(REPO_ROOT)}")


def build_target_dummy() -> None:
    clear_scene()
    body = material("mat_target_body_neutral", (0.35, 0.38, 0.42, 1.0))
    head = material("mat_target_head_zone", (0.95, 0.22, 0.18, 1.0))
    torso = material("mat_target_torso_zone", (0.20, 0.55, 0.95, 1.0))
    base = material("mat_target_base_dark", (0.08, 0.09, 0.10, 1.0))
    collision = material("mat_collision_proxy", (0.0, 1.0, 0.35, 0.28))

    cylinder("prop_target_dummy_01_body", (0.0, 0.0, 0.92), 0.28, 1.35, body)
    sphere("prop_target_dummy_01_head", (0.0, 0.0, 1.72), (0.22, 0.22, 0.22), head)
    cube("prop_target_dummy_01_torso_zone", (0.0, -0.285, 1.05), (0.22, 0.035, 0.32), torso)
    cylinder("prop_target_dummy_01_base", (0.0, 0.0, 0.05), 0.42, 0.10, base, vertices=40)
    cylinder("col_prop_target_dummy_01", (0.0, 0.0, 0.92), 0.34, 1.80, collision, vertices=20)

    empty("socket_hit_center", (0.0, -0.34, 1.15))
    empty("socket_head_center", (0.0, -0.22, 1.72))
    empty("socket_vfx", (0.0, -0.38, 1.25))

    save_and_export(
        "prop_target_dummy_01",
        "props",
        ["socket_hit_center", "socket_head_center", "socket_vfx"],
        "col_prop_target_dummy_01",
        [],
    )


def build_player_capsule_proxy() -> None:
    clear_scene()
    suit = material("mat_chr_proxy_suit_neutral", (0.22, 0.26, 0.30, 1.0))
    visor = material("mat_chr_proxy_visor_accent", (0.06, 0.58, 0.84, 1.0))
    guide = material("mat_chr_hitbox_guide", (1.0, 0.75, 0.08, 0.28))
    collision = material("mat_collision_proxy", (0.0, 1.0, 0.35, 0.22))

    cylinder("chr_player_capsule_proxy_01_lod0_body", (0.0, 0.0, 0.90), 0.34, 1.18, suit, vertices=24)
    sphere("chr_player_capsule_proxy_01_lod0_lower_cap", (0.0, 0.0, 0.31), (0.34, 0.34, 0.31), suit)
    sphere("chr_player_capsule_proxy_01_lod0_upper_cap", (0.0, 0.0, 1.49), (0.34, 0.34, 0.31), suit)
    cube("chr_player_capsule_proxy_01_lod0_forward_stripe", (0.0, -0.345, 1.25), (0.09, 0.02, 0.55), visor)
    cube("chr_player_capsule_proxy_01_lod0_feet_axis", (0.0, -0.28, 0.08), (0.38, 0.08, 0.05), visor)

    sphere("hitbox_chr_player_capsule_proxy_01_head", (0.0, 0.0, 1.55), (0.26, 0.26, 0.24), guide)
    cube("hitbox_chr_player_capsule_proxy_01_torso", (0.0, 0.0, 1.03), (0.58, 0.42, 0.62), guide)
    cube("hitbox_chr_player_capsule_proxy_01_pelvis", (0.0, 0.0, 0.58), (0.48, 0.38, 0.28), guide)
    cylinder("col_chr_player_capsule_proxy_01", (0.0, 0.0, 0.90), 0.38, 1.80, collision, vertices=20)

    empty("socket_root", (0.0, 0.0, 0.0))
    empty("socket_camera", (0.0, -0.10, 1.62))
    empty("socket_weapon_root", (0.0, -0.38, 1.28))
    empty("socket_hit_head", (0.0, -0.24, 1.55))
    empty("socket_hit_torso", (0.0, -0.34, 1.05))

    save_and_export(
        "chr_player_capsule_proxy_01",
        "characters",
        ["socket_root", "socket_camera", "socket_weapon_root", "socket_hit_head", "socket_hit_torso"],
        "col_chr_player_capsule_proxy_01",
        ["chr_player_capsule_proxy_01_lod0"],
    )


def build_first_person_arms() -> None:
    clear_scene()
    sleeve = material("mat_chr_dev_sleeve_neutral", (0.18, 0.21, 0.24, 1.0))
    glove = material("mat_chr_dev_glove_dark", (0.045, 0.048, 0.052, 1.0))
    skin = material("mat_chr_dev_skin_placeholder", (0.68, 0.50, 0.38, 1.0))
    accent = material("mat_chr_dev_arm_socket_accent", (0.05, 0.62, 0.86, 1.0))

    cylinder_between("chr_dev_arms_a_lod0_upper_arm_r", (0.22, 0.08, -0.22), (0.20, 0.46, -0.33), 0.075, sleeve)
    cylinder_between("chr_dev_arms_a_lod0_forearm_r", (0.20, 0.46, -0.33), (0.13, 0.75, -0.41), 0.070, sleeve)
    sphere("chr_dev_arms_a_lod0_elbow_r", (0.20, 0.46, -0.33), (0.08, 0.08, 0.08), sleeve)
    cube("chr_dev_arms_a_lod0_hand_r", (0.13, 0.78, -0.42), (0.12, 0.09, 0.08), glove)

    cylinder_between("chr_dev_arms_a_lod0_upper_arm_l", (-0.22, 0.08, -0.23), (-0.24, 0.44, -0.34), 0.075, sleeve)
    cylinder_between("chr_dev_arms_a_lod0_forearm_l", (-0.24, 0.44, -0.34), (-0.12, 0.66, -0.41), 0.070, sleeve)
    sphere("chr_dev_arms_a_lod0_elbow_l", (-0.24, 0.44, -0.34), (0.08, 0.08, 0.08), sleeve)
    cube("chr_dev_arms_a_lod0_hand_l", (-0.12, 0.66, -0.42), (0.12, 0.09, 0.08), glove)

    cube("chr_dev_arms_a_lod0_wrist_wrap_r", (0.15, 0.70, -0.40), (0.13, 0.04, 0.09), skin)
    cube("chr_dev_arms_a_lod0_wrist_wrap_l", (-0.15, 0.61, -0.40), (0.13, 0.04, 0.09), skin)
    cube("chr_dev_arms_a_lod0_weapon_alignment_bar", (0.0, 0.62, -0.39), (0.30, 0.02, 0.015), accent)

    armature(
        "rig_chr_dev_arms_a",
        [
            ("root", (0.0, 0.0, 0.0), (0.0, 0.08, -0.05), None),
            ("upper_arm.R", (0.22, 0.08, -0.22), (0.20, 0.46, -0.33), "root"),
            ("lower_arm.R", (0.20, 0.46, -0.33), (0.13, 0.75, -0.41), "upper_arm.R"),
            ("hand.R", (0.13, 0.75, -0.41), (0.13, 0.86, -0.42), "lower_arm.R"),
            ("upper_arm.L", (-0.22, 0.08, -0.23), (-0.24, 0.44, -0.34), "root"),
            ("lower_arm.L", (-0.24, 0.44, -0.34), (-0.12, 0.66, -0.41), "upper_arm.L"),
            ("hand.L", (-0.12, 0.66, -0.41), (-0.12, 0.76, -0.42), "lower_arm.L"),
        ],
    )

    empty("socket_camera", (0.0, 0.0, 0.0))
    empty("socket_weapon_root", (0.0, 0.62, -0.38))
    empty("socket_hand_r", (0.13, 0.78, -0.42))
    empty("socket_hand_l", (-0.12, 0.66, -0.42))

    save_and_export(
        "chr_dev_arms_a",
        "characters",
        ["socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l"],
        "none",
        ["chr_dev_arms_a_lod0"],
    )


def build_soldier_proxy() -> None:
    clear_scene()
    suit = material("mat_chr_soldier_suit_neutral", (0.20, 0.23, 0.26, 1.0))
    armor = material("mat_chr_soldier_armor_grey", (0.34, 0.37, 0.39, 1.0))
    accent = material("mat_chr_soldier_team_accent", (0.05, 0.56, 0.86, 1.0))
    hitbox = material("mat_chr_hitbox_guide", (1.0, 0.75, 0.08, 0.26))
    collision = material("mat_collision_proxy", (0.0, 1.0, 0.35, 0.20))

    cube("chr_dev_soldier_a_lod0_pelvis", (0.0, 0.0, 0.72), (0.42, 0.28, 0.22), armor)
    cube("chr_dev_soldier_a_lod0_torso", (0.0, -0.02, 1.14), (0.52, 0.34, 0.55), armor)
    cube("chr_dev_soldier_a_lod0_chest_accent", (0.0, -0.205, 1.20), (0.24, 0.03, 0.32), accent)
    cylinder("chr_dev_soldier_a_lod0_neck", (0.0, 0.0, 1.48), 0.11, 0.14, suit, vertices=16)
    sphere("chr_dev_soldier_a_lod0_head", (0.0, 0.0, 1.66), (0.22, 0.20, 0.22), suit)
    cube("chr_dev_soldier_a_lod0_visor", (0.0, -0.185, 1.67), (0.18, 0.025, 0.055), accent)

    cylinder_between("chr_dev_soldier_a_lod0_upper_arm_r", (0.32, -0.02, 1.32), (0.50, 0.02, 0.98), 0.075, suit)
    cylinder_between("chr_dev_soldier_a_lod0_lower_arm_r", (0.50, 0.02, 0.98), (0.38, -0.08, 0.67), 0.065, suit)
    cube("chr_dev_soldier_a_lod0_hand_r", (0.38, -0.08, 0.60), (0.10, 0.08, 0.08), suit)

    cylinder_between("chr_dev_soldier_a_lod0_upper_arm_l", (-0.32, -0.02, 1.32), (-0.50, 0.02, 0.98), 0.075, suit)
    cylinder_between("chr_dev_soldier_a_lod0_lower_arm_l", (-0.50, 0.02, 0.98), (-0.38, -0.08, 0.67), 0.065, suit)
    cube("chr_dev_soldier_a_lod0_hand_l", (-0.38, -0.08, 0.60), (0.10, 0.08, 0.08), suit)

    cylinder_between("chr_dev_soldier_a_lod0_upper_leg_r", (0.14, 0.0, 0.64), (0.16, 0.02, 0.36), 0.10, suit)
    cylinder_between("chr_dev_soldier_a_lod0_lower_leg_r", (0.16, 0.02, 0.36), (0.14, -0.02, 0.10), 0.085, suit)
    cube("chr_dev_soldier_a_lod0_boot_r", (0.14, -0.09, 0.05), (0.14, 0.20, 0.08), suit)

    cylinder_between("chr_dev_soldier_a_lod0_upper_leg_l", (-0.14, 0.0, 0.64), (-0.16, 0.02, 0.36), 0.10, suit)
    cylinder_between("chr_dev_soldier_a_lod0_lower_leg_l", (-0.16, 0.02, 0.36), (-0.14, -0.02, 0.10), 0.085, suit)
    cube("chr_dev_soldier_a_lod0_boot_l", (-0.14, -0.09, 0.05), (0.14, 0.20, 0.08), suit)

    sphere("hitbox_chr_dev_soldier_a_head", (0.0, 0.0, 1.66), (0.24, 0.22, 0.24), hitbox)
    cube("hitbox_chr_dev_soldier_a_torso", (0.0, 0.0, 1.12), (0.58, 0.42, 0.58), hitbox)
    cube("hitbox_chr_dev_soldier_a_pelvis", (0.0, 0.0, 0.70), (0.48, 0.36, 0.28), hitbox)
    cylinder("col_chr_dev_soldier_a_capsule", (0.0, 0.0, 0.90), 0.40, 1.80, collision, vertices=20)

    armature(
        "rig_chr_dev_soldier_a",
        [
            ("root", (0.0, 0.0, 0.0), (0.0, 0.0, 0.18), None),
            ("pelvis", (0.0, 0.0, 0.62), (0.0, 0.0, 0.84), "root"),
            ("spine", (0.0, 0.0, 0.84), (0.0, 0.0, 1.34), "pelvis"),
            ("neck", (0.0, 0.0, 1.34), (0.0, 0.0, 1.50), "spine"),
            ("head", (0.0, 0.0, 1.50), (0.0, 0.0, 1.78), "neck"),
            ("upper_arm.R", (0.30, 0.0, 1.30), (0.50, 0.02, 0.98), "spine"),
            ("lower_arm.R", (0.50, 0.02, 0.98), (0.38, -0.08, 0.66), "upper_arm.R"),
            ("hand.R", (0.38, -0.08, 0.66), (0.38, -0.12, 0.54), "lower_arm.R"),
            ("upper_arm.L", (-0.30, 0.0, 1.30), (-0.50, 0.02, 0.98), "spine"),
            ("lower_arm.L", (-0.50, 0.02, 0.98), (-0.38, -0.08, 0.66), "upper_arm.L"),
            ("hand.L", (-0.38, -0.08, 0.66), (-0.38, -0.12, 0.54), "lower_arm.L"),
            ("thigh.R", (0.14, 0.0, 0.62), (0.16, 0.02, 0.36), "pelvis"),
            ("calf.R", (0.16, 0.02, 0.36), (0.14, -0.02, 0.10), "thigh.R"),
            ("foot.R", (0.14, -0.02, 0.10), (0.14, -0.20, 0.04), "calf.R"),
            ("thigh.L", (-0.14, 0.0, 0.62), (-0.16, 0.02, 0.36), "pelvis"),
            ("calf.L", (-0.16, 0.02, 0.36), (-0.14, -0.02, 0.10), "thigh.L"),
            ("foot.L", (-0.14, -0.02, 0.10), (-0.14, -0.20, 0.04), "calf.L"),
        ],
    )

    empty("socket_root", (0.0, 0.0, 0.0))
    empty("socket_camera", (0.0, -0.16, 1.62))
    empty("socket_weapon_root", (0.0, -0.36, 1.24))
    empty("socket_hand_r", (0.38, -0.08, 0.60))
    empty("socket_hand_l", (-0.38, -0.08, 0.60))
    empty("socket_head", (0.0, -0.20, 1.66))
    empty("socket_vfx", (0.0, -0.26, 1.20))

    save_and_export(
        "chr_dev_soldier_a",
        "characters",
        ["socket_root", "socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l", "socket_head", "socket_vfx"],
        "col_chr_dev_soldier_a_capsule",
        ["chr_dev_soldier_a_lod0"],
    )


def build_ar_blockout() -> None:
    clear_scene()
    polymer = material("mat_wpn_ar_polymer_dark", (0.08, 0.085, 0.09, 1.0))
    metal = material("mat_wpn_ar_brushed_metal", (0.28, 0.30, 0.32, 1.0))
    accent = material("mat_wpn_ar_neutral_accent", (0.12, 0.36, 0.75, 1.0))

    cube("wpn_ar_01_lod0_receiver", (0.0, 0.0, 0.0), (0.26, 0.12, 0.09), metal)
    cylinder(
        "wpn_ar_01_lod0_barrel",
        (0.0, 0.48, 0.02),
        0.035,
        0.54,
        metal,
        vertices=24,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_ar_01_lod0_stock", (0.0, -0.31, 0.02), (0.16, 0.22, 0.07), polymer)
    cube("wpn_ar_01_lod0_magazine", (0.0, -0.02, -0.18), (0.09, 0.055, 0.20), polymer)
    cube("wpn_ar_01_lod0_grip", (0.0, -0.15, -0.17), (0.065, 0.055, 0.16), polymer)
    cube("wpn_ar_01_lod0_handguard", (0.0, 0.25, 0.0), (0.20, 0.22, 0.07), polymer)
    cube("wpn_ar_01_lod0_sight", (0.0, 0.03, 0.14), (0.09, 0.12, 0.045), accent)

    cube("wpn_ar_01_lod1_body", (0.0, 0.05, 0.0), (0.22, 0.62, 0.08), polymer)

    empty("socket_muzzle", (0.0, 0.78, 0.02))
    empty("socket_grip_r", (0.0, -0.15, -0.10))
    empty("socket_grip_l", (0.0, 0.25, -0.03))
    empty("socket_eject", (0.14, 0.04, 0.07))
    empty("socket_vfx", (0.0, 0.78, 0.02))

    save_and_export(
        "wpn_ar_01",
        "weapons",
        ["socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"],
        "none",
        ["wpn_ar_01_lod0", "wpn_ar_01_lod1"],
    )


def build_smg_blockout() -> None:
    clear_scene()
    polymer = material("mat_wpn_smg_polymer_dark", (0.07, 0.075, 0.08, 1.0))
    metal = material("mat_wpn_smg_brushed_metal", (0.30, 0.31, 0.32, 1.0))
    accent = material("mat_wpn_smg_neutral_accent", (0.02, 0.60, 0.52, 1.0))

    cube("wpn_smg_01_lod0_receiver", (0.0, 0.02, 0.0), (0.22, 0.22, 0.085), metal)
    cylinder(
        "wpn_smg_01_lod0_short_barrel",
        (0.0, 0.34, 0.01),
        0.032,
        0.30,
        metal,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_smg_01_lod0_stock_stub", (0.0, -0.22, 0.01), (0.14, 0.12, 0.06), polymer)
    cube("wpn_smg_01_lod0_box_magazine", (0.0, -0.02, -0.16), (0.08, 0.045, 0.18), polymer)
    cube("wpn_smg_01_lod0_grip", (0.0, -0.15, -0.14), (0.060, 0.050, 0.14), polymer, rotation=(math.radians(-8.0), 0.0, 0.0))
    cube("wpn_smg_01_lod0_top_rail", (0.0, 0.02, 0.12), (0.10, 0.22, 0.025), accent)
    cube("wpn_smg_01_lod0_front_stop", (0.0, 0.20, -0.08), (0.08, 0.055, 0.06), accent)

    cube("wpn_smg_01_lod1_body", (0.0, 0.08, 0.0), (0.19, 0.44, 0.075), polymer)

    empty("socket_muzzle", (0.0, 0.52, 0.01))
    empty("socket_grip_r", (0.0, -0.15, -0.09))
    empty("socket_grip_l", (0.0, 0.20, -0.08))
    empty("socket_eject", (0.12, 0.02, 0.06))
    empty("socket_vfx", (0.0, 0.52, 0.01))

    save_and_export(
        "wpn_smg_01",
        "weapons",
        ["socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"],
        "none",
        ["wpn_smg_01_lod0", "wpn_smg_01_lod1"],
    )


def build_sidearm_blockout() -> None:
    clear_scene()
    polymer = material("mat_wpn_sidearm_polymer_dark", (0.055, 0.058, 0.062, 1.0))
    metal = material("mat_wpn_sidearm_slide_metal", (0.32, 0.34, 0.36, 1.0))
    accent = material("mat_wpn_sidearm_neutral_accent", (0.86, 0.42, 0.08, 1.0))

    cube("wpn_sidearm_01_lod0_slide", (0.0, 0.08, 0.06), (0.13, 0.30, 0.065), metal)
    cube("wpn_sidearm_01_lod0_frame", (0.0, 0.03, 0.005), (0.12, 0.22, 0.055), polymer)
    cylinder(
        "wpn_sidearm_01_lod0_barrel",
        (0.0, 0.27, 0.065),
        0.026,
        0.18,
        metal,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_sidearm_01_lod0_grip", (0.0, -0.08, -0.13), (0.070, 0.070, 0.20), polymer, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_sidearm_01_lod0_trigger_guard", (0.0, 0.03, -0.065), (0.095, 0.070, 0.035), polymer)
    cube("wpn_sidearm_01_lod0_front_sight", (0.0, 0.22, 0.105), (0.040, 0.035, 0.025), accent)
    cube("wpn_sidearm_01_lod0_rear_sight", (0.0, -0.04, 0.105), (0.050, 0.035, 0.025), accent)

    cube("wpn_sidearm_01_lod1_body", (0.0, 0.05, 0.02), (0.11, 0.32, 0.07), polymer)

    empty("socket_muzzle", (0.0, 0.38, 0.065))
    empty("socket_grip_r", (0.0, -0.08, -0.10))
    empty("socket_grip_l", (0.0, 0.03, -0.06))
    empty("socket_eject", (0.08, 0.06, 0.10))
    empty("socket_vfx", (0.0, 0.38, 0.065))

    save_and_export(
        "wpn_sidearm_01",
        "weapons",
        ["socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"],
        "none",
        ["wpn_sidearm_01_lod0", "wpn_sidearm_01_lod1"],
    )


def build_arena_kit() -> None:
    clear_scene()
    floor = material("mat_env_grid_floor", (0.18, 0.19, 0.20, 1.0))
    line = material("mat_env_grid_line", (0.55, 0.58, 0.62, 1.0))
    wall = material("mat_env_wall_neutral", (0.24, 0.26, 0.28, 1.0))
    accent = material("mat_env_movement_accent", (0.05, 0.65, 0.90, 1.0))
    objective = material("mat_env_objective_accent", (0.95, 0.50, 0.12, 1.0))
    distance = material("mat_env_distance_marker", (0.70, 0.78, 0.86, 1.0))
    collision = material("mat_collision_proxy", (0.0, 1.0, 0.35, 0.16))

    cube("env_test_floor_grid_01", (0.0, 0.0, -0.03), (20.0, 20.0, 0.03), floor)
    cube("col_env_test_floor_grid_01", (0.0, 0.0, -0.025), (20.0, 20.0, 0.02), collision)
    for index in range(-10, 11):
        width = 0.025 if index % 5 else 0.055
        cube(f"env_test_grid_x_{index:+03d}", (index, 0.0, 0.002), (width, 20.0, 0.01), line)
        cube(f"env_test_grid_y_{index:+03d}", (0.0, index, 0.003), (20.0, width, 0.01), line)

    cube("env_test_wall_lane_left_01", (-8.0, 0.0, 1.0), (0.16, 8.0, 1.0), wall)
    cube("env_test_wall_lane_right_01", (8.0, 0.0, 1.0), (0.16, 8.0, 1.0), wall)
    cube("env_test_wall_back_01", (0.0, 8.0, 1.0), (8.0, 0.16, 1.0), wall)
    cube("col_env_test_wall_lane_left_01", (-8.0, 0.0, 1.0), (0.18, 8.0, 1.0), collision)
    cube("col_env_test_wall_lane_right_01", (8.0, 0.0, 1.0), (0.18, 8.0, 1.0), collision)
    cube("col_env_test_wall_back_01", (0.0, 8.0, 1.0), (8.0, 0.18, 1.0), collision)

    cube("env_test_doorway_frame_left_01", (-1.1, 7.0, 1.0), (0.18, 0.16, 1.0), wall)
    cube("env_test_doorway_frame_right_01", (1.1, 7.0, 1.0), (0.18, 0.16, 1.0), wall)
    cube("env_test_doorway_frame_header_01", (0.0, 7.0, 1.95), (1.25, 0.16, 0.15), wall)

    cube("env_test_cover_low_01", (-3.0, 2.4, 0.45), (0.95, 0.38, 0.45), wall)
    cube("env_test_cover_high_01", (3.0, 2.4, 0.85), (0.95, 0.38, 0.85), wall)
    cube("env_test_cover_crate_stack_01", (0.0, 2.7, 0.55), (0.55, 0.55, 0.55), wall)
    cube("env_test_cover_crate_stack_02", (0.0, 2.7, 1.10), (0.50, 0.50, 0.50), wall)
    cube("col_env_test_cover_low_01", (-3.0, 2.4, 0.45), (0.95, 0.38, 0.45), collision)
    cube("col_env_test_cover_high_01", (3.0, 2.4, 0.85), (0.95, 0.38, 0.85), collision)
    cube("col_env_test_cover_crate_stack_01", (0.0, 2.7, 0.80), (0.60, 0.60, 0.85), collision)

    cube("env_test_mantle_ledge_080", (-4.5, -2.5, 0.40), (0.8, 0.45, 0.40), accent)
    cube("env_test_mantle_ledge_120", (-2.5, -2.5, 0.60), (0.8, 0.45, 0.60), accent)
    cube("env_test_mantle_ledge_160", (-0.5, -2.5, 0.80), (0.8, 0.45, 0.80), accent)
    cube("env_test_wallrun_panel_01", (6.5, -2.0, 1.1), (0.10, 2.6, 0.55), accent)
    cube("env_test_wallrun_panel_02", (-6.5, -2.0, 1.1), (0.10, 2.6, 0.55), accent)

    ramp = cube("env_test_slide_ramp_01", (1.8, -4.0, 0.34), (1.2, 1.4, 0.08), accent)
    ramp.rotation_euler[0] = math.radians(18.0)
    apply_object_transform(ramp)
    col_ramp = cube("col_env_test_slide_ramp_01", (1.8, -4.0, 0.34), (1.2, 1.4, 0.08), collision)
    col_ramp.rotation_euler[0] = math.radians(18.0)
    apply_object_transform(col_ramp)

    launch_ramp = cube("env_test_launch_ramp_01", (4.8, -4.0, 0.46), (1.0, 1.2, 0.08), accent)
    launch_ramp.rotation_euler[0] = math.radians(24.0)
    apply_object_transform(launch_ramp)

    for step in range(4):
        cube(
            f"env_test_stair_step_{step + 1:02d}",
            (-4.8, -4.4 + step * 0.45, 0.10 + step * 0.18),
            (1.0, 0.22, 0.10 + step * 0.18),
            wall,
        )

    cube("env_test_spawn_pad_alpha_01", (-5.8, -7.0, 0.035), (0.85, 0.85, 0.035), objective)
    cube("env_test_spawn_marker_alpha_01", (-5.8, -6.78, 0.10), (0.18, 0.42, 0.035), objective)
    cube("env_test_spawn_pad_bravo_01", (5.8, -7.0, 0.035), (0.85, 0.85, 0.035), objective)
    cube("env_test_spawn_marker_bravo_01", (5.8, -6.78, 0.10), (0.18, 0.42, 0.035), objective)
    cube("env_test_control_marker_01", (0.0, -6.9, 0.08), (0.62, 0.62, 0.08), objective)

    for marker, y_position in [(5, -5.0), (10, 0.0), (20, 5.0), (40, 9.0)]:
        cube(f"env_test_distance_marker_{marker:02d}m", (-7.2, y_position, 0.55), (0.16, 0.22, 0.55), distance)

    empty("socket_spawn_alpha", (-5.8, -7.0, 0.08))
    empty("socket_spawn_bravo", (5.8, -7.0, 0.08))
    empty("socket_control_point", (0.0, -6.9, 0.18))

    save_and_export(
        "env_test_arena_kit_01",
        "environments",
        ["socket_spawn_alpha", "socket_spawn_bravo", "socket_control_point"],
        "per_piece_col_prefix",
        [],
    )


BUILD_GROUPS = {
    "target": [build_target_dummy],
    "prop": [build_target_dummy],
    "props": [build_target_dummy],
    "player": [build_player_capsule_proxy],
    "arms": [build_first_person_arms],
    "soldier": [build_soldier_proxy],
    "character": [build_player_capsule_proxy, build_first_person_arms, build_soldier_proxy],
    "characters": [build_player_capsule_proxy, build_first_person_arms, build_soldier_proxy],
    "ar": [build_ar_blockout],
    "smg": [build_smg_blockout],
    "sidearm": [build_sidearm_blockout],
    "weapon": [build_ar_blockout, build_smg_blockout, build_sidearm_blockout],
    "weapons": [build_ar_blockout, build_smg_blockout, build_sidearm_blockout],
    "arena": [build_arena_kit],
    "environment": [build_arena_kit],
    "environments": [build_arena_kit],
}
BUILD_GROUPS["all"] = [
    build_target_dummy,
    build_player_capsule_proxy,
    build_first_person_arms,
    build_soldier_proxy,
    build_ar_blockout,
    build_smg_blockout,
    build_sidearm_blockout,
    build_arena_kit,
]


def parse_only_arg() -> str:
    if "--" not in sys.argv:
        return "all"
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--only" not in args:
        return "all"
    index = args.index("--only")
    if index + 1 >= len(args):
        raise SystemExit(f"--only requires one of: {', '.join(sorted(BUILD_GROUPS))}")
    return args[index + 1]


def main() -> None:
    only = parse_only_arg()
    if only not in BUILD_GROUPS:
        raise SystemExit(f"Unknown --only value '{only}'. Expected one of: {', '.join(sorted(BUILD_GROUPS))}")

    for builder in BUILD_GROUPS[only]:
        builder()


if __name__ == "__main__":
    main()
