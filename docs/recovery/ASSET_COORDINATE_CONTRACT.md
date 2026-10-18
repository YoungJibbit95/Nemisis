# Canonical asset coordinate contract

## Phase 2C: physical M4 geometry correction, 2026-10-18

Earlier marker/length contracts passed an upside-down M4. The source GLB's
magazine extends along +X while sights lie along -X: actual up is -X, actual
barrel forward is -Z. Correct only the M4 offline normalization recipe from
Blender Euler[0,-90,180] to[0,+90,180]. Runtime instance Euler/matrix/renderer code
is unchanged. Other Project weapons have different authored axes; do not apply
this recipe in bulk.

Source SHA256 `b276eeeb2109baee336d02f366a961b126b827d2aa8983d62d3f59c7013abfc4`
is now checked before the existing Blender cook. Source GLB remains unchanged.
The old cooked Magazine Y range was[+0.00196,+0.18632], rear Sight_2
[-0.07457,-0.04680]; generated +Y-up sockets alone did not detect the inversion.
The new engine-import contract independently checks physical magazine below
sights, actual muzzle ring alignment within1mm and handguard underside contact.
This complements the original marker, metre-scale and forward contracts.

Generated anchors are geometry-based and **provisional**, not authored palm or
finger contact frames. Source sample definitions (raw glTF coordinates, metres):

| Anchor | Source point | Selection |
| --- | --- | --- |
| Firing grip | (.0834092991,.0308887340,.1185528245) | Base pistol-grip region X>.055,Z>.07; medianX/Z, centerlineY |
| Support | (.0250809807,.0308887340,-.22) | Barrel handguard underside, Z[-.225,-.215], centerlineY |
| Muzzle/VFX | (-.0063669803,.0307963407,-.4962268025) | Bounding-center of Barrel foremost ring, minZ within0.1mm |
| Eject | (.0198371671,.0554403607,-.0033874433) | Ejector_Lid outer maxY surface within0.1mm |

The existing normalizer rounds pre-recenter bounds to5 decimals; its translation
is(.03180,.06681,.19279) in Blender space. Config locations include that shift;
the existing root-to-firing-grip recenter then produces runtime R=(0,0,0),
L=(0,.0583283,.3385528), muzzle=(.0000924,.0897763,.6147796),
eject=(-.0245516,.0635721,.1219403). Frames stay +Z-forward/+Y-up. The actual lid
lies on runtime negativeX; source chirality/ejection art is unapproved. No
invented positiveX port, reflection or per-instance repair is introduced.

Reproduce with the existing tool, M4 only:

```powershell
& 'F:/Coding/Project/recovery/tools/blender-5.1.0-portable/blender-5.1.0-windows-x64/blender.exe' --background --factory-startup --python tools/blender/normalize_project_assets.py -- --repo-root . --only wpn_project_rifle_m4a1 --export --report build/m4-normalization-report.json
```

