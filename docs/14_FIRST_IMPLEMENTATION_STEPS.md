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

## Step 18 - GLB Scene Info Import For A0 Assets

Implement:

- Add NovaCore GLB/text glTF scene-info loading for container type, JSON/BIN byte sizes, and scene object counts.
- Attach imported scene info to `MeshCatalog` entries alongside generated metadata.
- Load generated A0 `.glb` scene info inside Nemisis `DevAssetBindings`.
- Track imported asset count plus total imported mesh, node, material, and binary byte counts.
- Surface imported A0 scene totals in startup logs and the Assets debug page.
- Extend asset-binding tests with deterministic tiny GLB fixtures.

Acceptance:

- NovaCore smoke tests validate GLB scene-info loading and imported mesh-catalog storage.
- `nemisis_game --smoke-test` logs `Dev mesh assets ready: 8/8 metadata=8 imported=8`.
- `nemisis_dev_asset_bindings_tests` verifies imported scene info is stored on required A0 mesh handles.
- The Assets debug page reports imported asset count and total mesh/node scene counts.

## Step 19 - CPU GLB Mesh Extraction For A0 Assets

Implement:

- Add NovaCore `GltfMeshData` for CPU-side primitive, position, normal, texcoord, and index storage.
- Read GLB accessors and buffer views with byte offsets, byte strides, FLOAT vector attributes, and unsigned index formats.
- Store imported CPU mesh data on `MeshCatalog` entries beside sidecar metadata and scene info.
- Update Nemisis `DevAssetBindings` to extract CPU mesh data for all required A0 assets.
- Track extracted asset count plus total primitive, vertex, index, and binary byte counts.
- Surface extracted mesh totals in startup logs and the Assets debug page.
- Extend engine and game tests with real tiny triangle GLB fixtures.

Acceptance:

- NovaCore smoke tests validate a triangle GLB imports with 1 primitive, 3 vertices, and 3 indices.
- `nemisis_game --smoke-test` logs `primitives=152 vertices=21368 indices=32304` for the current generated A0 set.
- `nemisis_dev_asset_bindings_tests` verifies extracted mesh data is stored on required A0 mesh handles.
- Docs and kanban move CPU GLB extraction from Doing to Done; renderer upload/draw remains next.

## Step 20 - Vulkan Runtime Probe And A0 Mesh Preview

Implement:

- Add a NovaCore SDK-free Vulkan runtime/device probe using dynamic loader entry points.
- Report Vulkan loader version, physical device name, and device type at renderer startup.
- Surface the Vulkan runtime summary in the Nemisis Assets debug page.
- Keep SDL debug renderer as the active visible path while the compiled Vulkan backend is still renderer-smoke-only.
- Draw a budgeted wireframe preview from extracted A0 environment GLB positions/indices on the Dev Shooting Range map.
- Update docs and kanban so Vulkan runtime detection is Done and compiled Vulkan backend work moves into the next renderer step.

Acceptance:

- `nemisis_game --smoke-test` logs Vulkan runtime/device detection for the local GPU.
- The Assets debug page shows renderer backend plus Vulkan runtime summary.
- Dev Shooting Range overlays an A0 GLB wireframe preview from CPU mesh data.
- NovaCore smoke tests cover Vulkan probe stability.

## Step 21 - Compiled Vulkan Backend And Shader Smoke

Implement:

- Expose the installed Vulkan SDK to CMake through `VULKAN_SDK=F:\VulkanSDK\1.4.350.0`.
- Add NovaCore `VulkanBackend` with SDL surface creation, instance/device/queue selection, swapchain, image views, render pass, framebuffers, command buffers, semaphores, fences, clear, present, and first debug draw.
- Add CMake shader compilation for `shaders/vulkan/debug_triangle.vert` and `.frag` through `glslc`.
- Add `RendererCreateInfo::preferVulkan` so tools/gameplay can opt into Vulkan without losing the SDL debug UI path.
- Add Nemisis `GameAppOptions`, `--vulkan`, and `--vulkan-smoke-test`.
- Register `nemisis_game_vulkan_smoke` in CTest.
- Keep normal `nemisis_game` on SDL debug rendering until Vulkan can draw the full menu/debug UI or mesh scene.

