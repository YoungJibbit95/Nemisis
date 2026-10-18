# Recovery progress

## 2026-10-19 Proper camera / physical grip integration candidate

- Enginef4d7c99 (Draft #16, on UI integration #15) preserves glTF chirality
  with a standard proper RH camera: physical view rows right/up/-forward and
  perspective W=-viewZ. Logical world yaw/pitch/forward remain unchanged.
  Actual asymmetric wrist/index/thumb fixtures reject the old551 camera with
  112 failures. Shared sky/world ray round trips and Vulkan depth remain valid.
- Main reviewed the six camera-math changes and independently rebuilt the full
  exact engine/server/sandbox:7/7 CPU suites pass5.06s. Complete SDL3/Vulkan
  static library builds and installs. Public headers and17 shader sources are
  identical to551; those modules already passed Vulkan1.0 SPIR-V validation.
  Both hosted MSVC/Clang checks pass, run39170676045.
- Game candidate source combines UI #28/#29, physical FR17 #30 and camera #31.
  Human-right input stays positive on the wire; applyLook converts it to world
  yaw, strafe follows physical camera-right, artistic camera offsets remain
  distinct from proper asset transforms, mesh roll counters camera roll. The
  offline Ready mount removes W1's screen-side compensation (+.11 -> -.11m),
  re-solving actual chr_project_male1 bones without changing skin topology.
  Positional recoil now agrees with unchanged authoritative world-yaw recoil.
- Both local candidate pins name exactf4d7c99; the full game+native-RmlUi+IK+
  Project build is running against its public library. This candidate pin will
  be published only after the local integration gate; main/defaults are not
  changed. The earlier local stop to raise compilation parallelism1->2 was
  scheduling, not a source/test failure. Graphical execution remains prohibited
  until a renewed explicit GUI grant. No visual or GPU performance acceptance.
- FP-1's first hosted Clang run passed44/45: the new geometry oracle assumed
  a single hand bone had >.75 weight. Actual Project palms blend hand/finger
  descendants (single hand max.572). The replacement uses the real source skin
  hand subtree, retains >.75 and the same geometric thresholds; R2591/L2553
  eligible vertices. Final FP-1/Cam CI is rerunning; no assertions disabled.

## 2026-10-19 Combined UI candidate dependency correction

- The first #29 native-UI CI jobs failed compiling DevRangeRenderScene.cpp:
  the isolated old UI pinfb33cb5 lacks RenderCamera3D::rollDegrees from R0.
  This is a real cross-branch API incompatibility, not a test to suppress.
- The optional UI candidate pin now uses combined engine5518818. Its full
  engine/server/sandbox CPU build and7/7 tests pass3.09s; full SDL3/Vulkan
  library build, all17 shaders/SPIR-V validation and hosted MSVC/Clang pass
  (engine #15, run39169931979). Combined game/native-UI CI/local acceptance
  follows. The ordinary engine pin3d673b7 is unchanged; no main merge occurred.

## 2026-10-18 Phase 5 orchestration checkpoint

- Actual main: game f15bc0e (#23 merged), engine 38e718b (#12/#13 merged).
  Game #25/#26/#27 remain Draft and have green hosted MSVC/Clang checks.
  Separate UI #28 and engine #14 remain Draft; #14 conflicts with engine main.
- Main locally combined engine main and UI #14 as5518818, preserving both
  CameraMath and VulkanUiRenderer plus their records; combined game source is
  143efac (#27 + main + #28). Engine/game CPU and Vulkan combination validation
  follows before any pin migration or claim of runtime compatibility.
- Three isolated specialist streams started as requested. Physical anatomical
  grip ownership is P0; existing screen-right and contact residual tests alone
  are insufficient. Physics audits/adopts a bounded solver without replacing
  KCC. Rendering continues R2 after existing R0/R1 and preserves UI resources.
- Main coordination and UI contracts: docs/coordination/PHASE5_INTEGRATION.md
  and UI_HANDOFF.md. UI remains owned by the separate UI chat; its branch and
  original dirty user worktrees are untouched. No automatic merges.
- Physical Escape still prohibits new GUI actions/graphical launches. Phase 4
  baseline-only evidence and the accidental post-stop launch remain recorded
  below; visual and GPU performance acceptance remain open.

## 2026-10-18 Phase 4 GUI Escape stop / limited evidence

- User explicitly renewed GUI permission for baseline/R0/R1 Hip/Look-down and
  sun/horizon comparison. Actual baseline native Vulkan capture was saved to
  F:/Coding/Project/recovery/phase4/visual/baseline_hip.jpg after P reset. Source
  manifest verifies identical W1 arms/body/rifle cooked hashes across baseline
  and R0/R1, same low-sun config and1280x720 client viewport. HUD time/tick was
  not frozen: this is qualitative evidence, not a pixel/performance benchmark.
- Baseline a687c88 ran on NVIDIA RTX3070Ti. Shutdown reports4685 samples/skins/
  vertex updates per subset, one logical pose owner, invalid0/unreachable0.
  M4 is visible on the right with the new W1 cook, but the open support fingers,
  wrist/arm silhouette and contact appearance remain below F01 quality. W1 is
  NOT visually accepted. These counters do not prove artist-quality contact.
- Physical Escape then stopped Computer Use. No further Computer Use API call
  followed. A root-agent orchestration mistake had chained the R0 Start-Process
  after the GUI close call without inspecting its stop result; R0 therefore
  still opened (PID100908). It was left unoperated; do not claim an R0 capture.
  No R0/R1 after images, Look-down, controlled sun/normal comparison or GPU
  timings were completed. Renew explicit GUI permission before any continuation.
- Exact hashes, paths, observations and stop are saved in visual/review-manifest.json.
  All13 reference images/local board remain in phase4/visual-references.
- Published Draft stack: game #25 -> #26 -> #27 (source2264e27, this followup
  documentation only), engine #12 -> #13. W1, game R0 and both engine PRs have
  green hosted MSVC/Clang CI. Game R1 hosted CI remains pending; its full local
  build and45/45 CPU tests passed156.03s, Vulkan link passed. No merges performed.
- Continue with W1 art/stock/finger acceptance, then geometry-specific FR17 W2;
  the remaining arsenal, class animation states and R2-R5 remain open as below.
## 2026-10-19 Ready transition measurement and offline correction

- Proper-camera runtime gate exposed three hip-side failures at Sprint-to-Idle
  ticks216-218, worst -.0554923m. All 288 contact checks still passed, maximum
  .000979131m; original unarmed ancestors turn beneath the Ready rotation mask.
  Added signed eye/chest/mount diagnostics while retaining strict positive side,
  1mm contact, anatomical skin and frustum assertions.
- Revised the existing offline solve from X=-.11,Z=.21 to X=-.16,Z=.17m;
  Y=1.54m and torso pose stay unchanged. Both physical chains remain reachable:
  Blender R/L residuals11.45um/.364um; original T/S deltas below7.37e-7m/1.13e-6.
  Pose SHA256 bc60ff4b2927764eaa702853a1dcea8c9331a7311c17fb38841df9f5e13b39eb.
  Physical-contact mutation checks pass. Runtime recook/288-tick gate and hosted
  CI are pending for this revised pose; no visual-quality result is claimed.

## 2026-10-19 Phase 5 camera/presentation source checkpoint

- Pair with engine f4d7c99b0c522ae872ae05a6eb42818629b16607, Draft #16 on #15.
  Renderer audit confirmed the old camera reflected the actual glTF anatomy.
  This source preserves positive human-right input while using proper camera
  right/up/forward artistic offsets and separate asset-local socket transforms.
  No source mirror, L/R exchange, movement-policy or command-wire change.
- Restored the offline Ready mount X=-.11m and re-solved actual .R firing/.L
  support arms. Blender residuals24.7um/.31um; original local T/S preserved
  within8.3e-7m/1.6e-6. Pose JSON SHA256 f6fe115bb008e447ac52fcd9fc40728a8a631062f928e9d54570e0365933ec2b.
- Standalone GCC13.2 PlayerView, CameraRig and FirstPersonRig tests pass,
  including81 yaw/pitch/roll attachment cases and human-right input/sway checks.
  Full cross-repo build/tests and MSVC/Clang CI remain pending at this checkpoint.
  Integration lead owns pin migration; no GUI launch or default-path change.

## 2026-10-18 Phase 5 FP-1 implementation checkpoint

- Isolated branch starts at game2264e27 (#27); engine pin3d673b7 unchanged.
  M4 Ready already owns .R firing/.L support. Added cook-time rejection of
  exchanged contacts, false palm identity and reflected frames, plus actual
  weighted-skin/physical-grip regression coverage. Python2/2 checks pass.
- Corrected FR17 physical up/forward in the existing offline normalizer and
  recooked its own measured pistol/support/front-ring sockets. Two exports are
  byte-identical, cooked SHA256 8099362f8974b65c81fb3883e0739042b8521d08d2e840cc9b247731a4036130.
  Actual imported geometry tests cover stock/firing/support/muzzle order and
  contact surfaces. Source remains unchanged; no runtime axis/mirror fix.
- Existing legacy IK regression now tests actual M4/FR17 skin ownership through
  switch, ADS, recoil and movement. Default still uses old unconstrained A1;
  no completed Project arsenal/default migration is claimed. Source anatomical
  audit confirms correct Rigify identity; image-space chirality/pose needs review.
- C++ full build/tests and MSVC/Clang CI remain pending at this implementation
  checkpoint. No GUI started. Visual acceptance remains open after Escape.

## 2026-10-18 Phase 4 R1 exact runtime integration

- Separate renderer Draft https://github.com/YoungJibbit95/Novacore-Engine/pull/13
  stacks on R0 #12. Final pin3d673b7345d295588367b466a8877e84dee112a9;
  implementation a68ea79, final3d673b7 changes documentation only. This game
  branch stacks on R0 integration #26 (571d264), which stacks on W1 #25 and #23.
  No automatic merge, rewrite or original-checkout edit occurred.
- R1 transforms instance normals by R*inverse(S), matching position order
  Ry*Rx*Rz. Unit light directions and bounded factors are packed to preserve the
  existing128-byte push limit; no new buffers/descriptors/lifetime framework.
  Independent tests cover geometric tangent orthogonality,180-degree lighting,
  tiny/invalid scales, NaN/Inf/FE_INVALID and precision. Measured error maxima:
  direction0.00371697 degrees; factor7.62939e-06. All45 public headers match R0.
- Full engine Release build and7/7 CPU tests pass1.63s; separate complete Vulkan
  static-library build and install pass. All15 shaders compile and pass SPIR-V
  validation; member offsets match C++ and total128bytes. R0 shaders stay frozen.
- Complete game build/relink against the exact public R1 library and45/45 CPU
  CTest cases pass156.03s. Local validation reuses the byte-identical R0 public
  headers and overrides only the imported public library path, avoiding another
  full compile and preserving R0 packages. Normal source-pin/CI builds need no
  such local cache hint or engine-private headers. Pin updated only after this
  cross-repository validation. No tests disabled or tolerances relaxed.
- R0 game Vulkan build completed all73 steps. Preserved review executable:
  recovery/phase4/review-r0/nemisis_game.exe SHA256
  0208996a03da69acf9c0919cf8633a9d24bf2ad3fe2b3bc63e6065768ce644f8.
  The same game objects link successfully with the exact R1 Vulkan library;
  recovery/phase4/review-r1/nemisis_game.exe SHA256
  24414568ed2c10af703dca68a5c7b89707f1d9933aec9a2a2d93979127adb77e.
  These review copies retain their compiled asset root
  build/phase4-r0-vulkan/project-fps-candidate; preserve that directory. Runtime
  DLL imports were inspected. No GUI launch is inferred from a successful link.
- Manual launcher: tools/run_project_rifle_ready.ps1 -Executable
  F:/Coding/Project/recovery/phase4/review-r1/nemisis_game.exe (add -LookDown for
  body). Use game FOV55/74/105; engine-only CPU cases40/110 are not game settings.
  Matched capture metadata/observations belong in recovery/phase4/visual and PR
  bodies; at this technical checkpoint no renewed GUI permission or Phase4
  Vulkan image exists. Reference board and hashes remain in visual-references.
- W1 and engine R0 hosted Windows/MSVC + Ubuntu/Clang checks are green. Game R0
  and R1 hosted results are tracked in their PRs; R1 publication/CI follows this
  commit. Technical validation is separate from visual acceptance.
- R1 fixes render-instance normals only. The existing importer's linear baked
  nonuniform normal transform and fixed-view specular/rim approximation remain
  explicit followups. Dynamic shadow maps, real texture binding, BRDF/IBL and
  LOD/performance packages R2-R5 have not been implemented in this slice.
- Weapon continuation: W2 FR17 actual barrel/stock forward/up correction and
  asset-specific sockets before Project SMG Hip/ADS; then W3 Glock, W4 suitable
  shotgun source or explicitly approved proxy, class-specific animation/switching
  and W7 actual visual matrix. Current Ready is still a developer pose over old
  Walk/Run; no arsenal/ADS/reload/third-person completion is claimed.

## 2026-10-18 Phase 4 R0 integration and visual reference handoff

- W1 Draft https://github.com/YoungJibbit95/Nemisis/pull/25 at e2b0488 has green
  Windows/MSVC and Ubuntu/Clang CI, run39165450688. It depends on still-open #23;
  no merges performed. Its PR body includes explicit F01-F03 visual targets.
- Exactly one renderer agent implemented R0 in its own engine worktree. Draft
  https://github.com/YoungJibbit95/Novacore-Engine/pull/12 targets main d4eee3a.
  New game pin: fa37f17cb77649434ce4893acc25437dbc0bb910. Runtime f0c90b7 plus
  documentation-only followup fa37f17; inspected diff contains only two docs.
- R0 replaces fixed sky ray factors/opposite screen-Y with the world camera's
  actual FOV/aspect/projection and yaw/pitch/roll basis. Sun remains the shared
  world-space toward-light direction. Game forwards presented camera roll and
  config/fallbacks use azimuth25/elevation20 degrees instead of elevation62.7.
- Engine full dependency-light build and6/6 CPU tests pass1.49s. Vulkan backend
  and all15 shaders compile; SPIR-V validation passes, sky push block128bytes.
  Exact-public-package Nemisis full Release build and45/45 CPU tests pass133.77s.
  First integration run caught one test's zero-roll assumption after its fixture
  gained12-degree camera roll. Assert now checks the same5-degree import-roll
  tolerance relative to camera; complete suite rerun passes. No test disabled.
- Separate Vulkan game directory build/phase4-r0-vulkan is configured; binary
  compilation/real GPU acceptance are still pending at this checkpoint. Original
  Phase3 EXE/DLLs copied to recovery/phase4/baseline-vulkan, EXE SHA256
  4cd67d719e7d2c94bbb3c64534797d94e943d8b8b07957701b8ac0cf65aa7ebc.
  R0 shaders/packages are frozen separately from the following R1 work.
- User reference ZIP extracted to recovery/phase4/visual-references,13/13 external
  images downloaded, decoded/verified, source/rights/hashes in download-status.json;
  original board/README/JSON preserved, local board also available. Reference
  binaries stay outside Git/game assets. FIRST_PERSON_RECOVERY records concrete
  F/S/M/P/L criteria, historical-image limits and matched Vulkan capture metadata.
- No renewed GUI authorization after physical Escape: no Phase4 GUI actions,
  Vulkan screenshots or performance improvements claimed. R1 instance normals
  remains a separate engine branch/PR. Current stable weapon slice is W1 only;
  W2 begins with actual FR17 forward/up/socket corrections, not the old markers.

## 2026-10-18 Phase 4 W1: inventory and right-handed developer reference

- Fresh GitHub audit: #19, #20, #21, #22 and baseline #24 merged; #23 remains
  Draft against main. Both Windows/MSVC and Ubuntu/Clang checks at its final
  8dc9ce3 head passed (run 39157419733). W1 depends on #23; main cce7bec was
  merged into this isolated recovery branch. Original dirty checkouts untouched.
- Exact engine pin remains a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7. Engine main
  d4eee3a differs from that runtime only in recovery documentation.
- FIRST_PERSON_RECOVERY records all four playable definitions, six source/cooked
  Project assets, evaluated sockets, class/profile gaps and explicit fallbacks.
  NCAR-as-shotgun is an unresolved content mismatch, not an approved proxy.
- Re-authored M4 developer Hip reference X=-.11 -> +.11 in Blender 5.1.0,
  preserving original anatomy, source meshes, positive scale and actual palm
  frames. Kept the reach-compatible torso; reversing it failed the real support
  arm reach check and was rejected. No runtime axis fix or mirror introduced.
- Permanent actual Ready tests verify primary-grip camera side over 288 movement/
  yaw ticks, forward hemisphere, positive scale and physical .R/.L parent bones.
  Independent contact/orientation and original binary/palette checks retained.
  Old Phase 3 left-pose fixture is rejected (285 side failures); new fixture passes.
- Full dependency-light Release build passes; 45/45 CPU CTest cases pass in
  105.47s. Actual Ready contact max 0.000999308m, minimum camera-forward dot
  0.643084 during original locomotion; both hand vertices in frustum (179 right /
  2275 left). These are not occlusion/pixel or artist-quality claims.
- Exactly one rendering subagent owns separate engine worktree/branch, R0 first;
  game files/pin remain main-agent owned. No GUI permission was renewed after
  physical Escape; no Phase 4 GUI input, screenshots or GPU acceptance claimed.
- Next: publish W1 Draft/CI, then FR17 geometry-specific offline cook and Project
  SMG integration. Separately validate exact R0 engine commit against Nemisis
  before updating its pin/config; R1 must remain a separate renderer PR.

## 2026-10-18 Phase 2D actual Project FPS runtime preview

- User has merged engine#11 and game#13â€“#17; current game main is
  58e55df. Candidate#18 remains open, now based on main, with green MSVC/Clang
  checks on45363f8. Runtime preview branch stacks on that exact candidate head.
  No engine pin change: a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7.
- `--project-fps-preview` now mounts generated metadata into the existing asset
  registry and loads/animates the actual9,536-arm/40,532-body-vertex subsets.
  Own DynamicVertices resources player/local/fps_arms and player/local/fps_body
  initially submitted in ViewModel and World layers respectively; the corrected
  placement below uses World for both to share the same anatomical projection. No A1 GPU resource,
  fallback/proxy, or hidden full-world-body skin/update passes in FPS preview.
- Existing authored locomotion sampling, original weights/scales/hierarchy and
  shared final weapon snapshot are retained. Missing data fails explicitly.
  Project hand contact remains blocked and the incompatible A1 IK flag is
  rejected explicitly; no silent substitution or guessed bone-scale removal.
- Full local GCC13.2 Release build and40/40 CPU cases pass, including real mesh
  submission/animation and actual GameApp preview smoke. The candidate's source
  hash/topology/vertex parity checks remain intact. Invalid-metadata negative
  control fails before graphics startup. See FIRST_PERSON_RECOVERY.md for IDs,
  launch commands, staging, camera test pose and known release blockers.
- Isolated M4 correction336cf7346abd65b29062625d6d9a42a62e97b220 published as
  DRAFT https://github.com/YoungJibbit95/Nemisis/pull/19 against the newly merged
  main.37/37 IK and34/34 default-OFF CPU cases passed previously; corrected GPU
  approval remains open after Escape. Source GLB is unchanged.
- No current GUI permission is inferred from the Phase2D request. Prepare a
  combined preview+M4 graphical build, then request fresh permission for the two
  required images or use the documented manual commands. Runtime integration,
  visual acceptance and completed Project weapon rig remain separate milestones.
- User subsequently approved the two preview screenshots. First actual RTX3070Ti
  Vulkan run loaded/submitted both real subsets but rapid movement exposed
  overlapping-crossfade rejection in the existing game animator. Fixed by
  finishing the active short fade before applying the latest request; real
  Project regression covers repeated rapid transitions and settling to idle.
  Initial image is a failed finding, not visual acceptance; repeat images pending.
- User rejected d60ce1f GPU placement: character180degrees reversed, camera
  independent of skeleton, floating/high arms and no weapon contact. Preserve
  this as failed acceptance, despite4619 successful updates per real subset.
- Corrected preview cook adds a single offline canonical scene parent to separate
  runtime GLBs; both measured foot-to-toe vectors now face gameplay+Z and rig XZ
  pivot is centered. Baseline subset/source assets and binary/key/weight data
  stay unchanged. New bone-authored eye reference follows the sampled head.
  Camera position, body and arms now share the real player/body transform;
  detached camera-relative arms mount and mismatched arms projection are removed.
  No forced visible hand pose: missing weapon-ready clips/constraints remain an
  explicit acceptance blocker. Corrected GPU inspection remains required.
- Corrected placement passes full local GCC Release40/40 CPU cases. Canonical
  skinning/data/anatomy contracts and actual GameApp smoke pass. In the source's
  unarmed idle pose there are0 arm vertices in the hipfire frustum (body look-down
  has37242); hipfire arm visibility is therefore still **FAILED**, pending real
  weapon-ready posing/constraints. Do not restore the floating mount to hide it.
- Separate palm authoring draft#21 at75ce615 has green MSVC/Clang CI (40 cases).
  M4 draft#19 remains separate and green; runtime draft#20 is not promoted.

## 2026-10-18 Phase 2C candidate / visual and integration gates OPEN

- Published DRAFT https://github.com/YoungJibbit95/Nemisis/pull/18, implementation
  bdd03b3, review base #16. Hosted MSVC/Clang checks belong to that PR head.

- Reverified engine #10/#11 and game #12â€“#16. Game #12 was merged by the user:
  main f5399961da9e01c341b141785f116d76bc931731. Game #13/#14/#15/#16 remain
  open drafts with green MSVC/Clang checks. Engine #10 ready/#11 draft are open
  with green checks. User-created game #17 has also passed MSVC/Clang; it includes
  only the earlier frozen #12/#13/#14 integration, not #15/#16.
- Separate integration branch recovery/phase2c-integration, HEAD83d9d00fb48d81547e3b3cd3e69ed9f44384baa1,
  combines #16 and current main ancestry without conflicts. No duplicate
  integration PR or remote merge was created. Original dirty worktrees preserved.
- Exact engine pin remains a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7. Engine5/5,
  fresh default-OFF34/34 and IK-ON37/37 CPU cases pass. Full SDL3/Vulkan graphical
  target compiles/links with HAS_SDL3/HAS_VULKAN and productive IK enabled.
- CLion selected nemisis_game Debug without IK arguments. Its cache still names
  F:/Coding/Project Nemisis/... and its EXE timestamp is2026-10-06. The recovery
  Phase2B Vulkan EXE was built17:15:53, after the supplied17:14 screenshots; those
  images cannot validate PR#16. Their exact source revision remains unknown.
  The nearby T-pose Project model matches a static showcase at(-7.55,0,-6.75),
  separate from the animated local player/local/body resource.
- User explicitly renewed GUI permission during Phase2C. Actual Vulkan on RTX3070Ti
  confirms the large legacy A1 geometry blocks the view; opt-in IK reduces its
  footprint but is not anatomically accepted. A separate read-only audit proved
  the M4 cook's source-up sign was wrong; the correction belongs in a separate
  focused asset PR. Held ADS/pitch/full movement acceptance remains open.
- New optional Project FPS cook/contracts and39/39 extended Release CPU cases
  pass. FIRST_PERSON_RECOVERY.md records exact reductions, public-engine subset
  parity, reproducible measurements and artist/IK blockers. Candidate files stay
  in build directories and are not registered in gameplay. No source asset,
  production rig, engine pin or legacy fallback is replaced/removed.
- Next: obtain artist-reviewed cut seams and palm frames, then a safe adaptation
  of the five-segment/nonuniform Project chains using existing ozz, followed by
  actual GPU hip/held ADS/pitch/reload/movement images. Technical candidate
  acceptance does not satisfy Visual Acceptance or Character Integration.

## 2026-10-18 Phase 2B productive constraints, opt-in / visual gate OPEN

- Published DRAFT https://github.com/YoungJibbit95/Nemisis/pull/16, implementation
  10610e7 (the following record-only commit changes no runtime). Review base #15;
  required hosted MSVC/Clang 37-case validation is attached to
  https://github.com/YoungJibbit95/Nemisis/pull/16/checks.

- Shared-pose PR https://github.com/YoungJibbit95/Nemisis/pull/15 is DRAFT,
  implementation 57835a0 (base frozen integration 5baab6137dc5e0a3a81a2b0e9ec62ac4f8a78fa0).
  Both hosted MSVC/Clang builds and 35 CPU cases pass:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39138283817.
- Branch recovery/09-production-hand-ik stacks on #15. New solver consumes the
  actual shared M4 grip matrices, accounts for authored socket translation AND
  rotation, applies independent arm-bone corrections before the single NovaCore
  skin and uses the existing dynamic arms resource. Final globals/socket access
  agree with final geometry. There is no second sampling/skeleton runtime.
- Exact ozz0.17.0 commit 744eb9d99f606eda849acb0b1204f7a3dc20bca1 and archive
  SHA256 are retained; shared CMake dependency and exact MIT notice serve game,
  integration test and probe. Capability defaults OFF; built capability still
  requires --first-person-hand-ik to activate. Ordinary launch keeps the baseline.
- Removed from the opt-in path: whole-mesh average correction and discontinuous
  ADS arm scale. The legacy default still uses its original averaging until
  visual acceptance. One existing artistic mount is retuned for reach, with
  no change to the final weapon, axes, GLBs or bone translations/scales.
- Fresh default-OFF complete GCC13.2/Ninja build and 34/34 CPU cases pass; no ozz
  population directory/targets. Uncompiled runtime flag rejects with exit2.
  Focused opt-in production tests pass actual bind/Idle/Aim, four M4 hip/ADS/
  combined poses, nine ADS steps, both hand orientations/contact, bone lengths,
  deformed weighted geometry, bounds, six invalid input categories and single
  sampling/skinning. All authored clip coverage is executed.
- Complete opt-in cross-repo build and 37/37 CPU cases pass with exact published
  engine a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7. The pin was updated only after
  that validation. This includes all standard cases, ozz probe, actual-M4
  integration unit and the productive headless Dev Range smoke.
- Review exposed a real engine blocker: Vulkan dynamic updates only wrote one
  alternating frame slot, so a render frame without a fixed tick could draw
  older arms. A separate focused engine PR retains the latest payload until
  each safe frame slot consumes it. No game-side repeated skin/upload workaround.
  Engine draft https://github.com/YoungJibbit95/Novacore-Engine/pull/11 bases on
  engine #10 e22d397. Runtime fix a687c88 passes full local5/5 and hosted MSVC/
  Clang5/5: https://github.com/YoungJibbit95/Novacore-Engine/actions/runs/39139271352.
  Both existing and new game PR dependencies remain open; none is auto-merged.
- Final default-OFF full build/34 CPU cases also pass against the new pin. After
  adding accepted vertex-update accounting, affected integration/headless tests
  pass again: applied=4, samples=4, skins=4, vertex_updates=4, invalid=0,
  unreachable=0. These are accepted renderer requests, not executed GPU frames.
- Full graphical GCC13.2/SDL3 3.2.22/Vulkan SDK1.4.350.0 target compiles/links,
  including the real conditional Vulkan backend and new IK. No graphical
  executable or GPU-labelled smoke was launched. `git diff --check`, CMake
  preset inventory, actionlint and copied MIT notice hash checks pass.

Reproduce local graphical build from this checkout (existing verified SDL source
cache, exact engine pin; no launch is part of these commands):

```powershell
cmake -S . -B build/phase2b-vulkan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=C:/Strawberry/c/bin/g++.exe -DNOVACORE_ENGINE_ROOT=F:/Coding/Project/recovery/phase2b/dynamic-mesh/Novacore-Engine -DNOVACORE_ENABLE_SDL3=ON -DNOVACORE_ENABLE_VULKAN=ON -DNOVACORE_FETCH_SDL3=ON -DFETCHCONTENT_SOURCE_DIR_SDL3=F:/Coding/Project/recovery/Nemisis/build/phase1-vulkan/_deps/sdl3-src -DSDL_SHARED=OFF -DSDL_STATIC=ON -DNEMISIS_COPY_RUNTIME_DATA=OFF -DNOVACORE_BUILD_SANDBOX=OFF -DNEMISIS_ENABLE_HAND_IK=ON -DNEMISIS_BUILD_OZZ_IK_PROBE=ON -DVulkan_INCLUDE_DIR=F:/VulkanSDK/1.4.350.0/Include -DVulkan_LIBRARY=F:/VulkanSDK/1.4.350.0/Lib/vulkan-1.lib -DVulkan_GLSLC_EXECUTABLE=F:/VulkanSDK/1.4.350.0/Bin/glslc.exe
cmake --build build/phase2b-vulkan --target nemisis_game --parallel 4
```

After actual GUI authorization/manual testing, run separately from source:
`./build/phase2b-vulkan/nemisis_game.exe --vulkan --dev-range` (legacy), then
`./build/phase2b-vulkan/nemisis_game.exe --vulkan --dev-range --first-person-hand-ik`.
These commands are provided for the next step, not reported as executed.

FIRST_PERSON_RECOVERY.md contains exact dataflow/timing, controls/manual Vulkan
commands, removal criterion and a precise deferred Project head-safe cooking
plan. No GUI authorization, graphical launch, images or GPU smoke is claimed.
The chosen character remains chr_project_male1; A1 arms remain temporary.

Next executable session step: recheck engine #10/#11 and game #12/#13/#14/#15/#16
heads, merge states and CI before any branch/pin changes. Engine #10 -> #11 and
game #12 -> #14, plus independent #13, precede shared pose #15 -> constraints #16.
The two new game PRs deliberately target integration/stacked branches, not main;
retarget only after upstream dependencies are integrated and the resulting diff
is reviewed. Do not auto-merge or rewrite history. Detached engine worktree
F:/Coding/Project/recovery/phase2b/Novacore-Engine stays exactly at the new pin;
verify its HEAD equals cmake/novacore-revision.txt for reproducible future builds.
Then obtain actual manual images/fresh GUI authorization for the documented
Vulkan matrix. Only that acceptance unlocks PR C legacy removal/default switch;
PR D Project head-safe cooking remains a separate plan. Preserve old/user trees.

## 2026-10-18 Phase 2B shared presentation pose

- Rechecked engine #10 e22d39708f09a80472c185ab52cc59b90a3f8c14 and game #12
  ab63baa72bf145798e2f7177a5666da8cc0519e3, #13
  8ba69f7a79dc087d3d829e7d83713ae0a3cdbf86, #14
  eab9c3966899f19f5f38388b5eaff89ac28c5faa: all OPEN, MSVC/Clang green;
  #13/#14 draft, #14 bases on #12. No PR/main merges or old-worktree edits.
- New frozen integration branch recovery/phase2b-integration at
  5baab6137dc5e0a3a81a2b0e9ec62ac4f8a78fa0 combines those exact game heads
  without conflicts (intermediate geometry merge retained in Git history).
  Fresh complete GCC13.2/Ninja build and 35/35 CPU cases pass, including real
  headless boot and ozz probe; 38 registered, three GPU-labelled cases excluded.
  NovaCore rerun: 5/5. Game built against detached exact engine pin
  15c1b684a430bb70235ea9407f78106d958812b6; engine #10's later change is docs only.
- PR A branch recovery/08-shared-weapon-pose bases on that integration, not main.
  Final weapon/muzzle/grip pose now has one pure evaluation and a fixed-tick
  cached owner. Renderer consumes its selected asset and instance unchanged;
  marker orientation is included. Camera is fixed-tick, with no interpolation.
  No extra animation advance, skinning or upload was introduced. Reset clears
  the snapshot; a loadout changed between ticks cannot change its weapon mesh.
- Legacy whole-arms averaging remains the baseline in this behavior-preserving
  extraction. Real M4 hip/ADS/combined camera/recoil and authored orientation
  tests cover the shared renderer/constraint input. Visual gate remains OPEN;
  no GUI launch, control or GPU-image claim is authorized by this work.
- Post-extraction complete local build and 35/35 CPU cases pass; focused real
  asset/shared pose suite passes. The missing-handle regression caught during
  extraction was corrected without weakening its existing assertion.

Next: publish/validate PR A, then stack production hand constraints after NovaCore
sampling and before its single skinning/upload. Bypass legacy averaging only in
the opt-in IK path; keep default off until manual/authorized GPU acceptance.

## 2026-10-18 Phase 2 selected character and authored hierarchy integration

- GitHub rechecked: engine #9 merged, main a727930; game #10/#11 merged, main
  741aeae2edeb2df67564ae0b2379ab06a3df5505. Older pending/unmerged notes are history.
  Original user checkouts and untracked configs/user/ in the old audit worktree
  remain untouched. New worktree: F:/Coding/Project/recovery/phase2/Nemisis.
- User explicitly chose chr_project_male1. FIRST_PERSON_RECOVERY.md records real
  dataflow/assets, proven shared transform failures, presentation duplicates,
  Rigify/clip/mask gaps, adoption decisions and remaining visual acceptance.
- Pin changed from dbdfc38 to exact published NovaCore
  15c1b684a430bb70235ea9407f78106d958812b6 (engine PR #10). That fixes authored
  socket locals and non-joint ancestry without per-asset Euler changes.
- New actual-assets test failed on old pin with 24 checks and bind drift up to
  2.00225 m; passes after the fix, including all required bind socket positions/
  directions, weighted bind geometry, actual animated deformation and updates.
- Project local body now gets its own dynamic player/local/body resource and
  external/world submission at metre scale / feet pivot. Static Project target/
  showcase handles stay separate. Missing pose does not substitute another model.
  Hidden full-body skinning/uploads are skipped while FPS camera rig is active.
- Fresh old-pin baseline full build / 34 CPU tests passed. Full changed-engine
  C++23 GCC 13.2/Ninja build and all 34/34 CPU tests pass. Engine full 5/5 local and
  hosted MSVC/Clang pass. Game hosted matrix also passes at e48c6e3:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39127318199.
- No GLBs/fixups, balance, movement/network simulation, renderer API or GPU test
  requirements changed. No GUI control / visual validation after Escape.
- This is a completed import/selected-world-body slice, not the complete FPS rig:
  authored head-safe Project FPS split and actual hand constraints remain open.
- Published PR https://github.com/YoungJibbit95/Nemisis/pull/12, implementation
  e48c6e38605a3c3abf971646ffd8901c63583c16. Engine PR #10 should integrate first;
  the exact published pin already works independently of the engine merge commit.
- Geometry PR #13 is independent and also hosted-green. Local review branch
  recovery/phase2-review combines both published implementations at259755f;
  complete CPU build/34 cases and SDL3 3.2.22/Vulkan SDK1.4.350.0 build pass.
  No GUI launch/inputs or GPU images: new authorization question remains pending.
  Optional ozz probe is being isolated from the productive game path.

Next: replace the whole-arms average translation with current-pose hand constraints
after final weapon/grip computation and before skinning; reviewed ozz TwoBoneIK
is the targeted library candidate. Separate presentation cleanup removes proven
extra geometry. Require a renewed authorized GPU image matrix before visual
acceptance or a binary asset migration. Do not merge automatically.

## 2026-10-18 optional ozz arm-IK adoption probe

- This separate branch is based on selected-character PR #12 (ab63baa), because
  it needs the exact authored-hierarchy engine pin15c1b68. No main/PR auto-merge.
- Added an opt-in CPU tool using ozz0.17.0, exact commit744eb9d and verified
  archive SHA256. Retained/copy MIT notice. Default game build remains free of
  the dependency; CI enables the option and runs35 CPU cases (plus3 excluded GPU
  entries). CMake3.30 is required only when opting in. No importer/tools/windowing.
- Uses NovaCore's existing skeleton, sampling and skinning plus IKTwoBoneJob
  quaternion corrections. Actual bind/Idle/Aim arms, both chains, three small
  reachable targets:18 solves. Tests preserve bone lengths, socket orientation/
  authored offsets, finite residuals below1mm, and actual weighted deformation.
  No productive dual animation runtime and no new custom IK solver.
- Full local GCC13.2/Ninja opt-in build and35 CPU tests pass. Final weighted-
  deformation assertions also pass the affected probe. Default-off configure has
  no ozz population/targets; actionlint and diff check pass. Source/hash/diagnostic
  build-size measurements and reproduction are in FIRST_PERSON_RECOVERY.md.
- This is a reproducible temporary adoption probe, not a solved M4/game hand rig.
  Whole-arms average translation still exists in production. The user's desired
  Project model still needs a safe authored FPS mask/split. GPU/UI permission
  remains pending; no further GUI launch/input/capture after the earlier stop.
- Published draft PR https://github.com/YoungJibbit95/Nemisis/pull/14, implementation
  c6a2d7e4dddeacb13c4a4a82024c056e740b2418, based on PR #12. Hosted full builds
  and35/35 CPU cases pass on Windows/MSVC and Ubuntu/Clang, including fresh
  hash-verified upstream archive fetching:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39128535291.
  Merge-tree check against independent geometry PR #13 is clean; no main merge.
  This final follow-up only records evidence. Engine/game original user checkouts
  still contain their original15/75 dirty entries; no files there were changed.

Next: with an authorized visual baseline, integrate one final weapon transform,
derive actual M4 grips in arms model space, apply the reviewed constraints after
base animation and before final skinning/upload. Then remove the average-shift
workaround and prove hip/ADS/reload/fire/movement contact with CPU + GPU evidence.
Do not turn this probe into a permanent second runtime/backend choice.

Historical Phase 1 start: GitHub verified PR #7, #8 and #9 merged; main was ff88eb0. The original
main checkout remains dirty with user work and must not be overwritten. Earlier
unmerged/pending notes below are historical. Phase 1 starts clean from this main;
the runtime pin was dbdfc38. Test audit and coordinate work used independent PRs.

## 2026-10-18 baseline and rules

- Published baseline: 6b0f411eab1a717656adfd7f70f10e2f76a5dcbf.
- Rules commit: d0f67b5; PR: https://github.com/YoungJibbit95/Nemisis/pull/7.
- Tested NovaCore: dbdfc38d092e018952ead43d9cb669333ebf70bc.
- Worktree: F:/Coding/Project/recovery/Nemisis. Original main has extensive user
  code/asset changes and untracked workflows; none were overwritten or imported.
- Executed: `cmake --preset local-debug-no-deps -DCMAKE_CXX_COMPILER=C:/Strawberry/c/bin/g++.exe -DNEMISIS_COPY_RUNTIME_DATA=OFF`,
  `cmake --build --preset local-debug-no-deps --parallel 4`,
  `ctest --test-dir build/local-debug-no-deps --output-on-failure --timeout 120`.
- Result: complete game/test build, 37/37 registered tests pass on GCC 13.2 /
  Ninja / Windows with SDL3/Vulkan disabled. Logs: build/baseline-*.log.
- Pre-existing validation gap: the three Vulkan-named smokes pass headlessly.
  They do not prove graphics behavior. Runtime copying was disabled only for
  this source-checkout validation; shipping staging has not been changed.
- Not run: Linux/Clang, Windows/MSVC, usable-GPU or playable visual validation.
  No local MSVC installation found. No baseline test failures on this toolchain.
- Removed: contradictory large-patch/silence rules and broad feature queues.
  No runtime code, assets or tests removed. GitHub push/PR succeeded; no merge.

Next: recovery/01-ci-baseline based on the rules PR; pin the tested NovaCore SHA,
add headless Windows/MSVC and Ubuntu/Clang CI, make hardware requirements explicit,
and run startup from source so missing staged configs cannot fake useful boot
coverage. Then classify tests and inspect the asset contract without touching
uncommitted source/GLB changes in the original checkout.

## 2026-10-18 Phase 1 game test audit

- Inspected all 33 sources / 177 test functions, 37 CTest registrations and the
  runtime-smoke runner. TEST_AUDIT.md records invariants, categories, overlap,
  KEEP/REVIEW/MOVE candidates and removal risk at function/behavior-group level.
- Consolidated one identical WeaponRoot socket reference and two redundant zero
  assignments. Renamed five tests whose old names overstated pistol correction,
  shipped balance, nearest-hit ordering, physics immutability or asset scope.
  Every test function, assertion and CTest registration remains.
- Exact camera yaw/negative mesh pitch/full roll are valid coordinate contracts;
  stage counts remain review candidates, not automatically deleted scaffolding.
  Four pure physics cases are later MOVE candidates with distinct edge scenarios.
- Fresh baseline built against detached, exact pinned NovaCore dbdfc38: all
  34 CPU cases pass locally (GCC 13.2/Ninja). No engine pin/API/runtime change.
- Published implementation ccdfa63744c88ed4562876a2a5bfa6d92063e233 in independent
  PR https://github.com/YoungJibbit95/Nemisis/pull/10 (base: main ff88eb0).
- Post-change complete local build and 34/34 CPU cases pass on GCC 13.2/Ninja;
  all 177 functions/assertions remain. `git diff --check` passes.
- Hosted complete builds and 34/34 CPU cases pass on both Windows/MSVC and
  Ubuntu/Clang at that exact implementation commit:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39122629852.
- Independent NovaCore PR #9 adds its generic hierarchy contract. Independent
  Nemisis PR #11 documents coordinates and adds two game contract functions;
  neither introduces a runtime/API/pin dependency or asset migration.
- Separate graphical build: SDL3 3.2.22 / Vulkan SDK 1.4.350.0, active RTX 3070 Ti,
  36/36 ready mesh assets. One live hipfire image inspected; physical Escape
  stopped Computer Use. Held ADS, grip/muzzle placement and animation remain
  visually unverified. The three GPU CTest cases were not run or disabled.
- The interactive session created untracked configs/user/ in this recovery
  worktree. Preserve these user settings; they are excluded from this PR. Original
  dirty user checkouts remain untouched. This evidence follow-up changes docs only.
- Next: use the existing Dev Range to review the current/candidate M4A1 image
  matrix in ASSET_COORDINATE_CONTRACT.md (PR #11): hip/held ADS, forward/up,
  muzzle/grips, scale and procedural reload/recoil. Require passing CPU contracts
  and reviewed images before a separate cook/catalog migration. No automatic merge.

## 2026-10-18 CI baseline implementation

- Added shared `ci-headless` configure/build/test presets with Ubuntu/Clang and
  Windows/MSVC variants. Workflow pins NovaCore using the full SHA in
  cmake/novacore-revision.txt and verifies the checkout before configuration.
- Pin: dbdfc38d092e018952ead43d9cb669333ebf70bc. Local integration used the sibling
  engine at 28dca6f (rules/docs only beyond that identical published runtime).
- Workflows use read-only permissions, verified Action commit SHAs, bounded
  timeouts, PR cancellation and failure logs; no new secrets or dependency cache.
- Preserved all 37 tests. CPU CTest presets run 34; the three existing graphics
  smokes retain the explicit `gpu` label and require an active Vulkan backend.
  A small shared CMake runner also checks process exit, real catalog loading,
  missing config/asset diagnostics and completed shutdown. They run from source.
  This replaces the original exit-code-only smoke invocation, not game runtime.
- Executed: configure via `ci-headless` reusing build/local-debug-no-deps with
  GCC 13.2, complete incremental build, and all 34 CPU CTest cases (pass).
  New smoke loads 38 catalog entries / 36 cooked mesh assets with no rejected or
  missing mesh registrations. Actionlint, preset listing and diff checks pass.
- GPU negative control: nemisis_game_vulkan_smoke fails as expected in the
  no-SDL3/no-Vulkan build. Actual Vulkan rendering is not validated by this check.
- No runtime code, assets or tests deleted; no alignment corrections changed.
  Runtime copying is disabled only in CI presets; shipping staging remains open.
- Hosted fresh MSVC/Clang runs pending. Next: inspect/fix those runs, record final
  PRs/commits, then classify test contracts and inspect the canonical asset path.
  Do not delete weapon fixups until representative-asset visual validation exists.
- Initial hosted matrix is running. Switched Windows to the documented
  Windows-2022 / VS2022 runner after the engine's Windows-2025 job could not
  resolve a VS2022 instance. This changes the environment, not test coverage.

## Phase 0 validation complete

- Rules: d0f67b5 / tracking dd8c870, PR #7.
- CI: 6f0331b plus runner correction 25a314d, PR
  https://github.com/YoungJibbit95/Nemisis/pull/8 (base: recovery/00-agent-rules).
- Fresh hosted run at 25a314d passed:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39120132884.
  Windows 2022 / MSVC 19.44: full build, 34/34 CPU tests pass.
  Ubuntu 24.04 / Clang 18.1.3: full build, 34/34 CPU tests pass.
  Both independently checked out the exact pinned NovaCore dbdfc38 SHA.
- Local negative controls: Vulkan smoke rejects a null/headless backend;
  running the smoke runner from an empty data directory also fails despite the
  game returning exit code zero. Logs: build/*negative-control.log.
- Pre-existing warnings include unused engine buildSummary without SDL and the
  unhandled Ceiling switch value in GreyboxCollision.cpp; neither was hidden.
- Actionlint and the final diff whitespace check pass. No merge performed.
- All 37 tests remain registered; 3 graphics smokes require real GPU validation.
  No claim of visual correctness or complete two-player gameplay is made.

Next bounded step: remove the unused per-asset correction fields in
DevRangeRenderScene.cpp, whose four weapon and two arm definitions all pass
zero angles already. Retain the rig's generic pose defaults and socket behavior;
rename the misleading ImportAxisCorrections test without deleting its canonical
axis assertions. This is behavior-preserving cleanup, not an asset recook.
Then classify all test contracts and verify runtime axes/units/transform order
before migrating a representative weapon. Actual asset/fixup removal still
requires representative-asset visual/ADS/socket/animation validation.

## Phase 1 bounded cleanup: inactive mount correction slots

- Removed yaw/pitch/roll correction members from the two private per-asset mount
  structs in DevRangeRenderScene.cpp, plus 18 explicit zero initializer values
  and their forwarding. All four weapon and both arm definitions were already
  zero; the shared rig descriptor still supplies the same zero defaults.
- Used named initialization for the remaining weapon pose/recoil/reload data.
  Socket binding, hip/ADS offsets, scale, recoil/reload response, the generic rig
  API, fallback assets and cooked/source binaries are unchanged.
- Renamed testDevRangeRenderSceneUsesPerWeaponImportAxisCorrections to
  testDevRangeRenderSceneKeepsCookedWeaponAxes. The contract remains valid:
  cooked SMG/sidearm submission retains forward/upright axes and mount scale.
  Every assertion and test invocation is retained; no replacement test needed.
- Executed complete incremental game/test build and all 34 CPU CTest cases:
  pass on GCC 13.2 / Ninja. Source-tree headless asset boot also passes.
  Engine checkout: 15485df, runtime identical to pinned dbdfc38. Diff check passes.
- This is removal of inactive data, not recooking or changing asset orientation.
  GPU visual/ADS/socket/animation validation remains required before any actual
  alignment-fix or cooked-asset migration. Hosted MSVC/Clang follow-up pending.

Next: classify the full test portfolio and verify the actual coordinate/transform
contract, then select wpn_project_rifle_m4a1 as a representative migration only
after visual validation is available. Do not assume the recovery plan's historical
Euler fixes still exist: current weapon mount angles were already all zero.

### Hosted cleanup validation

- Implementation commit f38b3b4; PR:
  https://github.com/YoungJibbit95/Nemisis/pull/9 (base: recovery/01-ci-baseline).
- Fresh hosted Windows/MSVC and Ubuntu/Clang builds both pass all 34 CPU tests:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39120683593.
  Both use the exact published NovaCore pin dbdfc38d092e018952ead43d9cb669333ebf70bc.
- No active alignment change, asset migration, test deletion, or GPU validation.
  Final follow-up only records this evidence; implementation is already tested.

## 2026-10-18 canonical asset coordinate contract

- Independent branch from Nemisis main ff88eb0; no dependency on game test-audit
  PR #10. NovaCore runtime pin stays dbdfc38d092e018952ead43d9cb669333ebf70bc.
- ASSET_COORDINATE_CONTRACT.md traces source/import/cook, hierarchy, skinning,
  buffers, model/view/projection, mounts, ADS/recoil and static/animated sockets
  to concrete functions. Cooked GLB is the single canonical runtime boundary.
- Verified C(x,y,z)=(x,z,-y), model T*Ry*Rx*Rz*S, camera pitch sign and separate
  viewmodel projection. Documented that metadata dimensions use Blender order,
  axis reports describe grip-to-muzzle displacement, and static scene bindings
  ignore marker orientation. Animated bridge uses name hints/identity offsets;
  normal/lighting transforms have separate limitations. No workaround added.
- Chose wpn_project_rifle_m4a1 from the real catalog. Read source/cooked geometry,
  hierarchy and sockets, recorded hashes/bounds/origins and classified active
  offline fixups, neutral/removed runtime slots, art poses and report aliases.
  Both GLBs, metadata/fixups and runtime placement remain unchanged.
- Added two game contract functions: actual M4A1 metadata/required unique sockets,
  node frames, finite geometry, metre scale and +Z length; exact reconstructed
  muzzle/hand positions across rotated/recoiling hip and ADS submissions.
  Resulting game function count is 179, with 37 CTest entries / 34 CPU cases.
- The generic nested TRS/matrix/nonuniform-scale marker test belongs to NovaCore
  and is included in engine audit PR #9 using its existing tiny GLB writer.
- Local validation against exact detached dbdfc38: fresh complete C++23 build and
  all 34/34 CPU cases pass via ci-headless (GCC 13.2/Ninja). Final unique-name
  checks also pass in both affected test executables. No tests disabled/deleted.
- Separate graphical build succeeded with pinned engine, SDL3 release-3.2.22 and
  Vulkan SDK 1.4.350.0. Active RTX 3070 Ti backend and 36/36 ready mesh assets were
  logged; one live hipfire image was inspected. Physical Escape stopped Computer
  Use; no further UI actions/captures. Held ADS, grip/muzzle images and animation
  are not fully visually validated. Three GPU CTest smokes were not run here.
- Published implementation 65ea16e4986831003298197080487ca1caeea171 in independent
  PR https://github.com/YoungJibbit95/Nemisis/pull/11 (base: main ff88eb0).
- Hosted complete builds and all 34/34 CPU cases pass on Windows/MSVC and
  Ubuntu/Clang at that exact implementation commit:
  https://github.com/YoungJibbit95/Nemisis/actions/runs/39123958512.
  This final evidence follow-up changes docs only. `git diff --check` passes.
- Independent game audit PR #10 preserves all 177 baseline functions; the two
  new contract functions here give 179 when combined. A merge-tree check against
  audit f13f325 passes without conflicts; no actual merge was performed.
- The interactive visual session used the separate game-audit worktree and
  created untracked configs/user/ there. Preserve those user settings; they are
  excluded from both PRs. Both original dirty user checkouts remain untouched.

Next executable step: review the M4A1 current/candidate image matrix in the
existing Dev Range with a usable backend before any recook/catalog switch.
Acceptance: canonical CPU checks pass, hip/held ADS/muzzle/grips/scale and
procedural reload/recoil are visually reviewed, no game-test regression. Preserve
existing source/cooked binaries and active offline fixups until that gate passes.
A separate authored-animation socket-offset fixture/fix can be planned without
mixing it into the rigid-weapon migration. No automatic merges.

## 2026-10-18 Phase 2 first-person geometry selection

- Independent main 741aeae worktree: F:/Coding/Project/recovery/phase2/presentation/Nemisis.
  Engine pin stays dbdfc38; no dependency on selected-character game PR #12.
  Current engine PR #10 separately repairs authored sockets/non-joint ancestors.
- Confirmed extra geometry: arms GLB contains lod0_weapon_alignment_bar and
  lod1_blockout; all were submitted alongside skinned arms. Game bindings now
  exclude these two presentation primitives, retaining original source nodes,
  markers, metadata, all weighted arm vertices and the unchanged GLB.
- Real arm mesh suppresses duplicate procedural hands/forearms/torso by default.
  Explicit showFirstPersonRigDiagnostics enables proxy inspection; existing
  proxies remain as a safe missing-arms fallback. Socket statistics/weapon VFX
  stay live even when proxies are hidden. Proxies do not prove skinned hand contact.
- Removed whole-arm copies from wallrun/throw/mantle developer visuals. Existing
  boot glow, energy platform and mantle target effects remain. Exactly one
  viewmodel arm submission owns the normal first-person representation.
- Existing count assertions now reflect intentional proxy removal; no test
  function deleted. Grip reconstruction still runs with explicit diagnostics.
  Added actual-asset filtering/weighted-vertex protection and one-owner/VFX/
  missing-resource fallback coverage. 181 functions, still 37 entries/34 CPU cases.
- Complete fresh C++23 GCC13.2/Ninja build and all 34 CPU cases pass against exact
  old pin. Game/render behavior changes are CPU-submission validated only;
  no new GUI interaction or GPU images after Escape. Original user files intact.

Next: inspect hip/held ADS/look-down/slide/wallrun images after renewed GUI
authorization. Do not claim a completed Project first-person body: the user chose
chr_project_male1, whose head-safe FPS split and actual hand constraints remain
separate work. No GLB recook, new Euler repair, balance or simulation changes.

Published CPU evidence: implementation c98fa12120eed4c00acc418be9553009e41baf2d,
draft PR https://github.com/YoungJibbit95/Nemisis/pull/13. Fresh hosted complete
Windows/MSVC and Ubuntu/Clang builds / all34 CPU cases pass:
https://github.com/YoungJibbit95/Nemisis/actions/runs/39127413992.
Local review commit259755f combines published #12/#13 without conflict and passes
full CPU build/34 cases plus an SDL3/Vulkan build against exact engine15c1b68.
That review branch is unpublished and does not merge main. No new GUI launch or
images; the explicit new GUI-authorization question remains pending. Keep this
PR draft until its visual acceptance. This follow-up changes documentation only.

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
## 2026-10-18 Phase 3 developer Ready integration

- Read-only audit reconfirmed missing Rifle/ADS/Reload/Fire/backward/strafe clips;
  original Hard stand/Walk/Run/T_pose are retained, never renamed as substitutes.
- Technical prerequisite: draft PR #22, implementation
  733c1b5936097b853f4981a518dd9df0585fa37c. Full build and 42/42 CPU cases; hosted
  MSVC and Clang pass: https://github.com/YoungJibbit95/Nemisis/actions/runs/39154228416.
  Review base 78bf2b7 combines merged #18 with draft #19/#20/#21; no main merge.
- Ready follow-up branch recovery/phase3-armed-ready uses that prerequisite.
  One logical NovaCore pose/time, original separate palettes, animated eye/chest
  sockets, one shared M4 pose, Project palms/ozz constraints, then both skins and
  GPU updates. Hidden full world body remains skipped. Exact engine remains
  a687c8816fbcc8a7f4d313cf092c6dc17a59f8f7; no engine source changes.
- Non-destructive Blender 5.1.0 authoring generates 72 explicit upper-body
  rotation tracks and real palm/M4 surface calibrations. Original T/S, weighted
  intermediate bones, geometry, inverse binds and existing clip bytes survive.
  Ready developer finger curl is provisional. New GLBs/.blend stay build-only.
- Initial short-cycle suite passed 45/45; expanded full-cycle checks exposed
  static-mount and incomplete-mask failures, now repaired with sampled chest
  mounting and an explicit complete upper-body rotation mask. Final full-suite,
  hosted-CI and graphical-build evidence is recorded below when available.
- No Phase 3 game GUI control has been performed. Offline Blender evidence is
  not Vulkan acceptance. Technically Integrated applies to developer Ready only;
  Animation Complete and Visually Accepted remain NO. Directional/ADS/reload,
  advanced movement and complete world-model animation remain explicit work.

Next: run/review the explicit Ready executable via tools/run_project_rifle_ready.ps1,
with renewed GUI authorization after Escape. Inspect grip/fingers/shoulder and
look-down first; then author directional locomotion and ADS in separate steps.
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

## 2026-10-18 UI foundation / native component showcase

User-requested UI work is isolated in `ui/00-technology-spike`. RmlUi 6.1 and
FreeType 2.13.3 replace bitmap typography for the new opt-in `--ui-showcase`.
Space Grotesk regular/bold and all license notices ship with the UI assets.
Existing GameMenu, settings, persistence, loadout and HUD remain intact.
The optional UI engine pin is fb33cb5502e6a4ea59ddc6183329529f25c0c279 (separate
engine draft PR #14); the ordinary engine pin is unchanged. No merge.

Full local GCC Release headless game build/34 CPU tests and Vulkan game build
pass; native document tests cover 720p/1080p/1440p geometry and controls/modal/
controller actions with stable idle resources. Authorized live 720p inspection
confirmed real typography, gradients/glow, button/modal/dropdown interaction and
Dev Range transition/back. Physical Escape stopped Computer Use; no subsequent
GUI actions were made. Larger-resolution visual/resize/GPU timing acceptance
remains open. See docs/ui/UI_ROADMAP.md and UI_DESIGN_SYSTEM.md for the bounded
milestone, reproducible launch and remaining phases.
