"""Ready additions preserve the actual previous geometry and animation data."""

import json
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/assets"))
import cook_project_rifle_ready as ready

OUTPUT = Path(sys.argv.pop(1))


class ReadyCookTests(unittest.TestCase):
    def test_physical_ownership_rejects_swaps_and_reflections(self):
        pose = json.loads(ready.POSE.read_text())
        weapon, _ = ready.read(ready.ROOT / "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb")
        ready.validate_physical_contacts(pose, weapon)
        for error in ("contacts", "palm_identity", "reflection"):
            with self.subTest(error=error):
                bad = copy.deepcopy(pose)
                if error == "contacts":
                    bad["weapon_contact_frames"]["R"], bad["weapon_contact_frames"]["L"] = (
                        bad["weapon_contact_frames"]["L"], bad["weapon_contact_frames"]["R"])
                elif error == "palm_identity":
                    bad["palms"]["R"]["source_bone"] = "DEF-hand.L"
                else:
                    for row in bad["palms"]["R"]["matrix"][:3]:
                        row[0] = -row[0]
                with self.assertRaises(ValueError):
                    ready.validate_physical_contacts(bad, weapon)

    def test_reproducible_additions_and_original_bytes(self):
        for name, generated in ready.cook(OUTPUT).items():
            self.assertEqual((OUTPUT / name).read_bytes(), generated)
        for name in ("arms", "body"):
            old, binary = ready.read(OUTPUT / (name + "_runtime.glb"))
            new, output = ready.read(OUTPUT / (name + "_ready.glb"))
            self.assertEqual(binary, output[: len(binary)])
            for key in ("meshes", "skins", "materials", "images", "textures"):
                self.assertEqual(old.get(key), new.get(key))
            self.assertEqual(old["animations"], new["animations"][:-1])
            self.assertEqual(old["accessors"], new["accessors"][: len(old["accessors"])])
            self.assertEqual(old["bufferViews"], new["bufferViews"][: len(old["bufferViews"])])
            for before, after in zip(old["nodes"], new["nodes"]):
                self.assertEqual(
                    {k: v for k, v in before.items() if k != "children"},
                    {k: v for k, v in after.items() if k != "children"},
                )
                self.assertEqual(
                    before.get("children", []), after.get("children", [])[: len(before.get("children", []))]
                )
            self.assertEqual(new["animations"][-1]["name"], "Developer Rifle Ready Idle")
            self.assertTrue(all(c["target"]["path"] == "rotation" for c in new["animations"][-1]["channels"]))
            # Primary/support semantic identity must survive every cook. Swapping
            # these could still make the distance-only IK checks look correct.
            for side in ("r", "l"):
                socket = next(i for i, n in enumerate(new["nodes"]) if n.get("name") == "socket_palm_" + side)
                parent = next(n for n in new["nodes"] if socket in n.get("children", []))
                self.assertEqual(parent["name"], "DEF-hand." + side.upper())
        original = ready.ROOT / "assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb"
        old, binary = ready.read(original)
        new, output = ready.read(OUTPUT / "rifle_ready.glb")
        self.assertEqual(binary, output)
        self.assertEqual(old["meshes"], new["meshes"])
        self.assertEqual(old["nodes"], new["nodes"][: len(old["nodes"])])


if __name__ == "__main__":
    unittest.main()
