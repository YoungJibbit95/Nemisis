# Authored world playtest candidate (Phase A)

Depends on NovaCore PR #40, which includes #37 and #38. Both engine pins
refer to the exact public candidate; do not merge before the dependencies
are integrated and both pins updated/retested.

Supported: one v1 document, one spawn marker, up to 4096 objects, axis-aligned
solid boxes and nonblocking visible boxes. IDs are preserved. The actual
catalog resolves IDs for diagnostics, but asset mesh instances are explicitly
unsupported until editor/game asset and collision parity is implemented.
Unknown content and unsafe spawn clearance are errors. Existing greybox
rendering and collision consume the same box list; no hardcoded range targets,
props or synthetic boundary collisions. Local profile excludes network and
legacy map modes. Leaving the scene by a large safety margin respawns the
player, not invisible walls. Escape exits; P resets. Advanced movement,
graphical acceptance and visual parity still require validation.

## CPU validation (no window or GPU)

    cmake --preset windows-vs2022-no-deps
    cmake --build --preset windows-vs2022-no-deps --target nemisis_game nemisis_authored_world_tests
    ctest --test-dir build/windows-vs2022-no-deps -C Debug -R nemisis_authored --output-on-failure
    .\build\windows-vs2022-no-deps\Debug\nemisis_game.exe --map .\configs\maps\editor-room.world.json --check-map

Use isolated game and engine worktrees; configure with
-DNOVACORE_ENGINE_ROOT=<absolute-engine-worktree> as needed.

## Graphical candidate (unverified here)

    cmake --preset windows-msvc-debug -DNOVACORE_ENGINE_ROOT=<absolute-engine-worktree>
    cmake --build --preset windows-msvc-debug --target nemisis_game
    .\build\windows-msvc-debug\Debug\nemisis_game.exe --map .\configs\maps\editor-room.world.json

Run from the game checkout so configs/cooked assets resolve. Launch requires
actual Vulkan. Open the same JSON in the editor, save to a separate path,
walk through the central doorway, test wall and nonblocking box, exit and
relaunch against modified JSON without rebuilding. Capture real GPU logs and
screenshots. Do not overwrite the shipped release.
