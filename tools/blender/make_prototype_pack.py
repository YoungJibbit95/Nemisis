"""Generate a small procedural prototype asset pack for Nemisis.

Run from the Nemisis repository root:
    blender --background --python tools/blender/make_prototype_pack.py

The script writes GLB files, a manifest, and a README under:
    assets/generated/prototype_pack
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
except ModuleNotFoundError as exc:
    raise SystemExit("Run this script with Blender, not plain Python.") from exc


REPO_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_ROOT = REPO_ROOT / "assets" / "generated" / "prototype_pack"


@dataclass(frozen=True)
class AssetSpec:
    asset_id: str
    filename: str
    category: str
    tags: tuple[str, ...]
    dimensions_m: tuple[float, float, float]
    origin_note: str
    description: str
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

    parser = argparse.ArgumentParser(description="Generate Nemisis prototype GLB assets.")
    parser.add_argument(
        "--only",
        default="all",
        choices=(
            "all",
            "weapon",
            "weapons",
            "smg",
            "character",
            "characters",
            "humanoid",
            "map",
            "maps",
            "environment",
            "wall",
            "floor",
            "crate",
            "cover",
            "ramp",
            "target",
            "target_stand",
        ),
        help="Generate one group or one asset.",
    )
    parser.add_argument(
        "--output",
        default=str(OUTPUT_ROOT),
        help="Output directory. Defaults to assets/generated/prototype_pack.",
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


def material(name: str, color: tuple[float, float, float, float]) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    mat.diffuse_color = color
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        if "Base Color" in bsdf.inputs:
            bsdf.inputs["Base Color"].default_value = color
        if "Alpha" in bsdf.inputs:
            bsdf.inputs["Alpha"].default_value = color[3]
        if "Roughness" in bsdf.inputs:
            bsdf.inputs["Roughness"].default_value = 0.76
        if "Metallic" in bsdf.inputs:
            bsdf.inputs["Metallic"].default_value = 0.0
    return mat


def root_empty(name: str) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    obj.empty_display_type = "PLAIN_AXES"
    obj.empty_display_size = 0.25
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
    vertices: int = 12,
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


def sphere(
    name: str,
    location: tuple[float, float, float],
    radius: float,
    mat: bpy.types.Material,
    parent: bpy.types.Object,
    scale: tuple[float, float, float] = (1.0, 1.0, 1.0),
    segments: int = 12,
    rings: int = 6,
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


def empty_socket(name: str, location: tuple[float, float, float], parent: bpy.types.Object) -> bpy.types.Object:
    obj = bpy.data.objects.new(name, None)
    obj.empty_display_type = "SPHERE"
    obj.empty_display_size = 0.08
    obj.location = location
    bpy.context.collection.objects.link(obj)
    return parent_to(obj, parent)


def build_proto_smg() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("wpn_proto_smg_01")

    dark = material("mat_smg_dark_polymer", (0.055, 0.06, 0.065, 1.0))
    metal = material("mat_smg_gunmetal", (0.32, 0.34, 0.35, 1.0))
    edge = material("mat_smg_control_teal", (0.1, 0.65, 0.72, 1.0))
    orange = material("mat_smg_safety_orange", (0.95, 0.38, 0.08, 1.0))

    cube("geo_receiver", (0.08, 0.0, 0.16), (0.38, 0.09, 0.12), metal, root)
    cube("geo_upper_rail", (0.12, 0.0, 0.245), (0.34, 0.055, 0.035), dark, root)
    cube("geo_stock", (-0.26, 0.0, 0.17), (0.24, 0.065, 0.085), dark, root)
    cube("geo_stock_pad", (-0.405, 0.0, 0.165), (0.045, 0.09, 0.13), dark, root)
    cylinder("geo_barrel", (0.39, 0.0, 0.18), 0.025, 0.34, metal, root, vertices=14, rotation=(0.0, math.pi / 2.0, 0.0))
    cylinder("geo_muzzle", (0.58, 0.0, 0.18), 0.036, 0.085, dark, root, vertices=14, rotation=(0.0, math.pi / 2.0, 0.0))
    cube("geo_front_handguard", (0.31, 0.0, 0.15), (0.22, 0.105, 0.09), dark, root)
    cube("geo_grip", (-0.055, 0.0, 0.0), (0.075, 0.07, 0.2), dark, root, rotation=(0.0, math.radians(-8.0), 0.0))
    cube("geo_magazine", (0.105, 0.0, -0.045), (0.085, 0.064, 0.23), dark, root, rotation=(0.0, math.radians(5.0), 0.0))
    cube("geo_trigger_guard_top", (0.015, 0.0, 0.065), (0.13, 0.025, 0.024), dark, root)
    cube("geo_trigger_guard_front", (0.075, 0.0, 0.025), (0.024, 0.025, 0.085), dark, root)
    cube("geo_trigger_guard_back", (-0.045, 0.0, 0.025), (0.024, 0.025, 0.085), dark, root)
    cube("geo_trigger", (0.025, -0.004, 0.025), (0.026, 0.018, 0.075), orange, root, rotation=(0.0, math.radians(-14.0), 0.0))
    cube("geo_rear_sight", (-0.055, 0.0, 0.292), (0.045, 0.07, 0.055), edge, root)
    cube("geo_front_sight", (0.315, 0.0, 0.284), (0.035, 0.06, 0.055), edge, root)
    cube("geo_mag_release_mark", (0.17, -0.048, 0.118), (0.045, 0.008, 0.032), orange, root)

    empty_socket("socket_grip", (-0.055, 0.0, 0.0), root)
    empty_socket("socket_muzzle", (0.63, 0.0, 0.18), root)
    empty_socket("socket_mag", (0.105, 0.0, -0.16), root)


def build_humanoid_proxy() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("chr_proto_humanoid_01")

    suit = material("mat_humanoid_suit_bluegrey", (0.24, 0.33, 0.42, 1.0))
    joint = material("mat_humanoid_joint_dark", (0.08, 0.095, 0.11, 1.0))
    skin = material("mat_humanoid_head_warm", (0.72, 0.58, 0.44, 1.0))
    accent = material("mat_humanoid_front_marker", (0.05, 0.82, 0.68, 1.0))

    cylinder("geo_torso", (0.0, 0.0, 1.08), 0.24, 0.68, suit, root, vertices=10)
    cube("geo_shoulder_bar", (0.0, -0.005, 1.38), (0.72, 0.22, 0.12), suit, root)
    cube("geo_pelvis", (0.0, 0.0, 0.76), (0.42, 0.2, 0.18), suit, root)
    cylinder("geo_neck", (0.0, 0.0, 1.49), 0.075, 0.13, joint, root, vertices=10)
    sphere("geo_head", (0.0, -0.015, 1.67), 0.17, skin, root, scale=(0.92, 0.86, 1.08), segments=12, rings=6)
    cube("geo_face_forward_mark", (0.0, -0.15, 1.66), (0.12, 0.015, 0.055), accent, root)
    cube("geo_chest_forward_mark", (0.0, -0.235, 1.18), (0.18, 0.018, 0.24), accent, root)

    for side, x in (("l", -0.43), ("r", 0.43)):
        cylinder(f"geo_{side}_upper_arm", (x, 0.0, 1.12), 0.065, 0.48, suit, root, vertices=10)
        cylinder(f"geo_{side}_forearm", (x, 0.0, 0.76), 0.058, 0.4, suit, root, vertices=10)
        sphere(f"geo_{side}_hand", (x, -0.005, 0.52), 0.075, joint, root, scale=(0.82, 0.8, 1.0), segments=10, rings=5)
        cylinder(f"geo_{side}_thigh", (x * 0.34, 0.0, 0.48), 0.08, 0.5, suit, root, vertices=10)
        cylinder(f"geo_{side}_shin", (x * 0.34, 0.0, 0.18), 0.07, 0.36, suit, root, vertices=10)
        cube(f"geo_{side}_foot", (x * 0.34, -0.08, 0.045), (0.17, 0.32, 0.09), joint, root)

    empty_socket("socket_root", (0.0, 0.0, 0.0), root)
    empty_socket("socket_head", (0.0, 0.0, 1.67), root)


def build_wall_panel() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_wall_panel_01")

    concrete = material("mat_wall_concrete_grey", (0.48, 0.5, 0.5, 1.0))
    trim = material("mat_wall_dark_trim", (0.12, 0.13, 0.14, 1.0))
    stripe = material("mat_wall_orange_id", (0.95, 0.43, 0.09, 1.0))

    cube("geo_wall_slab", (0.0, 0.0, 1.25), (4.0, 0.22, 2.5), concrete, root)
    cube("geo_top_cap", (0.0, -0.125, 2.54), (4.08, 0.08, 0.08), trim, root)
    cube("geo_bottom_cap", (0.0, -0.125, 0.04), (4.08, 0.08, 0.08), trim, root)
    for x in (-1.95, 1.95):
        cube(f"geo_side_trim_{x:+.0f}", (x, -0.13, 1.25), (0.08, 0.08, 2.52), trim, root)
    for x in (-1.2, 0.0, 1.2):
        cube(f"geo_vertical_panel_line_{x:+.1f}", (x, -0.135, 1.25), (0.035, 0.035, 2.18), trim, root)
    cube("geo_front_hazard_stripe", (0.0, -0.155, 0.36), (3.3, 0.025, 0.08), stripe, root)


def build_floor_tile() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_floor_tile_01")

    base = material("mat_floor_warm_grey", (0.42, 0.43, 0.4, 1.0))
    groove = material("mat_floor_groove_dark", (0.1, 0.105, 0.1, 1.0))
    edge = material("mat_floor_edge_blue", (0.08, 0.35, 0.75, 1.0))

    cube("geo_floor_slab", (0.0, 0.0, -0.04), (4.0, 4.0, 0.08), base, root)
    for x in (-1.0, 0.0, 1.0):
        cube(f"geo_floor_groove_x_{x:+.1f}", (x, 0.0, 0.012), (0.035, 4.02, 0.012), groove, root)
    for y in (-1.0, 0.0, 1.0):
        cube(f"geo_floor_groove_y_{y:+.1f}", (0.0, y, 0.014), (4.02, 0.035, 0.012), groove, root)
    cube("geo_floor_front_edge", (0.0, -2.03, 0.025), (4.05, 0.045, 0.035), edge, root)


def build_cover_crate() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_cover_crate_01")

    body = material("mat_crate_body_green", (0.24, 0.33, 0.25, 1.0))
    trim = material("mat_crate_trim_dark", (0.09, 0.1, 0.09, 1.0))
    mark = material("mat_crate_mark_yellow", (0.95, 0.82, 0.22, 1.0))

    cube("geo_crate_core", (0.0, 0.0, 0.5), (1.2, 1.2, 1.0), body, root)
    for z in (0.08, 0.92):
        cube(f"geo_crate_band_z_{z:.2f}", (0.0, -0.62, z), (1.25, 0.055, 0.08), trim, root)
        cube(f"geo_crate_band_back_z_{z:.2f}", (0.0, 0.62, z), (1.25, 0.055, 0.08), trim, root)
    for x in (-0.56, 0.56):
        cube(f"geo_crate_front_upright_{x:+.2f}", (x, -0.63, 0.5), (0.075, 0.05, 0.92), trim, root)
    cube("geo_crate_front_diag_a", (0.0, -0.655, 0.5), (1.35, 0.04, 0.07), trim, root, rotation=(0.0, math.radians(34.0), 0.0))
    cube("geo_crate_front_diag_b", (0.0, -0.66, 0.5), (1.35, 0.04, 0.07), trim, root, rotation=(0.0, math.radians(-34.0), 0.0))
    cube("geo_crate_cover_mark", (0.0, -0.685, 0.78), (0.42, 0.03, 0.09), mark, root)


def build_ramp() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_ramp_01")

    ramp_mat = material("mat_ramp_plate_grey", (0.38, 0.39, 0.41, 1.0))
    trim = material("mat_ramp_edge_dark", (0.08, 0.085, 0.09, 1.0))
    stripe = material("mat_ramp_white_mark", (0.86, 0.86, 0.82, 1.0))

    length = 3.0
    width = 2.0
    height = 1.0
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
    mesh_object("geo_ramp_wedge", verts, faces, ramp_mat, root)

    angle = -math.atan2(height, length)
    for x in (-0.65, 0.15, 0.95):
        z = (x + length / 2.0) / length * height + 0.035
        cube(f"geo_ramp_traction_{x:+.2f}", (x, 0.0, z), (0.05, 1.72, 0.025), stripe, root, rotation=(0.0, angle, 0.0))
    cube("geo_ramp_left_edge", (0.1, -1.03, 0.55), (3.08, 0.08, 0.08), trim, root, rotation=(0.0, angle, 0.0))
    cube("geo_ramp_right_edge", (0.1, 1.03, 0.55), (3.08, 0.08, 0.08), trim, root, rotation=(0.0, angle, 0.0))


def build_target_stand() -> None:
    clear_scene()
    configure_scene()
    root = root_empty("map_target_stand_01")

    stand = material("mat_target_stand_dark", (0.11, 0.1, 0.09, 1.0))
    plate = material("mat_target_plate_light", (0.78, 0.76, 0.7, 1.0))
    red = material("mat_target_red_zone", (0.92, 0.12, 0.09, 1.0))
    black = material("mat_target_black_zone", (0.03, 0.03, 0.03, 1.0))

    cube("geo_stand_base", (0.0, 0.0, 0.04), (0.95, 0.7, 0.08), stand, root)
    cube("geo_stand_crossbar", (0.0, -0.02, 0.5), (0.72, 0.08, 0.07), stand, root)
    for x in (-0.28, 0.28):
        cube(f"geo_stand_post_{x:+.2f}", (x, 0.0, 0.58), (0.07, 0.07, 1.08), stand, root)
    cube("geo_target_torso_plate", (0.0, -0.07, 1.02), (0.55, 0.055, 0.68), plate, root)
    cylinder("geo_target_head_plate", (0.0, -0.075, 1.5), 0.22, 0.055, plate, root, vertices=18, rotation=(math.pi / 2.0, 0.0, 0.0))
    cylinder("geo_target_outer_ring", (0.0, -0.111, 1.03), 0.18, 0.018, black, root, vertices=18, rotation=(math.pi / 2.0, 0.0, 0.0))
    cylinder("geo_target_red_center", (0.0, -0.125, 1.03), 0.095, 0.018, red, root, vertices=18, rotation=(math.pi / 2.0, 0.0, 0.0))


ASSETS: tuple[AssetSpec, ...] = (
    AssetSpec(
        "wpn_proto_smg_01",
        "wpn_proto_smg_01.glb",
        "weapon",
        ("all", "weapon", "weapons", "smg"),
        (1.07, 0.11, 0.45),
        "Origin at approximate right-hand grip center; forward axis is +X.",
        "Low-poly SMG/rifle proxy with barrel, stock, magazine, sights, and sockets.",
        build_proto_smg,
    ),
    AssetSpec(
        "chr_proto_humanoid_01",
        "chr_proto_humanoid_01.glb",
        "character",
        ("all", "character", "characters", "humanoid"),
        (1.03, 0.42, 1.84),
        "Origin at feet midpoint on the floor.",
        "Simplified humanoid proxy with clear forward marker and head/root sockets.",
        build_humanoid_proxy,
    ),
    AssetSpec(
        "map_wall_panel_01",
        "map_wall_panel_01.glb",
        "map",
        ("all", "map", "maps", "environment", "wall"),
        (4.08, 0.22, 2.58),
        "Origin at bottom center of the wall footprint.",
        "Straight wall module for greybox corridors and cover lanes.",
        build_wall_panel,
    ),
    AssetSpec(
        "map_floor_tile_01",
        "map_floor_tile_01.glb",
        "map",
        ("all", "map", "maps", "environment", "floor"),
        (4.05, 4.05, 0.08),
        "Origin at tile center on the top walking plane.",
        "Four-meter floor tile with shallow grid grooves and a blue front edge.",
        build_floor_tile,
    ),
    AssetSpec(
        "map_cover_crate_01",
        "map_cover_crate_01.glb",
        "map",
        ("all", "map", "maps", "environment", "crate", "cover"),
        (1.35, 1.2, 1.0),
        "Origin at bottom center.",
        "Chest-high cover crate with readable trim and front marking.",
        build_cover_crate,
    ),
    AssetSpec(
        "map_ramp_01",
        "map_ramp_01.glb",
        "map",
        ("all", "map", "maps", "environment", "ramp"),
        (3.08, 2.14, 1.0),
        "Origin centered on the lower ramp footprint.",
        "Simple one-meter-rise ramp wedge with edge rails and traction marks.",
        build_ramp,
    ),
    AssetSpec(
        "map_target_stand_01",
        "map_target_stand_01.glb",
        "map",
        ("all", "map", "maps", "environment", "target", "target_stand"),
        (0.95, 0.7, 1.72),
        "Origin at base center on the floor.",
        "Portable target stand for weapon and hit-test staging.",
        build_target_stand,
    ),
)


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


def export_glb(path: Path) -> None:
    ensure_dirs(path.parent)
    bpy.ops.export_scene.gltf(filepath=str(path), export_format="GLB")


def selected_assets(only: str) -> list[AssetSpec]:
    return [spec for spec in ASSETS if only in spec.tags]


def rel(path: Path) -> str:
    try:
        return path.relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def write_manifest(output_root: Path, records: list[dict[str, object]]) -> None:
    manifest = {
        "pack_id": "nemisis_prototype_pack_01",
        "generated_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
        "generator": "tools/blender/make_prototype_pack.py",
        "blender_version": bpy.app.version_string,
        "units": "meters",
        "external_assets": False,
        "assets": records,
    }
    (output_root / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


def write_readme(output_root: Path, records: list[dict[str, object]]) -> None:
    blender_path = bpy.app.binary_path or "blender"
    asset_lines = "\n".join(
        f"- `{record['file']}` - {record['description']}" for record in records
    )
    text = f"""# Nemisis Prototype Pack

