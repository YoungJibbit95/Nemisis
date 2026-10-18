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
