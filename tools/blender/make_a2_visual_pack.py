"""Generate the Nemisis A2 visual FPS test asset pack in Blender.

Run from the Nemisis repository root:
    blender --background --python tools/blender/make_a2_visual_pack.py

The script creates original procedural dev-art only. Weapon silhouettes are
generic FPS test assets and do not include real-world functional details,
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
SOURCE_ROOT = REPO_ROOT / "assets" / "source" / "blender" / "a2_visual_pack"
OUTPUT_ROOT = REPO_ROOT / "assets" / "generated" / "a2_visual_pack"
GENERATOR = "tools/blender/make_a2_visual_pack.py"


@dataclass(frozen=True)
class AssetSpec:
    asset_id: str
    category: str
    tags: tuple[str, ...]
    target_dimensions_m: tuple[float, float, float]
    origin_note: str
    description: str
    sockets: tuple[str, ...]
    socket_notes: dict[str, str]
    notes: tuple[str, ...]
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

    choices = sorted({tag for spec in ASSETS for tag in spec.tags} | {spec.asset_id for spec in ASSETS})
    parser = argparse.ArgumentParser(description="Generate Nemisis A2 visual FPS test assets.")
    parser.add_argument(
        "--only",
        default="all",
        choices=choices,
        help="Generate one group or one asset id.",
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
    obj.empty_display_size = 0.24
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


def mesh_object(
    name: str,
    verts: list[tuple[float, float, float]],
    faces: list[tuple[int, ...]],
    mat: bpy.types.Material,
    parent: bpy.types.Object,
) -> bpy.types.Object:
    mesh = bpy.data.meshes.new(f"{name}_mesh")
    mesh.from_pydata(verts, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    assign_material(obj, mat)
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
        "tags": list(spec.tags),
        "scale_meters": True,
        "runtime_up_axis": "Y",
        "gameplay_forward_axis": "+Z",
        "blender_up_axis": "+Z",
        "blender_forward_axis": "+Y",
        "dimensions_m": dimensions_m,
        "target_dimensions_m": spec.target_dimensions_m,
        "origin": spec.origin_note,
        "sockets": list(spec.sockets),
        "socket_notes": spec.socket_notes,
        "notes": list(spec.notes),
        "collision": spec.collision,
        "lods": list(spec.lods),
        "license": "original_project_asset",
        "external_assets": False,
        "visual_reference_note": "Original procedural dev-art using broad FPS and sci-fi test-asset language only; no brands, logos, copied protected silhouettes, or real functional implementation details.",
        "generated_by": GENERATOR,
        "generated_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "stats": stats,
    }


def save_and_export(spec: AssetSpec) -> dict[str, object]:
    ensure_dirs(SOURCE_ROOT, OUTPUT_ROOT)

    source_path = SOURCE_ROOT / f"{spec.asset_id}.blend"
    export_path = OUTPUT_ROOT / f"{spec.asset_id}.glb"
    metadata_path = OUTPUT_ROOT / f"{spec.asset_id}.metadata.json"

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
        "tags": list(spec.tags),
        "source": rel(source_path),
        "file": rel(export_path),
        "metadata": rel(metadata_path),
        "dimensions_m": dimensions_m,
        "target_dimensions_m": spec.target_dimensions_m,
        "origin": spec.origin_note,
        "description": spec.description,
        "sockets": list(spec.sockets),
        "notes": list(spec.notes),
        "collision": spec.collision,
        "lods": list(spec.lods),
        "bytes": export_path.stat().st_size,
        "stats": stats,
    }


def build_blackout_carbine() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a2_blackout_carbine_01")

    black = material("mat_a2_carbine_black_polymer", (0.018, 0.021, 0.023, 1.0))
    graphite = material("mat_a2_carbine_graphite_metal", (0.20, 0.215, 0.225, 1.0), metallic=0.12)
    rubber = material("mat_a2_carbine_rubber", (0.006, 0.007, 0.008, 1.0))
    teal = material("mat_a2_carbine_teal_socket_marks", (0.02, 0.70, 0.76, 1.0))
    orange = material("mat_a2_carbine_orange_range_mark", (1.0, 0.38, 0.04, 1.0))
    glass = material("mat_a2_carbine_smoke_glass", (0.04, 0.11, 0.13, 1.0), roughness=0.35)

    cube("wpn_a2_blackout_carbine_01_lod0_upper_receiver", (0.0, 0.055, 0.055), (0.165, 0.350, 0.095), graphite, root)
    cube("wpn_a2_blackout_carbine_01_lod0_lower_receiver", (0.0, -0.075, -0.020), (0.150, 0.255, 0.090), black, root)
    cube("wpn_a2_blackout_carbine_01_lod0_flush_top_rail", (0.0, 0.115, 0.145), (0.090, 0.520, 0.024), rubber, root)
    cube("wpn_a2_blackout_carbine_01_lod0_compact_handguard", (0.0, 0.360, 0.027), (0.185, 0.300, 0.095), black, root)
    cylinder(
        "wpn_a2_blackout_carbine_01_lod0_suppressed_barrel_proxy",
        (0.0, 0.585, 0.032),
        0.045,
        0.300,
        rubber,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cylinder(
        "wpn_a2_blackout_carbine_01_lod0_muzzle_ring",
        (0.0, 0.760, 0.032),
        0.052,
        0.042,
        graphite,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a2_blackout_carbine_01_lod0_minimal_stock", (0.0, -0.365, 0.035), (0.145, 0.250, 0.070), rubber, root)
    cube("wpn_a2_blackout_carbine_01_lod0_stock_pad", (0.0, -0.520, 0.040), (0.180, 0.040, 0.120), rubber, root)
    cube("wpn_a2_blackout_carbine_01_lod0_magazine", (0.0, -0.015, -0.205), (0.085, 0.070, 0.260), black, root, rotation=(math.radians(-5.5), 0.0, 0.0))
    cube("wpn_a2_blackout_carbine_01_lod0_grip", (0.0, -0.225, -0.165), (0.075, 0.065, 0.205), rubber, root, rotation=(math.radians(-10.0), 0.0, 0.0))
    cube("wpn_a2_blackout_carbine_01_lod0_front_stop", (0.0, 0.255, -0.092), (0.085, 0.045, 0.085), teal, root)
    cube("wpn_a2_blackout_carbine_01_lod0_micro_optic_mount", (0.0, -0.015, 0.195), (0.110, 0.100, 0.035), rubber, root)
    cube("wpn_a2_blackout_carbine_01_lod0_micro_optic_body", (0.0, -0.015, 0.250), (0.092, 0.075, 0.060), glass, root)
    cube("wpn_a2_blackout_carbine_01_lod0_orange_selector_mark", (-0.088, -0.045, 0.020), (0.012, 0.090, 0.036), orange, root)
    cube("wpn_a2_blackout_carbine_01_lod0_teal_muzzle_mark", (0.0, 0.720, 0.092), (0.070, 0.028, 0.018), teal, root)
    cube("wpn_a2_blackout_carbine_01_lod1_blockout", (0.0, 0.090, 0.015), (0.120, 1.170, 0.085), black, root)

    empty_socket("socket_muzzle", (0.0, 0.792, 0.032), root)
    empty_socket("socket_grip_r", (0.0, -0.225, -0.105), root)
    empty_socket("socket_grip_l", (0.0, 0.255, -0.080), root)
    empty_socket("socket_optic", (0.0, -0.015, 0.292), root)
    empty_socket("socket_mag", (0.0, -0.015, -0.335), root)
    empty_socket("socket_eject", (0.095, 0.025, 0.092), root)
    empty_socket("socket_vfx", (0.0, 0.792, 0.032), root)


def build_modular_rifle() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a2_modular_rifle_01")

    dark = material("mat_a2_modular_rifle_dark_polymer", (0.045, 0.049, 0.050, 1.0))
    metal = material("mat_a2_modular_rifle_gunmetal", (0.31, 0.325, 0.33, 1.0), metallic=0.14)
    panel = material("mat_a2_modular_rifle_cool_panel", (0.18, 0.25, 0.28, 1.0))
    teal = material("mat_a2_modular_rifle_teal_accents", (0.0, 0.66, 0.72, 1.0))
    orange = material("mat_a2_modular_rifle_orange_key", (1.0, 0.39, 0.05, 1.0))
    glass = material("mat_a2_modular_rifle_optic_glass", (0.035, 0.12, 0.14, 1.0), roughness=0.34)

    cube("wpn_a2_modular_rifle_01_lod0_receiver", (0.0, 0.030, 0.045), (0.180, 0.390, 0.105), metal, root)
    cube("wpn_a2_modular_rifle_01_lod0_rail_spine", (0.0, 0.155, 0.158), (0.100, 0.770, 0.026), dark, root)
    cube("wpn_a2_modular_rifle_01_lod0_modular_handguard", (0.0, 0.450, 0.030), (0.195, 0.500, 0.090), dark, root)
    for y in (0.245, 0.380, 0.515, 0.650):
        cube(f"wpn_a2_modular_rifle_01_lod0_side_slot_l_{y:.2f}", (-0.106, y, 0.030), (0.018, 0.070, 0.030), teal, root)
        cube(f"wpn_a2_modular_rifle_01_lod0_side_slot_r_{y:.2f}", (0.106, y, 0.030), (0.018, 0.070, 0.030), teal, root)
    cylinder(
        "wpn_a2_modular_rifle_01_lod0_barrel_visual",
        (0.0, 0.780, 0.043),
        0.030,
        0.370,
        metal,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cylinder(
        "wpn_a2_modular_rifle_01_lod0_muzzle_device_proxy",
        (0.0, 0.980, 0.043),
        0.040,
        0.065,
        dark,
        root,
        vertices=20,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a2_modular_rifle_01_lod0_adjustable_stock_beam", (0.0, -0.395, 0.055), (0.115, 0.320, 0.060), dark, root)
    cube("wpn_a2_modular_rifle_01_lod0_adjustable_stock_pad", (0.0, -0.600, 0.055), (0.190, 0.052, 0.145), dark, root)
    cube("wpn_a2_modular_rifle_01_lod0_magazine", (0.0, -0.030, -0.215), (0.095, 0.075, 0.305), panel, root, rotation=(math.radians(-5.0), 0.0, 0.0))
    cube("wpn_a2_modular_rifle_01_lod0_grip", (0.0, -0.240, -0.172), (0.076, 0.064, 0.215), dark, root, rotation=(math.radians(-9.0), 0.0, 0.0))
    cube("wpn_a2_modular_rifle_01_lod0_angled_foregrip", (0.0, 0.340, -0.120), (0.078, 0.060, 0.170), dark, root, rotation=(math.radians(8.0), 0.0, 0.0))
    cube("wpn_a2_modular_rifle_01_lod0_optic_base", (0.0, 0.045, 0.213), (0.125, 0.128, 0.036), dark, root)
    cube("wpn_a2_modular_rifle_01_lod0_optic_body", (0.0, 0.045, 0.275), (0.120, 0.100, 0.078), glass, root)
    cube("wpn_a2_modular_rifle_01_lod0_optic_orange_index", (0.0, 0.098, 0.330), (0.075, 0.020, 0.018), orange, root)
    cube("wpn_a2_modular_rifle_01_lod0_mag_orange_index", (0.0, -0.075, -0.070), (0.100, 0.018, 0.046), orange, root)
    cube("wpn_a2_modular_rifle_01_lod1_blockout", (0.0, 0.145, 0.018), (0.135, 1.340, 0.092), dark, root)

    empty_socket("socket_muzzle", (0.0, 1.025, 0.043), root)
    empty_socket("socket_grip_r", (0.0, -0.240, -0.110), root)
    empty_socket("socket_grip_l", (0.0, 0.340, -0.082), root)
    empty_socket("socket_optic", (0.0, 0.045, 0.320), root)
    empty_socket("socket_underbarrel", (0.0, 0.455, -0.085), root)
    empty_socket("socket_mag", (0.0, -0.030, -0.365), root)
    empty_socket("socket_eject", (0.102, 0.040, 0.100), root)
    empty_socket("socket_vfx", (0.0, 1.025, 0.043), root)


def build_striker_sidearm() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_a2_striker_sidearm_01")

    frame = material("mat_a2_striker_frame_black", (0.035, 0.037, 0.041, 1.0))
    slide = material("mat_a2_striker_slide_graphite", (0.275, 0.290, 0.305, 1.0), metallic=0.08)
    dark = material("mat_a2_striker_dark_detail", (0.010, 0.011, 0.013, 1.0))
    teal = material("mat_a2_striker_teal_sight", (0.0, 0.68, 0.73, 1.0))
    orange = material("mat_a2_striker_orange_index", (1.0, 0.36, 0.04, 1.0))

    cube("wpn_a2_striker_sidearm_01_lod0_slide", (0.0, 0.060, 0.062), (0.128, 0.315, 0.063), slide, root)
    cube("wpn_a2_striker_sidearm_01_lod0_slide_front_cut", (0.0, 0.195, 0.107), (0.110, 0.060, 0.014), dark, root)
    cube("wpn_a2_striker_sidearm_01_lod0_frame", (0.0, 0.010, 0.006), (0.115, 0.238, 0.056), frame, root)
    cube("wpn_a2_striker_sidearm_01_lod0_underbarrel_rail", (0.0, 0.100, -0.040), (0.096, 0.145, 0.026), dark, root)
    cylinder(
        "wpn_a2_striker_sidearm_01_lod0_front_opening",
        (0.0, 0.242, 0.064),
        0.022,
        0.052,
        dark,
        root,
        vertices=18,
        rotation=(math.pi / 2.0, 0.0, 0.0),
    )
    cube("wpn_a2_striker_sidearm_01_lod0_grip_core", (0.0, -0.112, -0.128), (0.078, 0.078, 0.225), frame, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a2_striker_sidearm_01_lod0_grip_panel_l", (-0.042, -0.112, -0.128), (0.012, 0.068, 0.175), dark, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a2_striker_sidearm_01_lod0_grip_panel_r", (0.042, -0.112, -0.128), (0.012, 0.068, 0.175), dark, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a2_striker_sidearm_01_lod0_trigger_guard", (0.0, 0.032, -0.066), (0.102, 0.080, 0.036), frame, root)
    cube("wpn_a2_striker_sidearm_01_lod0_trigger_proxy", (0.0, 0.002, -0.067), (0.032, 0.018, 0.064), dark, root, rotation=(math.radians(-9.0), 0.0, 0.0))
    cube("wpn_a2_striker_sidearm_01_lod0_front_sight_teal", (0.0, 0.202, 0.108), (0.033, 0.024, 0.019), teal, root)
    cube("wpn_a2_striker_sidearm_01_lod0_rear_sight_teal", (0.0, -0.078, 0.111), (0.052, 0.032, 0.022), teal, root)
    cube("wpn_a2_striker_sidearm_01_lod0_base_plate_orange", (0.0, -0.182, -0.256), (0.094, 0.084, 0.025), orange, root, rotation=(math.radians(-12.0), 0.0, 0.0))
    cube("wpn_a2_striker_sidearm_01_lod1_blockout", (0.0, 0.020, 0.010), (0.105, 0.375, 0.078), frame, root)

    empty_socket("socket_muzzle", (0.0, 0.287, 0.064), root)
    empty_socket("socket_grip_r", (0.0, -0.112, -0.120), root)
    empty_socket("socket_grip_l", (0.0, 0.018, -0.064), root)
    empty_socket("socket_optic", (0.0, -0.020, 0.128), root)
    empty_socket("socket_rail", (0.0, 0.120, -0.060), root)
    empty_socket("socket_eject", (0.075, 0.060, 0.102), root)
    empty_socket("socket_vfx", (0.0, 0.287, 0.064), root)


def build_pilot_operator() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("chr_a2_pilot_operator_01")

    suit = material("mat_a2_operator_suit_charcoal", (0.12, 0.16, 0.17, 1.0))
    armor = material("mat_a2_operator_armor_graphite", (0.30, 0.32, 0.32, 1.0))
    dark = material("mat_a2_operator_joint_black", (0.025, 0.028, 0.030, 1.0))
    visor = material("mat_a2_operator_visor_teal", (0.02, 0.72, 0.78, 1.0), roughness=0.32)
    orange = material("mat_a2_operator_orange_harness", (0.95, 0.36, 0.05, 1.0))
    teal = material("mat_a2_operator_teal_lights", (0.0, 0.62, 0.72, 1.0))
    pack = material("mat_a2_operator_jump_pack", (0.18, 0.22, 0.23, 1.0))
    hitbox = material("mat_a2_operator_hitbox_guide", (1.0, 0.76, 0.08, 0.22))
    collision = material("mat_a2_operator_collision_proxy", (0.0, 1.0, 0.36, 0.16))

    cube("chr_a2_pilot_operator_01_lod0_pelvis", (0.0, 0.0, 0.72), (0.44, 0.25, 0.20), armor, root)
    cube("chr_a2_pilot_operator_01_lod0_torso_suit", (0.0, -0.005, 1.10), (0.50, 0.30, 0.56), suit, root)
    cube("chr_a2_pilot_operator_01_lod0_chest_plate", (0.0, -0.175, 1.19), (0.390, 0.045, 0.390), armor, root)
    cube("chr_a2_pilot_operator_01_lod0_harness_vertical_l", (-0.105, -0.210, 1.18), (0.045, 0.020, 0.410), orange, root, rotation=(0.0, 0.0, math.radians(-6.0)))
    cube("chr_a2_pilot_operator_01_lod0_harness_vertical_r", (0.105, -0.210, 1.18), (0.045, 0.020, 0.410), orange, root, rotation=(0.0, 0.0, math.radians(6.0)))
    cube("chr_a2_pilot_operator_01_lod0_shoulder_bar", (0.0, -0.005, 1.38), (0.74, 0.20, 0.120), armor, root)
    cylinder("chr_a2_pilot_operator_01_lod0_neck", (0.0, 0.0, 1.49), 0.085, 0.135, dark, root, vertices=14)
    sphere("chr_a2_pilot_operator_01_lod0_pilot_helmet", (0.0, -0.010, 1.66), 0.190, armor, root, scale=(0.96, 0.88, 1.10), segments=16, rings=8)
    cube("chr_a2_pilot_operator_01_lod0_wide_visor", (0.0, -0.173, 1.660), (0.235, 0.026, 0.078), visor, root)
    cube("chr_a2_pilot_operator_01_lod0_helmet_brow", (0.0, -0.153, 1.733), (0.260, 0.036, 0.034), dark, root)
    cube("chr_a2_pilot_operator_01_lod0_helmet_teal_id", (0.0, -0.180, 1.585), (0.090, 0.018, 0.025), teal, root)
    cube("chr_a2_pilot_operator_01_lod0_mobility_pack", (0.0, 0.200, 1.13), (0.340, 0.105, 0.500), pack, root)
    cylinder("chr_a2_pilot_operator_01_lod0_pack_thruster_l", (-0.135, 0.270, 0.98), 0.045, 0.155, dark, root, vertices=14, rotation=(math.pi / 2.0, 0.0, 0.0))
    cylinder("chr_a2_pilot_operator_01_lod0_pack_thruster_r", (0.135, 0.270, 0.98), 0.045, 0.155, dark, root, vertices=14, rotation=(math.pi / 2.0, 0.0, 0.0))
    cube("chr_a2_pilot_operator_01_lod0_pack_teal_l", (-0.135, 0.355, 0.98), (0.060, 0.020, 0.100), teal, root)
    cube("chr_a2_pilot_operator_01_lod0_pack_teal_r", (0.135, 0.355, 0.98), (0.060, 0.020, 0.100), teal, root)

    for side, sign in (("r", 1.0), ("l", -1.0)):
        upper_arm_start = (sign * 0.365, -0.020, 1.315)
        upper_arm_end = (sign * 0.505, -0.005, 0.965)
        lower_arm_end = (sign * 0.395, -0.105, 0.650)
        cylinder_between(f"chr_a2_pilot_operator_01_lod0_upper_arm_{side}", upper_arm_start, upper_arm_end, 0.068, suit, root)
        cylinder_between(f"chr_a2_pilot_operator_01_lod0_forearm_{side}", upper_arm_end, lower_arm_end, 0.063, armor, root)
        sphere(f"chr_a2_pilot_operator_01_lod0_elbow_{side}", upper_arm_end, 0.070, dark, root, scale=(0.88, 0.88, 0.88), segments=12, rings=6)
        cube(f"chr_a2_pilot_operator_01_lod0_hand_{side}", (sign * 0.390, -0.115, 0.575), (0.105, 0.080, 0.085), dark, root)
        cube(f"chr_a2_pilot_operator_01_lod0_shoulder_pad_{side}", (sign * 0.405, -0.040, 1.365), (0.160, 0.105, 0.095), armor, root)
        cube(f"chr_a2_pilot_operator_01_lod0_wrist_light_{side}", (sign * 0.380, -0.130, 0.700), (0.045, 0.020, 0.055), teal, root)

        cylinder_between(f"chr_a2_pilot_operator_01_lod0_thigh_{side}", (sign * 0.145, 0.0, 0.640), (sign * 0.165, -0.010, 0.360), 0.088, suit, root)
        cylinder_between(f"chr_a2_pilot_operator_01_lod0_shin_{side}", (sign * 0.165, -0.010, 0.360), (sign * 0.145, -0.035, 0.115), 0.076, armor, root)
        cube(f"chr_a2_pilot_operator_01_lod0_knee_pad_{side}", (sign * 0.160, -0.095, 0.405), (0.125, 0.045, 0.095), armor, root)
        cube(f"chr_a2_pilot_operator_01_lod0_boot_{side}", (sign * 0.145, -0.095, 0.055), (0.150, 0.225, 0.090), dark, root)
        cube(f"chr_a2_pilot_operator_01_lod0_thigh_orange_tab_{side}", (sign * 0.225, -0.080, 0.560), (0.030, 0.026, 0.100), orange, root)

    sphere("hitbox_chr_a2_pilot_operator_01_head", (0.0, 0.0, 1.66), 0.225, hitbox, root, scale=(1.0, 0.92, 1.0), segments=12, rings=6)
    cube("hitbox_chr_a2_pilot_operator_01_torso", (0.0, 0.0, 1.12), (0.58, 0.42, 0.58), hitbox, root)
    cube("hitbox_chr_a2_pilot_operator_01_pelvis", (0.0, 0.0, 0.70), (0.48, 0.36, 0.28), hitbox, root)
    cylinder("col_chr_a2_pilot_operator_01_capsule", (0.0, 0.0, 0.90), 0.40, 1.80, collision, root, vertices=20)

    armature(
        "rig_chr_a2_pilot_operator_01",
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
    empty_socket("socket_backpack", (0.0, 0.315, 1.100), root)
    empty_socket("socket_vfx", (0.0, 0.360, 0.980), root)


def build_wallrun_panel() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_a2_wallrun_panel_01")

    slab = material("mat_a2_wallrun_panel_slab", (0.35, 0.38, 0.39, 1.0))
    dark = material("mat_a2_wallrun_panel_dark_trim", (0.055, 0.060, 0.064, 1.0))
    teal = material("mat_a2_wallrun_panel_teal_lane", (0.0, 0.68, 0.74, 1.0))
    orange = material("mat_a2_wallrun_panel_orange_snap", (0.98, 0.34, 0.03, 1.0))

    cube("map_a2_wallrun_panel_01_lod0_wall_slab", (0.0, 0.0, 1.30), (4.00, 0.22, 2.60), slab, root)
    cube("map_a2_wallrun_panel_01_lod0_top_cap", (0.0, -0.130, 2.62), (4.08, 0.085, 0.080), dark, root)
    cube("map_a2_wallrun_panel_01_lod0_bottom_cap", (0.0, -0.130, 0.04), (4.08, 0.085, 0.080), dark, root)
    for x in (-1.96, 1.96):
        cube(f"map_a2_wallrun_panel_01_lod0_side_cap_{x:+.2f}", (x, -0.135, 1.30), (0.080, 0.080, 2.58), dark, root)
    for x in (-1.25, 0.0, 1.25):
        cube(f"map_a2_wallrun_panel_01_lod0_vertical_channel_{x:+.2f}", (x, -0.145, 1.30), (0.040, 0.040, 2.12), dark, root)
    cube("map_a2_wallrun_panel_01_lod0_wallrun_lane", (0.0, -0.160, 1.45), (3.45, 0.026, 0.110), teal, root)
    cube("map_a2_wallrun_panel_01_lod0_wallrun_lane_upper", (0.0, -0.162, 1.78), (3.05, 0.024, 0.060), teal, root)
    cube("map_a2_wallrun_panel_01_lod0_left_snap_mark", (-1.82, -0.170, 0.36), (0.220, 0.024, 0.080), orange, root)
    cube("map_a2_wallrun_panel_01_lod0_right_snap_mark", (1.82, -0.170, 0.36), (0.220, 0.024, 0.080), orange, root)
    cube("map_a2_wallrun_panel_01_lod1_blockout", (0.0, 0.0, 1.30), (4.00, 0.18, 2.45), slab, root)

    empty_socket("socket_snap_left", (-2.0, 0.0, 0.0), root, 0.10)
    empty_socket("socket_snap_right", (2.0, 0.0, 0.0), root, 0.10)
    empty_socket("socket_wallrun_start", (-1.72, -0.180, 1.45), root)
    empty_socket("socket_wallrun_end", (1.72, -0.180, 1.45), root)
    empty_socket("socket_vfx", (0.0, -0.180, 1.58), root)


def build_slide_ramp() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_a2_slide_ramp_01")

    ramp_mat = material("mat_a2_slide_ramp_plate", (0.33, 0.35, 0.36, 1.0))
    dark = material("mat_a2_slide_ramp_edge_dark", (0.055, 0.058, 0.060, 1.0))
    teal = material("mat_a2_slide_ramp_teal_flow", (0.0, 0.64, 0.72, 1.0))
    orange = material("mat_a2_slide_ramp_orange_entry", (0.98, 0.35, 0.04, 1.0))

    length = 4.0
    width = 2.2
    height = 0.92
    verts = [
        (-length / 2.0, -width / 2.0, 0.0),
        (-length / 2.0, width / 2.0, 0.0),
        (length / 2.0, -width / 2.0, 0.0),
        (length / 2.0, width / 2.0, 0.0),
        (length / 2.0, -width / 2.0, height),
        (length / 2.0, width / 2.0, height),
    ]
    faces = [
        (0, 2, 3, 1),
        (2, 4, 5, 3),
        (0, 1, 5, 4),
        (0, 4, 2),
        (1, 3, 5),
    ]
    mesh_object("map_a2_slide_ramp_01_lod0_ramp_wedge", verts, faces, ramp_mat, root)

    angle = -math.atan2(height, length)
    for x in (-1.28, -0.55, 0.18, 0.91, 1.54):
        z = (x + length / 2.0) / length * height + 0.035
        cube(f"map_a2_slide_ramp_01_lod0_flow_strip_{x:+.2f}", (x, 0.0, z), (0.055, 1.78, 0.025), teal, root, rotation=(0.0, angle, 0.0))
    cube("map_a2_slide_ramp_01_lod0_left_edge", (0.10, -1.14, 0.50), (4.08, 0.075, 0.085), dark, root, rotation=(0.0, angle, 0.0))
    cube("map_a2_slide_ramp_01_lod0_right_edge", (0.10, 1.14, 0.50), (4.08, 0.075, 0.085), dark, root, rotation=(0.0, angle, 0.0))
    cube("map_a2_slide_ramp_01_lod0_entry_orange_bar", (-1.82, 0.0, 0.060), (0.060, 1.98, 0.045), orange, root)
    cube("map_a2_slide_ramp_01_lod0_exit_orange_bar", (1.96, 0.0, 0.945), (0.060, 1.98, 0.045), orange, root)
    cube("map_a2_slide_ramp_01_lod1_blockout", (0.0, 0.0, 0.46), (4.00, 2.10, 0.12), ramp_mat, root, rotation=(0.0, angle, 0.0))

    empty_socket("socket_snap_bottom", (-2.0, 0.0, 0.0), root, 0.10)
    empty_socket("socket_snap_top", (2.0, 0.0, 0.92), root, 0.10)
    empty_socket("socket_slide_entry", (-1.75, 0.0, 0.08), root)
    empty_socket("socket_slide_exit", (1.75, 0.0, 0.86), root)
    empty_socket("socket_vfx", (-0.15, 0.0, 0.43), root)


def build_cover_crate() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_a2_cover_crate_01")

    body = material("mat_a2_cover_crate_body", (0.22, 0.27, 0.28, 1.0))
    dark = material("mat_a2_cover_crate_dark_trim", (0.045, 0.050, 0.052, 1.0))
    teal = material("mat_a2_cover_crate_teal_corner", (0.0, 0.58, 0.66, 1.0))
    orange = material("mat_a2_cover_crate_orange_cover_mark", (0.96, 0.36, 0.04, 1.0))

    cube("map_a2_cover_crate_01_lod0_core", (0.0, 0.0, 0.62), (1.55, 1.15, 1.24), body, root)
    cube("map_a2_cover_crate_01_lod0_top_cap", (0.0, 0.0, 1.27), (1.68, 1.26, 0.080), dark, root)
    cube("map_a2_cover_crate_01_lod0_bottom_cap", (0.0, 0.0, 0.04), (1.68, 1.26, 0.080), dark, root)
    for x in (-0.76, 0.76):
        cube(f"map_a2_cover_crate_01_lod0_front_upright_{x:+.2f}", (x, -0.600, 0.62), (0.080, 0.055, 1.12), dark, root)
        cube(f"map_a2_cover_crate_01_lod0_back_upright_{x:+.2f}", (x, 0.600, 0.62), (0.080, 0.055, 1.12), dark, root)
    for z in (0.34, 0.91):
        cube(f"map_a2_cover_crate_01_lod0_front_band_{z:.2f}", (0.0, -0.630, z), (1.52, 0.045, 0.070), dark, root)
        cube(f"map_a2_cover_crate_01_lod0_back_band_{z:.2f}", (0.0, 0.630, z), (1.52, 0.045, 0.070), dark, root)
    cube("map_a2_cover_crate_01_lod0_front_orange_cover_mark", (0.0, -0.665, 0.84), (0.500, 0.032, 0.090), orange, root)
    cube("map_a2_cover_crate_01_lod0_left_teal_corner", (-0.850, -0.640, 1.08), (0.050, 0.040, 0.190), teal, root)
    cube("map_a2_cover_crate_01_lod0_right_teal_corner", (0.850, -0.640, 1.08), (0.050, 0.040, 0.190), teal, root)
    cube("map_a2_cover_crate_01_lod1_blockout", (0.0, 0.0, 0.62), (1.55, 1.15, 1.24), body, root)

    empty_socket("socket_snap_center", (0.0, 0.0, 0.0), root, 0.10)
    empty_socket("socket_cover_peak", (0.0, -0.62, 1.26), root)
    empty_socket("socket_vfx", (0.0, -0.68, 0.84), root)


def build_range_hero_prop() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("prop_a2_range_hero_01")

    base = material("mat_a2_range_hero_base", (0.18, 0.21, 0.22, 1.0))
    dark = material("mat_a2_range_hero_dark_trim", (0.045, 0.050, 0.052, 1.0))
    teal = material("mat_a2_range_hero_teal_light", (0.0, 0.72, 0.78, 1.0))
    orange = material("mat_a2_range_hero_orange_panel", (1.0, 0.36, 0.04, 1.0))
    glass = material("mat_a2_range_hero_transparent_sight", (0.04, 0.72, 0.78, 0.34), roughness=0.25)

    cylinder("prop_a2_range_hero_01_lod0_floor_base", (0.0, 0.0, 0.070), 0.38, 0.140, dark, root, vertices=24)
    cylinder("prop_a2_range_hero_01_lod0_rotary_plinth", (0.0, 0.0, 0.245), 0.245, 0.210, base, root, vertices=20)
    cube("prop_a2_range_hero_01_lod0_center_pylon", (0.0, 0.0, 0.610), (0.250, 0.190, 0.620), base, root)
    cube("prop_a2_range_hero_01_lod0_orange_control_face", (0.0, -0.105, 0.610), (0.190, 0.026, 0.360), orange, root)
    cube("prop_a2_range_hero_01_lod0_teal_status_bar", (0.0, -0.125, 0.810), (0.135, 0.020, 0.050), teal, root)
    cylinder("prop_a2_range_hero_01_lod0_target_disc_outer", (0.0, -0.040, 1.120), 0.235, 0.050, dark, root, vertices=28, rotation=(math.pi / 2.0, 0.0, 0.0))
    cylinder("prop_a2_range_hero_01_lod0_target_disc_teal", (0.0, -0.070, 1.120), 0.160, 0.026, teal, root, vertices=28, rotation=(math.pi / 2.0, 0.0, 0.0))
    cylinder("prop_a2_range_hero_01_lod0_target_disc_center", (0.0, -0.090, 1.120), 0.060, 0.024, orange, root, vertices=20, rotation=(math.pi / 2.0, 0.0, 0.0))
    cube("prop_a2_range_hero_01_lod0_holo_panel", (0.0, -0.190, 1.330), (0.520, 0.018, 0.220), glass, root)
    cube("prop_a2_range_hero_01_lod0_holo_panel_top", (0.0, -0.205, 1.455), (0.540, 0.018, 0.030), teal, root)
    cube("prop_a2_range_hero_01_lod0_side_handle_l", (-0.315, 0.0, 0.530), (0.060, 0.160, 0.250), dark, root)
    cube("prop_a2_range_hero_01_lod0_side_handle_r", (0.315, 0.0, 0.530), (0.060, 0.160, 0.250), dark, root)
    cube("prop_a2_range_hero_01_lod1_blockout", (0.0, 0.0, 0.720), (0.760, 0.470, 1.360), base, root)

    empty_socket("socket_interact", (0.0, -0.180, 0.735), root)
    empty_socket("socket_target_center", (0.0, -0.120, 1.120), root)
    empty_socket("socket_vfx", (0.0, -0.210, 1.350), root)
    empty_socket("socket_snap_center", (0.0, 0.0, 0.0), root, 0.10)


ASSETS: tuple[AssetSpec, ...] = (
    AssetSpec(
        asset_id="wpn_a2_blackout_carbine_01",
        category="weapon",
        tags=("all", "weapon", "weapons", "carbine", "blackout", "wpn_a2_blackout_carbine_01"),
        target_dimensions_m=(0.19, 1.31, 0.55),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Compact blackout-style generic carbine blockout with suppressed-looking front mass, micro optic, magazine, and orange/teal inspection marks.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_optic", "socket_mag", "socket_eject", "socket_vfx"),
        socket_notes={
            "socket_muzzle": "Visual muzzle/VFX reference, points along Blender +Y.",
            "socket_grip_r": "Approximate right-hand grip anchor for first-person/world-model tests.",
            "socket_grip_l": "Front support hand anchor.",
            "socket_optic": "Top attachment/reference point for sight alignment.",
            "socket_mag": "Magazine pickup/reload reference point.",
            "socket_eject": "Right-side casing/VFX reference.",
            "socket_vfx": "Alias for muzzle effect placement.",
        },
        notes=("original_generic_dev_art", "fps_visible_testasset", "orange_teal_accents", "no_brand_or_real_functional_details"),
        collision="none",
        lods=("wpn_a2_blackout_carbine_01_lod0", "wpn_a2_blackout_carbine_01_lod1"),
        build=build_blackout_carbine,
    ),
    AssetSpec(
        asset_id="wpn_a2_modular_rifle_01",
        category="weapon",
        tags=("all", "weapon", "weapons", "rifle", "assault_rifle", "modular", "wpn_a2_modular_rifle_01"),
        target_dimensions_m=(0.20, 1.63, 0.64),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Generic modular assault-rifle silhouette with long rail, optic mass, side slots, foregrip, and readable attachment sockets.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_optic", "socket_underbarrel", "socket_mag", "socket_eject", "socket_vfx"),
        socket_notes={
            "socket_muzzle": "Visual muzzle/VFX reference, points along Blender +Y.",
            "socket_grip_r": "Approximate right-hand grip anchor.",
            "socket_grip_l": "Angled foregrip/support hand anchor.",
            "socket_optic": "Optic/sight alignment reference.",
            "socket_underbarrel": "Generic lower attachment test socket.",
            "socket_mag": "Magazine pickup/reload reference point.",
            "socket_eject": "Right-side casing/VFX reference.",
            "socket_vfx": "Alias for muzzle effect placement.",
        },
        notes=("original_generic_dev_art", "fps_visible_testasset", "modular_attachment_language", "orange_teal_accents"),
        collision="none",
        lods=("wpn_a2_modular_rifle_01_lod0", "wpn_a2_modular_rifle_01_lod1"),
        build=build_modular_rifle,
    ),
    AssetSpec(
        asset_id="wpn_a2_striker_sidearm_01",
        category="weapon",
        tags=("all", "weapon", "weapons", "sidearm", "striker", "pistol", "wpn_a2_striker_sidearm_01"),
        target_dimensions_m=(0.13, 0.47, 0.38),
        origin_note="Origin at approximate right-hand grip/root; Blender forward axis is +Y.",
        description="Compact generic striker-style sidearm blockout with slide/frame separation, underbarrel rail, sights, and color-coded socket marks.",
        sockets=("socket_muzzle", "socket_grip_r", "socket_grip_l", "socket_optic", "socket_rail", "socket_eject", "socket_vfx"),
        socket_notes={
            "socket_muzzle": "Visual muzzle/VFX reference, points along Blender +Y.",
            "socket_grip_r": "Right-hand grip anchor.",
            "socket_grip_l": "Support hand or inspection anchor.",
            "socket_optic": "Tiny top sight/optic alignment reference.",
            "socket_rail": "Lower rail attachment test socket.",
            "socket_eject": "Right-side casing/VFX reference.",
            "socket_vfx": "Alias for muzzle effect placement.",
        },
        notes=("original_generic_dev_art", "fps_visible_testasset", "compact_sidearm", "orange_teal_accents"),
        collision="none",
        lods=("wpn_a2_striker_sidearm_01_lod0", "wpn_a2_striker_sidearm_01_lod1"),
        build=build_striker_sidearm,
    ),
    AssetSpec(
        asset_id="chr_a2_pilot_operator_01",
        category="character",
        tags=("all", "character", "characters", "operator", "pilot", "proxy", "chr_a2_pilot_operator_01"),
        target_dimensions_m=(1.02, 0.61, 1.84),
        origin_note="Origin at feet/root on the floor.",
        description="Sci-fi pilot/operator proxy with helmet visor, harness marks, mobility pack, humanoid rig, hitbox guides, and gameplay sockets.",
        sockets=("socket_root", "socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l", "socket_head", "socket_backpack", "socket_vfx"),
        socket_notes={
            "socket_root": "Feet/root placement point.",
            "socket_camera": "Approximate eye/camera reference.",
            "socket_weapon_root": "Weapon attachment reference.",
            "socket_hand_r": "Right hand reference point.",
            "socket_hand_l": "Left hand reference point.",
            "socket_head": "Head target/attachment reference.",
            "socket_backpack": "Mobility pack attachment/VFX reference.",
            "socket_vfx": "Pack thruster/VFX reference.",
        },
        notes=("original_generic_dev_art", "pilot_operator_proxy", "humanoid_rig", "hitbox_guides", "orange_teal_accents"),
        collision="col_chr_a2_pilot_operator_01_capsule",
        lods=("chr_a2_pilot_operator_01_lod0",),
        build=build_pilot_operator,
    ),
    AssetSpec(
        asset_id="map_a2_wallrun_panel_01",
        category="map",
        tags=("all", "map", "maps", "environment", "wall", "wall_run", "wallrun", "panel", "map_a2_wallrun_panel_01"),
        target_dimensions_m=(4.08, 0.22, 2.66),
        origin_note="Origin at bottom center of the wall footprint; snaps on a 4 m lane module.",
        description="Wallrun panel module with teal runnable lane markers, orange snap marks, and left/right connection sockets.",
        sockets=("socket_snap_left", "socket_snap_right", "socket_wallrun_start", "socket_wallrun_end", "socket_vfx"),
        socket_notes={
            "socket_snap_left": "Left grid connection point.",
            "socket_snap_right": "Right grid connection point.",
            "socket_wallrun_start": "Suggested wall-run entry reference.",
            "socket_wallrun_end": "Suggested wall-run exit reference.",
            "socket_vfx": "Center lane effect/reference point.",
        },
        notes=("wall_run", "4m_grid_module", "movement_readability", "orange_teal_accents"),
        collision="visual_only_use_simple_wall_collision",
        lods=("map_a2_wallrun_panel_01_lod0", "map_a2_wallrun_panel_01_lod1"),
        build=build_wallrun_panel,
    ),
    AssetSpec(
        asset_id="map_a2_slide_ramp_01",
        category="map",
        tags=("all", "map", "maps", "environment", "ramp", "slide", "slide_ramp", "map_a2_slide_ramp_01"),
        target_dimensions_m=(4.08, 2.35, 0.98),
        origin_note="Origin centered on the lower ramp footprint; top snap is 0.92 m above floor.",
        description="Slide ramp module with a low readable slope, teal flow strips, orange entry/exit bars, and snap sockets.",
        sockets=("socket_snap_bottom", "socket_snap_top", "socket_slide_entry", "socket_slide_exit", "socket_vfx"),
        socket_notes={
            "socket_snap_bottom": "Lower grid connection point.",
            "socket_snap_top": "Upper grid connection point.",
            "socket_slide_entry": "Suggested slide entry reference.",
            "socket_slide_exit": "Suggested slide exit reference.",
            "socket_vfx": "Center movement effect/reference point.",
        },
        notes=("slide", "movement_readability", "4m_grid_module", "orange_teal_accents"),
        collision="visual_only_use_simple_ramp_collision",
        lods=("map_a2_slide_ramp_01_lod0", "map_a2_slide_ramp_01_lod1"),
        build=build_slide_ramp,
    ),
    AssetSpec(
        asset_id="map_a2_cover_crate_01",
        category="map",
        tags=("all", "map", "maps", "environment", "cover", "crate", "module", "map_a2_cover_crate_01"),
        target_dimensions_m=(1.70, 1.30, 1.31),
        origin_note="Origin at bottom center; height is chest-cover scale for FPS testing.",
        description="Chest-high cover crate/module with readable cover mark, trim, teal corners, and snap/cover sockets.",
        sockets=("socket_snap_center", "socket_cover_peak", "socket_vfx"),
        socket_notes={
            "socket_snap_center": "Grid placement reference.",
            "socket_cover_peak": "Peek/cover height reference on the front face.",
            "socket_vfx": "Front cover mark effect/reference point.",
        },
        notes=("cover", "chest_high", "arena_module", "orange_teal_accents"),
        collision="visual_only_use_box_collision",
        lods=("map_a2_cover_crate_01_lod0", "map_a2_cover_crate_01_lod1"),
        build=build_cover_crate,
    ),
    AssetSpec(
        asset_id="prop_a2_range_hero_01",
        category="prop",
        tags=("all", "map", "maps", "prop", "props", "range", "hero_prop", "target", "prop_a2_range_hero_01"),
        target_dimensions_m=(0.78, 0.50, 1.47),
        origin_note="Origin at floor center of the prop footprint.",
        description="Small range hero prop: a generic scoring/target beacon with teal disc, orange control panel, and holographic sight plane.",
        sockets=("socket_interact", "socket_target_center", "socket_vfx", "socket_snap_center"),
        socket_notes={
            "socket_interact": "Player interaction/UI reference.",
            "socket_target_center": "Hitscan target center reference.",
            "socket_vfx": "Holographic panel effect/reference point.",
            "socket_snap_center": "Grid placement reference.",
        },
        notes=("range_hero_prop", "target_center", "interactive_test_prop", "orange_teal_accents"),
        collision="visual_only_use_simple_cylinder_or_box_collision",
        lods=("prop_a2_range_hero_01_lod0", "prop_a2_range_hero_01_lod1"),
        build=build_range_hero_prop,
    ),
)


def selected_assets(only: str) -> list[AssetSpec]:
    return [spec for spec in ASSETS if only == spec.asset_id or only in spec.tags]


def write_pack_manifest(records: list[dict[str, object]]) -> None:
    ensure_dirs(OUTPUT_ROOT)
    manifest = {
        "pack_id": "nemisis_a2_visual_pack",
        "generated_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "generator": GENERATOR,
        "blender_version": bpy.app.version_string,
        "units": "meters",
        "source_root": rel(SOURCE_ROOT),
        "output_root": rel(OUTPUT_ROOT),
        "external_assets": False,
        "asset_count": len(records),
        "assets": records,
    }
    manifest_path = OUTPUT_ROOT / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Wrote {rel(manifest_path)}")


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