Procedural greybox/test assets generated by `tools/blender/make_prototype_pack.py`.
No external assets were downloaded or embedded.

## Contents

{asset_lines}

## Scale And Origins

- Units are meters.
- Mesh transforms are applied; per-asset origins are described in `manifest.json`.
- Weapon forward axis is `+X`; map parts use floor contact at or near `Z=0`.
- The floor tile origin is on its top walking plane so it can be snapped at `Z=0`.

## Regenerate

From the Nemisis repository root:

```powershell
blender --background --python tools/blender/make_prototype_pack.py
```

If Blender is not on PATH, use the current local Blender binary:

```powershell
& "{blender_path}" --background --python tools/blender/make_prototype_pack.py
```

Generate only one group:

```powershell
blender --background --python tools/blender/make_prototype_pack.py -- --only weapon
blender --background --python tools/blender/make_prototype_pack.py -- --only character
blender --background --python tools/blender/make_prototype_pack.py -- --only map
```
"""
    (output_root / "README.md").write_text(text, encoding="utf-8")


def main() -> None:
    args = parse_args()
    output_root = Path(args.output).resolve()
    ensure_dirs(output_root)

    specs = selected_assets(args.only)
    if not specs:
        raise SystemExit(f"No assets matched --only {args.only!r}.")

    records: list[dict[str, object]] = []
    for spec in specs:
        spec.build()
        stats = mesh_stats()
        export_path = output_root / spec.filename
        export_glb(export_path)
        record = {
            "id": spec.asset_id,
            "category": spec.category,
            "file": rel(export_path),
            "dimensions_m": spec.dimensions_m,
            "origin": spec.origin_note,
            "description": spec.description,
            "stats": stats,
        }
        records.append(record)
        print(f"Wrote {rel(export_path)}")

    write_manifest(output_root, records)
    write_readme(output_root, records)
    print(f"Wrote {rel(output_root / 'manifest.json')}")
    print(f"Wrote {rel(output_root / 'README.md')}")


if __name__ == "__main__":
    main()
