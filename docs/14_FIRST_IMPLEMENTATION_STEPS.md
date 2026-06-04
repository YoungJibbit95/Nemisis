# 14 First Implementation Steps

## Step 1 - Repo Skeleton

Create:

- `CMakeLists.txt`
- `CMakePresets.json`
- NovaCore dependency lookup.
- `game/include/nemisis`
- `game/src`
- `assets`
- `configs`
- `docs`

Acceptance:

- Root contains all expected game folders.
- Build files describe the game target and its NovaCore dependency.

## Step 2 - Game Runtime

Implement:

- Thin `main.cpp`.
- `nemisis::game::GameApp`.
- NovaCore runtime loop integration.
- Sandbox scene bootstrap.

Acceptance:

- Game uses NovaCore instead of copying engine code.
- Game-specific code lives outside `main.cpp`.

## Step 3 - Game ECS Usage

Implement:

- Camera entity creation.
- Game-owned system boundaries.
- Future player/weapon/mode system folders.

Acceptance:

- Game creates entities through NovaCore APIs.
- No NovaCore internals are copied into Nemisis.

## Step 4 - Platform/Input

Implement:

- SDL3 window backend when available.
- Headless fallback.
- Input system.
- MKB activity tracking.
- Controller connected/disconnected tracking.

Acceptance:

- Game opens a window when SDL3 is installed.
- Game can run briefly in fallback mode.
- Input system reports device availability.

## Step 5 - Renderer Stub

Implement:

- Renderer API.
- Null fallback.
- Vulkan placeholder backend name.
- Clear color frame settings.

Acceptance:

- Game can call begin/end frame.
- Renderer state logs backend used.

## Step 6 - Server/Net Usage

Implement:

- Direct-connect UI hooks later.
- Client session usage through NovaCore.
- Game-mode replication requirements.

Acceptance:

- Dedicated server runtime defaults remain in NovaCore.
- Nemisis only owns game rules and match requirements.

## Step 7 - Verification

Run when tools exist:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

If CMake/MSVC/Ninja/Vulkan SDK are missing, verification is limited to file inspection.







