"""Recook cataloged Blender source assets to their runtime GLB outputs.

Run from the Nemisis repository root:
    blender --background --factory-startup --python tools/blender/recook_blender_catalog_exports.py -- --repo-root . --tag weapon
"""

from __future__ import annotations

import argparse
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import bpy
    from mathutils import Vector
except ModuleNotFoundError as exc:
    raise SystemExit("Run this script with Blender, not plain Python.") from exc


DEFAULT_CATALOG = Path("configs/assets/nemisis_assets.json")
AXES = ("X", "Y", "Z")


def blender_args() -> list[str]:
    argv = sys.argv
    if "--" not in argv:
        return []
    return argv[argv.index("--") + 1 :]


def rel(root: Path, path: Path) -> str:
    return path.resolve().relative_to(root.resolve()).as_posix()


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def rounded(values: Vector | tuple[float, float, float]) -> list[float]:
    return [round(float(values[0]), 5), round(float(values[1]), 5), round(float(values[2]), 5)]


def dominant_axis(delta: list[float]) -> str | None:
    if max(abs(v) for v in delta) < 0.0001:
        return None
    axis_index = max(range(3), key=lambda index: abs(delta[index]))
    return ("+" if delta[axis_index] >= 0.0 else "-") + AXES[axis_index]


def axis_from_positions(positions: dict[str, list[float]]) -> str | None:
    muzzle = positions.get("socket_muzzle") or positions.get("socket_vfx")
    grip = positions.get("socket_grip_r") or positions.get("socket_weapon_root") or positions.get("socket_root")
    if muzzle is None or grip is None:
        return None
    return dominant_axis([muzzle[index] - grip[index] for index in range(3)])


def runtime_from_blender(value: Vector) -> list[float]:
    return [round(float(value.x), 5), round(float(value.z), 5), round(-float(value.y), 5)]


def collect_socket_positions() -> tuple[dict[str, list[float]], dict[str, list[float]]]:
    blender_positions: dict[str, list[float]] = {}
    runtime_positions: dict[str, list[float]] = {}
    for obj in bpy.context.scene.objects:
        if not obj.name.startswith("socket_"):
            continue
        world = obj.matrix_world.translation
        blender_positions[obj.name] = rounded(world)
        runtime_positions[obj.name] = runtime_from_blender(world)
    return blender_positions, runtime_positions


def scene_dimensions() -> list[float] | None:
    points: list[Vector] = []
    for obj in bpy.context.scene.objects:
        if obj.type != "MESH" or obj.name.startswith("col_"):
            continue
        for corner in obj.bound_box:
            points.append(obj.matrix_world @ Vector(corner))
    if not points:
        return None
    mins = Vector((min(p.x for p in points), min(p.y for p in points), min(p.z for p in points)))
    maxs = Vector((max(p.x for p in points), max(p.y for p in points), max(p.z for p in points)))
    return rounded(maxs - mins)


def should_recook(entry: dict[str, Any], args: argparse.Namespace) -> bool:
    asset_id = str(entry.get("id", ""))
    tags = set(entry.get("tags", []))
    source = str(entry.get("source", ""))
    cooked = str(entry.get("cooked", ""))
    if not asset_id or not source.endswith(".blend") or not cooked.endswith(".glb"):
        return False
    if args.asset and asset_id not in set(args.asset):
        return False
    if args.tag and not tags.intersection(args.tag):
        return False
    if args.source_prefix and not any(source.startswith(prefix) for prefix in args.source_prefix):
        return False
    return True


