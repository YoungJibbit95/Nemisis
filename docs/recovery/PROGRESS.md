# Recovery progress

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