Acceptance:

- NovaCore standalone config/build/test passes with Vulkan SDK visible.
- Nemisis config/build/test passes with 20 tests.
- `nemisis_game --smoke-test` still logs `SDL debug renderer created`.
- `nemisis_game --vulkan-smoke-test` logs Vulkan 1.4.350, swapchain creation, first graphics pipeline creation, and active device `NVIDIA GeForce RTX 3070 Ti`.
- Generated SPIR-V files exist under the NovaCore build subdirectory, e.g. `cmake-build-codex-vulkan/Novacore-Engine/generated/novacore/shaders`.

## Step 22 - Vulkan 3D Greybox Primitive Path

Implement:

- Keep NovaCore's Vulkan backend private to engine implementation and expose only `Renderer` plus presentation data types.
- Add `RenderCamera3D` and `RenderBox3D` to the frame description.
- Add Vulkan world-box shaders compiled through `glslc`.
- Add a depth image/view, depth attachment, and depth-tested graphics pipeline.
- Draw greybox primitives through push constants as first 3D world geometry.
- Add Nemisis `--dev-range` and `--vulkan-dev-range-smoke-test`.
- Feed the Dev Shooting Range camera and greybox primitives into NovaCore's Vulkan world renderer.
- Register `nemisis_game_vulkan_dev_range_smoke` in CTest.

Acceptance:

- `nemisis_game --vulkan-dev-range-smoke-test` logs `Vulkan world box graphics pipeline created`.
- The same smoke logs `Vulkan world box draw submission active: boxes=16`.
- CTest passes with 21 Nemisis tests.
- NovaCore standalone smoke tests still pass.

## Step 23 - Vulkan GLB Mesh Upload And Prototype Asset Render

Implement:

- Add NovaCore `RenderMesh3D` frame submissions for asset id, transform, and color; Step 25 promotes the mesh payload from imported CPU data to stable renderer-owned handles.
- Add Vulkan `world_mesh` shaders with position/normal vertex input and simple normal-based shading.
- Add device-local vertex and index buffers with host-visible staging uploads through one-time command buffers.
- Cache uploaded GPU mesh assets by render asset id inside the Vulkan backend.
- Draw uploaded GLB primitives through indexed Vulkan draw calls in the same depth-tested world pass as greybox boxes.
- Generate a Blender prototype pack for SMG, humanoid, wall, floor, cover crate, ramp, and target stand.
- Add prototype-pack metadata and asset-catalog entries so `DevAssetBindings` imports them as required Dev Sandbox renderables.
- Submit A0 arena/weapon/character assets plus prototype-pack map/character/weapon assets from the Dev Range.

Acceptance:

- `nemisis_game --vulkan-dev-range-smoke-test` logs `Vulkan world mesh graphics pipeline created`.
- The same smoke logs `Dev mesh assets ready: 15/15 metadata=15 imported=15`.
- The same smoke uploads `env_test_arena_kit_01`, `wpn_ar_01`, `wpn_proto_smg_01`, and prototype map pieces.
- The same smoke logs `Vulkan world mesh draw submission active: meshes=12`.
- CTest passes with 21 Nemisis tests.
- NovaCore standalone smoke tests still pass.

## Step 24 - Vulkan 3D Default Launch Profile

Implement:

- Make `GameAppOptions` default to Vulkan, `require_vulkan=true`, auto-enter Dev Range, and lock the Dev Range during normal Vulkan play.
- Make plain `nemisis_game.exe` start the real 3D Vulkan Dev Range instead of the SDL debug menu.
- Make `nemisis_game --smoke-test` validate the same default Vulkan 3D profile.
- Add explicit legacy flags: `--sdl-debug`, `--no-vulkan`, and `--sdl-debug-smoke-test`.
- Keep `--menu` available for deliberate menu testing instead of accidental blank Vulkan UI states.
- Add a first-person arms proxy and 3D aim marker to the Vulkan Dev Range frame.
- Add NovaCore `RendererCreateInfo::requireVulkan` so SDL debug fallback can be disabled for the default profile.

