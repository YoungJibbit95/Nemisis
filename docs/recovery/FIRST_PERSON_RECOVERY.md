# First-person recovery: diagnosis and bounded implementation

## Phase 2B opt-in production hand constraints (visual gate OPEN)

The game now has an opt-in constraint path using ozz-animation **0.17.0**, exact
commit `744eb9d99f606eda849acb0b1204f7a3dc20bca1`. Archive SHA256 and MIT notice
are unchanged. `cmake/OzzAnimation.cmake` shares that one dependency between
the probe and game. NovaCore remains the only skeleton/clip-sampling/skinning
runtime; ozz supplies only `IKTwoBoneJob` quaternion corrections.

Build capability `NEMISIS_ENABLE_HAND_IK` defaults OFF. Even when built ON,
normal launch keeps legacy presentation; `--first-person-hand-ik` selects the
replacement. The flag is rejected when capability was not compiled. The CI
matrix builds both the probe and capability; a dedicated headless Dev Range
smoke requires nonzero, equal sample/constraint/skin/accepted-vertex-update counts and no invalid or
unreachable frames. Those are CPU mesh updates, not evidence of GPU execution.

Actual dataflow is fixed at 60 Hz:

1. Authoritative movement/weapons update; camera and procedural animation follow.
2. One final presentation snapshot evaluates weapon mount, ADS, sway, recoil,
   reload/procedural offsets, scale and exactly one cooked muzzle alignment.
3. Its final M4 grip matrices and the arms instance become model-space targets.
4. `PlayerSkeletalAnimator::update` advances/samples NovaCore once. After this
   base pose, each wrist target removes its authored hand-socket translation
   and rotation. Bind-calibrated per-side poles follow the shoulder parent;
   current-pose bend axes supply the ozz corrections. Only local rotations change.
5. NovaCore evaluates the corrected globals and skins the weighted mesh once.
   `globalPose()` and `socketTransform()` expose those same final globals.
6. The existing dynamic arms resource receives one vertex update for that tick.
7. `onFrame` submits the cached weapon and the exact arms instance used above,
   with one arms ViewModel submission. There is no render interpolation here;
   repeated render frames do not advance, skin or upload again. Multiple fixed
   ticks retain the existing per-tick update policy, with no added uploads.

