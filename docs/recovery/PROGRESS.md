# Recovery progress

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
