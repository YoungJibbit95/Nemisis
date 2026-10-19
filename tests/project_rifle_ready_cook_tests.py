"""Ready additions preserve the actual previous geometry and animation data."""

import json
import copy
import math
import struct
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools/assets"))
import cook_project_rifle_ready as ready

OUTPUT = Path(sys.argv.pop(1))


class ReadyCookTests(unittest.TestCase):
    def test_actual_finger_rotations_retain_positive_source_hinges(self):
        pose = json.loads(ready.ARMED_IDLE_POSE.read_text())
        original, _ = ready.read(ready.ROOT / "assets/processed/normalized/project_assets/chr_project_male1.glb")
        nodes = {n.get("name"): n for n in original["nodes"]}

        def multiply(a, b):
            x, y, z, w = a
            i, j, k, r = b
            return [w*i+x*r+y*k-z*j, w*j-x*k+y*r+z*i,
                    w*k+x*j-y*i+z*r, w*r-x*i-y*j-z*k]

        def rotation(axis, degrees):
            self.assertAlmostEqual(sum(v*v for v in axis), 1, delta=1e-5)
            radians = math.radians(degrees) / 2
            return [v * math.sin(radians) for v in axis] + [math.cos(radians)]

        def validate(authored):
            self.assertEqual(len(authored["finger_surface_goals"]), 10)
            for identity, finger in authored["finger_surface_goals"].items():
                side, name = identity.split("/")
                limits = (90, 95, 85) if name == "thumb" else (110, 110, 85)
                for i, (angle, limit) in enumerate(zip(finger["curl_degrees"], limits)):
                    self.assertGreaterEqual(angle, 0)
                    self.assertLessEqual(angle, limit)
                    joint = f"DEF-{name}.{i+1:02}.{side}"
                    delta = rotation(finger["gltf_hinge_axes_local"][i], angle)
                    if i == 0:
                        abduction = finger["mcp_abduction_degrees"]
                        opposition = finger["thumb_opposition_degrees"]
                        self.assertLessEqual(abs(abduction), 60 if name == "thumb" else 20)
                        self.assertLessEqual(abs(opposition), 60 if name == "thumb" else 0)
                        delta = multiply(rotation(finger["gltf_mcp_abduction_axis_local"], abduction), delta)
                        delta = multiply(rotation(finger["gltf_thumb_opposition_axis_local"], opposition), delta)
                    expected = multiply(nodes[joint].get("rotation", [0, 0, 0, 1]), delta)
                    actual = authored["joint_pose"][joint]["rotation_xyzw"]
                    error = min(max(abs(a-b) for a, b in zip(actual, expected)),
                                max(abs(a+b) for a, b in zip(actual, expected)))
                    self.assertLess(error, 2e-5, joint + " must retain the measured hinge instead of a free IK twist")

        validate(pose)
        bad = copy.deepcopy(pose)
        bad["joint_pose"]["DEF-f_index.01.R"]["rotation_xyzw"] = multiply(
            bad["joint_pose"]["DEF-f_index.01.R"]["rotation_xyzw"], [0, math.sqrt(.5), 0, math.sqrt(.5)])
        with self.assertRaises(AssertionError):
            validate(bad)

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
        armed = json.loads(ready.ARMED_IDLE_POSE.read_text())
        ready.validate_physical_contacts(armed, weapon)
        # Finger longitudinal +Z reaches the trigger; index-to-pinky is +Y.
        # The old fallback frame pointed every primary finger down the handle.
        frame = armed["weapon_contact_frames"]["R"]["matrix"]
        self.assertEqual([r[2] for r in frame[:3]], [0, 0, 1])
        self.assertEqual([r[0] for r in frame[:3]], [0, 1, 0])

    def test_reproducible_additions_and_original_bytes(self):
        for name, generated in ready.cook(OUTPUT).items():
            self.assertEqual((OUTPUT / name).read_bytes(), generated)
        for name in ("arms", "body"):
            old, binary = ready.read(OUTPUT / (name + "_runtime.glb"))
            new, output = ready.read(OUTPUT / (name + "_ready.glb"))
            self.assertEqual(binary, output[: len(binary)])
            for key in ("meshes", "skins", "materials", "images", "textures"):
                self.assertEqual(old.get(key), new.get(key))
            self.assertEqual(old["animations"], new["animations"][:-4])
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
            self.assertEqual(new["animations"][-4]["name"], "Developer Rifle Ready Idle")
            self.assertTrue(all(c["target"]["path"] == "rotation" for c in new["animations"][-4]["channels"]))
            self.assertEqual(new["animations"][-3]["name"], "Project M4 Armed Idle")
            mount = next(i for i, n in enumerate(new["nodes"]) if n.get("name") == "socket_rifle_ready")
            for clip in new["animations"][-3:]:
                targets = [(c["target"]["node"], c["target"]["path"]) for c in clip["channels"]]
                self.assertEqual(len(targets), len(set(targets)), "one owner per animated node/path")
                mount_paths = {c["target"]["path"] for c in clip["channels"] if c["target"]["node"] == mount}
                self.assertEqual(mount_paths, {"translation", "rotation"})

            def values(accessor):
                item = new["accessors"][accessor]
                self.assertEqual(item["componentType"], 5126)
                width = {"SCALAR": 1, "VEC3": 3, "VEC4": 4}[item["type"]]
                view = new["bufferViews"][item["bufferView"]]
                start = view.get("byteOffset", 0) + item.get("byteOffset", 0)
                stride = view.get("byteStride", width * 4)
                return [struct.unpack_from("<" + "f" * width, output, start + i * stride)
                        for i in range(item["count"])]

            for hint, composed in zip(("walk", "run"), new["animations"][-2:]):
                self.assertEqual(composed["name"], "Developer Rifle Ready " + hint.title() + " Composition")
                original = next(a for a in old["animations"] if hint in a["name"].lower())
                masked = {c["target"]["node"] for c in new["animations"][-4]["channels"]}
                for channel in original["channels"]:
                    if channel["target"]["path"] == "rotation" and channel["target"]["node"] in masked:
                        continue
                    match = next(c for c in composed["channels"] if c["target"] == channel["target"])
                    self.assertEqual(composed["samplers"][match["sampler"]], original["samplers"][channel["sampler"]])
                original_duration = max(t[0] for sampler in original["samplers"] for t in values(sampler["input"]))
                composed_duration = max(t[0] for sampler in composed["samplers"] for t in values(sampler["input"]))
                self.assertEqual(original_duration, composed_duration)
                for channel in new["animations"][-4]["channels"]:
                    match = next(c for c in composed["channels"] if c["target"] == channel["target"])
                    self.assertEqual(values(composed["samplers"][match["sampler"]]["output"]),
                                     values(new["animations"][-4]["samplers"][channel["sampler"]]["output"]))
                for channel in composed["channels"]:
                    if channel["target"]["node"] != mount:
                        continue
                    expected = new["nodes"][mount][channel["target"]["path"]]
                    for actual in values(composed["samplers"][channel["sampler"]]["output"]):
                        self.assertTrue(all(abs(a-b) < 1e-6 for a, b in zip(actual, expected)),
                                        "developer fallback owns its actual default mount TRS throughout the clip")
            self.assertNotIn("matrix", new["nodes"][mount])
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
