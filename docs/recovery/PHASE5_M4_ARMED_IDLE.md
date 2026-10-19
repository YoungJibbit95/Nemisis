# Phase 5: authored M4 Idle and coherent fade endpoints

Status: technical M4 preview candidate. Vulkan and artist acceptance remain
open. Normal CLion/start-script selection still uses the existing legacy
profile. Review this package explicitly with `--project-rifle-ready` after a
new graphical authorization and a separately built executable.

The recorded c04 Vulkan hip images showed a receiver dominating the right side,
an unreadable firing hand and open support fingers. The old frustum gate passed
with 158 right-hand vertices and therefore did not establish readable grip
placement. Those actual images remain the failed baseline; this package has no
Vulkan after-image yet.

## Authored geometry and pose

`Project M4 Armed Idle` is a distinct one-second held reference clip. It bakes
72 actual Project upper-body/finger rotations and animates the existing
`socket_rifle_ready` translation/rotation. It uses the original character's
arms/body union, deform weights, hierarchy and anatomical eye. The explicit
offline mount is (-.13,1.555,.295)m. Its five-millimetre final height adjustment
restored firing-hand readability after distributing the trigger-finger curl.
The eye remains (.02029883,1.69091368,.07649009)m in the authored pose; the
74-degree camera, projection and model scale are unchanged.

The actual `DEF-hand.R` owns the measured pistol grip and trigger; `.L` owns
the forward handguard. The R contact is a proper frame whose longitudinal
finger axis is +Z and index-to-pinky axis is +Y. Its heel is measured 15mm rearward
and 9.5mm lower on the actual grip surface, inside the existing 20mm firing
region. The support contact is measured 4.5mm toward the fingers on the real
bottom handguard surface, inside its unchanged 10mm region. The older Developer
Ready fallback also receives these corrected contact frames; its former frame
was never visually accepted. No new contact-marker owner or hand-name swap is
introduced.

Finger authoring uses positive curls around measured source hinge axes, bounded
MCP abduction and thumb opposition. Free finger-chain IK could put skin near a
surface while twisting a chain root or hyperextending a knuckle. The checked
pose instead records the actual hinge axes and angles. A quaternion regression
reconstructs the real glTF local rotations from the original source joints and
rejects a free 90-degree index twist. The firing index uses a distributed
70-degree PIP/37.5-degree DIP curl and meets the physical `Trigger` mesh.

Offline evaluated skin evidence, Blender 5.1.0/adfe2921d5f3:

- Ten source-weighted distal finger patches meet real weapon surfaces within
  2mm. Maximum signed nearest-triangle penetration is 0.833mm; this remains an
  offline surface diagnostic, without a watertight collision/self-contact claim.
- Actual R index-to-Trigger skin distance is 0.0118mm.
- All 417 rear-stock vertices are queried against the actual rendered character
  subset union. Closest shoulder-skin distance is 3.780mm; maximum signed
  penetration is 0. The old frozen pose measured 52.75mm minimum separation.
- Maximum local translation drift is 0.81 micrometres, scale drift 8.2e-7 and
  affine reconstruction error 7.2e-7. Original source assets remain unchanged.

The authoring preparation retains 116 original runtime node frames exactly and
merges the original 103-joint arms/body palette union for Blender. It strips
materials/animations only from the temporary authoring copy. The production
81/57 skin palettes and original geometry/clip accessors remain intact.

## Runtime fade ownership

An initial Idle-only implementation exposed the old post-sample mask seam.
AnimationRuntime blended raw unarmed Run while the displayed pose used a
separate Ready mask. Returning to Idle therefore blended from a different pose
than the one on screen. Production-input tick 216 crossed camera-right by 31.08mm
and missed the L target by 13.21mm; rapid-request tick 304 also failed. A temporary
fade flag addressed only part of that seam and was removed.

The cook now retains every original source clip plus `Developer Rifle Ready
Idle`, and appends:

- `Project M4 Armed Idle` for Idle selection.
- `Developer Rifle Ready Walk Composition` and `Developer Rifle Ready Run
  Composition` as explicit developer fallbacks. They combine the existing Ready
  rotations with original unmasked motion keys and own their corresponding
  default mount TRS.

The existing AnimationRuntime now owns both displayed fade endpoints. These
exact clips skip the post-sample rotation mask. No new animation clock or pose
manager is added. The cook checks original BIN/accessor preservation, exact
unmasked channel/time references, original clip duration, unique (node,path)
channels, matching masked rotations and explicit mount TRS at both endpoints.

