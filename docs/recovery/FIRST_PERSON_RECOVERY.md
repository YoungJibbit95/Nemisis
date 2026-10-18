# First-person recovery: diagnosis and bounded implementation

## Phase 2D: actual Project FPS runtime preview (2026-10-18)

`--project-fps-preview` now loads, animates and submits the real Project subsets.
This is **Technically Integrated as a developer preview**, **not Visually
Accepted**, **not Weapon Rig Completed**. Seam/camera/animation limitations below
remain release blockers. Normal launch keeps the existing A1 path unchanged.

Build with `-DNEMISIS_BUILD_PROJECT_FPS_CANDIDATE=ON`; the existing cook generates
the same geometry plus its registry manifest/metadata. The game target depends
on this cook and stages `project-fps-candidate` beside the EXE (including MSVC
configuration subdirectories). Runtime checks the staged folder, then the
configured build root; it never loads Blender files. Missing/invalid preview
assets fail explicitly before graphics startup. No new manager or asset loader.

| Asset ID | Dedicated GPU resource | Render layer |
| --- | --- | --- |
| chr_project_male1_fp_body_preview | player/local/fps_body | World |
| chr_project_male1_fp_arms_preview | player/local/fps_arms | World, same projection/root as body |
| chr_project_male1 (complete external player) | player/local/body | World, suppressed in FPS |

The two candidate records are mounted into the existing AssetRegistry/MeshCatalog
only for the preview. Body uses the actual player feet/root, metre scale and
player yaw, plus existing movement-presentation offsets. It is not a static
mesh in front of the camera. Its head/neck/hair geometry remains excluded by the
validated offline recipe. Following the user's GPU rejection, arms use exactly
the same player root, metre scale, body yaw/pitch/roll and World projection as
the body. The floating camera mount[0,-1.28,.45] is removed. Hands remain unbound
to the weapon; no A1 whole-mesh averaging or forced ready-pose is substituted.

The old supposedly normalized character actually faces -Z: both toe joints are
over0.05m behind their corresponding foot joints along Z, contrary to gameplay
+Z. The existing subset cook now additionally emits arms_runtime.glb and
body_runtime.glb. One shared offline scene parent turns that measured forward
direction to+Z and centers the rig's horizontal root at the player pivot.
Original subset GLBs/maps, source GLBs, every original local TRS/key/weight and
the complete binary chunks remain unchanged; no runtime Euler repair is added.
Independent tests verify anatomical forward, root centering, unchanged bytes
and actual skinned vertices transformed exactly once.

New socket_fps_eye is authored from the midpoint of ORG-eye.L/R in the actual
head-parent frame, not the displaced eye-control handles or old guessed camera
socket. Its bind position matches that anatomical midpoint. The sampled body
pose supplies this eye point each tick; the camera's position is transformed
through the same player/body matrix before the shared weapon pose is evaluated.
Aim orientation stays under the existing player camera controls. Body/arms share
the authored locomotion clock and shoulder placement. This is a skeleton-linked
camera position, not a camera floating beside an independently mounted model.

The existing PlayerSkeletalAnimator evaluates authored Hard stand/Walk/Run on
both subsets using the same locomotion input/delta/transition clock. Unsupported
ADS/reload/fire upper-body layering is disabled for these subsets; source keys,
nonuniform scales and all weighted intermediate segments are retained. The same
fixed tick samples/skins each visible subset once and submits one accepted
vertex update each. Extra render frames only consume the cached presentation.
The hidden full78,456-vertex world body receives no FPS skin/update passes.
Its existing independent animated external representation remains available.

The first renewed Vulkan run exposed a gameplay adapter bug during rapid movement
changes: NovaCore rejects a second crossfade while the first is active. The
adapter now finishes that short fade before consuming the latest locomotion
request. A real-subset regression exercises repeated interrupted requests and
settling back to idle; no engine/runtime or animation data changes are needed.
Preview failures now distinguish missing resources, animator diagnostics and
GPU vertex update rejection.

The preview fixes the initial loadout to ar_01 / wpn_project_rifle_m4a1 and
suppresses loadout switching/pickups and user-settings persistence in this mode.
No A1 GPU resource is registered or drawn in the Project mode; missing Project
resources never fall back to A1 or procedural arm/body proxies. The full Project
showcase/targets retain their separate static resource.

Start from a graphical build's runtime data directory:

```powershell
& ./build/phase2c-vulkan/nemisis_game.exe --vulkan --dev-range --project-fps-preview
& ./build/phase2c-vulkan/nemisis_game.exe --vulkan --dev-range --project-fps-preview --project-fps-look-down
```

