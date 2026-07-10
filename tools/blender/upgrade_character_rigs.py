"""Upgrade existing procedural character .blend sources with deforming rigs and clips."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import bpy

sys.path.insert(0, str(Path(__file__).resolve().parent))
from rigging_helpers import rig_procedural_character, scene_rig_metadata


def blender_args() -> list[str]:
    return sys.argv[sys.argv.index("--") + 1 :] if "--" in sys.argv else []


def main() -> int:
    parser = argparse.ArgumentParser(description="Add deforming weights and validation clips to character blend sources.")
    parser.add_argument("blend", nargs="+")
    parser.add_argument("--first-person", action="store_true")
    args = parser.parse_args(blender_args())

    results: list[dict[str, object]] = []
    for value in args.blend:
        path = Path(value).resolve()
        bpy.ops.wm.open_mainfile(filepath=str(path))
        asset_id = path.stem
        armature = next((obj for obj in bpy.context.scene.objects if obj.type == "ARMATURE"), None)
        if armature is None:
            results.append({"asset": asset_id, "status": "missing_armature"})
            continue
        rig_procedural_character(asset_id, armature, first_person=args.first_person or "arms" in asset_id)
        if hasattr(bpy.context.preferences.filepaths, "save_version"):
            bpy.context.preferences.filepaths.save_version = 0
        bpy.ops.wm.save_as_mainfile(filepath=str(path))
        results.append({"asset": asset_id, "status": "upgraded", **scene_rig_metadata(asset_id)})

    print(json.dumps({"results": results}, indent=2))
    return 1 if any(result["status"] != "upgraded" for result in results) else 0


if __name__ == "__main__":
    raise SystemExit(main())
