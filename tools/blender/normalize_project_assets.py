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
        "dimensions_m_blender_xyz": [round(v, 5) for v in size],
        "center_xyz_blender": [round(v, 5) for v in center],
        "runtime_dimensions_m": [round(size[0], 5), round(size[2], 5), round(size[1], 5)],
    }


def rounded_triplet(values: Any) -> list[float]:
    return [round(float(values[index]), 5) for index in range(3)]


def runtime_from_blender_local(value: Vector) -> list[float]:
    return [round(float(value.x), 5), round(float(value.z), 5), round(-float(value.y), 5)]


def dominant_axis(delta: list[float]) -> str | None:
    if max(abs(component) for component in delta) <= 0.0001:
        return None
    axis_index = max(range(3), key=lambda index: abs(delta[index]))
    axes = ["X", "Y", "Z"]
    return ("+" if delta[axis_index] >= 0.0 else "-") + axes[axis_index]


def socket_report_from_objects(root: bpy.types.Object, socket_objects: list[bpy.types.Object]) -> dict[str, Any]:
    blender_positions: dict[str, list[float]] = {}
    runtime_positions: dict[str, list[float]] = {}
    inverse_root = root.matrix_world.inverted()
    for obj in socket_objects:
        local = obj.location.copy() if obj.parent == root else inverse_root @ obj.matrix_world.translation
        blender_positions[obj.name] = rounded_triplet(local)
        runtime_positions[obj.name] = runtime_from_blender_local(local)

    def axis_from_positions(positions: dict[str, list[float]]) -> str | None:
        muzzle = positions.get("socket_muzzle")
        if muzzle is None:
            return None
        base = positions.get("socket_grip_r") or positions.get("socket_weapon_root") or [0.0, 0.0, 0.0]
        return dominant_axis([muzzle[index] - base[index] for index in range(3)])

    return {
        "blender_socket_positions": blender_positions,
        "runtime_socket_positions": runtime_positions,
        "blender_socket_forward_axis": axis_from_positions(blender_positions),
        "runtime_socket_forward_axis": axis_from_positions(runtime_positions),
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


def ensure_root_empty(asset_id: str, objects: list[bpy.types.Object]) -> bpy.types.Object:
    roots = transform_roots(objects)
    if len(roots) == 1 and roots[0].name == asset_id:
        return roots[0]
    root = bpy.data.objects.new(asset_id, None)
    bpy.context.collection.objects.link(root)
    for obj in roots:
        obj.parent = root
        obj.matrix_parent_inverse = root.matrix_world.inverted()
    return root


def add_socket_empties(root: bpy.types.Object, sockets: list[dict[str, Any]]) -> list[str]:
    created: list[str] = []
    for socket in sockets:
        name = str(socket.get("name", "")).strip()
        if not name:
            continue
        obj = bpy.data.objects.get(name)
        if obj is None:
            obj = bpy.data.objects.new(name, None)
            bpy.context.collection.objects.link(obj)
        obj.empty_display_type = str(socket.get("display", "PLAIN_AXES"))
        obj.empty_display_size = float(socket.get("display_size", 0.045))
        obj.parent = root
        obj.matrix_parent_inverse.identity()
        obj.location = Vector(socket.get("location", [0.0, 0.0, 0.0]))
        rotation = socket.get("rotation_euler_deg", [0.0, 0.0, 0.0])
        obj.rotation_euler = Euler([math.radians(float(value)) for value in rotation], "XYZ")
        obj.scale = (1.0, 1.0, 1.0)
        created.append(name)
    bpy.context.view_layer.update()
    return created


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
        export_skins=True,
        export_animations=True,
        export_animation_mode="ACTIONS",
    )