Local CLI: Blender5.1.0, build adfe2921d5f3. Portable archive from the
[official release directory](https://download.blender.org/release/Blender5.1/),
verified against its official SHA256
`bc184226962904e3f26c5809ba7ec86bbeb670060825b046930b0c02a30a4eee`.
Two independent CLI exports produced identical cooked GLB SHA256
`1a9350d4f59d2f50b35906f11eaa8ad8e72539f8c80f83146ff1cb2ab341ff0b`.
The existing metadata/report record is updated for M4 only. Full anatomical
grasp, held ADS and camera/movement acceptance still require rendered evidence.

Local full GCC13.2 Release build and37/37 CPU cases pass against exact engine
`a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7`, including existing actual-M4 hand
constraints through hip/ADS transitions. A negative control temporarily loaded
the old cooked bytes: new physical-up, muzzle-ring and support-contact contracts
all failed, while restored corrected bytes pass. The source was never modified.
The first parallel4 build exhausted compiler memory; parallel1 completed. A test
initially overlooked the existing cook's Barrel-to-LOD0 rename, was corrected to
the actual exported node, and the complete37-case suite was rerun successfully.

Verified baseline: Nemisis `ff88eb0a7997f238cb76381dc377525c942991c1`, NovaCore runtime
`dbdfc38d092e018952ead43d9cb669333ebf70bc` (unchanged game pin), 2026-10-18.
This contract describes actual code and separates required cooked-content policy
from limitations that are not yet enforced by the general importer/runtime.

## Canonical boundary

**The cooked GLB is the canonical runtime boundary.** Normalize source orientation,
metre dimensions, origin and sockets offline in Blender; export Y-up glTF once.
Downstream import, upload and instance rendering must not add an asset-specific
axis repair. A raw GLB used as authoring input is not automatically canonical game
content merely because its container follows glTF. Importing it into Blender and
exporting it again are standard basis conversions, not two runtime fixup layers.

The concrete Blender-to-glTF basis is `C(x,y,z) = (x,z,-y)` (a rotation, determinant
+1, not a handedness reflection). `runtime_from_blender_local()` implements this
for the socket **report only**; `export_glb(export_yup=True)` handles the actual
export. Applying the report conversion to imported vertices again would be wrong.

## Spaces and conventions

| Stage/space | Verified convention | Source/function |
| --- | --- | --- |
| Source Blender target | +Z up, -Y weapon-forward; numeric lengths treated as metres by the cook. This is project authoring policy, not a universal forward axis for every .blend. | `tools/blender/normalize_project_assets.py`: `inspect_entry`, `scale_to_axis`; `asset_normalization_fixups.json.coordinate_contract` |
| Source GLB -> Blender | `import_asset()` calls Blender glTF import, then source-specific root transforms are applied in Blender space. Do not apply those Blender Euler values directly in runtime GLB space. | `normalize_project_assets.py`: `import_asset`, `rotation_matrix`, `apply_matrix_to_roots` |
| Cooked glTF/GLB | Right-handed asset coordinates, metres, +Y up, +Z object-front; quaternion XYZW. Local transform is T*R*S; arrays for node matrices/inverse binds are column-major. | [glTF specification, coordinates and transformations](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#coordinate-system-and-units); exporter `export_yup=True`; actual M4A1 GLB |
| Import internal matrix | Row-major storage `value[row*4+col]`, column-vector mathematics. Array layout conversion does not change axes. Matrix values are read as glTF column-major into that internal representation. | NovaCore `GltfDocument.cpp`: `GltfMat4`, `readMatrixTransform`, `readTrsTransform`, `transformPoint` |
| Node hierarchy | `Wnode = Wparent * Lnode`. Static mesh vertices receive this transform once. Named markers receive the same transform; their directions are normalized transformed local +Z/+Y. | `collectMeshNodeTransforms`, `applyNodeTransform`, `loadGltfMeshData` |
| Imported static model space | `GltfPrimitiveData.nodeTransformBaked=true`. Marker `worldPosition` means asset-scene space, not Nemisis world space. Marker and baked mesh must be instanced together without replaying node transforms. | `GltfDocument.cpp`; `GltfNodeMarker` in `GltfDocument.hpp` |
| Imported skinned space | Skin-bound positions remain unbaked mesh-local data. Animation matrices use row-major storage, column vectors and XYZW; bridge converts column-major inverse binds by indexing. Palette is joint-global * inverse-bind. | `GltfAnimationBridge.cpp`: `toAnimationMatrix`, `skinGltfMesh`; `AnimationTypes.cpp`: `matrixFromTransform`; `Skeleton.cpp`: `evaluateGlobalPose` |
| CPU resource -> GPU | Mesh catalog/renderer keep imported positions. `buildVertices()` copies position components to the vertex buffer; upload introduces no axis swizzle or per-weapon correction. | `DevAssetBindings::bindRequiredDevAssets`, `Renderer::registerMeshResource`, Vulkan `buildVertices`, `uploadMeshPrimitive` |
| Instance/world | Column vectors: `M = T * Ry(yaw) * Rx(pitch) * Rz(roll) * S`; angles here are degrees. Scale acts first, then roll, pitch, yaw, translation. | `VulkanBackend.cpp`: `meshRotation`, `modelMatrixForMesh`; game `transformMeshLocalPoint` |
| Camera | At zero angles view forward is +Z, screen-right basis +X, world-up +Y. Forward = (sin(yaw)cos(pitch), sin(pitch), cos(yaw)cos(pitch)). Camera positive pitch looks upward; mesh positive X rotation tips +Z downward, hence mesh pitch = -camera pitch. | Vulkan `cameraForward`, `lookAtLeftHanded`; `PlayerView.cpp`; `PlayerFirstPersonRig.cpp`: `basisFromView` |
| View/projection | Backend explicitly names its +Z-view-depth construction left-handed. This does not imply a glTF importer reflection. `clip = P * V * M * p`; perspective w=z, near maps to depth 0 and far to 1, with negative projection Y scale. GLSL consumes column-major push-constant matrices. | Vulkan `Mat4::at`, `perspectiveVulkan`, `viewProjectionForCamera`, `pushConstantsForMesh`; `shaders/vulkan/world_mesh.vert` |
| Viewmodel projection | Uses the same camera position/orientation in a separate depth pass, with FOV clamped from world-FOV minus 6 degrees to 52..82, near 0.004 m and far 8 m. A world projection and viewmodel projection are not interchangeable screenshot oracles. | Vulkan frame submission, `RenderMeshLayer::ViewModel` |
| First-person placement | Camera-relative hip/ADS offsets and sway/animation are artistic presentation. Camera yaw, negated pitch, roll, recoil and reload additives produce the instance rotation; ADS also reduces weapon scale. | `PlayerFirstPersonRig.cpp`: `evaluateFirstPersonRig`, `basisFromMeshRotation` |
| Static cooked sockets -> world | `pWorld = M * pMarker`. Muzzle alignment solves translation as `targetMuzzle - Ry*Rx*Rz*S*cookedMuzzle`. Grips use that same solved weapon transform. Marker position is consumed; marker forward/up are currently not consumed by this scene path. | `DevRangeRenderScene.cpp`: `cookedSocketRig`, `alignMeshPositionToSocket`, `cookedSocketWorldPosition`, `bindFirstPersonRigToCookedWeaponSockets`, `appendFirstPersonMeshes` |

Engine source references above are in the [pinned NovaCore tree](https://github.com/YoungJibbit95/Novacore-Engine/tree/dbdfc38d092e018952ead43d9cb669333ebf70bc).
Storage order, algebra, asset-front, world-forward and view-depth conventions are
separate facts. In particular, glTF object +Z front is not a promise that a glTF
camera looks along +Z: glTF cameras look along their local -Z, whereas this game
constructs its own +Z-view camera and does not import a glTF camera for gameplay.
No universal RH/LH label should be used to invent another mesh flip.

## Required sockets and interpretation

| Asset class | Required cooked nodes | Optional/conditional nodes |
| --- | --- | --- |
| Representative rigid weapon | `socket_muzzle`, `socket_grip_r`, `socket_grip_l` | `socket_eject` for casing origin; `socket_vfx` for tools/fallback effects |
| Full character used for player presentation | `socket_root`, `socket_camera`, `socket_weapon_root`, `socket_hand_r`, `socket_hand_l` | `socket_head`, `socket_vfx` when their consumer needs them |
| First-person arms | `socket_camera`, `socket_weapon_root`, `socket_hand_r`, `socket_hand_l` | Extra authored attachment points as required by that rig |

These are the Nemisis cook/migration requirements, not a claim that every existing
fallback asset satisfies them. `tools/assets/asset_readiness_check.py` already
checks class-specific nodes through `expected_required_sockets()` and metadata
node presence. Generic NovaCore metadata validation does not enforce game socket
classes. The existing readiness minimum is only muzzle for weapons and root/
camera/weapon-root for non-prototype full characters; the migration requirements
above additionally require both grips/hands. Arms already require camera,
weapon-root and both hands in readiness tooling. Current scene fallback also
tolerates missing sockets. Do not remove
those fallback paths merely because the target contract is stricter.

For canonical rigid weapons, the firing-grip origin is zero, the muzzle lies
forward along the weapon's +Z length direction, and muzzle/grip marker frames use
local +Z forward/+Y up. Grip-to-muzzle displacement may include lateral/vertical
artistic offsets: it need not be exactly parallel to the muzzle orientation.
Keep node names unique within the selected asset scene and require finite,
nondegenerate transforms. Ejection direction is effect-specific; current code
uses its position and assigns the weapon pose, not an authored eject-frame basis.

For static assets, marker positions already include ancestor scale/rotation/
translation. Apply instance scale once and normalize transformed directions.
For animation, the intended full socket transform is animated joint-global times
authored joint-local socket transform, then the model instance. The generic
`Skeleton::evaluateSocket()` supports that formula. **The glTF bridge currently
does not preserve authored socket attachment/offsets:** `buildGltfAnimationAsset()`
selects a joint by name hints and initializes socket local transform to identity.
`PlayerSkeletalAnimator::socketTransform()` therefore exposes that heuristic joint
space, not arbitrary authored glTF socket semantics. Non-joint ancestor transforms
in imported skin hierarchies also require targeted evidence before broad claims.
These are separate follow-up seams, not justification for a runtime weapon flip.

Position correctness also does not establish normal/lighting correctness:
`applyNodeTransform()` transforms normals with the linear matrix rather than an
inverse transpose, and `world_mesh.vert` passes normalized input normals without
the instance model rotation. The new nonuniform-scale test covers positions and
marker axes, not those lighting paths. Address them separately if a product
regression requires it; do not compensate with a weapon orientation repair.

## Representative M4A1 evidence

The actual catalog still maps `wpn_project_rifle_m4a1` to
`assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb`; `ar_01`
selects it in `firstPersonWeaponMount()`. It is a useful first migration candidate:
a rigid primary weapon, existing sockets, both hip/ADS mounts and an A2 fallback.

| Observation | Original authoring GLB | Current cooked GLB |
| --- | --- | --- |
| File | `assets/project_assets/weapon_rifle_m4a1.glb` | `assets/processed/normalized/project_assets/wpn_project_rifle_m4a1.glb` |
| SHA256 | `b276eeeb2109baee336d02f366a961b126b827d2aa8983d62d3f59c7013abfc4` | `630f0fc6806a86da26bd0643be6b8040656de74a0c223e17866f83e4fd5afa27` |
| Evaluated bounds dimensions XYZ, metres | approximately (0.266320, 0.063605, 0.858843), excluding ignored helper geometry | approximately (0.063605, 0.266320, 0.858843) |
| Skin/animation | none | none; first-person motion is procedural presentation |
| Socket nodes | none | grip_r (0,0,0), muzzle (-0.022,0.070,0.525), grip_l (-0.034,0.030,0.250), eject (0.014,0.060,0.170), vfx at muzzle |
| Socket frames | absent | +Z forward and +Y up on current exported markers |

Bounds were measured from vertex data after node transforms, not guessed from
metadata. The cook changes the source's sideways height/width arrangement; source
and cooked GLBs must not be treated as interchangeable runtime inputs. Export
can reindex/split vertices, so identical vertex counts are not a migration rule.
The current code alone does not prove the original incident's historical cause;
it establishes the present source/cook/import responsibilities without inventing
an importer-specific rotation fix.

`dimensions_m`/`target_dimensions_m` retain Blender/source semantic order
[width, forward length, height] for weapons (see `asset_readiness_check.py`). They
are **not runtime XYZ extents**. `socket_forward_axis` and
`runtime_socket_forward_axis` are dominant-axis labels derived from muzzle minus
grip position, not the socket node's quaternion/basis. The generic runtime metadata
struct does not load those dimension/report fields. Actual node frames and
geometry must be checked separately, as the added CPU tests do.

## Active, inactive and redundant corrections

| Path/data | Current role | Decision |
| --- | --- | --- |
| Project character: Blender Euler (0,0,180) | Active offline source orientation | Retain until an independently validated recook replaces it |
| M4A1, NCAR, FR17, Glock19, P320: (0,-90,180) | Active offline source orientation before canonical export | Retain; never replay in renderer |
| AFR120: (0,0,180) | Active offline source orientation | Retain |
| Glock19/P320 length scaling to 0.187/0.203 Blender Y | Active unit/dimension normalization | Retain; artistic runtime scale is a separate purpose |
| Remove ignored meshes, bounds origin, then grip recenter | Active authoring cleanup and socket-origin policy | Retain while source requires it; do not simply delete source binaries |
| Blender export Y-up conversion | Necessary standard basis conversion C | Retain at cooked-output boundary only |
| Per-asset weapon/arm correction slots | Already removed in merged PR #9; all values had been zero | Do not reintroduce them |
| Generic rig correction fields | Still present, neutral for current scene mounts; can represent pose offsets | Review naming separately, retain pose capability; not an active per-asset repair |
| Hip/ADS offsets, scale, recoil/reload and camera pitch sign | Active artistic pose or camera/model conversion | Retain; not import fixups |
| `export` vs `normalized_export`, `socket_forward_axis` vs runtime alias | Duplicated reporting consumed by readiness tooling; not extra geometry transforms | Consolidate only after updating those consumers; no metadata deletion here |

## New focused CPU evidence

No binary fixture or new framework is introduced. Existing test executables gain:

* Engine `testGltfMeshAndMarkersShareNestedTransform` (NovaCore audit PR #9): tiny generated triangle,
  parent quaternion/TRS plus child column-major matrix and nonuniform scale;
  exact vertex/marker positions and normalized forward/up verify one hierarchy.
  This standards contract is engine-owned, reuses its existing writer, and adds
  no runtime dependency or reason to change the game pin.
* `testRepresentativeCookedWeaponContract`: real catalog/metadata/GLB selection,
  actual required marker presence/frames, finite geometry, grip origin, forward
  muzzle, long +Z axis and metre-scale size. It does not pin exact cook Euler values.
* `testCookedMuzzleAndGripsFollowRenderedWeaponTransform`: reconstruct final muzzle
  and rendered hand/grip positions with an independent quaternion/matrix oracle,
  combined camera yaw/pitch/roll, recoil and both hip/ADS scales. No stage counts.

These assert CPU geometry and submission behavior, not pixel correctness or a
shader execution result. Existing rig/animation/camera and Vulkan smokes remain.

## Visual status and first migration gate

A separate local build used GCC 13.2, SDL3 release-3.2.22 and Vulkan SDK 1.4.350.0
against the exact pinned engine. Runtime logged an active NVIDIA GeForce RTX
3070 Ti Vulkan backend and all 36 required mesh assets ready. One live Dev Range
hipfire image was inspected. Computer Use was then stopped with physical Escape;
no further UI inputs/captures were made. Stable held ADS, muzzle/grip image
placement and animation were **not fully visually validated**. This does not
constitute approval of the existing viewmodel or a successful migration.

Next migration is a separate visually reviewed M4A1 PR:

1. Preserve the source/cooked hashes above and record the cook/tool version.
   Compare the existing cook with a candidate exported into an ignored temporary
   output directory; do not overwrite the catalog's GLB during investigation.
2. Keep source-specific normalization at the offline boundary. Prove dimensions,
   origin, required node frames and static hierarchy on the candidate. Resolve
   report aliases only if needed, without adding a new correction layer.
3. Compare current/candidate in the existing Dev Range: hipfire, held ADS, forward/
   up, muzzle, both grips, scale, procedural reload/recoil and compatible arms.
   No claim of authored weapon animation is appropriate: this GLB has no clips.
4. Accept only with passing CPU contracts, an active backend, reviewed images
   for those states and no regressions in game tests. If images are unavailable
   or input automation is stopped, leave catalog, fixups and both GLBs unchanged.

No renderer, KCC, movement-feel, balance, importer API, pin or large asset change
is part of this contract PR. The authored-animation socket bridge requires its
own small fixture/fix/engine PR and exact cross-repo validation if pursued.