Run is also the existing selector fallback for the Sprint request and several
advanced states. These compositions are not authored armed Walk/Run/Sprint or
directional animation. Crouch, ADS, fire, reload, slide, mantle, airborne states,
world-model weapon animation and full combat animation acceptance remain open.

## Strict CPU gates

The isolated serial GCC 13.2/O2 harness uses the coherent public NovaCore
`14d33a77a9eedef6a48227226d1ef80d593015b9` headless package. It builds the actual
11 scene/player/hand-constraint translation units and runs the existing Ready
target. Two independent sequences retain the 1mm/.5-degree palm gates, positive
actual rolled camera-right side, reachable hands, immutable eye/mount, original
palette identities, common body/arms root and exactly-once sampling/skinning:

| Sequence | Samples | Maximum palm error | Minimum rolled firing-side margin |
| --- | ---: | ---: | ---: |
| Original 288 direct stress frames plus rapid interrupted requests |350|0.989147mm|+126.665mm|
| Separate actual CharacterAnimationInput composition plus interruptions |350|0.952050mm|+132.282mm|

Production composition supplies actual `viewVectors(view).horizontalForward`
and `.horizontalRight`; it adds no aim/turn offsets. These contact/fade tests do
not establish authored animation quality for their fallback selections.

An independent 160x90 CPU depth raster inspects the actual skinned hand and M4
triangles at the anatomical hip eye. The unchanged gates require a bounded
receiver, a clear central aiming region and visible R hand around the physical
pistol grip. Final measured Base bounds are 24.53% viewport width/76.36% height,
with its top at 52.65% of screen height. Weapon occupancy is 9.4167%; 30 weapon
pixels occupy the central 32x27 region. 284/625 R-hand pixels remain after weapon
occlusion (45.44%), including 128 pixels around the firing grip. This is a geometry
regression, not a Vulkan material/lighting or visual acceptance oracle.

Negative control: the former f59 Developer Ready asset, cooked from the previous
source recipe, fails the same receiver/readability gates: unclipped projected
Base bounds 184.6%/291.7% and 0 firing-grip-region hand pixels. The positive files
were restored byte-for-byte afterward. The old source image is not relabeled
as an accepted state.

Pure-Python preservation/hinge tests: 3/3 passed. Repeated Blender authoring
reproduced the exact metadata SHA256 below. Full combined game build, hosted
MSVC/Clang checks and a renewed Vulkan comparison are integration gates; the main
agent owns them and the reserved CMake dependency for the new metadata.

## Reproduction and immutable source evidence

After building the existing Project FPS candidate:

```powershell
python tools/assets/prepare_project_rifle_authoring.py --candidate-dir build/local-debug-no-deps/project-fps-candidate --output build/armed-idle-authoring/rig.glb
blender --background --factory-startup --offline-mode --threads 1 --python-exit-code 1 --python tools/assets/author_project_rifle_ready.py -- --profile armed-idle --source build/armed-idle-authoring/rig.glb --output build/armed-idle-authoring
python tools/assets/cook_project_rifle_ready.py --candidate-dir build/local-debug-no-deps/project-fps-candidate --check
python tests/project_rifle_ready_cook_tests.py build/local-debug-no-deps/project-fps-candidate
ctest --test-dir build/local-debug-no-deps -R nemisis_project_rifle_ready_tests --output-on-failure
```

Review the generated JSON/`.blend` before replacing checked authoring metadata.
Blender and generated GLBs remain authoring/build dependencies; no `.blend` is
loaded at runtime. The unchanged default `ready` authoring profile retains the
f59 Developer reference. All assets are the original Project content; no
reference-game meshes or animations were copied.

SHA256 evidence:

```text
project_m4_armed_idle_pose.json
4e49052d6052f593c1cdeb7bffe39588075e74a69c10f7c0552aeda1f3485462
arms_ready.glb
3b0598713fed45fa73d53cbe1dca5b7c6c38b487fed1074ef0d37e3b9d15e26f
body_ready.glb
f8ee637677b996c4c0df991bed10352d5905e6a08ab1ad36f3e2fc902afedd92
rifle_ready.glb
e8e84fd55adf16a9d420b5fc5f43d623c1a1bcbda492ac967f96d335dd254a3b
```
