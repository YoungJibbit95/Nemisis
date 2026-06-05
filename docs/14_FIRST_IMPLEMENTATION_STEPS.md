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

## Step 10 - Playable Dev Sandbox And Shot Trace

Implement:

- Feed Nemisis `InputActionMap` from NovaCore `Window::inputSnapshot`.
- Add `DevSandbox` telemetry for live gameplay state while the app runs.
- Log controls and runtime state for movement, weapon, pending commands, and shot traces.
- Add state-colored renderer clear feedback for early visual iteration.
- Add deterministic shot trace generation with seed, shot index, spread, recoil, range, and damage.
- Extend weapon config with range, spread, and recoil tuning.

Acceptance:

- `nemisis_game` reacts to MKB/controller input through the action map.
- Running the game logs playable dev sandbox controls and telemetry.
- Firing emits deterministic shot trace data.
- CMake declares `nemisis_dev_sandbox_tests` and `nemisis_weapon_shot_tests`.
- `docs/15_PLAYABLE_DEV_SANDBOX.md` documents controls, run flow, and current limits.

## Step 11 - Shooter Test Field

Implement:

- Add mouse and right-stick look actions.
- Add `PlayerViewComponent` for yaw/pitch.
- Apply look before fixed-tick movement.
- Transform movement commands relative to player view.
- Aim shot traces along view forward.
- Add a debug target dummy with sphere hits, damage, health, elimination, and reset.
- Extend dev sandbox telemetry with target health, hits, hit result, and elimination state.

Acceptance:

- MKB and controller can steer look through `PlayerInputCommand::look`.
- Forward movement follows current yaw.
- Firing forward at the default target applies damage.
- Dev telemetry reports target health and hit state.
- CMake declares `nemisis_player_view_tests` and `nemisis_debug_target_tests`.

## Step 12 - Command Loopback Bridge

Implement:

- Add `CommandPacket` and `CommandAck` payloads for local client command traffic.
- Serialize and deserialize `PlayerInputCommand` data into deterministic little-endian byte payloads.
- Add a loopback bridge that sends pending local commands through a NovaCore loopback channel.
- Process the packet through a server-side acknowledgement handoff.
- Trim acknowledged commands from `PlayerCommandQueue`.
- Expose sent packet, received packet, acknowledgement, and ack tick telemetry in the dev sandbox.

Acceptance:

- `nemisis_game` includes command message and loopback bridge sources.
- `GameApp::onFixedTick` runs send, server process, and client ack handling every fixed tick.
- `PlayerNetworkComponent::lastServerAcknowledgedTick` advances when commands are acknowledged.
- Dev telemetry includes command packet and ack counters.
- CMake declares `nemisis_command_messages_tests` and `nemisis_loopback_command_bridge_tests`.

## Step 13 - Asset Production Handoff

Implement:

- Add a game-side asset production plan for original Blender-to-glTF content.
- Define coordinate, scale, naming, socket, collision, material, LOD, metadata, and export rules.
- Add first Blender-agent briefs for target dummy, AR, movement arena kit, first-person arms, and third-person player proxy.
- Add source/export asset directory documentation.
- Add the first Blender-only generator script for dev primitive source/export files.

Acceptance:

- `docs/16_ASSET_PRODUCTION_PLAN.md` exists and can be handed to an asset agent.
- `assets/briefs/BLENDER_AGENT_BRIEFS.md` contains concrete first asset tasks.
- Asset README files point to the current production plan.
- `tools/blender/make_dev_primitives.py` is ready to run when Blender is installed.
- No unclear-license binary assets are added.

## Step 14 - Runtime Asset Catalog And IDE Runability

Implement:

- Add `configs/assets/nemisis_assets.json` as the game-owned runtime asset catalog.
- Add `GameAssetCatalog` to load the catalog through NovaCore's asset manifest/registry backbone.
- Queue dev-sandbox preload ids through a NovaCore streaming zone.
- Add `nemisis_asset_catalog_tests`.
- Add a Visual Studio 2022 no-deps CMake preset for IDE bootstrapping.
- Copy runtime config/asset folders beside `nemisis_game` after build.
- Add `nemisis_game --smoke-test` as a short runtime launch mode.
- Add Blender/Codex agent instructions, job template, and helper runner script.
- Add an IDE/toolchain/Blender CLI runbook.

