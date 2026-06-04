# 14 First Implementation Steps

## Step 1 - Repo Skeleton

Create:

- `CMakeLists.txt`
- `CMakePresets.json`
- `vcpkg.json`
- `engine/include`
- `engine/src`
- `game/src`
- `server/src`
- `tools`
- `assets`
- `configs`
- `shaders`
- `docs`

Acceptance:

- Root contains all expected folders.
- Build files describe engine, game, and server targets.

## Step 2 - Core Runtime

Implement:

- Logging.
- Assert macros.
- Fixed timestep application loop.
- Config placeholder.

Acceptance:

- Game and server can share runtime loop code.
- Server can run without window/render code.

## Step 3 - ECS M1

Implement:

- `EntityId`.
- Entity create/destroy.
- Liveness checks.
- Basic components.
- Minimal component storage.

Acceptance:

- Create/destroy works.
- Stale IDs fail liveness.
- Destroy removes components.

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

## Step 6 - Server/Net Skeleton

Implement:

- Server config.
- Fixed tick server loop.
- Client session placeholder.
- Loopback message queue.

Acceptance:

- Dedicated server executable starts.
- Server ticks at target tick rate.
- Loopback abstraction exists for later listen-server path.

## Step 7 - Verification

Run when tools exist:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

If CMake/MSVC/Ninja/Vulkan SDK are missing, verification is limited to file inspection.