The second command initializes real player look pitch to-70 degrees (also on P
reset) for a repeatable body screenshot; subsequent ordinary mouse input still
controls look. It requires preview mode. These commands do not authorize GUI
control after the latest physical Escape stop; renewed permission is required.
For manual proof, record both views with the same commit/EXE hash/engine pin,
then walk, crouch, sprint/slide, shoot/reload and inspect camera intersections.

The earlier local GCC13.2 Release build and40/40 CPU cases passed (39 prior +1 actual game
preview smoke). Real subset render tests cover three authored locomotion states,
hip/high/low camera angles, exactly one dynamic submission each, correct layers,
player-root placement, repeat-render stability, separate external representation
and missing-resource rejection. The earlier8,188 hip arm vertices were measured
with the rejected floating mount, not evidence for correct shoulder placement.
That visibility-only assertion is replaced by anatomical root/projection and
skinned-coordinate parity coverage. With the source's unarmed idle/walk clips,
hands may leave the hipfire frustum; a genuine weapon-ready animation/constraint
is required rather than moving disconnected arms into view again.
Runtime smoke records equal body/arms sample/skin/update counts, world_updates=0
and both actual Project submissions. Invalid metadata fails startup explicitly.

`--project-fps-preview --first-person-hand-ik` rejects with exit2 and a concrete
reason. Project palm frames have not been authored/approved; A1's adapter assumes
uniform frames and direct parents and cannot compute the Project wrist rotation
from a product of only the selected joints. **ozz itself does support non-direct
ancestor joints**; five segments alone are not an upstream solver limitation.
The intermediate .001 bones must stay weighted and retain their TRS. Upstream
nonuniform-scale tests cover a common transformed parent, not proof of arbitrary
per-segment animated scale/shear plus a rigid palm orientation. A bounded
full-affine ancestor-chain adapter/authoring proposal still needs verification.
No new solver or stripped-scale workaround was introduced.

Other open visual/animation limitations: no authored weapon-ready/reload/fire/
crouch/slide Project clips; corresponding states currently use authored idle/run
fallbacks and existing whole-body presentation offsets. Crouch torso-camera and
foot/ground behavior, sleeve joins and unbound hand contact are not accepted.
The separate M4 source-up fix is draft PR#19, implementation336cf73; combine it
for corrected weapon images, without including its asset changes in this PR.