Acceptance:

- `GameApp::onStartup` loads the asset catalog and queues dev preload requests.
- Asset ids for weapon, target dummy, arena kit, first-person arms, and player proxy are declared.
- `configs/assets/nemisis_assets.json` parses as JSON.
- Build output receives `configs/` and `assets/` when `NEMISIS_COPY_RUNTIME_DATA` is enabled.
- CTest includes `nemisis_game_smoke`.
- `tools/blender/run_make_dev_primitives.ps1` can run the generator once Blender is available.
- `docs/17_IDE_TOOLCHAIN_AND_BLENDER_RUNBOOK.md` documents IDE and Blender CLI setup.

## Step 15 - Debug Range Health, TTK, And Runtime Pages

Implement:

- Move command packet byte IO onto NovaCore's shared `PacketWriter` and `PacketReader`.
- Add `PlayerHealthComponent` plus reset, alive, and damage application helpers.
- Add deterministic damage result output for future authoritative server damage events.
- Add weapon metrics for range band, damage at distance, shots-to-eliminate, and TTK estimate.
- Add AR/SMG 150 HP TTK tests against the current BO-like baseline.
- Add relative mouse mode request flow while Dev Shooting Range gameplay is active.
- Expand debug overlay into Gameplay, Network, and Assets/Render pages.

Acceptance:

- `nemisis_game` includes player health and weapon metrics sources.
- Local player spawning attaches a health component.
- `nemisis_player_health_tests` and `nemisis_weapon_metrics_tests` are declared in CMake.
- `GameMenu` can toggle debug visibility and cycle debug pages.
- `GameApp` asks NovaCore for relative mouse mode only when gameplay is active.
- Docs and kanban mark the new runtime/debug/TTK slice as complete.

## Step 16 - Greybox Runtime And A0 Asset Generation

Implement:

- Add a game-owned `GreyboxWorld` data model for the Dev Shooting Range.
- Define floor, walls, cover, ramps, spawns, range markers, and target lane as stable primitive ids.
- Spawn the local player from the greybox world spawn.
- Feed current view data into dev sandbox telemetry so UI/debug drawing can visualize player direction.
- Draw the first visible top-down greybox range through NovaCore SDL debug rectangles and lines.
- Expand Blender automation to generate A0 target, character proxy, arms, soldier, weapon, and arena-kit assets.
- Generate `.blend`, `.glb`, and metadata files through Blender CLI with an explicit `-BlenderPath`.
- Add greybox world tests.

Acceptance:

- Dev Shooting Range visually represents player, target, walls, cover, ramps, range markers, and aim direction.
- Local player starts at the greybox spawn.
- A0 Blender source/export files exist under `assets/source/blender` and `assets/export/gltf`.
- `configs/assets/nemisis_assets.json` references the generated A0 runtime ids.
- CTest includes `nemisis_greybox_world_tests`.
- Docs and kanban define Greybox Phase 0 as active and identify the next mesh/KCC steps.

## Step 17 - Metadata Mesh Handles And First Greybox Collision

Implement:

- Add NovaCore glTF metadata loading and validation.
- Add NovaCore renderable asset validation and placeholder `MeshHandle`/`MeshCatalog` registration.
- Add Nemisis `DevAssetBindings` for required A0 dev assets.
- Bind generated A0 asset metadata into mesh handles at startup.
- Report mesh/metadata readiness in logs and the Assets debug page.
- Add first greybox player collision resolution for world bounds, floor grounding, walls, and cover.
- Feed collision hit/block telemetry into the Dev Sandbox.
- Add asset-binding and greybox-collision tests.

Acceptance:

- `nemisis_game --smoke-test` logs `Dev mesh assets ready: 8/8 metadata=8`.
- `nemisis_dev_asset_bindings_tests` validates required A0 asset bindings.
- `nemisis_greybox_collision_tests` validates bounds, cover pushout, and free spawn lane.
- NovaCore smoke tests validate glTF metadata and mesh handles.
- Docs and kanban move metadata/mesh-handle shim and first collision from Next to Done.







