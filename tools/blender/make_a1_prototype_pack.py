"""Generate the Nemisis A1 prototype visual asset pack in Blender.

Run from the Nemisis repository root:
    blender --background --python tools/blender/make_a1_prototype_pack.py

The script creates original procedural blockout/dev-art only. Weapon silhouettes are
generic game-asset references and do not include real-world functional details,
protected marks, copied geometry, or external assets.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable

try:
    import bpy
    from mathutils import Vector
except ModuleNotFoundError as exc:
    raise SystemExit("Run this script with Blender, not plain Python.") from exc


REPO_ROOT = Path(__file__).resolve().parents[2]
SOURCE_ROOT = REPO_ROOT / "assets" / "source" / "blender"
EXPORT_ROOT = REPO_ROOT / "assets" / "export" / "gltf"
PACK_MANIFEST = EXPORT_ROOT / "a1_prototype_pack_manifest.json"


@dataclass(frozen=True)
class AssetSpec:
    asset_id: str
    category_dir: str
    category: str
    tags: tuple[str, ...]
    dimensions_m: tuple[float, float, float]
    origin_note: str
    description: str
    sockets: tuple[str, ...]
    collision: str
    lods: tuple[str, ...]
    build: Callable[[], None]


def ensure_dirs(*paths: Path) -> None:
    for path in paths:
        path.mkdir(parents=True, exist_ok=True)


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Generate Nemisis A1 prototype blockout assets.")
    parser.add_argument(
        "--only",
        default="all",
        choices=(
            "all",
            "weapon",
            "weapons",
            "compact_rifle",
            "modern_rifle",
            "sidearm",
            "character",
            "characters",
            "operator",
            "arms",
        ),
        help="Generate one group or one asset.",
    )
    return parser.parse_args(argv)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()

    for data_block in (
        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.armatures,
    ):
        for item in list(data_block):
            if item.users == 0:
                data_block.remove(item)


def configure_scene() -> None:
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0
    try:
        bpy.context.scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        pass


def material(
    name: str,
    color: tuple[float, float, float, float],
    metallic: float = 0.0,
    roughness: float = 0.78,
) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = color
    if color[3] < 1.0:
        mat.blend_method = "BLEND"
        if hasattr(mat, "show_transparent_back"):
            mat.show_transparent_back = True

    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        if "Base Color" in bsdf.inputs:
            bsdf.inputs["Base Color"].default_value = color
        if "Alpha" in bsdf.inputs:
            bsdf.inputs["Alpha"].default_value = color[3]
        if "Roughness" in bsdf.inputs:
            bsdf.inputs["Roughness"].default_value = roughness
        if "Metallic" in bsdf.inputs:
            bsdf.inputs["Metallic"].default_value = metallic
    return mat


def root_empty(name: str) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    obj.empty_display_type = "PLAIN_AXES"
    obj.empty_display_size = 0.22
    bpy.context.collection.objects.link(obj)
    return obj


def parent_to(obj: bpy.types.Object, parent: bpy.types.Object) -> bpy.types.Object:
    obj.parent = parent
    return obj


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
    dimensions: tuple[float, float, float],
    mat: bpy.types.Material,
    parent: bpy.types.Object,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_mesh"
    obj.scale = dimensions
    assign_material(obj, mat)
    apply_object_transform(obj)
    return parent_to(obj, parent)


def cylinder(
    name: str,
    location: tuple[float, float, float],
    radius: float,
    depth: float,
    mat: bpy.types.Material,
    parent: bpy.types.Object,
    vertices: int = 20,
    rotation: tuple[float, float, float] = (0.0, 0.0, 0.0),
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=vertices,
        radius=radius,
        depth=depth,
        end_fill_type="NGON",
        location=location,
        rotation=rotation,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_mesh"
    assign_material(obj, mat)
    apply_object_transform(obj)
    return parent_to(obj, parent)


def cylinder_between(
    name: str,
    start: tuple[float, float, float],
    end: tuple[float, float, float],
    radius: float,
    mat: bpy.types.Material,
    parent: bpy.types.Object,
    vertices: int = 14,
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
        end_fill_type="NGON",
        location=(start_v + end_v) * 0.5,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_mesh"
    obj.rotation_euler = direction.to_track_quat("Z", "Y").to_euler()
    assign_material(obj, mat)
    apply_object_transform(obj)
    return parent_to(obj, parent)


def sphere(
    name: str,
    location: tuple[float, float, float],
    radius: float,
    mat: bpy.types.Material,
    parent: bpy.types.Object,
    scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
    segments: int = 16,
    rings: int = 8,
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=segments,
        ring_count=rings,
        radius=radius,
        location=location,
    )
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_mesh"
    obj.scale = scale
    assign_material(obj, mat)
    apply_object_transform(obj)
    return parent_to(obj, parent)


def empty_socket(
    name: str,
    location: tuple[float, float, float],
    parent: bpy.types.Object,
    display_size: float = 0.08,
) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    obj.empty_display_type = "SPHERE"
    obj.empty_display_size = display_size
    obj.location = location
    bpy.context.collection.objects.link(obj)
    return parent_to(obj, parent)


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
    parent: bpy.types.Object,
) -> bpy.types.Object:
    bpy.ops.object.armature_add(enter_editmode=True, location=(0.0, 0.0, 0.0))
    obj = bpy.context.object
    obj.name = name
    obj.data.name = f"{name}_data"
    obj.data.display_type = "STICK"

    edit_bones = obj.data.edit_bones
    for bone in list(edit_bones):
        edit_bones.remove(bone)

    created: dict[str, bpy.types.EditBone] = {}
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
    return parent_to(obj, parent)


def mesh_stats() -> dict[str, int]:
    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    vertices = sum(len(obj.data.vertices) for obj in mesh_objects)
    polygons = sum(len(obj.data.polygons) for obj in mesh_objects)
    triangles = 0
    for obj in mesh_objects:
        for poly in obj.data.polygons:
            triangles += max(1, len(poly.vertices) - 2)
    return {
        "mesh_objects": len(mesh_objects),
        "vertices": vertices,
        "polygons": polygons,
        "triangles_estimate": triangles,
    }


def scene_dimensions() -> tuple[float, float, float]:
    mesh_objects = [obj for obj in bpy.context.scene.objects if obj.type == "MESH"]
    if not mesh_objects:
        return (0.0, 0.0, 0.0)

    depsgraph = bpy.context.evaluated_depsgraph_get()
    minimum = Vector((math.inf, math.inf, math.inf))
    maximum = Vector((-math.inf, -math.inf, -math.inf))

    for obj in mesh_objects:
        evaluated = obj.evaluated_get(depsgraph)
        for corner in evaluated.bound_box:
            world = evaluated.matrix_world @ Vector(corner)
            minimum.x = min(minimum.x, world.x)
            minimum.y = min(minimum.y, world.y)
            minimum.z = min(minimum.z, world.z)
            maximum.x = max(maximum.x, world.x)
            maximum.y = max(maximum.y, world.y)
            maximum.z = max(maximum.z, world.z)

    dimensions = maximum - minimum
    return (round(dimensions.x, 4), round(dimensions.y, 4), round(dimensions.z, 4))


def rel(path: Path) -> str:
    try:
        return path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def metadata_for(
    spec: AssetSpec,
    source_path: Path,
    export_path: Path,
    stats: dict[str, int],
    dimensions_m: tuple[float, float, float],
) -> dict[str, object]:
    return {
        "id": spec.asset_id,
        "source": rel(source_path),
        "export": rel(export_path),
        "category": spec.category,
        "scale_meters": True,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "blender_forward_axis": "+Y",
        "dimensions_m": dimensions_m,
        "target_dimensions_m": spec.dimensions_m,
        "origin": spec.origin_note,
        "sockets": list(spec.sockets),
        "collision": spec.collision,
        "lods": list(spec.lods),
        "license": "original_project_asset",
        "external_assets": False,
        "visual_reference_note": "Original procedural blockout inspired by broad sci-fi shooter silhouettes; no protected marks, copied geometry, or real functional implementation details.",
        "generated_by": "tools/blender/make_a1_prototype_pack.py",
        "generated_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "stats": stats,
    }


def save_and_export(spec: AssetSpec) -> dict[str, object]:
    source_dir = SOURCE_ROOT / spec.category_dir
    export_dir = EXPORT_ROOT / spec.category_dir
    ensure_dirs(source_dir, export_dir)

    source_path = source_dir / f"{spec.asset_id}.blend"
    export_path = export_dir / f"{spec.asset_id}.glb"
    metadata_path = export_dir / f"{spec.asset_id}.metadata.json"

    if hasattr(bpy.context.preferences.filepaths, "save_version"):
        bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(source_path))
    stats = mesh_stats()
    dimensions_m = scene_dimensions()
    bpy.ops.export_scene.gltf(filepath=str(export_path), export_format="GLB")

    metadata = metadata_for(spec, source_path, export_path, stats, dimensions_m)
    metadata["bytes"] = export_path.stat().st_size
    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    print(f"Wrote {rel(source_path)}")
    print(f"Wrote {rel(export_path)}")
    print(f"Wrote {rel(metadata_path)}")

    return {
        "id": spec.asset_id,
        "category": spec.category,
        "source": rel(source_path),
        "export": rel(export_path),
        "metadata": rel(metadata_path),
        "dimensions_m": dimensions_m,
        "target_dimensions_m": spec.dimensions_m,
        "description": spec.description,
        "sockets": list(spec.sockets),
        "collision": spec.collision,
        "lods": list(spec.lods),
        "bytes": export_path.stat().st_size,
        "stats": stats,
    }


def build_compact_rifle() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a1_compact_rifle_01")

    polymer = material("mat_a1_compact_rifle_polymer", (0.055, 0.060, 0.064, 1.0))
    metal = material("mat_a1_compact_rifle_gunmetal", (0.30, 0.32, 0.33, 1.0), metallic=0.1)
    rubber = material("mat_a1_compact_rifle_rubber", (0.025, 0.027, 0.030, 1.0))
    accent = material("mat_a1_compact_rifle_teal_mark", (0.06, 0.58, 0.66, 1.0))
    safe = material("mat_a1_compact_rifle_safety_orange", (0.90, 0.34, 0.08, 1.0))

    cube("wpn_a1_compact_rifle_01_lod0_upper_body", (0.0, 0.09, 0.055), (0.165, 0.36, 0.095), metal, root)
    cube("wpn_a1_compact_rifle_01_lod0_lower_body", (0.0, -0.04, -0.020), (0.150, 0.255, 0.085), polymer, root)
    cube("wpn_a1_compact_rifle_01_lod0_top_rail", (0.0, 0.105, 0.140), (0.090, 0.465, 0.026), rubber, root)
    cube("wpn_a1_compact_rifle_01_lod0_handguard", (0.0, 0.365, 0.030), (0.185, 0.280, 0.090), polymer, root)
    cylinder(
        "wpn_a1_compact_rifle_01_lod0_front_tube",
        (0.0, 0.535, 0.035),
        0.035,
        0.245,
        metal,
        root,
        vertices=18,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cylinder(
        "wpn_a1_compact_rifle_01_lod0_muzzle_mark",
        (0.0, 0.670, 0.035),
        0.044,
        0.055,
        rubber,
        root,
        vertices=18,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a1_compact_rifle_01_lod0_rear_support", (0.0, -0.295, 0.035), (0.135, 0.205, 0.070), rubber, root)
    cube("wpn_a1_compact_rifle_01_lod0_stock_pad", (0.0, -0.425, 0.040), (0.175, 0.035, 0.115), rubber, root)
    cube("wpn_a1_compact_rifle_01_lod0_magazine", (0.0, -0.005, -0.195), (0.082, 0.065, 0.250), polymer, root, rotation=(math.radians(-5.0), 0.0, 0.0))
    cube("wpn_a1_compact_rifle_01_lod0_grip", (0.0, -0.180, -0.170), (0.070, 0.062, 0.190), rubber, root, rotation=(math.radians(-10.0), 0.0, 0.0))
    cube("wpn_a1_compact_rifle_01_lod0_support_stop", (0.0, 0.255, -0.085), (0.090, 0.040, 0.080), accent, root)
    cube("wpn_a1_compact_rifle_01_lod0_rear_sight", (0.0, -0.095, 0.185), (0.070, 0.050, 0.050), accent, root)
    cube("wpn_a1_compact_rifle_01_lod0_front_sight", (0.0, 0.410, 0.178), (0.056, 0.045, 0.052), accent, root)
    cube("wpn_a1_compact_rifle_01_lod0_side_mark", (-0.088, 0.050, 0.030), (0.014, 0.100, 0.035), safe, root)
    cube("wpn_a1_compact_rifle_01_lod1_blockout", (0.0, 0.110, 0.020), (0.130, 0.800, 0.085), polymer, root)

    empty_socket("socket_muzzle", (0.0, 0.705, 0.035), root)
    empty_socket("socket_grip_r", (0.0, -0.180, -0.105), root)
    empty_socket("socket_grip_l", (0.0, 0.255, -0.075), root)
    empty_socket("socket_eject", (0.095, 0.040, 0.090), root)
    empty_socket("socket_vfx", (0.0, 0.705, 0.035), root)


def build_modern_rifle() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a1_modern_rifle_01")

    polymer = material("mat_a1_modern_rifle_polymer", (0.070, 0.075, 0.078, 1.0))
    metal = material("mat_a1_modern_rifle_metal", (0.34, 0.35, 0.35, 1.0), metallic=0.12)
    tan = material("mat_a1_modern_rifle_tan_panel", (0.48, 0.44, 0.34, 1.0))
    blue = material("mat_a1_modern_rifle_blue_accent", (0.05, 0.32, 0.70, 1.0))
    glass = material("mat_a1_modern_rifle_optic_glass", (0.04, 0.12, 0.16, 1.0), roughness=0.35)

    cube("wpn_a1_modern_rifle_01_lod0_receiver", (0.0, 0.030, 0.040), (0.170, 0.360, 0.105), metal, root)
    cube("wpn_a1_modern_rifle_01_lod0_upper_rail", (0.0, 0.110, 0.150), (0.095, 0.620, 0.028), polymer, root)
    cube("wpn_a1_modern_rifle_01_lod0_handguard", (0.0, 0.390, 0.030), (0.185, 0.430, 0.085), polymer, root)
    cylinder(
        "wpn_a1_modern_rifle_01_lod0_barrel_visual",
        (0.0, 0.680, 0.040),
        0.030,
        0.365,
        metal,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cylinder(
        "wpn_a1_modern_rifle_01_lod0_muzzle_tip",
        (0.0, 0.880, 0.040),
        0.037,
        0.060,
        polymer,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a1_modern_rifle_01_lod0_rear_stock", (0.0, -0.335, 0.045), (0.150, 0.275, 0.085), polymer, root)
    cube("wpn_a1_modern_rifle_01_lod0_stock_pad", (0.0, -0.520, 0.050), (0.185, 0.045, 0.130), polymer, root)
    cube("wpn_a1_modern_rifle_01_lod0_magazine", (0.0, -0.020, -0.205), (0.090, 0.070, 0.285), tan, root, rotation=(math.radians(-5.0), 0.0, 0.0))
    cube("wpn_a1_modern_rifle_01_lod0_grip", (0.0, -0.205, -0.170), (0.072, 0.060, 0.205), polymer, root, rotation=(math.radians(-9.0), 0.0, 0.0))
    cube("wpn_a1_modern_rifle_01_lod0_support_grip", (0.0, 0.285, -0.130), (0.075, 0.055, 0.160), polymer, root, rotation=(math.radians(5.0), 0.0, 0.0))
    cube("wpn_a1_modern_rifle_01_lod0_optic_base", (0.0, 0.035, 0.205), (0.120, 0.120, 0.036), polymer, root)
    cube("wpn_a1_modern_rifle_01_lod0_optic_body", (0.0, 0.035, 0.260), (0.105, 0.090, 0.070), glass, root)
    cube("wpn_a1_modern_rifle_01_lod0_forward_mark", (0.0, 0.555, 0.110), (0.130, 0.055, 0.025), blue, root)
    cube("wpn_a1_modern_rifle_01_lod0_mag_mark", (0.0, -0.055, -0.075), (0.094, 0.018, 0.045), blue, root)
    cube("wpn_a1_modern_rifle_01_lod1_blockout", (0.0, 0.135, 0.020), (0.132, 1.050, 0.090), polymer, root)

    empty_socket("socket_muzzle", (0.0, 0.925, 0.040), root)
    empty_socket("socket_grip_r", (0.0, -0.205, -0.110), root)
    empty_socket("socket_grip_l", (0.0, 0.285, -0.080), root)
    empty_socket("socket_eject", (0.100, 0.045, 0.095), root)
    empty_socket("socket_vfx", (0.0, 0.925, 0.040), root)


def build_compact_sidearm() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a1_compact_sidearm_01")

    polymer = material("mat_a1_sidearm_polymer", (0.048, 0.050, 0.054, 1.0))
    slide = material("mat_a1_sidearm_slide", (0.29, 0.305, 0.32, 1.0), metallic=0.08)
    dark = material("mat_a1_sidearm_dark_detail", (0.018, 0.020, 0.022, 1.0))
    accent = material("mat_a1_sidearm_orange_mark", (0.92, 0.40, 0.08, 1.0))

    cube("wpn_a1_compact_sidearm_01_lod0_slide", (0.0, 0.060, 0.060), (0.125, 0.300, 0.060), slide, root)
    cube("wpn_a1_compact_sidearm_01_lod0_frame", (0.0, 0.010, 0.006), (0.115, 0.225, 0.054), polymer, root)
    cylinder(
        "wpn_a1_compact_sidearm_01_lod0_front_opening",
        (0.0, 0.235, 0.063),
        0.022,
        0.052,
        dark,
        root,
        vertices=18,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a1_compact_sidearm_01_lod0_grip", (0.0, -0.100, -0.125), (0.075, 0.075, 0.215), polymer, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a1_compact_sidearm_01_lod0_grip_backstrap", (0.0, -0.146, -0.120), (0.085, 0.030, 0.205), dark, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a1_compact_sidearm_01_lod0_trigger_guard", (0.0, 0.030, -0.063), (0.096, 0.074, 0.034), polymer, root)
    cube("wpn_a1_compact_sidearm_01_lod0_trigger_visual", (0.0, 0.000, -0.064), (0.035, 0.018, 0.062), dark, root, rotation=(math.radians(-8.0), 0.0, 0.0))
    cube("wpn_a1_compact_sidearm_01_lod0_front_sight", (0.0, 0.197, 0.104), (0.034, 0.026, 0.021), accent, root)
    cube("wpn_a1_compact_sidearm_01_lod0_rear_sight", (0.0, -0.070, 0.106), (0.050, 0.032, 0.024), accent, root)
    cube("wpn_a1_compact_sidearm_01_lod0_base_plate", (0.0, -0.165, -0.245), (0.092, 0.082, 0.026), dark, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a1_compact_sidearm_01_lod1_blockout", (0.0, 0.020, 0.010), (0.105, 0.355, 0.075), polymer, root)

    empty_socket("socket_muzzle", (0.0, 0.280, 0.063), root)
    empty_socket("socket_grip_r", (0.0, -0.100, -0.115), root)
    empty_socket("socket_grip_l", (0.0, 0.015, -0.060), root)
    empty_socket("socket_eject", (0.075, 0.060, 0.100), root)
    empty_socket("socket_vfx", (0.0, 0.280, 0.063), root)


def build_stylized_operator() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("chr_a1_stylized_operator_01")

    suit = material("mat_a1_operator_fabric", (0.19, 0.24, 0.27, 1.0))
    armor = material("mat_a1_operator_armor", (0.34, 0.36, 0.36, 1.0))
    dark = material("mat_a1_operator_dark_joint", (0.045, 0.050, 0.055, 1.0))
    visor = material("mat_a1_operator_visor_teal", (0.04, 0.72, 0.78, 1.0), roughness=0.38)
    accent = material("mat_a1_operator_team_accent", (0.06, 0.31, 0.78, 1.0))
    pack = material("mat_a1_operator_mobility_pack", (0.22, 0.25, 0.24, 1.0))
    hitbox = material("mat_a1_operator_hitbox_guide", (1.0, 0.76, 0.08, 0.24))
    collision = material("mat_a1_operator_collision_proxy", (0.0, 1.0, 0.36, 0.18))

    cube("chr_a1_stylized_operator_01_lod0_pelvis_armor", (0.0, 0.0, 0.72), (0.44, 0.25, 0.20), armor, root)
    cube("chr_a1_stylized_operator_01_lod0_torso_suit", (0.0, -0.005, 1.10), (0.50, 0.30, 0.55), suit, root)
    cube("chr_a1_stylized_operator_01_lod0_chest_plate", (0.0, -0.175, 1.18), (0.38, 0.045, 0.38), armor, root)
    cube("chr_a1_stylized_operator_01_lod0_chest_accent", (0.0, -0.205, 1.20), (0.155, 0.020, 0.230), accent, root)
    cube("chr_a1_stylized_operator_01_lod0_shoulder_bar", (0.0, -0.005, 1.38), (0.74, 0.20, 0.120), armor, root)
    cylinder("chr_a1_stylized_operator_01_lod0_neck", (0.0, 0.0, 1.49), 0.085, 0.135, dark, root, vertices=14)
    sphere("chr_a1_stylized_operator_01_lod0_helmet", (0.0, -0.010, 1.66), 0.185, armor, root, scale=(0.95, 0.88, 1.08), segments=16, rings=8)
    cube("chr_a1_stylized_operator_01_lod0_visor", (0.0, -0.170, 1.660), (0.205, 0.026, 0.070), visor, root)
    cube("chr_a1_stylized_operator_01_lod0_helmet_brow", (0.0, -0.152, 1.725), (0.245, 0.035, 0.030), dark, root)
    cube("chr_a1_stylized_operator_01_lod0_back_pack", (0.0, 0.190, 1.14), (0.330, 0.095, 0.470), pack, root)
    cube("chr_a1_stylized_operator_01_lod0_pack_left_mark", (-0.130, 0.250, 1.03), (0.055, 0.032, 0.170), visor, root)
    cube("chr_a1_stylized_operator_01_lod0_pack_right_mark", (0.130, 0.250, 1.03), (0.055, 0.032, 0.170), visor, root)

    for side, sign in (("r", 1.0), ("l", -1.0)):
        upper_arm_start = (sign * 0.365, -0.020, 1.315)
        upper_arm_end = (sign * 0.505, -0.005, 0.965)
        lower_arm_end = (sign * 0.395, -0.105, 0.650)
        cylinder_between(f"chr_a1_stylized_operator_01_lod0_upper_arm_{side}", upper_arm_start, upper_arm_end, 0.068, suit, root)
        cylinder_between(f"chr_a1_stylized_operator_01_lod0_forearm_{side}", upper_arm_end, lower_arm_end, 0.063, armor, root)
        sphere(f"chr_a1_stylized_operator_01_lod0_elbow_{side}", upper_arm_end, 0.070, dark, root, scale=(0.88, 0.88, 0.88), segments=12, rings=6)
        cube(f"chr_a1_stylized_operator_01_lod0_hand_{side}", (sign * 0.390, -0.115, 0.575), (0.105, 0.080, 0.085), dark, root)
        cube(f"chr_a1_stylized_operator_01_lod0_shoulder_pad_{side}", (sign * 0.405, -0.040, 1.365), (0.160, 0.105, 0.095), armor, root)

        cylinder_between(f"chr_a1_stylized_operator_01_lod0_thigh_{side}", (sign * 0.145, 0.0, 0.640), (sign * 0.165, -0.010, 0.360), 0.088, suit, root)
        cylinder_between(f"chr_a1_stylized_operator_01_lod0_shin_{side}", (sign * 0.165, -0.010, 0.360), (sign * 0.145, -0.035, 0.115), 0.076, armor, root)
        cube(f"chr_a1_stylized_operator_01_lod0_knee_pad_{side}", (sign * 0.160, -0.095, 0.405), (0.125, 0.045, 0.095), armor, root)
        cube(f"chr_a1_stylized_operator_01_lod0_boot_{side}", (sign * 0.145, -0.095, 0.055), (0.150, 0.225, 0.090), dark, root)

    sphere("hitbox_chr_a1_stylized_operator_01_head", (0.0, 0.0, 1.66), 0.225, hitbox, root, scale=(1.0, 0.92, 1.0), segments=12, rings=6)
    cube("hitbox_chr_a1_stylized_operator_01_torso", (0.0, 0.0, 1.12), (0.58, 0.42, 0.58), hitbox, root)
    cube("hitbox_chr_a1_stylized_operator_01_pelvis", (0.0, 0.0, 0.70), (0.48, 0.36, 0.28), hitbox, root)
    cylinder("col_chr_a1_stylized_operator_01_capsule", (0.0, 0.0, 0.90), 0.40, 1.80, collision, root, vertices=20)

    armature(
        "rig_chr_a1_stylized_operator_01",
        [
            ("root", (0.0, 0.0, 0.0), (0.0, 0.0, 0.18), None),
            ("pelvis", (0.0, 0.0, 0.62), (0.0, 0.0, 0.84), "root"),
            ("spine", (0.0, 0.0, 0.84), (0.0, 0.0, 1.34), "pelvis"),
            ("neck", (0.0, 0.0, 1.34), (0.0, 0.0, 1.50), "spine"),
            ("head", (0.0, 0.0, 1.50), (0.0, 0.0, 1.80), "neck"),
            ("upper_arm.R", (0.34, 0.0, 1.32), (0.50, -0.01, 0.97), "spine"),
            ("lower_arm.R", (0.50, -0.01, 0.97), (0.39, -0.10, 0.65), "upper_arm.R"),
            ("hand.R", (0.39, -0.10, 0.65), (0.39, -0.13, 0.53), "lower_arm.R"),
            ("upper_arm.L", (-0.34, 0.0, 1.32), (-0.50, -0.01, 0.97), "spine"),
            ("lower_arm.L", (-0.50, -0.01, 0.97), (-0.39, -0.10, 0.65), "upper_arm.L"),
            ("hand.L", (-0.39, -0.10, 0.65), (-0.39, -0.13, 0.53), "lower_arm.L"),
            ("thigh.R", (0.14, 0.0, 0.62), (0.16, -0.01, 0.36), "pelvis"),
            ("calf.R", (0.16, -0.01, 0.36), (0.14, -0.04, 0.11), "thigh.R"),
            ("foot.R", (0.14, -0.04, 0.11), (0.14, -0.21, 0.04), "calf.R"),
            ("thigh.L", (-0.14, 0.0, 0.62), (-0.16, -0.01, 0.36), "pelvis"),
            ("calf.L", (-0.16, -0.01, 0.36), (-0.14, -0.04, 0.11), "thigh.L"),
            ("foot.L", (-0.14, -0.04, 0.11), (-0.14, -0.21, 0.04), "calf.L"),
        ],
        root,
    )

    empty_socket("socket_root", (0.0, 0.0, 0.0), root)
    empty_socket("socket_camera", (0.0, -0.165, 1.62), root)
    empty_socket("socket_weapon_root", (0.0, -0.360, 1.235), root)
    empty_socket("socket_hand_r", (0.390, -0.115, 0.575), root)
    empty_socket("socket_hand_l", (-0.390, -0.115, 0.575), root)
    empty_socket("socket_head", (0.0, -0.180, 1.660), root)
    empty_socket("socket_backpack", (0.0, 0.270, 1.160), root)
    empty_socket("socket_vfx", (0.0, -0.260, 1.200), root)


def build_first_person_arms() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("chr_a1_fp_arms_01")

    sleeve = material("mat_a1_fp_arms_sleeve", (0.18, 0.23, 0.26, 1.0))
    glove = material("mat_a1_fp_arms_glove", (0.035, 0.038, 0.042, 1.0))
    armor = material("mat_a1_fp_arms_wrist_armor", (0.33, 0.35, 0.35, 1.0))
    accent = material("mat_a1_fp_arms_alignment_mark", (0.05, 0.55, 0.78, 1.0))

    cylinder_between("chr_a1_fp_arms_01_lod0_upper_arm_r", (0.235, 0.060, -0.205), (0.205, 0.455, -0.315), 0.072, sleeve, root)
    cylinder_between("chr_a1_fp_arms_01_lod0_forearm_r", (0.205, 0.455, -0.315), (0.125, 0.775, -0.405), 0.066, sleeve, root)
    cube("chr_a1_fp_arms_01_lod0_wrist_plate_r", (0.142, 0.680, -0.385), (0.130, 0.048, 0.082), armor, root, rotation=(math.radians(-6.0), 0.0, 0.0))
    cube("chr_a1_fp_arms_01_lod0_hand_r", (0.122, 0.810, -0.420), (0.120, 0.092, 0.082), glove, root)

    cylinder_between("chr_a1_fp_arms_01_lod0_upper_arm_l", (-0.235, 0.055, -0.215), (-0.245, 0.430, -0.330), 0.072, sleeve, root)
    cylinder_between("chr_a1_fp_arms_01_lod0_forearm_l", (-0.245, 0.430, -0.330), (-0.122, 0.655, -0.408), 0.066, sleeve, root)
    cube("chr_a1_fp_arms_01_lod0_wrist_plate_l", (-0.148, 0.590, -0.392), (0.130, 0.048, 0.082), armor, root, rotation=(math.radians(7.0), 0.0, 0.0))
    cube("chr_a1_fp_arms_01_lod0_hand_l", (-0.122, 0.690, -0.424), (0.120, 0.092, 0.082), glove, root)
    cube("chr_a1_fp_arms_01_lod0_weapon_alignment_bar", (0.0, 0.650, -0.395), (0.330, 0.018, 0.016), accent, root)
    cube("chr_a1_fp_arms_01_lod1_blockout", (0.0, 0.430, -0.345), (0.580, 0.620, 0.075), sleeve, root)

    armature(
        "rig_chr_a1_fp_arms_01",
        [
            ("root", (0.0, 0.0, 0.0), (0.0, 0.10, -0.04), None),
            ("upper_arm.R", (0.235, 0.060, -0.205), (0.205, 0.455, -0.315), "root"),
            ("lower_arm.R", (0.205, 0.455, -0.315), (0.125, 0.775, -0.405), "upper_arm.R"),
            ("hand.R", (0.125, 0.775, -0.405), (0.125, 0.890, -0.425), "lower_arm.R"),
            ("upper_arm.L", (-0.235, 0.055, -0.215), (-0.245, 0.430, -0.330), "root"),
            ("lower_arm.L", (-0.245, 0.430, -0.330), (-0.122, 0.655, -0.408), "upper_arm.L"),
            ("hand.L", (-0.122, 0.655, -0.408), (-0.122, 0.770, -0.425), "lower_arm.L"),
        ],
        root,
    )

    empty_socket("socket_camera", (0.0, 0.0, 0.0), root)
    empty_socket("socket_weapon_root", (0.0, 0.645, -0.390), root)
    empty_socket("socket_hand_r", (0.122, 0.810, -0.420), root)
    empty_socket("socket_hand_l", (-0.122, 0.690, -0.424), root)
    empty_socket("socket_vfx", (0.0, 0.820, -0.390), root)


ASSETS: tuple[AssetSpec, ...] = (
    AssetSpec(
        asset_id="wpn_a1_compact_rifle_01",
        category_dir="weapons",
        category="weapon",
        tags=("all", "weapon", "weapons", "compact_rifle"),
        dimensions_m=(0.19, 1.13, 0.49),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Compact generic rifle blockout with short receiver, enclosed front silhouette, compact rear support, readable magazine, and inspection sockets.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"),
        collision="none",
        lods=("wpn_a1_compact_rifle_01_lod0", "wpn_a1_compact_rifle_01_lod1"),
        build=build_compact_rifle,
    ),
    AssetSpec(
        asset_id="wpn_a1_modern_rifle_01",
        category_dir="weapons",
        category="weapon",
        tags=("all", "weapon", "weapons", "modern_rifle"),
        dimensions_m=(0.19, 1.45, 0.57),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Generic modern assault-rifle-style blockout with long front rail silhouette, rear support, optic mass, magazine, and animation sockets.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"),
        collision="none",
        lods=("wpn_a1_modern_rifle_01_lod0", "wpn_a1_modern_rifle_01_lod1"),
        build=build_modern_rifle,
    ),
    AssetSpec(
        asset_id="wpn_a1_compact_sidearm_01",
        category_dir="weapons",
        category="weapon",
        tags=("all", "weapon", "weapons", "sidearm"),
        dimensions_m=(0.13, 0.45, 0.35),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Generic compact sidearm blockout with clear slide/frame/grip silhouette, simple sights, and first-person sockets.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_eject", "socket_vfx"),
        collision="none",
        lods=("wpn_a1_compact_sidearm_01_lod0", "wpn_a1_compact_sidearm_01_lod1"),
        build=build_compact_sidearm,
    ),
    AssetSpec(
        asset_id="chr_a1_stylized_operator_01",
        category_dir="characters",
        category="character",
        tags=("all", "character", "characters", "operator"),
        dimensions_m=(1.02, 0.56, 1.84),
        origin_note="Origin at feet/root on the floor.",
        description="Stylized sci-fi operator blockout combining readable military gear, pilot-like helmet/visor language, mobility-pack shapes, rig, hitbox guides, and sockets.",
        sockets=("socket_root", "socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l", "socket_head", "socket_backpack", "socket_vfx"),
        collision="col_chr_a1_stylized_operator_01_capsule",
        lods=("chr_a1_stylized_operator_01_lod0",),
        build=build_stylized_operator,
    ),
    AssetSpec(
        asset_id="chr_a1_fp_arms_01",
        category_dir="characters",
        category="character",
        tags=("all", "character", "characters", "arms"),
        dimensions_m=(0.58, 0.89, 0.44),
        origin_note="Origin at camera/root alignment point.",
        description="Optional first-person arms blockout with simple sleeves, gloves, wrist armor, rig bones, and weapon alignment sockets.",
        sockets=("socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l", "socket_vfx"),
        collision="none",
        lods=("chr_a1_fp_arms_01_lod0", "chr_a1_fp_arms_01_lod1"),
        build=build_first_person_arms,
    ),
)


def selected_assets(only: str) -> list[AssetSpec]:
    return [spec for spec in ASSETS if only in spec.tags]


def write_pack_manifest(records: list[dict[str, object]]) -> None:
    manifest = {
        "pack_id": "nemisis_a1_prototype_pack",
        "generated_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "generator": "tools/blender/make_a1_prototype_pack.py",
        "blender_version": bpy.app.version_string,
        "units": "meters",
        "external_assets": False,
        "asset_count": len(records),
        "assets": records,
    }
    ensure_dirs(PACK_MANIFEST.parent)
    PACK_MANIFEST.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {rel(PACK_MANIFEST)}")


def main() -> None:
    args = parse_args()
    specs = selected_assets(args.only)
    if not specs:
        raise SystemExit(f"No assets matched --only {args.only!r}.")

    records: list[dict[str, object]] = []
    for spec in specs:
        spec.build()
        records.append(save_and_export(spec))

    write_pack_manifest(records)


if __name__ == "__main__":
    main()
