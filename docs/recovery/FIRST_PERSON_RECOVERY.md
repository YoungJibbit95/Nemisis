# First-person recovery: diagnosis and bounded implementation

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