Acceptance:

- `nemisis_game --smoke-test` logs `Launch profile: renderer=vulkan require_vulkan=true start_screen=dev_range lock_dev_range=true`.
- The same smoke logs `Vulkan world mesh draw submission active: meshes=13`.
- The same smoke logs `Vulkan world box draw submission active: boxes=21`.

## Step 25 - Renderer Mesh Resource Lifecycle And Dev Range Scene Builder

Implementation:

- Promote NovaCore mesh rendering from transient frame-owned CPU mesh pointers to renderer-owned `MeshResourceHandle` registrations.
- Add mesh resource registration, lookup, release, stats, Vulkan upload queue, resident/failed/pending tracking, and deferred GPU destruction.
- Keep `RenderMesh3D` frame submissions small: stable handle, asset id, transform, yaw, and debug tint.
- Register all required Dev Sandbox GLB assets once from Nemisis after renderer creation.
- Release registered Dev Sandbox resources on game shutdown.
- Move Dev Range 3D composition into `DevRangeRenderSceneBuilder` so `GameApp` only gathers player state and orchestrates frame flow.
- Add scene-builder stats for world boxes, mesh instances, skipped mesh handles, first-person meshes, and aim marker boxes.
- Update the Assets debug page with CPU/GPU mesh residency, upload queue, failed uploads, deferred destroys, and indexed primitive/vertex/index totals.
- Add `nemisis_dev_range_render_scene_tests`.

Acceptance:

- `novacore_smoke_tests` covers mesh resource rejection, duplicate registration, stats, release, slot reuse, and generation bumping.
- `ctest --test-dir cmake-build-codex-vulkan --output-on-failure` reports 22/22 Nemisis tests passing.
- `nemisis_game --vulkan-dev-range-smoke-test` logs `Renderer dev mesh resources registered: 15/15`.
- The same smoke logs `Vulkan mesh resident: chr_dev_arms_a`.
- The same smoke logs `Vulkan world mesh draw submission active: meshes=13`.
- The same smoke logs `Vulkan world box draw submission active: boxes=21`.

## Step 26 - Vulkan World Lines, Lighting Tuning, And KCC Ramp/Step Pass

Implementation:

- Add NovaCore `RenderWorldLighting` to `RenderFrameInfo`.
- Feed compact sun-direction plus ambient lighting through Vulkan push constants for world boxes and GLB meshes.
- Add NovaCore `RenderLine3D` and a Vulkan `world_line` pipeline for depth-tested in-world debug rays.
- Submit Dev Range aim rays and KCC ground-normal lines through the real Vulkan 3D path.
- Add `configs/render/dev_range_render.json` for Dev Range lighting, FOV, clip planes, and debug line visibility.
- Add `nemisis::render::DevRenderTuning` with config parsing, clamping, and hot reload through `GameApp`.
- Expand greybox collision with ramp height sampling, walkable ground normals, low-step handling, and ledge blocking.
- Add low-step and mid-ledge training pieces to the Dev Range greybox.
- Surface KCC ground surface, ramp/step state, and normals in dev telemetry/debug metrics.
- Add `nemisis_render_tuning_tests` and expand collision/scene-builder tests.

Acceptance:

- `novacore_smoke_tests` passes after shader/layout changes.
- `ctest --test-dir cmake-build-codex-vulkan --output-on-failure` reports 23/23 Nemisis tests passing.
- `nemisis_game --vulkan-dev-range-smoke-test` logs `Vulkan world line graphics pipeline created`.
- The same smoke logs `Vulkan world line draw submission active: lines=1`.
- The same smoke logs `Vulkan world box draw submission active: boxes=23`.
- `nemisis_game --sdl-debug-smoke-test` still reaches the explicit legacy SDL path for UI/debug testing.
- CTest passes with 21 Nemisis tests.
- NovaCore standalone smoke tests still pass.