def recook_entry(root: Path, entry: dict[str, Any], args: argparse.Namespace) -> dict[str, Any]:
    source = root / entry["source"]
    cooked = root / entry["cooked"]
    metadata_path = cooked.with_suffix(".metadata.json")
    if not source.exists():
        return {"id": entry.get("id"), "status": "missing_source", "source": rel(root, source)}

    bpy.ops.wm.open_mainfile(filepath=str(source))
    if hasattr(bpy.context.preferences.filepaths, "save_version"):
        bpy.context.preferences.filepaths.save_version = 0
    bpy.context.scene.unit_settings.system = "METRIC"
    bpy.context.scene.unit_settings.scale_length = 1.0
    bpy.context.view_layer.update()

    blender_positions, runtime_positions = collect_socket_positions()
    blender_axis = axis_from_positions(blender_positions)
    runtime_axis = axis_from_positions(runtime_positions)
    dimensions = scene_dimensions()

    if args.dry_run:
        return {
            "id": entry["id"],
            "status": "dry_run",
            "source": rel(root, source),
            "cooked": rel(root, cooked),
            "blender_socket_forward_axis": blender_axis,
            "runtime_socket_forward_axis": runtime_axis,
        }

    cooked.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=str(cooked),
        export_format="GLB",
        export_yup=True,
        export_skins=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
    )

    metadata: dict[str, Any] = {}
    if metadata_path.exists():
        try:
            metadata = load_json(metadata_path)
        except json.JSONDecodeError:
            metadata = {}
    metadata.update(
        {
            "id": entry["id"],
            "source": rel(root, source),
            "export": rel(root, cooked),
            "scale_meters": True,
            "runtime_up_axis": "Y",
            "gameplay_forward_axis": "+Z",
            "generated_by": metadata.get("generated_by", "tools/blender/recook_blender_catalog_exports.py"),
            "recooked_by": "tools/blender/recook_blender_catalog_exports.py",
            "recooked_at_utc": datetime.now(timezone.utc).replace(microsecond=0).isoformat(),
            "bytes": cooked.stat().st_size,
        }
    )
    if dimensions is not None:
        metadata["dimensions_m"] = dimensions
        metadata.setdefault("target_dimensions_m", dimensions)
    if blender_positions:
        metadata["sockets"] = sorted(blender_positions)
        metadata["blender_socket_positions"] = blender_positions
        metadata["runtime_socket_positions"] = runtime_positions
    if blender_axis is not None:
        metadata["blender_forward_axis"] = blender_axis
        metadata["blender_socket_forward_axis"] = blender_axis
    if runtime_axis is not None:
        metadata["runtime_socket_forward_axis"] = runtime_axis
    if "weapon" in entry.get("tags", []) or str(entry.get("id", "")).startswith("wpn_"):
        metadata["origin"] = "Recooked from normalized Blender source; weapon sockets are Blender -Y forward and runtime +Z forward."
        metadata["socket_generation"] = "Recooked from checked-in .blend sockets by tools/blender/recook_blender_catalog_exports.py."
    armatures = [obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"]
    if armatures:
        armature = armatures[0]
        metadata["skin"] = armature.name
        metadata["skinned_meshes"] = sorted(
            obj.name
            for obj in bpy.context.scene.objects
            if obj.type == "MESH"
            and any(modifier.type == "ARMATURE" and modifier.object == armature for modifier in obj.modifiers)
        )
        metadata["animation_clips"] = sorted(action.name for action in bpy.data.actions)
        metadata["rig_type"] = "deforming_armature"
    write_json(metadata_path, metadata)

    return {
        "id": entry["id"],
        "status": "recooked",
        "source": rel(root, source),
        "cooked": rel(root, cooked),
        "metadata": rel(root, metadata_path),
        "bytes": cooked.stat().st_size,
        "blender_socket_forward_axis": blender_axis,
        "runtime_socket_forward_axis": runtime_axis,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Recook cataloged Blender source assets to runtime GLB files.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--catalog", default=str(DEFAULT_CATALOG))
    parser.add_argument("--asset", action="append", default=[])
    parser.add_argument("--tag", action="append", default=[])
    parser.add_argument("--source-prefix", action="append", default=[])
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args(blender_args())


def main() -> int:
    args = parse_args()
    root = Path(args.repo_root).resolve()
    catalog_path = root / args.catalog
    catalog = load_json(catalog_path)
    entries = [entry for entry in catalog.get("assets", []) if should_recook(entry, args)]
    results = [recook_entry(root, entry, args) for entry in entries]
    print(json.dumps({"count": len(results), "results": results}, indent=2))
    failed = [result for result in results if result.get("status") not in {"recooked", "dry_run"}]
    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
