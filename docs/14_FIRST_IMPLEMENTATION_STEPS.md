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

## Step 8 - Gameplay Tick Bridge

Implement:

- Convert NovaCore `InputActionMap` states into `PlayerInputCommand`.
- Add MKB and controller default bindings for the first gameplay command path.
- Feed fixed-tick movement from real command data.
- Add weapon runtime state for the active local weapon.
- Simulate fire, cooldown, reload, dry fire, and shot index deterministically.
- Add focused input command and weapon simulation tests.

Acceptance:

- `nemisis_game` includes `InputCommandBuilder` and `WeaponSimulation`.
- `GameApp::onFixedTick` advances movement and active weapon state from the same command.
- Controller left-stick and trigger inputs reach `PlayerInputCommand`.
- Weapon config includes `reload_time`.
- CMake declares `nemisis_input_command_tests` and `nemisis_weapon_simulation_tests`.

## Step 9 - Player Entity And Command Queue

Implement:

- Add player ECS components for identity, local ownership, network metadata, and active loadout.
- Add local player spawn helper that creates a NovaCore entity with transform, movement state, and weapon runtime state.
- Move local `GameApp` movement and weapon runtime simulation onto the player entity.
- Add a tick-ordered `PlayerCommandQueue` for unacknowledged local commands.
- Update network metadata with last processed command tick and pending command count.
- Add player spawn and command queue tests.

Acceptance:

- `nemisis_game` includes player spawn and command queue sources.
- Local player state lives on a NovaCore entity.
- `GameApp::onFixedTick` pushes commands before simulation.
- Movement updates the player transform from component-backed movement state.
- CMake declares `nemisis_player_command_queue_tests` and `nemisis_player_spawn_tests`.