The original engine pin wrote a dynamic pose only to the current Vulkan frame
slot and immediately discarded its payload. Since two slots alternate, a render
without a new fixed tick could draw old vertices. A separate engine correction
must retain the latest payload until each slot is refreshed after its own fence;
the tested replacement pin is `a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7`
([engine draft #11](https://github.com/YoungJibbit95/Novacore-Engine/pull/11)).
CPU scheduling tests and graphical compilation are separate from actual GPU
execution; the latter still requires the visual gate.

The raw old mount could not reach the left real M4 grip (hip/ADS wrist distances
0.787/0.884 m against a 0.660 m chain). The replacement retunes the **existing**
artistic A1 mount to hip `(0.28,-0.15,1.00)`, ADS `(0.055,-0.015,0.82)`, uniform
scale `0.90` throughout ADS. Weapon placement stays unchanged. The replacement
does not run the average correction before or after IK. No axis repair, new
mount-correction layer, bone stretch or source/cooked asset change was added.

Invalid chains, indices, nonfinite data, singular/nonuniform/reflected frames
return the base pose transactionally. Unreachable targets use bounded closest
two-bone output, preserve translation/scale and report reach flags/residuals.
The temporary A1 rig is explicitly calibrated by its verified authored chains;
this is not a finished or automatically retargeted Project-character rig.

### Manual Vulkan acceptance and legacy removal

No new GUI authorization exists after the physical Escape stop. No graphical
launch, inputs, screenshots or GPU smoke were performed for this phase. Both
new PRs remain draft until the appropriate acceptance is recorded.

From the `recovery/09-production-hand-ik` checkout with its recorded engine pin:

```powershell
# CMake >=3.30; Windows with VS2022, Vulkan SDK and SDL3 fetch access.
cmake --preset windows-msvc-debug -DNOVACORE_ENGINE_ROOT=F:/Coding/Project/recovery/phase2b/Novacore-Engine -DNEMISIS_ENABLE_HAND_IK=ON -DNEMISIS_BUILD_OZZ_IK_PROBE=ON
cmake --build --preset windows-msvc-debug --parallel 4
# Run separately, from the source checkout. Normal launch is the baseline.
.\build\windows-msvc-debug\Debug\nemisis_game.exe --vulkan --dev-range
.\build\windows-msvc-debug\Debug\nemisis_game.exe --vulkan --dev-range --first-person-hand-ik
```

The machine used for CPU work has GCC, not MSVC. The equivalent local graphical
build commands/results, exact commits and hosted checks are recorded in
PROGRESS.md. A successful build does not mean either command above was launched.

Actual default controls are defined in `InputBindings.cpp` and
`configs/input/default_input.json`: WASD move, mouse look, left mouse fire,
**hold** right mouse for ADS, R reload after firing, Shift sprint (double tap for
tactical sprint), C slide, Space jump/double-jump/mantle, LeftAlt dash, 4 rifle,
5 SMG, 6 sidearm, F pickup, P reset range, V cycle drill, F1 debug/Tab next page.
Use the range wallrun geometry while airborne/moving; no new wallrun key exists.

For each launch select rifle with 4 and reset with P. Compare the same idle/hip,
held ADS, yaw, high/low pitch, recoil while firing, reload, walk/sprint and
slide/wallrun views. Retain a small image for each case and record build SHA,
mode, input and frame; confirm both hand contacts/orientation, no elbow flips,
one arms mesh, no major view obstruction, forward weapon and coherent muzzle
feedback/aim. Reload/fire remain existing procedural presentation plus Idle/Aim
fallback, not newly authored clips. Gameplay ray origin/balance are unchanged.
Compare legacy and IK on identical CPU inputs first; manual images supply the
remaining visible-geometry acceptance. Record any unreachable/invalid shutdown
counts and inspect the corresponding case before enabling by default.

Remove `bindFirstPersonArmsMeshToRigSockets`, its legacy mount variant and the
temporary activation switch **only after** CPU contracts and MSVC/Clang CI pass,
the combined Vulkan image matrix confirms both contacts throughout all cases,
and visual approval is recorded. Then choose the new default in a separate PR;
do not leave two permanent productive rig paths. PR C is intentionally pending.

### Deferred Project head-safe migration (plan only)

PR D starts after hand-IK visual acceptance. Preserve `chr_project_male1` and all
source GLBs. In an isolated asset change, duplicate the Project mesh non-
destructively offline using the existing Blender cooking entry points. Author
explicit face/region selections for head/neck, torso/legs, arms/hands and every
clothing layer; material exclusion alone is unsafe. Store the selection recipe
with a source hash so topology changes cannot silently reuse stale face indices.
Inspect seams and triangles with mixed neck/arm weights in Blender rather than
adding runtime vertex heuristics.

Export separate canonical FPS arms and head-safe visible-body variants, retaining
actual weights, bind matrices, animation channels, attachment sockets and the
necessary ancestor closure. Establish the actual Project deforming arm chains,
twist bones and socket frames explicitly before adapting the small constraint
mapping; A1 names/calibration are not a Project retargeting solution. Keep the
world-body resource and static showcase/targets separate. First-person variants
replace, rather than overlap, the hidden complete local world body.

Cook only retained visible primitives/vertices and remap the used skin palette
plus required ancestors consistently; verify JOINTS/WEIGHTS, bind parity, socket
parity and clip deformation before measuring actual subset skin/upload cost.
Do not claim savings from joint counts alone. Preserve clothing, prevent head
visibility at maximum pitch/camera movement and validate external view, near
clip, hip/ADS/reload/movement on Vulkan. No binary migration or new asset platform
has begun in this session; the expensive full FPS body remains suppressed.

The sections below record the preceding extraction and historical diagnosis.

## Phase 2B shared weapon presentation

The immutable combined base is 5baab6137dc5e0a3a81a2b0e9ec62ac4f8a78fa0
(exact #12/#13/#14 heads in PROGRESS.md), tested with engine
15c1b684a430bb70235ea9407f78106d958812b6. It builds and passes 35 CPU cases.

`evaluateFirstPersonPresentation` extracts the existing game mounts, procedural
rig, single cooked muzzle alignment and legacy arms placement. It returns the
final weapon instance plus complete cooked grip/muzzle frames: markers already
contain asset-node ancestry, followed by exactly one instance transform.
Forward/up define the authored socket rotation; invalid axes yield no IK target.
No asset Euler repair or new rig manager is involved.

`GameApp::onFixedTick` updates simulation, camera and procedural animation at
60 Hz, then owns this cached presentation before the arm animator's one update,
skinning and dynamic vertex upload. `onFrame` has no interpolation or animation
advance and submits the exact cached weapon and arms transform. Repeated render
frames reuse the upload. The selected asset is part of the snapshot, including
when the UI changes the loadout before the next tick. Reset invalidates it;
tool/initial render callers without a snapshot evaluate the same pure function.

This slice preserves the old average whole-arms correction. It supplies the
shared targets for the following IK slice; it does not establish hand contact.
GPU acceptance remains open after the earlier Escape stop. Do not remove the
legacy path or start the Project body asset split before the replacement is
visually accepted. The A1 arms remain a temporary different-character asset.

Baseline: Nemisis 741aeae2edeb2df67564ae0b2379ab06a3df5505, engine pin dbdfc38,
2026-10-18. The user explicitly selected **chr_project_male1**. FP arms remain the
existing A1 temporary asset; do not present it as a matching split of that model.

## Confirmed data and causes

| Asset | Verified source data | Relevant limitation |
| --- | --- | --- |
| chr_project_male1 | 706 skin joints, 78,456 weighted vertices, 7 material primitives; Hard stand, Walk, Run and zero-duration T_pose | No authored FPS split or reload/ADS/fire clips. Body material includes face/hands; Jacket includes arms, so material exclusion alone is not a safe head mask. |
| chr_a1_fp_arms_01 | 7 skin joints, 480 weighted vertices; both upper_arm -> lower_arm -> hand chains; Idle/Aim | Temporary arms, different character. No reload/fire clips; alignment-bar and blockout-LOD nodes are also in the GLB. |
| wpn_project_rifle_m4a1 | Rigid, 9,358 vertices, 13 primitives, muzzle/grips/eject/VFX nodes | No authored weapon clips; reload/recoil are procedural. |

`GameApp::registerDevMeshResources` and its fixed-tick update previously animated
only the A1 operator and arms. The selected Project character was a static
showcase/target, and `appendLocalPlayerBodyMesh` explicitly chose the A1 model.
Resource handles are shared per asset ID: animating the Project asset's shared
handle would also animate all target/showcase instances with the local pose.

`GltfAnimationBridge::buildGltfAnimationAsset` guessed socket joints by names and
discarded socket local TRS. It also omitted non-joint ancestors. Both actual arms
and the Project character have a required Y180 non-joint ancestor. Old-pin C++
negative control: 24 failed attachment/geometry checks; maximum bind drift was
1.75027 m for arms and 2.00225 m for the Project character.

Engine PR #10 preserves those nodes and authored attachment hierarchy, with skin
palette slots unchanged. Against exact engine 15c1b684a430bb70235ea9407f78106d958812b6,
actual bind socket positions/forward/up match static node markers. Bind skinning
drift is at float-rounding scale (arms 3.04e-7 m, Project 7.54e-7 m locally).
These checks do not establish pixel appearance.

`bindFirstPersonArmsMeshToRigSockets` averages STATIC marker-position errors and
translates the whole arms mesh. It does not constrain bones. Its bind hand delta
(.244,-.004,-.120) differs from the M4 grip delta (-.034,.030,.250), so translation
alone cannot meet both targets. Actual animated sockets and a controlled arm
constraint after base animation are needed; authored hand-socket offsets matter.

`PlayerSkeletalAnimator` samples imported clips; it does not retarget
CharacterAnimationFrame.pose onto imported bones. Project's first chest/spine
name match is an unweighted, childless Rigify control, not a reliable upper-body
deformation root. Arms have no chest root. Reload/fire fallback does not create
missing authored animation. Do not describe the current upper-body layer as a
complete arm/weapon attachment solution.

`appendFirstPersonRigPrimitives` adds proxy hands/forearms/torso, even with world
debug lines disabled. `appendMovementTechVisuals` can add three copies of the
whole arms mesh when its debug branch is enabled. Bindings also submit the arms
alignment guide and blockout LOD. These extra submissions are confirmed; which
ones obscured the old screenshot is unverified without renewed image inspection.

## Implemented first bounded game step

Use the selected Project body for the local external/world presentation at its
verified metre scale and feet pivot. It has a dedicated dynamic resource
`player/local/body`; shared Project targets/showcase remain static. Missing local
animation does not silently select another character or an unanimated T-pose.
FPS still suppresses the full head-bearing body until a safe authored mask/split
is available; this step is **not completion of first-person body visibility**.

Body skinning/upload is skipped while the camera rig hides this full body.
Otherwise the existing single pose/skin update feeds its dedicated resource.
Local GCC Debug diagnostic samples (12 updates, no GPU, not a benchmark) were
old/new arms 0.242/0.163 ms and old/new Project 59.5/45.2 ms. The large character is
materially more expensive than the temporary A1 body. No release-frame-time or
GPU-performance conclusion follows from these timings.

## Open-source adoption gate

| Candidate / exact reviewed revision | License, dependencies and platforms | Decision / replacement boundary |
| --- | --- | --- |
| [ozz 0.17.0](https://github.com/guillaumeblanc/ozz-animation/releases/tag/0.17.0), 744eb9d99f606eda849acb0b1204f7a3dc20bca1 | MIT; runtime ozz_animation -> ozz_base. Upstream MSVC and Linux Clang CI. Requires CMake 3.30; disable tools/FBX/glTF/samples/tests/data and align MSVC CRT. | Targeted IKTwoBoneJob prototype for the proven arm-contact problem is justified. It accepts existing model matrices and returns local quaternion corrections; no second clip/skeleton runtime is needed. No library adopted in this first bounded step. |
| [fastgltf 0.9.1](https://github.com/spnda/fastgltf/releases/tag/v0.9.1), 705564d4d54500ae88befb0db9e361e447923655 | MIT plus simdjson 4.6.11 Apache 2.0; C++17; upstream MSVC and Clang CI. | No current parsing blocker or measured advantage. Replacing parser/accessors requires full mesh/material/skin/animation/error adapters; it does not fix downstream bridge/attachment policy. |
| [cgltf 1.15](https://github.com/jkuhlmann/cgltf/releases/tag/v1.15), 360db1a95480fe102ae9c69b27c5d101167ff5ba | MIT, embedded JSMN notice; one C implementation TU, no external runtime dependency. Windows/Linux CI; explicit release Clang matrix not verified. | Retain as a future small importer alternative. Same adapter/parity work; no reason to migrate during this repair. |

Sources: upstream release licenses/CMake/CI, ozz
[IKTwoBoneJob](https://github.com/guillaumeblanc/ozz-animation/blob/0.17.0/include/ozz/animation/runtime/ik_two_bone_job.h)
and [official application order](https://github.com/guillaumeblanc/ozz-animation/blob/0.17.0/samples/framework/utils.cc).
The initial comparison did not measure build-time/binary-size deltas or install a
dependency; the subsequent opt-in probe is recorded below. Any adoption must pin its commit, retain licenses/notices and
validate both platforms. Keep IK presentation-only; no claim of cross-platform
bit-identical library output. Jolt/VMA/network replacements solve no current
first-person blocker and remain outside scope.

## Optional ozz adoption probe

`-DNEMISIS_BUILD_OZZ_IK_PROBE=ON` builds `nemisis_ozz_arm_ik_probe` and adds one CPU
CTest case (35 CPU cases with the option, 34 without). The default game path has
no new dependency or solver; disabled configuration was checked to have no ozz
population/targets. CI explicitly enables the probe on MSVC and Clang.

The exact ozz0.17.0 commit above is fetched using source-archive SHA256
`a29a8a014b571487d9a6dabebeed57627fc19bf1f0ebf531b0c76384840c3b74`.
The MIT notice is retained in third_party/licenses and copied beside the probe.
Only ozz_animation/ozz_base build; tools, samples, importers, FBX and data are off.
The opt-in configuration requires CMake3.30; the default game's minimum remains
3.27. CRT selection follows the parent. No ozz Skeleton, SamplingJob or second
clip runtime is used: NovaCore samples and skins the actual A1 arms asset.

The probe converts row-major matrices by elements, derives the current bend
plane, runs IKTwoBoneJob, applies local quaternion corrections on the right and
preserves authored hand-socket orientation/offsets. It checks both real arm
chains for bind/Idle/Aim poses and three small reachable targets per pose:
18 successful solves, preserved bone lengths/orientation and actual weighted
mesh deformation. Local residuals were1.49e-8..1.69e-7m. These are model-space
CPU targets, **not final M4 grip integration or visual approval**.

First isolated GCC Debug ozz configure/build took23.55s with an already-built
engine (uncontrolled diagnostic measurement). Debug archives: base556,154 bytes,
animation2,005,102 bytes; full linked probe10,882,991 bytes. Those are artifact
sizes, not the incremental cost of adding ozz to the game. No GPU/release
performance claim. Full local opt-in build passes35 CPU cases; final skinning
assertions also pass the affected probe. Product adoption still requires final
weapon/grip ownership, runtime integration, the authored FPS-body decision and
the renewed visual gate. The probe replaces the need to invent a new IK solver;
it does not yet replace the game's whole-mesh placement workaround.

Published [draft probe PR #14](https://github.com/YoungJibbit95/Nemisis/pull/14),
implementation c6a2d7e4dddeacb13c4a4a82024c056e740b2418. Fresh hosted Windows/MSVC
and Ubuntu/Clang builds and35 CPU cases pass, with archive hash verification:
https://github.com/YoungJibbit95/Nemisis/actions/runs/39128535291.
Merge order: engine #10, selected-character/pin #12, then this optional probe.
Geometry #13 is independent and combines cleanly. Main and all user changes remain
untouched; no GUI authorization/visual acceptance is inferred from these results.

## Remaining acceptance

There is one existing procedural weapon mount, then cooked muzzle alignment;
current skinned hands still need constraints. Next integrate final weapon/grip
targets with the current arm pose before final skinning, preferably using the
reviewed ozz solver instead of writing a new IK library. Wrist target must account
for authored hand-socket offset/orientation, not equate hand joint to grip.

The desired character needs a deliberate head-safe FPS body/arms mask or authored
split, including its clothing. Preserve both GLBs and offline fixups. Prove
locomotion, hip/held ADS, fire/reload, camera yaw/pitch, slide/wallrun, and external
body appearance on a usable GPU before calling the recovery visually complete.
No new GUI control is authorized after the earlier physical Escape stop; this
phase has so far used source, CPU tests and logs only.
