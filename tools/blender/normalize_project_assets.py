#!/usr/bin/env python3
"""Audit and optionally normalize imported Nemisis weapon/character assets.

Run this script inside Blender:

    blender --background --factory-startup --python tools/blender/normalize_project_assets.py -- --repo-root .

By default it writes a JSON report and does not export binary assets. Pass
``--export`` to write normalized GLBs from the fixup metadata.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path
from typing import Any

import bpy
from mathutils import Euler, Matrix, Vector


MODEL_EXTENSIONS = {".blend", ".fbx", ".gltf", ".glb"}
DEFAULT_KEYWORDS = (
    "weapon",
    "wpn",
    "rifle",
    "smg",
    "sidearm",
    "glock",
    "p320",
    "m4",
    "afr",
    "ncar",
    "fr17",
    "hk416",
    "hk-416",
    "character",
    "chr",
    "arms",
    "operator",
)
AXIS_INDEX = {"X": 0, "Y": 1, "Z": 2}


def blender_args() -> list[str]:
    if "--" in sys.argv:
        return sys.argv[sys.argv.index("--") + 1 :]
    return sys.argv[1:]


def rel(root: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def resolve_path(repo_root: Path, value: str | None) -> Path | None:
    if not value:
        return None
    path = Path(value)
    if not path.is_absolute():
        path = repo_root / path
    return path.resolve()


def read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, data: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def reset_scene() -> None:
    for obj in list(bpy.data.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.context.view_layer.update()


def import_asset(path: Path) -> list[bpy.types.Object]:
    suffix = path.suffix.lower()
    if suffix == ".blend":
        bpy.ops.wm.open_mainfile(filepath=str(path))
        return list(bpy.data.objects)
    if suffix in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=str(path))
        return list(bpy.context.scene.objects)
    if suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(path))
        return list(bpy.context.scene.objects)
    raise ValueError(f"Unsupported asset extension: {path}")


def ignored_names(entry: dict[str, Any], config: dict[str, Any]) -> tuple[set[str], tuple[str, ...]]:
    names = set(config.get("global_ignore_mesh_names", []))
    names.update(entry.get("normalization", {}).get("remove_mesh_names", []))
    prefixes = tuple(config.get("global_ignore_mesh_name_prefixes", []))
    prefixes += tuple(entry.get("normalization", {}).get("remove_mesh_name_prefixes", []))
    return names, prefixes


def is_ignored_mesh(obj: bpy.types.Object, names: set[str], prefixes: tuple[str, ...]) -> bool:
    if obj.type != "MESH":
        return False
    return obj.name in names or any(obj.name.startswith(prefix) for prefix in prefixes)


def managed_objects(objects: list[bpy.types.Object], names: set[str], prefixes: tuple[str, ...]) -> list[bpy.types.Object]:
    return [obj for obj in objects if not is_ignored_mesh(obj, names, prefixes)]


def managed_meshes(objects: list[bpy.types.Object], names: set[str], prefixes: tuple[str, ...]) -> list[bpy.types.Object]:
    return [obj for obj in objects if obj.type == "MESH" and not is_ignored_mesh(obj, names, prefixes)]


def ignored_meshes(objects: list[bpy.types.Object], names: set[str], prefixes: tuple[str, ...]) -> list[bpy.types.Object]:
    return [obj for obj in objects if is_ignored_mesh(obj, names, prefixes)]


def bounds_for_meshes(meshes: list[bpy.types.Object]) -> dict[str, Any] | None:
    points: list[Vector] = []
    for obj in meshes:
        for corner in obj.bound_box:
            points.append(obj.matrix_world @ Vector(corner))
    if not points:
        return None
    mins = [min(point[i] for point in points) for i in range(3)]
    maxs = [max(point[i] for point in points) for i in range(3)]
    size = [maxs[i] - mins[i] for i in range(3)]
    center = [(mins[i] + maxs[i]) * 0.5 for i in range(3)]
    return {
        "min_xyz_blender": [round(v, 5) for v in mins],
        "max_xyz_blender": [round(v, 5) for v in maxs],
        "size_xyz_blender": [round(v, 5) for v in size],
        "center_xyz_blender": [round(v, 5) for v in center],
        "runtime_dimensions_m": [round(size[0], 5), round(size[2], 5), round(size[1], 5)],
    }


def non_applied_mesh_transforms(meshes: list[bpy.types.Object]) -> list[dict[str, Any]]:
    issues: list[dict[str, Any]] = []
    for obj in meshes:
        scaled = any(abs(component - 1.0) > 0.001 for component in obj.scale)
        rotated = any(abs(component) > 0.001 for component in obj.rotation_euler)
        if scaled or rotated:
            issues.append(
                {
                    "name": obj.name,
                    "rotation_euler": [round(v, 5) for v in obj.rotation_euler],
                    "scale": [round(v, 5) for v in obj.scale],
                }
            )
    return issues


def top_level_transforms(objects: list[bpy.types.Object]) -> list[dict[str, Any]]:
    top_level = []
    for obj in objects:
        if obj.parent is not None:
            continue
        top_level.append(
            {
                "name": obj.name,
                "type": obj.type,
                "location": [round(v, 5) for v in obj.location],
                "rotation_euler": [round(v, 5) for v in obj.rotation_euler],
                "scale": [round(v, 5) for v in obj.scale],
            }
        )
    return top_level


def transform_roots(objects: list[bpy.types.Object]) -> list[bpy.types.Object]:
    object_set = set(objects)
    return [obj for obj in objects if obj.parent not in object_set]


def apply_matrix_to_roots(objects: list[bpy.types.Object], matrix: Matrix) -> None:
    for obj in transform_roots(objects):
        obj.matrix_world = matrix @ obj.matrix_world
    bpy.context.view_layer.update()


def rotation_matrix(degrees: list[float]) -> Matrix:
    radians = [math.radians(value) for value in degrees]
    return Euler(radians, "XYZ").to_matrix().to_4x4()


def scale_to_axis(
    meshes: list[bpy.types.Object],
    objects: list[bpy.types.Object],
    axis: str,
    target_size: float,
) -> float | None:
    current = bounds_for_meshes(meshes)
    if not current:
        return None
    axis_index = AXIS_INDEX[axis.upper()]
    current_size = current["size_xyz_blender"][axis_index]
    if current_size <= 0.00001:
        return None
    factor = target_size / current_size
    apply_matrix_to_roots(objects, Matrix.Scale(factor, 4))
    return factor


def recenter(
    meshes: list[bpy.types.Object],
    objects: list[bpy.types.Object],
    mode: str,
    center_axes: list[str] | None,
    floor_axis: str | None,
) -> list[float] | None:
    if mode == "preserve":
        return None
    current = bounds_for_meshes(meshes)
    if not current:
        return None
    mins = current["min_xyz_blender"]
    center = current["center_xyz_blender"]
    delta = [0.0, 0.0, 0.0]
    for axis in center_axes or []:
        index = AXIS_INDEX[axis.upper()]
        delta[index] = -center[index]
    if floor_axis:
        index = AXIS_INDEX[floor_axis.upper()]
        delta[index] = -mins[index]
    apply_matrix_to_roots(objects, Matrix.Translation(Vector(delta)))
    return [round(v, 5) for v in delta]


def remove_ignored_meshes(objects: list[bpy.types.Object], names: set[str], prefixes: tuple[str, ...]) -> list[str]:
    removed = []
    for obj in list(objects):
        if not is_ignored_mesh(obj, names, prefixes):
            continue
        removed.append(obj.name)
        bpy.data.objects.remove(obj, do_unlink=True)
    bpy.context.view_layer.update()
    return removed


def ensure_root_empty(asset_id: str, objects: list[bpy.types.Object]) -> None:
    roots = transform_roots(objects)
    if len(roots) == 1 and roots[0].name == asset_id:
        return
    root = bpy.data.objects.new(asset_id, None)
    bpy.context.collection.objects.link(root)
    for obj in roots:
        obj.parent = root
        obj.matrix_parent_inverse = root.matrix_world.inverted()


def export_glb(path: Path, objects: list[bpy.types.Object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        if obj.name in bpy.data.objects:
            obj.select_set(True)
    bpy.ops.export_scene.gltf(
        filepath=str(path),
        export_format="GLB",
        use_selection=True,
        export_yup=True,
    )


def output_path(repo_root: Path, entry: dict[str, Any], output_root: Path | None) -> Path:
    configured = Path(entry["output"])
    if output_root:
        if configured.is_absolute():
            return output_root / configured.name
        return output_root / configured
    return resolve_path(repo_root, entry["output"])  # type: ignore[return-value]


def inspect_entry(
    repo_root: Path,
    config: dict[str, Any],
    entry: dict[str, Any],
    export: bool,
    output_root: Path | None,
) -> dict[str, Any]:
    source = resolve_path(repo_root, entry["source"])
    if not source or not source.exists():
        return {
            "id": entry.get("id"),
            "source": entry.get("source"),
            "error": "Source file does not exist.",
        }

    reset_scene()
    imported = import_asset(source)
    names, prefixes = ignored_names(entry, config)
    ignored = ignored_meshes(imported, names, prefixes)
    managed = managed_objects(imported, names, prefixes)
    meshes = managed_meshes(imported, names, prefixes)
    before = bounds_for_meshes(meshes)

    result: dict[str, Any] = {
        "id": entry["id"],
        "source": rel(repo_root, source),
        "category": entry.get("category"),
        "aliases": entry.get("aliases", []),
        "ignored_meshes_present": [obj.name for obj in ignored],
        "managed_object_count": len(managed),
        "managed_mesh_count": len(meshes),
        "top_level_transforms": top_level_transforms(managed)[:30],
        "non_applied_mesh_transforms": non_applied_mesh_transforms(meshes)[:50],
        "bounds_before": before,
        "configured_findings": entry.get("findings", []),
        "normalization": entry.get("normalization", {}),
    }

    normalization = entry.get("normalization", {})
    removed = remove_ignored_meshes(imported, names, prefixes)
    managed = managed_objects(list(bpy.context.scene.objects), set(), ())
    meshes = [obj for obj in managed if obj.type == "MESH"]

    rotation = normalization.get("rotation_euler_deg", [0.0, 0.0, 0.0])
    if any(abs(value) > 0.0001 for value in rotation):
        apply_matrix_to_roots(managed, rotation_matrix(rotation))

    scale_applied = None
    scale_to = normalization.get("scale_to_dimension_m")
    if scale_to:
        scale_applied = scale_to_axis(
            meshes,
            managed,
            str(scale_to["axis"]),
            float(scale_to["size"]),
        )

    origin = normalization.get("origin", {"mode": "preserve"})
    translation = recenter(
        meshes,
        managed,
        str(origin.get("mode", "preserve")),
        list(origin.get("center_axes", [])),
        origin.get("floor_axis"),
    )
    after = bounds_for_meshes(meshes)

    result["removed_meshes"] = removed
    result["scale_factor_applied"] = round(scale_applied, 6) if scale_applied else None
    result["translation_applied"] = translation
    result["bounds_after_configured_normalization"] = after

    if export:
        if normalization.get("ensure_root_empty", True):
            ensure_root_empty(entry["id"], managed)
            managed = managed_objects(list(bpy.context.scene.objects), set(), ())
        out = output_path(repo_root, entry, output_root)
        export_glb(out, managed)
        result["exported"] = rel(repo_root, out) if not output_root else str(out)

    return result


def scan_inventory(repo_root: Path, config: dict[str, Any]) -> dict[str, Any]:
    roots = [resolve_path(repo_root, root) for root in config.get("scan_roots", ["assets"])]
    keywords = tuple(config.get("inventory_keywords", DEFAULT_KEYWORDS))
    records: list[dict[str, Any]] = []
    hk416_matches: list[str] = []
    for root in roots:
        if not root or not root.exists():
            continue
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.suffix.lower() not in MODEL_EXTENSIONS:
                continue
            lower = path.as_posix().lower()
            matches = [keyword for keyword in keywords if keyword in lower]
            if not matches:
                continue
            relative = rel(repo_root, path)
            records.append(
                {
                    "path": relative,
                    "extension": path.suffix.lower(),
                    "matched_keywords": matches,
                    "bytes": path.stat().st_size,
                }
            )
            if "hk416" in lower or "hk-416" in lower:
                hk416_matches.append(relative)
    by_extension: dict[str, int] = {}
    for record in records:
        by_extension[record["extension"]] = by_extension.get(record["extension"], 0) + 1
    return {
        "weapon_character_model_count": len(records),
        "by_extension": by_extension,
        "hk416_path_matches": hk416_matches,
        "records": records,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit and optionally normalize Nemisis imported assets.")
    parser.add_argument("--repo-root", default=".", help="Nemisis repository root.")
    parser.add_argument(
        "--config",
        default="assets/metadata/asset_normalization_fixups.json",
        help="Fixup metadata path relative to repo root.",
    )
    parser.add_argument(
        "--report",
        default="assets/metadata/asset_normalization_report.json",
        help="Audit report path relative to repo root. Use empty string to skip writing.",
    )
    parser.add_argument("--export", action="store_true", help="Write normalized GLB outputs.")
    parser.add_argument("--output-root", default=None, help="Optional root for exported outputs.")
    parser.add_argument("--only", action="append", default=[], help="Limit to one or more asset ids.")
    args = parser.parse_args(blender_args())

    repo_root = Path(args.repo_root).resolve()
    config_path = resolve_path(repo_root, args.config)
    if not config_path or not config_path.exists():
        raise SystemExit(f"Config not found: {args.config}")
    config = read_json(config_path)

    output_root = Path(args.output_root).resolve() if args.output_root else None
    selected = set(args.only)
    entries = [
        entry
        for entry in config.get("assets", [])
        if not selected or entry.get("id") in selected
    ]
    if selected and len(entries) != len(selected):
        found = {entry.get("id") for entry in entries}
        missing = sorted(selected - found)
        raise SystemExit(f"Unknown --only asset id(s): {', '.join(missing)}")

    results = [
        inspect_entry(repo_root, config, entry, args.export, output_root)
        for entry in entries
    ]
    report = {
        "schema_version": 1,
        "mode": "export" if args.export else "audit",
        "repo_root": ".",
        "config": rel(repo_root, config_path),
        "coordinate_contract": config.get("coordinate_contract", {}),
        "inventory": scan_inventory(repo_root, config),
        "assets": results,
    }

    if args.report:
        report_path = resolve_path(repo_root, args.report)
        if report_path:
            write_json(report_path, report)
    print(json.dumps(report, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