Renewed actual RTX3070Ti Vulkan run at integration d60ce1f16437ef4ea6355168d4f9d1a89c9b166d
(preview6302df8 + M4#19) produced real hipfire and movement/look-down images.
User inspection **rejected** the floating/high arms, reversed character and
independent camera placement. Those images are findings, not visual acceptance.
The run ended normally after4619 samples/skins/vertex updates for each subset,
world_updates=0 and one active submission per subset. Corrected anatomical
placement must be reviewed again; weapon-ready/ADS/reload contact remains open.

## Phase 2C Project FPS technical candidate (2026-10-18)

**Technically Correct for subset skinning; not Visually Accepted; not Character
Integrated.** Gameplay still uses the A1 test arms. This candidate changes no
product registration/default, IK solver, renderer or legacy placement path.

Enable `-DNEMISIS_BUILD_PROJECT_FPS_CANDIDATE=ON` on an existing headless preset
to generate and test it. This optional capability requires Python3.10+ (standard
library only); ordinary builds retain no new dependency. CI enables it alongside
the existing ozz probe and hand-IK options. Alternatively:

```powershell
python tools/assets/cook_project_fps_candidate.py --output build/project-fps-candidate
python tools/assets/cook_project_fps_candidate.py --output build/project-fps-candidate --check
```

Outputs are `arms.glb`, `body.glb`, binary source oracle maps, `report.json` and
shared original texture byte copies under the requested build directory. Large
duplicate binaries are not committed. Both original Project GLBs are unchanged.
The manifest `assets/metadata/project_fps_candidate_regions.json` freezes 170
inclusive source triangle ranges, primitive topology and SHA256 of both inputs.
Source/topology drift fails before writes and requires explicit region review.
Initial ranges came from the offline deform-weight audit; neither cooking nor
gameplay uses positional/weight-threshold face-removal heuristics.

| Asset | Vertices | Triangles | Primitives | Skin palette / runtime pose nodes |
| --- | ---: | ---: | ---: | ---: |
| Original world | 78,456 | 108,582 | 7 | 706 / 709 |
| Candidate arms | 9,536 | 15,942 | 2 | 81 / 84 |
| Candidate body | 40,532 | 53,682 | 5 | 57 / 60 |

Arms include Jacket sleeves and Body hands. Body excludes these exact triangles,
all triangles touching positive head/neck weights, and the entire Neck/Hair
primitives, including four residual low-weight Neck triangles. No material hide
or runtime vertex deletion. Sleeve/neck boundaries still need artist approval;
head/neck-weight exclusion alone is not a rendered camera-clipping guarantee.

The cook preserves original node indices/hierarchy/TRS, materials, all selected
vertex attributes/weights, inverse binds, all seven socket transforms and exact
animation sampler bytes for retained nodes. JOINTS and inverse-bind rows are
remapped together; actual positive influences, ancestors and socket ancestors
determine the retained palette. Unrelated animation channels are omitted. All
four clip names/durations remain, including zero-duration T_pose. Required split
segments and nonuniform scales are retained. Both variants share texture copies.

Public NovaCore importer/bridge contracts compare every selected triangle and
winding, position/normal/UV, weight and positive joint mapping. Bind and every
clip at 0%,37%,83% compare retained globals, full socket frames and all skinned
subset vertices against the original. Maximum local subset drift: **0 m**.
Authored clips deform arms/body by up to1.112/0.985m. Negative tests reject source
hash drift, topology drift, invalid/overlapping ranges and wrong palette closure;
they also check disjoint regions, no positive head/neck influence and exact recook.
Full local Release build and39/39 CPU cases pass (37 existing +2 candidate cases).

GCC13.2/Ninja Release on AMD Ryzen5 5500, exact engine
`a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7`, Walk(1_24),20 warmup +120 measured
samples, no running game during measurement:

| Asset | Median CPU ms | p95 CPU ms | Position+normal arrays | Vulkan payload per consumed slot* |
| --- | ---: | ---: | ---: | ---: |
| Full original | 9.428 | 11.919 | 1,882,944B | 2,510,592B |
| Arms | 0.968 | 1.563 | 228,864B | 305,152B |
| Body | 2.289 | 3.350 | 972,768B | 1,297,024B |

Reproduce: configure Release with the candidate option, then run
`build/candidate-release/nemisis_project_fps_candidate_tests.exe --benchmark`.
Public sampling/global/skin APIs are timed, including their own allocations and
validation. Import/file IO and GPU work are excluded. Single-machine numbers
are not a frame-budget guarantee. *Payload is calculated from pinned
VulkanMeshVertex (8floats), not measured GPU upload time. Gameplay does not yet
submit these candidates, so actual candidate upload/draw costs remain unmeasured.

Concrete blockers: actual left chain is DEF-upper_arm.L -> DEF-upper_arm.L.001
-> DEF-forearm.L -> DEF-forearm.L.001 -> DEF-hand.L (palette436..440), right
560..564. All five segments carry weights. Weighted DEF-shoulder.L/R are siblings
under ORG-shoulder, not upper-arm parents. The current A1 adapter requires three
direct joints and uniform frames; Project clips genuinely animate nonuniform
scales. Do not discard split bones/scales or choose similarly named controls.

Preserved hand socket offsets are approximately0.414m R /0.487m L from the wrists,
with nonuniform scale; they are not usable authored palm-contact frames. Artist
work must define palm/finger frames and sleeve/torso/neck boundaries (140 mixed
arm-boundary triangles,297 mixed head/neck-boundary triangles). No caps/seams
were invented. Only then adapt the existing ozz path safely for these actual
segments/scales, and render the complete acceptance matrix before registration.
No second sampling/IK framework is justified by this candidate.

## Phase 2B opt-in production hand constraints (visual gate OPEN)

Published [draft game #16](https://github.com/YoungJibbit95/Nemisis/pull/16),
stacked after [shared-pose draft #15](https://github.com/YoungJibbit95/Nemisis/pull/15).
PROGRESS.md records exact pins, integration ancestry, local results and the next
executable session step. Hosted checks belong to the PRs; no merge is automatic.

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

## 2026-10-18 Phase 3: affine Project hand adapter

The isolated review base `recovery/phase3-baseline` (78bf2b7) combines merged
#18 with the still-draft #19/#20/#21. It is a review dependency, not a merge into
main. NovaCore remains exactly a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7.

The existing PlayerHandConstraints API now recognizes the actual Project chain:
upper_arm -> upper_arm.001 -> forearm -> forearm.001 -> hand, on each side.
It reuses ozz IKTwoBoneJob with bounded affine correction passes and a polar
wrist orientation solve. Source local translations/scales, weighted .001 joints,
fingers, inverse binds and weights remain unchanged. Final world-space socket
residuals decide success (1 mm position, 0.5 degrees orientation); ozz's reach
flag alone is insufficient. Invalid inputs roll back both arms transactionally;
unreachable inputs retain the best finite bounded pose and report its error.

This step uses #21's proposal sockets for technical tests. It does not approve
their physical contact depth/normal, author finger wrap, activate Project weapon
IK in GameApp, or claim an armed animation. The old explicit Project/legacy-IK
CLI rejection remains until an actual authored Ready variant is integrated.
No second skeleton runtime or engine/private-header dependency was added.
Two CMake calls now use the game's own MinGW runtime helper so the public
installed NovaCore package works as documented.

Validation: complete GCC13.2 Release build against the exact public engine
package; 42/42 CPU CTest cases passed in 99.54 s. The new adapter test covers
24 real bind/clip-phase/world-transform cases, including nonuniform model scale:
maximum measured world contact residual 0.000469843 m. Independent affine
orientation, real vertex deformation, all T/S and nonselected rotations,
malformed ancestry, unreachable targets and aliased invalid rollback are checked.
An earlier complete-suite attempt exhausted process memory in an existing scene
test while Blender authoring overlapped; the isolated scene and complete serial
suite subsequently pass, without changing tests or tolerances. GPU tests were
not run. Physical Escape authorization has not been renewed for Phase 3.

Status: Technically Integrated (adapter/test boundary only); Animation Complete:
no; Visually Accepted: no. Next executable step: finish the non-destructive
Blender Ready source variant, measure actual palm/M4 surfaces, and feed body and
arms from one logical pose before enabling the armed preview.
## 2026-10-18 Phase 3: developer Rifle Ready and one logical pose

The opt-in `--project-rifle-ready` mode uses the actual Project arms/body and
corrected M4A1. It enables the candidate and Project hand constraints together;
normal gameplay remains on the prior path until visual acceptance. The older
unarmed `--project-fps-preview` stays available for comparison and still rejects
its legacy hand-IK combination. The Ready mode is a developer animation slice,
not the complete Phase 3 combat/locomotion milestone.

`PlayerSkeletalAnimator` owns one NovaCore AnimationRuntime for both optimized
subsets. It validates common source-node hierarchy, bind matrices and overlapping
clip keys, merges their channels once, and caches maps to the original 81/57
skin palettes. `samplePose` advances once; an unconsumed sample cannot advance
again. The sampled eye drives camera position. The new chest socket then drives
one rigid Shared Weapon Pose, followed by both Project palm constraints, final
FK and skinning of each visible subset. `skinPose` consumes the sample once.
Render-only frames use the cached weapon/mesh snapshot. The hidden full world
body receives no additional pose or GPU updates.

The authored mount is attached to ORG-spine.003, the measured common ancestor of
both shoulder branches, also retained in both original skin palettes. Its local
frame is calibrated in the actual Blender Ready pose. The rigid weapon takes
its forward and projected up axes from that sampled affine socket; the skeleton
retains all affine scale/shear for IK/skinning. This avoids both a static world
mount and hand-to-weapon feedback. Rendering, muzzle and both contact goals use
exactly the resulting weapon transform and the same world projection as body
and arms. Camera orientation and authoritative movement/shots remain gameplay
state; this slice adds no gameplay recoil or root motion.

The new source `assets/metadata/project_rifle_ready_pose.json` contains 72
Blender-authored rotation tracks, an explicit upper-body rotation mask, physical
palm surface frames, real M4 surface contacts and the calibrated chest mount.
The mask includes unchanged rotations in the actual spine/shoulder/twist
branches: selecting only visibly changed joints allowed unarmed Run helpers to
leak into the Ready layer. Moving poses keep every original sampled translation
and nonuniform scale. The Project solver preserves the authored elbow plane
and all weighted .001 local corrections; its bind plane is only a degenerate-arm
fallback. The existing proposal and original hand sockets remain available.

The standard-library cook appends `Developer Rifle Ready Idle` and new socket
nodes/accessors to build-only variants. It preserves old geometry, weights,
original 81/57 palettes, inverse binds, node transforms and every original clip
accessor/BIN prefix. No Walk/Run clip is renamed into an armed or directional
clip. No large new GLB or .blend is committed. Source character and M4 hashes
are enforced. Finger curl is a reproducible offline developer authoring seed;
its final contact/trigger/thumb quality still needs artist review.

To reproduce the source, first build the existing FPS candidate, then:

```powershell
python tools/assets/prepare_project_rifle_authoring.py --candidate-dir build/local-debug-no-deps/project-fps-candidate --output build/ready-authoring/rig.glb
# Blender 5.1.0, build adfe2921d5f3; run in background, with a writable TEMP/TMP.
blender --background --factory-startup --offline-mode -noaudio --threads 1 --python-exit-code 1 --python tools/assets/author_project_rifle_ready.py -- --source build/ready-authoring/rig.glb --output build/ready-authoring
# Review the .blend / source JSON before replacing the checked-in authoring JSON.
```

The authoring code checks actual rest/pose conversion, local TRS reconstruction
and unchanged bone translation/scale. CI needs Python only and deterministically
cooks the reviewed JSON. Blender is neither a runtime dependency nor required
on hosted CI. Authorship/license: developer pose on the user's original Project
assets; no external game animations.

Configure the normal pinned-source build with both
`-DNEMISIS_BUILD_PROJECT_FPS_CANDIDATE=ON -DNEMISIS_ENABLE_HAND_IK=ON` and a usable
SDL3/Vulkan SDK for graphical review. Launch explicitly with
`tools/run_project_rifle_ready.ps1 -Executable <absolute-vulkan-exe>`; add
`-LookDown` for the body check. The launcher reports the exact EXE and SHA256,
uses this checkout as working directory, and requires the Vulkan backend. It
was syntax-checked without launching a GUI. An old CLion/start-script EXE does
not establish validation of this new mode.

Manual visual gate after renewed authorization: inspect Ready from the
anatomical eye, primary/support wrist and fingers, stock/shoulder placement,
look-down torso/legs and head clipping, then movement/stop/turn continuity.
Offline Blender images inspect the authored subset pose only, not game pixels,
materials, GPU synchronization or full third-person acceptance.

Technically Integrated: developer Ready/shared-pose/contact boundary. Animation
Complete: NO. Visually Accepted: NO. Original Walk/Run remain the lower-body
motion beneath a Ready rotation layer; backward/strafe/diagonal clips, a distinct
Sprint hold, ADS/aim, recoil distribution, reload hand release/magazine handling,
advanced movement and the corresponding full world-model states are still open.
Do not remove legacy paths or make this developer slice the default yet.

Next executable step: review the explicit Ready Vulkan build, refine the authored
finger/stock/shoulder pose, then author and separately validate directional
lower-body clips and ADS. Keep further work in bounded PRs; no automatic merges.
Final local Ready evidence (GCC13.2 Release, exact public NovaCore package at the
pin above): complete build and 45/45 CPU CTest cases pass in 83.19 s. The Ready
end-to-end test covers 288 fixed ticks across complete Idle/Walk/Run/stop cycles,
yaw plus body pitch/roll, two render-only submissions per tick, original palette
sizes, immutable eye/chest mount across hand IK, no A1 fallback, actual M4
muzzle/contact reconstruction and exactly-once pose consumption. Maximum actual
Ready world contact error: 0.000979131 m; orientation <=0.5 degrees. Both hands
intersect the hip camera frustum (106 right / 2275 left weighted vertices); this
is not a pixel/occlusion check. Shared unarmed poses also match both independently
skinned original palettes. The separate 24 affine rig cases pass, maximum error
0.00099753 m. The final 72-track authoring JSON reproduces byte-for-byte under
Blender 5.1.0/adfe2921d5f3: SHA256
f6fe115bb008e447ac52fcd9fc40728a8a631062f928e9d54570e0365933ec2b.
No GPU test or in-game visual acceptance is inferred from these CPU results.
Published Ready follow-up: https://github.com/YoungJibbit95/Nemisis/pull/23,
implementation b06ec18, canonical source-newline fix
12de58466f2c883caae36f1f35a0eda16b009ba4; base is draft #22. No automatic merges.

Subsequent GUI evidence supersedes the earlier pre-GUI status above. The user
explicitly renewed Phase 3 Ready/Look-down/movement GUI authorization. The
separate Release Vulkan executable built successfully at
F:/Coding/Project/recovery/phase3/integration/Nemisis/build/phase3-vulkan/nemisis_game.exe,
SHA256 4cd67d719e7d2c94bbb3c64534797d94e943d8b8b07957701b8ac0cf65aa7ebc.
It uses the pinned public engine/SDL3 package and Vulkan SDK 1.4.350.0. The local
installed-engine export needs its public static SDL3 dependency supplied by the
build-only graphical-package-dependencies.cmake hint; the normal documented
pinned-source build does not require that local package hint.

The real NVIDIA GeForce RTX 3070 Ti backend and both Project subsets/new M4
variant were logged. An actual Ready game image was inspected and saved locally:
F:/Coding/Project/recovery/phase3/visual/ready_hip_vulkan.jpg. Bent, attached arms
and the M4 are visible. Finger/trigger/stock/shoulder quality and lateral weapon
placement remain provisional; this is not visual acceptance. Offline Blender
previews are a separate reference and use a different projection convention.

The user subsequently stopped Computer Use with the physical Escape key. All
further GUI input stopped immediately. Look-down and the controlled movement
comparison were not completed. No post-stop relaunch, GUI action or GPU-smoke
claim follows from this evidence. Renew explicit GUI permission before resuming.
Technically Integrated: developer Ready/shared pose/contact only. Animation
Complete: NO. Visually Accepted: NO. CI status at the published runtime head is
tracked on PR #23; this final evidence update is documentation only.

## Phase 4 W1: arsenal inventory and right-handed reference

Audited 2026-10-18 against `core_trio.json`, `WeaponSystem.cpp`,
`DevRangeRenderScene.cpp::firstPersonWeaponMount`, the actual GLB nodes and the
asset manifest. There are four playable definitions, not six. All six Project
weapons also appear in the static showcase. Source prefix below is
`assets/project_assets/weapon_`; cooked prefix is
`assets/processed/normalized/project_assets/wpn_project_`; suffix is `.glb`.
These are existing source/cooked files, not newly imported assets.

| Gameplay ID / class | Source -> cooked suffix | Presentation/profile | Integration / visual acceptance |
| --- | --- | --- | --- |
| `ar_01` / assault rifle | `rifle_m4a1` -> `rifle_m4a1` | Rifle; separate `rifle_m4a1_ready_preview` developer variant | Project Ready shares one body/arms pose and physical palm constraints. Right hip reference re-authored in W1; **not visually accepted**. |
| `smg_01` / SMG | `smg_fr17` -> `smg_fr17` | Legacy procedural SMG | Project skeleton/profile absent; orientation and real contacts need W2. **Not accepted**. |
| `shotgun_01` / shotgun | `rifle_ncar` -> `rifle_ncar` | Legacy long-gun mount; pickup label `BREACH RIFLE` | **Content mismatch**: rifle geometry, no proven shotgun mechanism. No approved proxy and no Project integration. **Not accepted**. |
| `sidearm_01` / sidearm | `sidearm_glock19` -> `sidearm_glock19` | Legacy procedural sidearm | Project pistol grip, support/release, slide/magazine reload absent. **Not accepted**. |
| none / alternative rifle | `rifle_afr120` -> `rifle_afr120` | Showcase only | No gameplay definition or Project profile; no new weapon/balance added. **Not accepted**. |
| none / alternative sidearm | `sidearm_p320` -> `sidearm_p320` | Showcase only | No gameplay definition or Project profile. **Not accepted**. |

Every current cooked marker declares +Y up / +Z forward. That declaration does
not prove physical orientation. M4 #19 established real sights above magazine,
barrel/muzzle forward and unit scale. FR17's tall appendages currently extend
upward and require geometric classification/re-cook before claiming the same.
Glock, P320, NCAR and AFR120 still need independent geometry inspection. No
orientation repair was added in the runtime.

Positions below are metres in each cooked asset scene, rounded for readability;
they were evaluated through the real node hierarchy, not copied from metadata.
All right grip origins are `(0,0,0)`. These legacy grip markers are positional
proposals; only the M4 Ready variant additionally has measured physical palm
frames. None of these original GLBs contains skins or animation clips.

| Asset suffix | Muzzle XYZ | Left support XYZ | Eject XYZ | Sight / magazine / profile gaps |
| --- | --- | --- | --- | --- |
| `rifle_m4a1` | (.00009,.08978,.61478) | (0,.05833,.33855) | (-.02455,.06357,.12194) | Named Sight/Sight_2/Magazine/Stock/Trigger; no accepted sight-aperture/ADS or magazine-motion frame. Actual source ejection is -X; preserve it. |
| `smg_fr17` | (-.020,.080,.310) | (-.030,.030,.160) | (.012,.060,.080) | Generic Plane/Cylinder parts; muzzle/grips/eject provisional, no sight or magazine-motion frame. |
| `sidearm_glock19` | (-.012,.045,.135) | (-.020,0,.005) | (.002,.055,.055) | One mesh; no validated sight, magazine, slide or palmar frames. |
| `rifle_ncar` | (-.035,.090,.575) | (-.053,.040,.290) | (.025,.070,.180) | Generic parts; no shotgun shell/pump/mechanism evidence. |
| `rifle_afr120` | (-.028,.100,.742) | (-.042,.040,.330) | (.014,.080,.230) | Single mesh; no accepted sight, magazine or animation profile. |
| `sidearm_p320` | (-.014,.050,.148) | (-.023,0,.005) | (.002,.060,.063) | Single mesh; no validated sight, slide or magazine-motion frames. |

Existing fallback assets remain explicit: M4 and shotgun ->
`wpn_a2_modular_rifle_01`, SMG -> `wpn_a2_blackout_carbine_01`, sidearm ->
`wpn_a2_striker_sidearm_01`. Legacy arms use `chr_a1_fp_arms_01`; active Project
Ready instead uses the actual Project subsets and never falls back to A1.
The separate Project FPS preview/Ready currently locks weapon switching to AR.
Thus legacy SMG/pistol availability does **not** establish Project arsenal support.

### Right-hand contract and W1 developer reference

Hipfire means the actual firing grip is on camera-right; the primary physical
palm remains parented to `DEF-hand.R`, support to `DEF-hand.L`. ADS may center
the sight while retaining those identities. No negative scale, reflected mesh,
swapped bone labels or runtime asset-axis repair is permitted. Contact goals,
muzzle and render submission must all use the same final weapon matrix.

Phase 3 Vulkan evidence placed the M4 left of the anatomical eye. The W1 offline
Blender source moves the artistic mount from X=-.11 to X=+.11 metres and solves
the existing real arm chains again. It retains the reach-compatible torso pose,
actual palms, source geometry and all local translation/scale. Simply reversing
the torso twist made the support arm unreachable and was rejected. No renderer
camera/basis change is used to move the weapon.

The permanent Ready regression now tests camera-relative primary-grip side
through the existing 288 Idle/Walk/Run/stop ticks and yaw/body pitch/roll; it also
checks forward direction and positive instance scales. The deterministic cook
checks that physical .R/.L palm markers attach to the actual corresponding hand
bones. Existing independent final contact/orientation, immutable eye/chest,
original binary/palette preservation and render-matrix tests remain mandatory.
These checks establish a mathematical developer reference, **not** acceptable
finger wrap, shoulder posture, third-person anatomy or game pixels.

State acceptance remains open for all four playable weapons: true directional
armed locomotion, Sprint hold, ADS enter/hold/exit, fire distribution, class reload
with hand release and magazine/slide/shell handling, equip/switch, jump/fall/land
and world-model armed attachment. Ready-over-original Walk/Run is not a completed
armed movement set. Do not enable the developer slice by default or remove legacy.

Next packages: W2 FR17 offline physical orientation/sockets then actual Project
SMG Hip/ADS; W3 Glock; W4 suitable shotgun source or explicitly approved proxy;
W5 alternatives only if selected; W6 class state authoring and switching; W7 real
game matrix. All GUI acceptance requires renewed explicit permission after the
last physical Escape stop. No Phase 4 GUI inspection has occurred.

### Phase 4 visual reference handoff

The user's `Nemisis_Visual_References_Codex.zip` is extracted locally at
`F:/Coding/Project/recovery/phase4/visual-references`. All 13 external images
were downloaded with the provided standard-library script and decoded/verified;
F01 initially returned HTTP403, then its original URL succeeded with the original
source Referer. `download-status.json` preserves hashes, original sources, image
formats/dimensions and rights. `reference_board_local.html` uses the downloaded
images; the original board/JSON/README are unchanged. No reference binaries are
committed, republished or imported as textures/models.

- F01 ([The Finals](https://the-finals.it.malavida.com/windows/)): Hip weapon and
  actual firing wrist in the lower right; central target area free; continuous
  arm/wrist/grip silhouettes and visible fore-end support without gaps or gross
  penetration. The mathematical W1 side check is only a prerequisite.
- F02 ([Titanfall 2](https://www.eneba.com/hub/games/most-underrated-games/)) and
  F03 ([Apex ADS](https://www.gamestar.de/artikel/apex-legends-guide-so-verbessert-ihr-euren-aim,3341325.html)):
  actual centered sight aperture, visible target through the sight and stable
  anatomical hand identities during enter/hold/exit and motion. The supplied F02
  still is ADS, not proof of animated movement; use it for sight readability.
- S01-S03 guide sky/sun/haze; M01-M02 real materials; P01-P02 metal/roughness/
  reflections; L01-L03 cast shadows/industrial lighting. Renderer packages own
  these criteria; later atmospheric/PBR/shadow targets are not claimed by R0.

Reproducible Vulkan comparison must record game and engine commits, EXE and
cooked-asset hashes, viewport/FOV, camera position/yaw/pitch/roll, fixed-tick or
held presentation state and scene. Match those values before/after. Capture
Hip, ADS, look-down and each supported movement/fire/reload/switch state
separately; missing states stay open. `00_Nemisis_IST_Zustand` contains genuine
historical user screenshots from different builds, not a matched current
baseline. The Phase 3 Ready image is also historical evidence, not Phase 4 pixels.
New GUI input/captures require renewed explicit permission, or user-supplied
matched screenshots. No visible Phase 4 improvement is inferred from CPU tests.

FR17 audit refinement: an actual side projection of the existing vertices places
the muzzle/barrel `Cylinder.010` at negative Z (front extremity -.18478m), while
stock `Plane.024` reaches positive Z (.30477m). The current +.310Z muzzle marker
therefore points at the stock as well as the grips being upside down. W2 must
correct forward and up together in the existing offline cook, then recalibrate
that asset's own contacts/sights. Do not attach hands to these old targets.

## Phase 5 FP-1: physical ownership and FR17 correction

Starting game revision: `2264e27ffb1ca76244d9fb49d377f9af9bc7de32` (#27).
Engine pin remains `3d673b7345d295588367b466a8877e84dee112a9`. No renderer,
camera, gameplay balance, ammo state or runtime axis correction changes here.

The M4 Ready producer already sends `DEF-hand.R` to its physical pistol grip
and `DEF-hand.L` to its handguard. Swapping those bones would introduce a bug.
Its previous tests, however, did not independently tie palm targets to physical
grip regions or check the resulting visible hand vertices. The Ready cook now
rejects exchanged contacts, incorrect source-palm identity and reflected frames.
The 288-tick Ready regression checks actual physical anchor proximity and the
original hand-weighted skinned vertices in addition to palm residual/orientation.

The normal launch still uses A1 arms without hand constraints. A1's authored
right wrist is X=-.125,Z=.775m and left wrist X=+.122,Z=.655m; translating that
whole mesh toward averaged sockets cannot change their fore/aft order. The new
legacy compatibility regression uses the existing production IK adapter with
the actual M4 and corrected FR17. It covers both targets and visible weighted
hand geometry through 4 alternating weapon selections and 9 ADS/recoil/movement
samples per selection. It retains the old unconstrained pose as a failing-contact
negative control. These checks do not make A1 the requested Project character.
Changing the default launch remains an integration/visual decision.

The actual Project source is anatomically consistent. Source eyes lie .08720m
ahead of the head toward -Z; toes lie .09450m ahead of feet toward -Z. Source
right wrist X=.85549, index Z=.08525, pinky Z=.15065, thumb Z=.10575; its palm
approach normal is (-.07576,-.99712,-.00438). The existing proper 180-degree
runtime player rotation preserves this chirality: front becomes +Z, right wrist
X=-.75234, index Z=.02368, pinky Z=-.04172, thumb Z=.00318, palm approach
(+.07576,-.99712,+.00438). This is a camera/composition acceptance question,
not evidence of mislabeled source hands. No mesh reflection or bulk L/R exchange
was introduced. The image-space anatomical interpretation still needs review.

FR17 had a separate proven content error. Its raw physical muzzle is +Z and its
pistol grip extends toward raw +X. The old [0,-90,180] recipe inverted physical
up and forward; its muzzle marker pointed at the stock. Only this asset now uses
the existing Blender normalizer with Euler [0,+90,0]. New R is inside Plane.012
pistol handle, L intersects the forward Plane.003 underside, and muzzle is the
actual Cylinder.010 front-ring center. Canonical R=(0,0,0), L=(0,.0850358,.1997735),
muzzle=(-.0000289,.1328695,.2916486)m. The real stock is behind R. These geometry
contracts are checked through the actual public engine importer, not metadata
axis declarations. FR17 eject and full finger/ADS/animation profiles stay open.

Reproduce the existing cook with Blender5.1.0, build adfe2921d5f3:

```powershell
blender --background --factory-startup --threads 1 --python tools/blender/normalize_project_assets.py -- --repo-root . --only wpn_project_smg_fr17 --export --report build/fr17-normalization-report.json
```

Unchanged FR17 source SHA256:
`72dc57988cfbc84b018fd9687e26d2885ac2ceb57f7a7a13b45892944ac21a2e`.
Two independent exports produced cooked SHA256:
`e1f3ee9bb2cabdb36150d7ecf3a93cbb147047ab1250d8c45ea7bb7ebc6a7038`.
Python cook and mutation checks pass (2 cases). C++ build/full tests and hosted
MSVC/Clang CI are pending integration at this commit. No GUI was launched.
F01-F03 remain art direction: actual right trigger hand, left front support,
natural wrists/fingers, clear center and readable ADS. Vulkan visual acceptance,
camera chirality/composition and performance remain open after the Escape stop.
