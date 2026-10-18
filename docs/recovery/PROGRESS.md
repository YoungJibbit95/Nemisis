# Recovery progress

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
