# Authored world playtest candidate (Phase A)

This validation-only candidate pins both engine files to d73a657efc814cf85035638e846918be5e9b422d,
an isolated public assembly of NovaCore #40 and #42 (both parent histories).
This is not an integration into main and must not be merged before the individual
engine PRs are independently reviewed, integrated, and the actual main-reachable
revision pinned and retested.

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


## Same-engine validation assembly

Use the pinned engine branch editor/phase-a-validation-assembly for both the
editor and game. In the Nemisis worktree:

    cmake --preset windows-msvc-debug -DNOVACORE_ENGINE_ROOT=<absolute-pinned-engine-worktree> -DNOVACORE_BUILD_WORLD_EDITOR=ON -DNOVACORE_REQUIRE_EDITOR_NATIVE=ON
    cmake --build --preset windows-msvc-debug --target nemisis_game novacore_world_editor
    .\build\windows-msvc-debug\Debug\nemisis_game.exe --map .\configs\maps\editor-room.world.json --vulkan-smoke

The smoke exits successfully only after actual Vulkan world frame submission,
authored spawn validation and render/static collider parity checks. The automated
Linux software-Vulkan job drives the real editor on a virtual 1280x800 display,
selects door-left in the hierarchy, changes X by 0.25 m, saves, launches Play Map,
reopens and resaves, and loads both files with the same game executable.
The job is a candidate test; do not claim success until its current-head CI passes.
Physical GPU and human interactive/visual acceptance remain required.


## Readiness regression

The original CI artifact (run 46166234933, job 141044266761) contained
successful Vulkan initialization and world/UI draw-submission messages, despite
a timeout waiting for the buffered backend log. The interaction harness now
waits for the editor's explicitly flushed submitted-frame readiness line and
checks the frame count, geometry count and extent while the process is alive.
It also waits for a flushed successful child-process exit report and verifies
the game executable hash remains unchanged throughout the original/modified
map tests. The existing readiness deadline was not increased.


The next run (46167732560) confirmed the readiness signal, editor modification
of door-left X from -3 to -2.75, saved JSON, actual game launch and exit code
zero. Its remaining failure was an X11 BadWindow after xdotool windowclose
destroyed the client window without a window manager. The harness now sends
the advertised WM_DELETE_WINDOW ClientMessage and requires zero editor exit
status plus renderer cleanup. The message layout/protocol was exercised with a
real X11 client under local Xvfb before publication. This does not alter the
editor's graphical acceptance criteria or disable any test.


## Windows one-command launcher

From the Nemisis checkout:

    powershell -NoProfile -File .\tools\Launch-WorldEditor.ps1

The launcher checks the two identical engine pins and verifies their
reachability from the fetched remote main. It creates a separate detached
engine worktree under build/world-editor-local and an isolated incremental
Debug build for both executables. It never resets, stashes, or overwrites your
existing release. It launches from the project root so the asset catalog and
cooked data resolve. It opens the example map without authorizing overwrite.

While Phase A remains an unmerged candidate, explicitly opt in:

    powershell -NoProfile -File .\tools\Launch-WorldEditor.ps1 -CandidateRef editor/phase-a-validation-assembly

Use -EngineRepository to locate a different original engine repository,
-EngineCheckout to use an already clean exact pinned engine worktree,
-Map to choose another JSON file, -SaveAs only to authorize a destination,
-ValidateOnly for pin/worktree/catalog checks without building, or -BuildOnly
to build without opening a window. The existing Visual Studio 2022, SDL3,
Vulkan SDK and glslc requirements remain. The script does not install tools.

Expected executables under build/world-editor-local/g<12-char game SHA>-e<12-char engine SHA>:
Debug/nemisis_game.exe and Novacore-Engine/Debug/novacore_world_editor.exe.
The launcher reports exact paths only after both are present. The Windows
graphical launch remains a local acceptance gate.


The full revision IDs are recorded in the isolated build directory before
CMake runs. The Windows CI launcher contract uses a disposable local Git
fixture to test exact pin validation, unmerged-revision rejection, explicit
candidate opt-in, dirty checkout rejection and preservation of the original
checkout. It does not build or exercise real graphical hardware.
