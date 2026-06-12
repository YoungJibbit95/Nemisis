"""Render a deterministic preview sheet for the Nemisis A2 visual pack.

Run from the Nemisis repository root:
    blender --background --python tools/blender/render_a2_preview.py

This imports the generated GLB files, arranges them on a simple studio floor,
and writes a PNG preview beside the pack manifest. It is intentionally small
and dependency-free so Codex or another asset agent can run it headlessly.
"""

from __future__ import annotations

import argparse
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
PACK_ROOT = REPO_ROOT / "assets" / "generated" / "a2_visual_pack"
MANIFEST_PATH = PACK_ROOT / "manifest.json"
DEFAULT_OUTPUT = PACK_ROOT / "a2_visual_pack_preview.png"


def parse_args() -> argparse.Namespace:
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Render the Nemisis A2 visual pack preview.")
    parser.add_argument("--manifest", default=str(MANIFEST_PATH), help="Path to the A2 manifest JSON.")
    parser.add_argument("--output", default=str(DEFAULT_OUTPUT), help="PNG output path.")
    parser.add_argument("--resolution", type=int, default=1800, help="Square preview resolution in pixels.")
    return parser.parse_args(argv)


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for collection in (
        bpy.data.meshes,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.cameras,
        bpy.data.lights,
        bpy.data.curves,
    ):
        for datablock in list(collection):
            collection.remove(datablock)


def make_material(name: str, color: tuple[float, float, float, float], roughness: float = 0.82) -> bpy.types.Material:
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    principled = material.node_tree.nodes.get("Principled BSDF")
    if principled:
        principled.inputs["Base Color"].default_value = color
        principled.inputs["Roughness"].default_value = roughness
        principled.inputs["Metallic"].default_value = 0.05
    return material


def imported_world_bounds(objects: list[bpy.types.Object]) -> tuple[Vector, Vector]:
    mins = Vector((math.inf, math.inf, math.inf))
    maxs = Vector((-math.inf, -math.inf, -math.inf))
    found = False

    for obj in objects:
        if obj.type not in {"MESH", "EMPTY", "ARMATURE"}:
            continue
        corners = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
        for corner in corners:
            mins.x = min(mins.x, corner.x)
            mins.y = min(mins.y, corner.y)
            mins.z = min(mins.z, corner.z)
            maxs.x = max(maxs.x, corner.x)
            maxs.y = max(maxs.y, corner.y)
            maxs.z = max(maxs.z, corner.z)
            found = True

    if not found:
        return Vector((-0.5, -0.5, 0.0)), Vector((0.5, 0.5, 1.0))
    return mins, maxs


def translate_objects(objects: list[bpy.types.Object], delta: Vector) -> None:
    for obj in objects:
        if obj.parent is None:
            obj.location += delta


def scale_objects(objects: list[bpy.types.Object], factor: float) -> None:
    for obj in objects:
        if obj.parent is None:
            obj.scale = (obj.scale.x * factor, obj.scale.y * factor, obj.scale.z * factor)


def add_label(text: str, location: tuple[float, float, float]) -> None:
    bpy.ops.object.text_add(location=location, rotation=(math.radians(64.0), 0.0, 0.0))
    label = bpy.context.object
    label.name = f"label_{text}"
    label.data.body = text
    label.data.align_x = "CENTER"
    label.data.align_y = "CENTER"
    label.data.size = 0.11
    label.data.materials.append(make_material(f"mat_label_{text}", (0.82, 0.9, 0.92, 1.0), 0.75))


def add_floor() -> None:
    floor_mat = make_material("mat_preview_floor", (0.05, 0.057, 0.064, 1.0), 0.9)
    stripe_teal = make_material("mat_preview_teal_stripe", (0.0, 0.58, 0.62, 1.0), 0.72)
    stripe_orange = make_material("mat_preview_orange_stripe", (0.95, 0.42, 0.12, 1.0), 0.72)

    bpy.ops.mesh.primitive_cube_add(size=1.0, location=(0.0, 0.0, -0.045))
    floor = bpy.context.object
    floor.name = "preview_floor"
    floor.dimensions = (15.5, 11.0, 0.08)
    floor.data.materials.append(floor_mat)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    for idx, x in enumerate((-6.8, 0.0, 6.8)):
        bpy.ops.mesh.primitive_cube_add(size=1.0, location=(x, -4.35, 0.005))
        stripe = bpy.context.object
        stripe.name = f"preview_floor_stripe_{idx}"
        stripe.dimensions = (3.5, 0.045, 0.02)
        stripe.data.materials.append(stripe_orange if idx == 1 else stripe_teal)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)


