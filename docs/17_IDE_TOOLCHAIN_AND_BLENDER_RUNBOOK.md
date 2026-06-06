# 17 IDE Toolchain And Blender Runbook

## Goal

Nemisis should be runnable from common CMake-aware IDEs while the engine remains a separate sibling repo.

Supported IDE paths:

- Visual Studio 2022: open the Nemisis folder and select `windows-vs2022-no-deps` or `windows-msvc-debug`.
- Visual Studio Code: install CMake Tools, open the folder, select a CMake preset.
- CLion: open the folder and import CMake presets.
- Rider C++ / Qt Creator: open the root `CMakeLists.txt` or CMake presets where supported.

## Presets

`local-debug-no-deps`:

- Generator: Ninja.
- Resolves NovaCore from `../Novacore-Engine`.
- Best for tests and headless/null-renderer work.

`windows-vs2022-no-deps`:

- Generator: Visual Studio 17 2022.
- Resolves NovaCore from `../Novacore-Engine`.
- Disables SDL3 and Vulkan in the NovaCore subproject.
- Best when Ninja is missing but Visual Studio Build Tools are installed.

`windows-msvc-debug`:

- Generator: Visual Studio 17 2022.
- Does not require Ninja.
- Does not require `VCPKG_ROOT`.
- Uses NovaCore's SDL3 FetchContent fallback when no installed SDL3 package is found.
- Best default for VSCode/Visual Studio on Windows.

`windows-ninja-vcpkg-debug`:

- Generator: Ninja.
- Uses vcpkg for full dependency work.
- Intended for SDL3, Vulkan, and future renderer testing once vcpkg and Ninja exist.

`windows-msvc-vcpkg-debug`:

- Generator: Visual Studio 17 2022.
- Uses vcpkg for SDL3/Vulkan dependency work.
- Does not require Ninja.
- Best path for visible SDL debug renderer testing in VSCode/Visual Studio.

## Minimal Build

Visual Studio generator:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --test-dir build/windows-msvc-debug -C Debug
.\build\windows-msvc-debug\Debug\nemisis_game.exe
```

CLion/default Ninja build directory:

```powershell
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
.\cmake-build-debug\nemisis_game.exe
```

Visible SDL debug renderer with explicit vcpkg:

```powershell
cmake --preset windows-msvc-vcpkg-debug
cmake --build --preset windows-msvc-vcpkg-debug
.\build\windows-msvc-vcpkg-debug\Debug\nemisis_game.exe
```

Ninja generator:

```powershell
cmake --preset local-debug-no-deps
cmake --build --preset local-debug-no-deps
ctest --test-dir build/local-debug-no-deps
.\build\local-debug-no-deps\nemisis_game.exe
```

Runtime data:

- CMake copies `configs/` and `assets/` beside `nemisis_game` after build when `NEMISIS_COPY_RUNTIME_DATA` is ON.
- On MinGW builds, CMake copies required MinGW runtime DLLs beside all Nemisis executable targets so direct shell launch works without editing PATH.
- Visual Studio debugger working directory is set to the repository root for `nemisis_game`.
- Config loading still works best when launching from the repository root or the executable directory produced by CMake.
- NovaCore probes the Vulkan runtime dynamically, so the game can report Vulkan loader/device availability even before the compiled Vulkan backend is active.

If the game logs `SDL3 unavailable; using headless window fallback`, the build directory was configured before SDL3 was available or with `NOVACORE_ENABLE_SDL3=OFF`. Reconfigure the build directory, or delete it and configure again.

Smoke run:

```powershell
.\build\local-debug-no-deps\nemisis_game.exe --smoke-test
```

`--smoke-test` exits after a few frames and is registered as `nemisis_game_smoke` in CTest.

## Blender CLI

Blender CLI is the normal `blender.exe`; there is no separate CLI package.

Recommended Windows path:

1. Download Blender from `https://www.blender.org/download/`.
2. Install it or unpack the zip.
3. Find `blender.exe`, usually under `C:\Program Files\Blender Foundation\Blender <version>\blender.exe`.
4. Add that folder to the user PATH, or pass the full path to the helper script.
5. Verify:

```powershell
blender --version
```

Run Nemisis dev primitive generation:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all
```

Or with an explicit path:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only target -BlenderPath "C:\Program Files\Blender Foundation\Blender 4.5\blender.exe"
```

Current local Blender path used by Codex:

```powershell
.\tools\blender\run_make_dev_primitives.ps1 -Only all -BlenderPath "F:\Program Files\Blender Foundation\Blender 5.1\blender.exe"
```

Direct background mode:

```powershell
blender --background --python tools/blender/make_dev_primitives.py -- --only all
```

Blender's official command-line manual documents `--background`, `--python`, and the `--` separator for script arguments.

## Current Environment Notes

In the current Codex shell:

- CMake is available.
- Ninja is not in PATH by default, but CLion's bundled Ninja can be used by prepending its directory to PATH.
- MSVC `cl` is not in PATH.
- `g++` is not in PATH.
- Visual Studio Build Tools are not visible to CMake in this shell.
- Vulkan runtime is installed; smoke runs detect Vulkan 1.4.341 on `NVIDIA GeForce RTX 3070 Ti`.
- Vulkan SDK is not visible to CMake yet, so the current visible renderer remains SDL debug.
- Blender is installed at `F:\Program Files\Blender Foundation\Blender 5.1\blender.exe`, but not visible in PATH.

So the repo is prepared for IDE/toolchain pickup. Local builds have been verified through CLion's bundled MinGW/Ninja path, and Blender asset generation works when the explicit Blender path is passed to the helper.
