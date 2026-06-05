"""Generate first Nemisis dev primitives in Blender.

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


def material(name: str, color: tuple[float, float, float, float]) -> bpy.types.Material:
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    if bsdf is not None:
        bsdf.inputs["Base Color"].default_value = color
        bsdf.inputs["Roughness"].default_value = 0.72
        bsdf.inputs["Metallic"].default_value = 0.0
    return mat


def assign_material(obj: bpy.types.Object, mat: bpy.types.Material) -> bpy.types.Object:
    obj.data.materials.append(mat)
    return obj


def apply_object_transform(obj: bpy.types.Object) -> bpy.types.Object:
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
) -> bpy.types.Object:
    bpy.ops.mesh.primitive_cube_add(size=1.0, location=location)
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


def build_arena_kit() -> None:
    clear_scene()
    floor = material("mat_env_grid_floor", (0.18, 0.19, 0.20, 1.0))
    line = material("mat_env_grid_line", (0.55, 0.58, 0.62, 1.0))
    wall = material("mat_env_wall_neutral", (0.24, 0.26, 0.28, 1.0))
    accent = material("mat_env_movement_accent", (0.05, 0.65, 0.90, 1.0))
    objective = material("mat_env_objective_accent", (0.95, 0.50, 0.12, 1.0))

    cube("env_test_floor_grid_01", (0.0, 0.0, -0.03), (10.0, 10.0, 0.03), floor)
    for index in range(-10, 11):
        width = 0.025 if index % 5 else 0.055
        cube(f"env_test_grid_x_{index:+03d}", (index, 0.0, 0.002), (width, 10.0, 0.01), line)
        cube(f"env_test_grid_z_{index:+03d}", (0.0, index, 0.003), (10.0, width, 0.01), line)

    cube("env_test_wall_01", (-4.0, 2.0, 1.0), (0.12, 2.0, 1.0), wall)
    cube("env_test_cover_low_01", (-1.5, 2.5, 0.45), (0.75, 0.35, 0.45), wall)
    cube("env_test_cover_high_01", (1.5, 2.5, 0.85), (0.75, 0.35, 0.85), wall)
    cube("env_test_mantle_ledge_080", (-2.5, -2.5, 0.40), (0.8, 0.45, 0.40), accent)
    cube("env_test_mantle_ledge_120", (0.0, -2.5, 0.60), (0.8, 0.45, 0.60), accent)
    cube("env_test_mantle_ledge_160", (2.5, -2.5, 0.80), (0.8, 0.45, 0.80), accent)
    cube("env_test_wallrun_panel_01", (4.0, 0.0, 1.1), (0.10, 2.3, 0.55), accent)
    cube("env_test_spawn_pad_01", (-3.0, -4.0, 0.03), (0.65, 0.65, 0.03), objective)
    cube("env_test_control_marker_01", (3.0, -4.0, 0.08), (0.45, 0.45, 0.08), objective)

    ramp = cube("env_test_slide_ramp_01", (0.0, 4.0, 0.35), (1.0, 1.2, 0.08), accent)
    ramp.rotation_euler[0] = math.radians(18.0)
    apply_object_transform(ramp)

    save_and_export(
        "env_test_arena_kit_01",
        "environments",
        [],
        "per_piece_col_prefix",
        [],
    )


def parse_only_arg() -> str:
    if "--" not in sys.argv:
        return "all"
    args = sys.argv[sys.argv.index("--") + 1 :]
    if "--only" not in args:
        return "all"
    index = args.index("--only")
    if index + 1 >= len(args):
        raise SystemExit("--only requires target, weapon, arena, or all")
    return args[index + 1]


def main() -> None:
    only = parse_only_arg()
    valid = {"all", "target", "weapon", "arena"}
    if only not in valid:
        raise SystemExit(f"Unknown --only value '{only}'. Expected one of: {', '.join(sorted(valid))}")

    if only in {"all", "target"}:
        build_target_dummy()
    if only in {"all", "weapon"}:
        build_ar_blockout()
    if only in {"all", "arena"}:
        build_arena_kit()


if __name__ == "__main__":
    main()
