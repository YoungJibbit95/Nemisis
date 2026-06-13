#!/usr/bin/env python3
"""Audit Nemisis runtime assets for import-readiness.

The script is intentionally read-only for source/export assets. When an output
path is provided it writes a scan report JSON, normally under assets/processed.
"""

from __future__ import annotations

import argparse
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


ASSET_ID_RE = re.compile(r"^(wpn|chr|map|prop|env|mat)_[a-z0-9_]+$")
GLB_JSON_CHUNK = 0x4E4F534A
GLB_BIN_CHUNK = 0x004E4942

RECOMMENDED_METADATA_FIELDS = (
    "id",
    "source",
    "export",
    "category",
    "scale_meters",
    "runtime_up_axis",
    "gameplay_forward_axis",
    "sockets",
    "collision",
    "lods",
    "license",
)

RICH_METADATA_FIELDS = (
    "origin",
    "dimensions_m",
    "target_dimensions_m",
    "external_assets",
)


def utc_now() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat()


def rel(root: Path, path: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return path.as_posix()


def read_json(path: Path) -> tuple[dict[str, Any] | None, str | None]:
    try:
        return json.loads(path.read_text(encoding="utf-8")), None
    except Exception as exc:  # pragma: no cover - diagnostic path
        return None, str(exc)


def add_issue(issues: list[dict[str, str]], severity: str, check: str, message: str) -> None:
    issues.append({"severity": severity, "check": check, "message": message})


def parse_glb(path: Path) -> tuple[dict[str, Any] | None, dict[str, Any]]:
    info: dict[str, Any] = {
        "path": str(path),
        "valid_glb": False,
        "glb_version": None,
        "declared_length": None,
        "json_chunk_bytes": 0,
        "bin_chunk_bytes": 0,
        "chunk_types": [],
        "error": None,
    }
    try:
        data = path.read_bytes()
        if len(data) < 20 or data[:4] != b"glTF":
            info["error"] = "Not a binary glTF/GLB file."
            return None, info
        info["glb_version"] = struct.unpack_from("<I", data, 4)[0]
        info["declared_length"] = struct.unpack_from("<I", data, 8)[0]
        offset = 12
        gltf_json: dict[str, Any] | None = None
        while offset + 8 <= len(data):
            chunk_len, chunk_type = struct.unpack_from("<II", data, offset)
            offset += 8
            chunk = data[offset : offset + chunk_len]
            offset += chunk_len
            info["chunk_types"].append(hex(chunk_type))
            if chunk_type == GLB_JSON_CHUNK:
                info["json_chunk_bytes"] = chunk_len
                gltf_json = json.loads(chunk.rstrip(b" \t\r\n\0"))
            elif chunk_type == GLB_BIN_CHUNK:
                info["bin_chunk_bytes"] += chunk_len
        info["valid_glb"] = gltf_json is not None and info["glb_version"] == 2
        return gltf_json, info
    except Exception as exc:  # pragma: no cover - diagnostic path
        info["error"] = str(exc)
        return None, info


def gltf_summary(gltf_json: dict[str, Any] | None) -> dict[str, Any]:
    if not gltf_json:
        return {
            "asset_version": None,
            "nodes": [],
            "meshes": [],
            "materials": [],
            "external_buffer_uris": [],
            "external_image_uris": [],
        }
    nodes = [node.get("name", "") for node in gltf_json.get("nodes", [])]
    meshes = [mesh.get("name", "") for mesh in gltf_json.get("meshes", [])]
    materials = [mat.get("name", "") for mat in gltf_json.get("materials", [])]
    return {
        "asset_version": gltf_json.get("asset", {}).get("version"),
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "external_buffer_uris": [
            buf.get("uri")
            for buf in gltf_json.get("buffers", [])
            if buf.get("uri")
        ],
        "external_image_uris": [
            img.get("uri")
            for img in gltf_json.get("images", [])
            if img.get("uri")
        ],
    }


def expected_required_sockets(asset_id: str, category: str, tags: list[str]) -> set[str]:
    if asset_id.startswith("wpn_"):
        return {"socket_muzzle"}
    if asset_id.startswith("chr_") and ("arms" in asset_id or "first_person_arms" in tags):
        return {"socket_camera", "socket_weapon_root", "socket_hand_r", "socket_hand_l"}
    if asset_id.startswith("chr_") and "proto" not in asset_id:
        return {"socket_root", "socket_camera", "socket_weapon_root"}
    if "wallrun" in tags or "wall_run" in tags:
        return {"socket_wallrun_start", "socket_wallrun_end"}
    if "slide" in tags:
        return {"socket_slide_entry", "socket_slide_exit"}
    return set()


def classify_status(issues: list[dict[str, str]]) -> str:
    severities = {issue["severity"] for issue in issues}
    if "error" in severities:
        return "needs_manual_work"
    if "manual" in severities:
        return "needs_collision_or_metadata_work"
    if "warning" in severities:
        return "ready_with_notes"
    return "game_ready"


def find_blender(root: Path, explicit: str | None) -> str | None:
    candidates: list[str] = []
    if explicit:
        candidates.append(explicit)
    found = shutil.which("blender") or shutil.which("blender.exe")
    if found:
        candidates.append(found)
    drive = root.drive or "C:"
    candidates.extend(
        [
            str(Path(drive + os.sep) / "Program Files/Blender Foundation/Blender 5.1/blender.exe"),
            str(Path("C:/Program Files/Blender Foundation/Blender 5.1/blender.exe")),
            str(Path("C:/Program Files/Blender Foundation/Blender 4.3/blender.exe")),
        ]
    )
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return candidate
    return None


def blender_version(blender: str) -> str | None:
    try:
        result = subprocess.run(
            [blender, "--version"],
            check=False,
            capture_output=True,
            text=True,
            timeout=20,
        )
        first_line = (result.stdout or result.stderr).splitlines()[0]
        return first_line.strip()
    except Exception:
        return None


def run_blender_scan(root: Path, blender: str, blend_paths: list[Path]) -> tuple[dict[str, Any], str | None]:
    if not blend_paths:
        return {}, None

    scan_script = r'''
import json
import math
import sys
from pathlib import Path

import bpy

out_path = Path(sys.argv[sys.argv.index("--") + 1])
blend_paths = [Path(p) for p in sys.argv[sys.argv.index("--") + 2:]]
results = {}

for blend_path in blend_paths:
    try:
        bpy.ops.wm.open_mainfile(filepath=str(blend_path))
        objects = list(bpy.data.objects)
        mesh_objects = [obj for obj in objects if obj.type == "MESH"]
        socket_objects = [obj for obj in objects if obj.name.startswith("socket_")]
        collision_objects = [obj for obj in objects if obj.name.startswith("col_")]
        non_applied = []
        for obj in mesh_objects:
            scaled = any(abs(component - 1.0) > 0.001 for component in obj.scale)
            rotated = any(abs(component) > 0.001 for component in obj.rotation_euler)
            if scaled or rotated:
                non_applied.append({
                    "name": obj.name,
                    "scale": [round(v, 5) for v in obj.scale],
                    "rotation_euler": [round(v, 5) for v in obj.rotation_euler],
                })
        material_names = sorted({slot.material.name for obj in mesh_objects for slot in obj.material_slots if slot.material})
        asset_root_name = blend_path.stem
        root_objects = [obj for obj in objects if obj.name == asset_root_name]
        roots_at_origin = []
        for obj in root_objects:
            roots_at_origin.append({
                "name": obj.name,
                "type": obj.type,
                "location": [round(v, 5) for v in obj.location],
            })
        results[str(blend_path)] = {
            "unit_system": bpy.context.scene.unit_settings.system,
            "scale_length": bpy.context.scene.unit_settings.scale_length,
            "object_count": len(objects),
            "mesh_count": len(mesh_objects),
            "material_count": len(material_names),
            "materials": material_names,
            "sockets": sorted(obj.name for obj in socket_objects),
            "socket_types": {obj.name: obj.type for obj in socket_objects},
            "collisions": sorted(obj.name for obj in collision_objects),
            "root_objects": roots_at_origin,
            "non_applied_mesh_transforms": non_applied[:50],
            "non_applied_mesh_transform_count": len(non_applied),
        }
    except Exception as exc:
        results[str(blend_path)] = {"error": str(exc)}

out_path.write_text(json.dumps(results, indent=2), encoding="utf-8")
'''
    with tempfile.TemporaryDirectory() as tmp:
        script_path = Path(tmp) / "nemisis_blender_readiness_scan.py"
        output_path = Path(tmp) / "blend_scan.json"
        script_path.write_text(scan_script, encoding="utf-8")
        command = [
            blender,
            "--background",
            "--factory-startup",
            "--python",
            str(script_path),
            "--",
            str(output_path),
            *[str(path.resolve()) for path in blend_paths],
        ]
        result = subprocess.run(command, cwd=root, capture_output=True, text=True, timeout=240)
        if result.returncode != 0:
            return {}, (result.stderr or result.stdout).strip()
        data, error = read_json(output_path)
        if error:
            return {}, error
        return data or {}, None


def audit(root: Path, with_blender: bool, blender_path: str | None) -> dict[str, Any]:
    catalog_path = root / "configs/assets/nemisis_assets.json"
    catalog, catalog_error = read_json(catalog_path)
    if catalog_error or not catalog:
        raise SystemExit(f"Could not read {catalog_path}: {catalog_error}")

    entries = catalog.get("assets", [])
    cooked_paths = {
        (root / entry["cooked"]).resolve()
        for entry in entries
        if entry.get("cooked")
    }
    blend_sources = sorted(
        {
            (root / entry["source"]).resolve()
            for entry in entries
            if str(entry.get("source", "")).endswith(".blend") and (root / entry["source"]).exists()
        }
    )

    blender = find_blender(root, blender_path) if with_blender else None
    blender_scan: dict[str, Any] = {}
    blender_error: str | None = None
    if with_blender and blender:
        blender_scan, blender_error = run_blender_scan(root, blender, blend_sources)
    elif with_blender:
        blender_error = "Blender executable was not found."

    records: list[dict[str, Any]] = []
    for entry in entries:
        asset_id = entry.get("id", "")
        kind = entry.get("kind", "")
        category = entry.get("category", kind)
        tags = entry.get("tags", [])
        issues: list[dict[str, str]] = []

        if not ASSET_ID_RE.match(asset_id):
            add_issue(issues, "error", "naming", f"Asset id does not match lowercase Nemisis id rules: {asset_id}")

        source_rel = entry.get("source")
        source_path = root / source_rel if source_rel else None
        if not source_path or not source_path.exists():
            add_issue(issues, "error", "source", f"Source path is missing: {source_rel}")

        record: dict[str, Any] = {
            "id": asset_id,
            "kind": kind,
            "tags": tags,
            "source": source_rel,
            "issues": issues,
        }

        if kind == "material":
            record["status"] = classify_status(issues)
            records.append(record)
            continue

        cooked_rel = entry.get("cooked")
        cooked_path = root / cooked_rel if cooked_rel else None
        if not cooked_path or not cooked_path.exists():
            add_issue(issues, "error", "export", f"Cooked GLB is missing: {cooked_rel}")
            record["status"] = classify_status(issues)
            records.append(record)
            continue

        if cooked_path.suffix.lower() != ".glb":
            add_issue(issues, "error", "export_format", f"Cooked export should be .glb: {cooked_rel}")

        metadata_path = cooked_path.with_name(cooked_path.stem + ".metadata.json")
        metadata, metadata_error = read_json(metadata_path) if metadata_path.exists() else (None, "missing")
        if metadata_error:
            add_issue(issues, "error", "metadata", f"Metadata is missing or invalid: {rel(root, metadata_path)}")
            metadata = {}

        gltf_json, glb_info = parse_glb(cooked_path)
        summary = gltf_summary(gltf_json)
        if not glb_info["valid_glb"] or summary["asset_version"] != "2.0":
            add_issue(issues, "error", "glb", f"GLB is not valid glTF 2.0: {cooked_rel}")
        if not glb_info["bin_chunk_bytes"]:
            add_issue(issues, "error", "glb", f"GLB has no binary chunk: {cooked_rel}")
        if summary["external_buffer_uris"] or summary["external_image_uris"]:
            add_issue(issues, "manual", "external_references", "GLB contains external buffer/image URIs.")
        if not summary["materials"]:
            add_issue(issues, "warning", "materials", "No material slots are present in the GLB.")

        metadata_id = metadata.get("id")
        if metadata_id != asset_id:
            add_issue(issues, "error", "metadata_id", f"Metadata id '{metadata_id}' does not match catalog id '{asset_id}'.")
        if cooked_path.stem != asset_id:
            add_issue(issues, "warning", "naming", f"GLB filename stem '{cooked_path.stem}' does not match asset id '{asset_id}'.")

        for field in RECOMMENDED_METADATA_FIELDS:
            if field not in metadata:
                add_issue(issues, "warning", "metadata_field", f"Recommended metadata field is missing: {field}")
        for field in RICH_METADATA_FIELDS:
            if field not in metadata:
                add_issue(issues, "warning", "metadata_field", f"Readiness metadata field is missing: {field}")

        if metadata.get("scale_meters") is not True:
            add_issue(issues, "error", "scale", "Metadata must declare scale_meters: true.")
        if metadata.get("runtime_up_axis") != "Y":
            add_issue(issues, "warning", "orientation", f"runtime_up_axis should be Y for current importer expectations: {metadata.get('runtime_up_axis')}")
        if metadata.get("gameplay_forward_axis") != "+Z":
            add_issue(issues, "manual", "orientation", f"gameplay_forward_axis is {metadata.get('gameplay_forward_axis')}; current asset plan expects +Z.")

        nodes = set(summary["nodes"])
        sockets = sorted(name for name in nodes if name.startswith("socket_"))
        collisions = sorted(name for name in nodes if name.startswith("col_"))
        metadata_sockets = set(metadata.get("sockets", []))
        missing_socket_nodes = sorted(metadata_sockets - nodes)
        if missing_socket_nodes:
            add_issue(issues, "error", "sockets", f"Metadata sockets missing from GLB nodes: {', '.join(missing_socket_nodes)}")
        required_sockets = expected_required_sockets(asset_id, str(metadata.get("category", category)), tags)
        missing_required = sorted(required_sockets - nodes)
        if missing_required:
            add_issue(issues, "error", "sockets", f"Required import sockets are missing: {', '.join(missing_required)}")

        collision = str(metadata.get("collision", ""))
        if collision.startswith("col_") and collision not in nodes:
            add_issue(issues, "error", "collision", f"Declared collision object is missing from GLB nodes: {collision}")
        elif collision.startswith("visual_only") or collision in {"box", "walkable_box", "ramp_wedge"}:
            add_issue(issues, "manual", "collision", f"Collision is documented as runtime/simple collision only: {collision}")

        lods = metadata.get("lods", [])
        if lods and not any(any(str(lod) in node for node in nodes) for lod in lods):
            add_issue(issues, "warning", "lods", "Declared LOD names are not visible in GLB node names.")

        blend_key = str(source_path.resolve()) if source_path and source_path.suffix.lower() == ".blend" else None
        source_scan = blender_scan.get(blend_key) if blend_key else None
        if source_scan:
            if source_scan.get("unit_system") not in {"METRIC", "NONE"} or not math.isclose(float(source_scan.get("scale_length", 1.0)), 1.0, rel_tol=0, abs_tol=0.001):
                add_issue(issues, "warning", "blender_units", f"Unexpected Blender unit settings: {source_scan.get('unit_system')} scale {source_scan.get('scale_length')}")
            if source_scan.get("non_applied_mesh_transform_count", 0):
                add_issue(issues, "manual", "blender_transforms", f"{source_scan.get('non_applied_mesh_transform_count')} mesh objects have non-applied scale or rotation in source .blend.")
            source_sockets = set(source_scan.get("sockets", []))
            source_missing_sockets = sorted(metadata_sockets - source_sockets)
            if source_missing_sockets:
                add_issue(issues, "error", "source_sockets", f"Metadata sockets missing from .blend source: {', '.join(source_missing_sockets)}")
        elif with_blender and blend_key:
            add_issue(issues, "warning", "blender_scan", "No Blender scan data was produced for this source .blend.")

        record.update(
            {
                "cooked": cooked_rel,
                "metadata": rel(root, metadata_path),
                "status": classify_status(issues),
                "glb": {
                    "valid": glb_info["valid_glb"],
                    "asset_version": summary["asset_version"],
                    "bin_chunk_bytes": glb_info["bin_chunk_bytes"],
                    "nodes": len(summary["nodes"]),
                    "meshes": len(summary["meshes"]),
                    "materials": len(summary["materials"]),
                    "sockets": sockets,
                    "collisions": collisions,
                    "external_buffer_uris": summary["external_buffer_uris"],
                    "external_image_uris": summary["external_image_uris"],
                },
                "metadata_summary": {
                    "category": metadata.get("category"),
                    "scale_meters": metadata.get("scale_meters"),
                    "runtime_up_axis": metadata.get("runtime_up_axis"),
                    "gameplay_forward_axis": metadata.get("gameplay_forward_axis"),
                    "blender_forward_axis": metadata.get("blender_forward_axis"),
                    "origin": metadata.get("origin"),
                    "dimensions_m": metadata.get("dimensions_m"),
                    "collision": metadata.get("collision"),
                    "sockets": metadata.get("sockets", []),
                    "license": metadata.get("license"),
                    "external_assets": metadata.get("external_assets"),
                },
            }
        )
        if source_scan:
            record["blender_source"] = {
                "unit_system": source_scan.get("unit_system"),
                "scale_length": source_scan.get("scale_length"),
                "object_count": source_scan.get("object_count"),
                "mesh_count": source_scan.get("mesh_count"),
                "material_count": source_scan.get("material_count"),
                "sockets": source_scan.get("sockets", []),
                "collisions": source_scan.get("collisions", []),
                "root_objects": source_scan.get("root_objects", []),
                "non_applied_mesh_transform_count": source_scan.get("non_applied_mesh_transform_count", 0),
            }
        records.append(record)

    all_glbs = sorted((root / "assets").rglob("*.glb"))
    orphan_glbs = [rel(root, path) for path in all_glbs if path.resolve() not in cooked_paths]
    uncataloged_blends = [
        rel(root, path)
        for path in sorted((root / "assets").rglob("*.blend"))
        if path.resolve() not in blend_sources and "preview" not in path.name
    ]

    by_status: dict[str, int] = {}
    by_severity: dict[str, int] = {}
    for record in records:
        by_status[record["status"]] = by_status.get(record["status"], 0) + 1
        for issue in record["issues"]:
            by_severity[issue["severity"]] = by_severity.get(issue["severity"], 0) + 1

    return {
        "generated_at_utc": utc_now(),
        "repo_root": str(root),
        "catalog": rel(root, catalog_path),
        "summary": {
            "catalog_entries": len(entries),
            "runtime_mesh_or_scene_entries": sum(1 for entry in entries if entry.get("kind") != "material"),
            "glb_files_under_assets": len(all_glbs),
            "orphan_glbs": len(orphan_glbs),
            "uncataloged_blends": len(uncataloged_blends),
            "by_status": by_status,
            "by_issue_severity": by_severity,
        },
        "blender": {
            "requested": with_blender,
            "path": blender,
            "version": blender_version(blender) if blender else None,
            "scan_error": blender_error,
        },
        "records": records,
        "orphan_glbs": orphan_glbs,
        "uncataloged_blends": uncataloged_blends,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Audit Nemisis asset import readiness.")
    parser.add_argument("--repo-root", default=".", help="Nemisis repository root.")
    parser.add_argument("--with-blender", action="store_true", help="Inspect .blend source files through Blender CLI.")
    parser.add_argument("--blender", default=None, help="Explicit blender executable path.")
    parser.add_argument("--output", default=None, help="Optional JSON output path.")
    parser.add_argument("--fail-on-error", action="store_true", help="Exit non-zero when error-severity issues are found.")
    args = parser.parse_args()

    root = Path(args.repo_root).resolve()
    result = audit(root, args.with_blender, args.blender)
    text = json.dumps(result, indent=2)
    if args.output:
        output = (root / args.output).resolve()
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(text + "\n", encoding="utf-8")
    print(text)

    if args.fail_on_error:
        error_count = result["summary"]["by_issue_severity"].get("error", 0)
        return 1 if error_count else 0
    return 0


if __name__ == "__main__":
    sys.exit(main())
