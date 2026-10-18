"""Fail-closed source/selection gates, plus byte-for-byte recook verification."""
import copy
import importlib.util
import json
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("cook", ROOT / "tools/assets/cook_project_fps_candidate.py")
cook = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cook)
OUTPUT = Path(sys.argv.pop(1))


class CandidateCookTests(unittest.TestCase):
    def test_source_hash_gate_precedes_cooking(self):
        recipe = json.loads(cook.RECIPE.read_text(encoding="utf-8"))
        for field in ("source", "original_source"):
            invalid = copy.deepcopy(recipe)
            invalid[field]["sha256"] = "0" * 64
            with self.assertRaisesRegex(ValueError, "Source hash changed"):
                cook.load_source(invalid)

    def test_invalid_region_and_palette_fail_closed(self):
        for ranges in ([[2, 1]], [[0, 2], [2, 3]], [[-1, 0]], [[0, 9]]):
            with self.assertRaises(ValueError):
                cook.ordinals(ranges, 9)
        recipe = json.loads(cook.RECIPE.read_text(encoding="utf-8"))
        source, binary = cook.load_source(recipe)
        invalid = copy.deepcopy(recipe)
        invalid["primitives"][0]["vertex_count"] += 1
        with self.assertRaisesRegex(ValueError, "topology changed"):
            cook.cook_variant(source, binary, invalid, "arms")
        invalid = copy.deepcopy(recipe)
        invalid["variants"]["arms"]["skin_slots"].append(705)
        with self.assertRaisesRegex(ValueError, "closure changed"):
            cook.cook_variant(source, binary, invalid, "arms")

    def test_committed_recipe_recooks_exactly(self):
        recipe = json.loads(cook.RECIPE.read_text(encoding="utf-8"))
        outputs, _ = cook.cook(recipe)
        for relative, data in outputs.items():
            with self.subTest(file=relative):
                self.assertEqual((OUTPUT / relative).read_bytes(), data)

    def test_regions_are_disjoint_and_exclude_head_influences(self):
        recipe = json.loads(cook.RECIPE.read_text(encoding="utf-8"))
        source, binary = cook.load_source(recipe)
        forbidden = set(recipe["regions"]["head_neck_joints"])
        slots = {i for i, node in enumerate(source["skins"][0]["joints"])
                 if source["nodes"][node]["name"] in forbidden}
        for primitive, region in zip(source["meshes"][0]["primitives"], recipe["primitives"], strict=True):
            arms = set(cook.ordinals(region["arms"], region["triangle_count"]))
            body = set(cook.ordinals(region["body"], region["triangle_count"]))
            self.assertFalse(arms & body, "arms/body must not duplicate any source triangle")
            indices = cook.values(source, binary, primitive["indices"])
            joints = cook.values(source, binary, primitive["attributes"]["JOINTS_0"])
            weights = cook.values(source, binary, primitive["attributes"]["WEIGHTS_0"])
            for triangle in arms | body:
                for corner in range(3):
                    vertex = indices[triangle * 3 + corner][0]
                    self.assertFalse(any(j in slots and w > 0 for j, w in zip(joints[vertex], weights[vertex])),
                                     "candidate must not retain a positive head/neck influence")
            if region["material"] in ("Neck", "Hair2"):
                self.assertFalse(arms | body, "neck/hair geometry excluded explicitly")


if __name__ == "__main__":
    unittest.main()