def setup_lighting_and_camera(resolution: int) -> None:
    bpy.ops.object.light_add(type="AREA", location=(-3.7, -5.0, 7.0))
    key = bpy.context.object
    key.name = "preview_key_light"
    key.data.energy = 560.0
    key.data.size = 5.0

    bpy.ops.object.light_add(type="POINT", location=(5.8, 2.4, 3.2))
    rim = bpy.context.object
    rim.name = "preview_teal_rim_light"
    rim.data.energy = 170.0
    rim.data.color = (0.15, 0.92, 1.0)

    bpy.ops.object.camera_add(location=(0.0, -13.0, 8.0), rotation=(math.radians(58.0), 0.0, 0.0))
    camera = bpy.context.object
    camera.name = "preview_camera"
    camera.data.type = "ORTHO"
    camera.data.ortho_scale = 13.0
    bpy.context.scene.camera = camera

    supported_engines = {item.identifier for item in bpy.context.scene.render.bl_rna.properties["engine"].enum_items}
    bpy.context.scene.render.engine = "BLENDER_EEVEE_NEXT" if "BLENDER_EEVEE_NEXT" in supported_engines else "BLENDER_EEVEE"
    if hasattr(bpy.context.scene, "eevee"):
        bpy.context.scene.eevee.taa_render_samples = 64
    bpy.context.scene.world = bpy.data.worlds.new("preview_world")
    bpy.context.scene.world.color = (0.015, 0.018, 0.022)
    bpy.context.scene.render.resolution_x = resolution
    bpy.context.scene.render.resolution_y = resolution
    bpy.context.scene.view_settings.view_transform = "Filmic"
    bpy.context.scene.view_settings.look = "Medium High Contrast"
    bpy.context.scene.view_settings.exposure = 0.0
    bpy.context.scene.view_settings.gamma = 1.0


def load_manifest(path: Path) -> list[dict[str, object]]:
    if not path.exists():
        raise SystemExit(f"Missing manifest: {path}")
    manifest = json.loads(path.read_text(encoding="utf-8"))
    assets = manifest.get("assets", [])
    if not isinstance(assets, list) or not assets:
        raise SystemExit(f"Manifest has no assets: {path}")
    return assets


def import_asset(asset: dict[str, object], location: tuple[float, float, float], max_footprint: float) -> None:
    asset_id = str(asset["id"])
    file_path = REPO_ROOT / str(asset["file"])
    if not file_path.exists():
        raise SystemExit(f"Missing GLB for {asset_id}: {file_path}")

    before = set(bpy.context.scene.objects)
    bpy.ops.import_scene.gltf(filepath=str(file_path))
    imported = [obj for obj in bpy.context.scene.objects if obj not in before]
    for obj in imported:
        obj.name = f"{asset_id}_{obj.name}"

    mins, maxs = imported_world_bounds(imported)
    size = maxs - mins
    footprint = max(size.x, size.y, 0.001)
    height = max(size.z, 0.001)
    scale = min(max_footprint / footprint, 2.0 / height, 1.35)
    scale_objects(imported, scale)

    mins, maxs = imported_world_bounds(imported)
    center = (mins + maxs) * 0.5
    delta = Vector((location[0], location[1], location[2])) - Vector((center.x, center.y, mins.z))
    translate_objects(imported, delta)

    display_name = asset_id.replace("_01", "").replace("wpn_a2_", "").replace("map_a2_", "").replace("chr_a2_", "").replace("prop_a2_", "")
    add_label(display_name.replace("_", " "), (location[0], location[1] - 1.12, 0.035))


def main() -> None:
    args = parse_args()
    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)

    clear_scene()
    add_floor()

    assets = load_manifest(Path(args.manifest))
    positions = [
        (-4.6, 2.25, 0.0),
        (-1.55, 2.25, 0.0),
        (1.55, 2.25, 0.0),
        (4.55, 2.15, 0.0),
        (-4.6, -2.15, 0.0),
        (-1.55, -2.15, 0.0),
        (1.55, -2.15, 0.0),
        (4.55, -2.15, 0.0),
    ]

    for asset, location in zip(assets, positions, strict=True):
        category = str(asset.get("category", ""))
        max_footprint = 2.25 if category == "character" else 2.9
        import_asset(asset, location, max_footprint)

    setup_lighting_and_camera(args.resolution)
    bpy.context.scene.render.filepath = str(output_path)
    bpy.ops.wm.save_as_mainfile(filepath=str(PACK_ROOT / "a2_visual_pack_preview.blend"))
    bpy.ops.render.render(write_still=True)
    print(f"Rendered A2 visual pack preview: {output_path}")


if __name__ == "__main__":
    main()