def parent_sockets_to_bones(socket_objects: list[bpy.types.Object], socket_bones: dict[str, str]) -> dict[str, str]:
    armature = next((obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"), None)
    if armature is None:
        return {}
    parented: dict[str, str] = {}
    for socket in socket_objects:
        bone_name = socket_bones.get(socket.name)
        if not bone_name or bone_name not in armature.data.bones:
            continue
        world = socket.matrix_world.copy()
        socket.parent = armature
        socket.parent_type = "BONE"
        socket.parent_bone = bone_name
        socket.matrix_world = world
        parented[socket.name] = bone_name
    bpy.context.view_layer.update()
    return parented


def recenter_root_to_socket(root: bpy.types.Object, socket: bpy.types.Object) -> None:
    bpy.context.view_layer.update()
    offset = root.matrix_world.inverted() @ socket.matrix_world.translation
    for child in list(root.children):
        child.location -= offset
    bpy.context.view_layer.update()


def ensure_lod0_node(asset_id: str, meshes: list[bpy.types.Object]) -> str | None:
    if not meshes:
        return None
    name = f"{asset_id}_lod0"
    primary = max(meshes, key=lambda obj: len(obj.data.polygons))
    primary.name = name
    primary.data.name = f"{name}_mesh"
    return name


def weapon_socket_notes() -> dict[str, str]:
    return {
        "socket_muzzle": "Muzzle/VFX point at runtime +Z after Blender -Y forward normalization.",
        "socket_grip_r": "Right-hand firing grip anchor for first-person and future IK retargeting.",
        "socket_grip_l": "Left support-hand anchor on the fore-end.",
        "socket_eject": "Right-side casing ejection reference.",
        "socket_vfx": "Muzzle flash/tracer fallback, intentionally colocated with socket_muzzle.",
    }


def character_socket_notes() -> dict[str, str]:
    return {
        "socket_root": "Feet/root pivot at normalized floor center.",
        "socket_camera": "Approximate eye reference; exported runtime socket faces +Z forward.",
        "socket_weapon_root": "Approximate chest/shoulder weapon mount for viewmodel handoff.",
        "socket_hand_r": "Right hand reference for firing grip retargeting.",
        "socket_hand_l": "Left hand reference for support grip and traversal cues.",
        "socket_head": "Head/helmet reference for hit and VFX alignment.",
        "socket_vfx": "Torso VFX fallback anchor.",
    }


def action_names() -> list[str]:
    return sorted(action.name for action in bpy.data.actions)


def armature_names() -> list[str]:
    return sorted(obj.name for obj in bpy.data.objects if obj.type == "ARMATURE")


def write_asset_metadata(
    repo_root: Path,
    config_path: Path,
    config: dict[str, Any],
    entry: dict[str, Any],
    out: Path,
    bounds_after: dict[str, Any] | None,
    socket_report: dict[str, Any],
) -> Path:
    metadata_path = out.with_name(out.stem + ".metadata.json")
    metadata = read_json(metadata_path) if metadata_path.exists() else {}
    contract = config.get("coordinate_contract", {})
    sockets = [str(socket.get("name")) for socket in entry.get("normalization", {}).get("add_sockets", []) if socket.get("name")]
    category = str(entry.get("category", "mesh"))
    target_dimensions = entry.get("target_dimensions_m")
    if target_dimensions is None and bounds_after:
        target_dimensions = bounds_after.get("dimensions_m_blender_xyz")

    metadata.update(
        {
            "id": entry["id"],
            "source": entry["source"],
            "export": rel(repo_root, out),
            "category": category,
            "scale_meters": True,
            "runtime_up_axis": contract.get("runtime_up_axis", "Y"),
            "gameplay_forward_axis": contract.get("gameplay_forward_axis", "+Z"),
            "blender_up_axis": contract.get("blender_target_up_axis", "+Z"),
            "blender_forward_axis": contract.get("blender_target_forward_axis", "-Y"),
            "dimensions_m": target_dimensions,
            "target_dimensions_m": target_dimensions,
            "sockets": sockets,
            "collision": metadata.get("collision", "none"),
            "lods": metadata.get("lods", [f"{entry['id']}_lod0"]),
            "license": metadata.get("license", "original_project_asset"),
            "external_assets": False,
            "generated_by": "tools/blender/normalize_project_assets.py",
            "normalized_export": rel(repo_root, out),
            "normalization_status": "exported_runtime_y_up_positive_z_forward",
            "socket_forward_axis": socket_report.get("runtime_socket_forward_axis"),
            "runtime_socket_forward_axis": socket_report.get("runtime_socket_forward_axis"),
            "blender_socket_forward_axis": socket_report.get("blender_socket_forward_axis"),
            "socket_generation": f"Generated by tools/blender/normalize_project_assets.py from {rel(repo_root, config_path)}.",
        }
    )

    if category == "weapon":
        metadata["origin"] = (
            "Root origin is socket_grip_r; source axes are normalized to Blender +Z up/-Y forward so "
            "Blender export_yup produces runtime Y-up/+Z-forward sockets."
        )
        metadata["socket_notes"] = weapon_socket_notes()
    elif category == "character":
        clips = action_names()
        rigs = armature_names()
        if rigs:
            metadata["skin"] = metadata.get("skin") or rigs[0]
            metadata["rig_type"] = "deforming_armature"
            metadata["skinned_meshes"] = sorted(
                obj.name
                for obj in bpy.context.scene.objects
                if obj.type == "MESH"
                and any(modifier.type == "ARMATURE" for modifier in obj.modifiers)
            )
        if clips:
            metadata["animation_clips"] = clips
        metadata["origin"] = (
            "Normalized Project character GLB; helper meshes removed, floor-centered, and rotated to the "
            "runtime Y-up/+Z-forward contract."
        )
        metadata["socket_notes"] = character_socket_notes()
        metadata["first_person_arms"] = metadata.get(
            "first_person_arms",
            "not_separated_from_full_body; runtime uses procedural first-person rig and socket-bound hands",
        )
        roles = metadata.get("animation_clip_roles", {})
        for clip in clips:
            if "run" in clip.lower():
                roles.setdefault(clip, "locomotion_run_reference")
            elif "walk" in clip.lower():
                roles.setdefault(clip, "locomotion_walk_reference")
            elif "pose" in clip.lower():
                roles.setdefault(clip, "retarget_bind_pose_reference")
            else:
                roles.setdefault(clip, "idle_stance_reference")
        metadata["animation_clip_roles"] = roles
        skeleton = metadata.get("skeleton", {})
        skeleton.update(
            {
                "skin": metadata.get("skin"),
                "skinned_mesh_nodes": metadata.get("skinned_meshes", []),
                "animation_clips": clips,
                "source_pose": skeleton.get("source_pose", "T_pose"),
                "retarget_axis_contract": "Blender +Z up/-Y forward; exported runtime Y-up/+Z forward",
                "retarget_status": "source skin and clips are present; runtime first-person rig now binds hands and weapon feedback to cooked sockets",
                "runtime_socket_contract": [
                    "socket_camera",
                    "socket_weapon_root",
                    "socket_hand_r",
                    "socket_hand_l",
                ],
            }
        )
        metadata["skeleton"] = skeleton
        metadata["first_person_view"] = {
            "body_visibility": "hide full local body for camera; use camera-bound torso, hands, and arms only",
            "arms_source_asset": "chr_a1_fp_arms_01",
            "weapon_socket": "socket_weapon_root",
            "right_hand_socket": "socket_hand_r",
            "left_hand_socket": "socket_hand_l",
            "camera_socket": "socket_camera",
            "status": "socket-ready with source skeleton/animation clips; final authored first-person arms split still needed",
        }

    write_json(metadata_path, metadata)
    return metadata_path


def output_path(repo_root: Path, entry: dict[str, Any], output_root: Path | None) -> Path:
    configured = Path(entry["output"])
    if output_root:
        if configured.is_absolute():
            return output_root / configured.name
        return output_root / configured
    return resolve_path(repo_root, entry["output"])  # type: ignore[return-value]


def inspect_entry(
    repo_root: Path,
    config_path: Path,
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
        "target_dimensions_m": entry.get("target_dimensions_m"),
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
    result["lod0_node"] = ensure_lod0_node(entry["id"], meshes)

    result["removed_meshes"] = removed
    result["scale_factor_applied"] = round(scale_applied, 6) if scale_applied else None
    result["translation_applied"] = translation
    result["bounds_after_configured_normalization"] = after
    result["configured_sockets"] = [socket.get("name") for socket in normalization.get("add_sockets", [])]

    if export:
        root = None
        runtime_socket_report: dict[str, Any] = {}
        if normalization.get("ensure_root_empty", True):
            root = ensure_root_empty(entry["id"], managed)
            managed = managed_objects(list(bpy.context.scene.objects), set(), ())
        added_sockets = []
        if root and normalization.get("add_sockets"):
            added_sockets = add_socket_empties(root, list(normalization.get("add_sockets", [])))
            socket_objects = [bpy.data.objects[name] for name in added_sockets if name in bpy.data.objects]
            if entry.get("category") == "weapon" and "socket_grip_r" in bpy.data.objects:
                recenter_root_to_socket(root, bpy.data.objects["socket_grip_r"])
            result["bone_parented_sockets"] = parent_sockets_to_bones(
                socket_objects,
                dict(normalization.get("socket_bones", {})),
            )
            runtime_socket_report = socket_report_from_objects(root, socket_objects)
            managed = managed_objects(list(bpy.context.scene.objects), set(), ())
        result["exported_sockets"] = added_sockets
        result["runtime_socket_report"] = runtime_socket_report
        result["runtime_socket_contract_ok"] = (
            entry.get("category") != "weapon"
            or runtime_socket_report.get("runtime_socket_forward_axis") == "+Z"
        )
        out = output_path(repo_root, entry, output_root)
        export_glb(out, managed)
        metadata_path = write_asset_metadata(repo_root, config_path, config, entry, out, after, runtime_socket_report)
        result["exported"] = rel(repo_root, out) if not output_root else str(out)
        result["metadata_written"] = rel(repo_root, metadata_path) if not output_root else str(metadata_path)

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
        inspect_entry(repo_root, config_path, config, entry, args.export, output_root)
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
