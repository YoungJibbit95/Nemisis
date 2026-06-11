# 19 Project Kanban

This file mirrors the active work while GitHub Projects write access is blocked by the missing `project` token scope.

## Done

- [x] Split NovaCore engine and Nemisis game repositories.
- [x] Add C++23 CMake skeletons.
- [x] Add fixed-tick runtime loop.
- [x] Add custom ECS foundation.
- [x] Add window/input snapshot bridge.
- [x] Add MKB and controller gameplay input.
- [x] Add transient mouse look axes.
- [x] Add movement command simulation foundation.
- [x] Add AR/SMG/shotgun/sidearm weapon data and simulation.
- [x] Add local player entity composition.
- [x] Add command queue and loopback command acknowledgement.
- [x] Add first debug target, shot traces, and target damage.
- [x] Add NovaCore asset manifest, registry, and streaming queue backbone.
- [x] Add Nemisis game asset catalog.
- [x] Add Blender asset plan, agent briefs, and generator script.
- [x] Fix `windows-msvc-debug` so it no longer depends on Ninja or an unset `VCPKG_ROOT`.
- [x] Add SDL debug renderer primitives and bitmap debug text.
- [x] Add barebones main menu, mode selection, dev shooting range entry, and debug overlay.
- [x] Add NovaCore packet bitstream usage for Nemisis command/ack payloads.
- [x] Add relative mouse request flow for active dev-range gameplay.
- [x] Add player health/damage helpers and measured TTK tests.
- [x] Expand debug UI into Gameplay, Network, and Assets pages.
- [x] Fix visible dev launches by auto-fetching SDL3 when vcpkg/package SDL3 is missing.
- [x] Copy MinGW runtime DLLs beside executable targets for direct shell runs.
- [x] Add NovaCore SDL debug line primitives for world/map overlays.
- [x] Add Nemisis `GreyboxWorld` data for the first Dev Shooting Range.
- [x] Draw a visible top-down greybox range with player, view direction, target, cover, ramps, and range markers.
- [x] Spawn the local player from the greybox world definition.
- [x] Generate A0 Blender source/export assets for target, player proxy, first-person arms, soldier proxy, AR, SMG, sidearm, and arena kit.
- [x] Add NovaCore glTF metadata validation and mesh-handle placeholders.
- [x] Bind generated A0 Nemisis assets into NovaCore `MeshCatalog` handles.
- [x] Add first greybox collision queries for bounds, cover/walls, and floor grounding.
- [x] Surface mesh/metadata readiness and collision state in debug telemetry/UI.
- [x] Add NovaCore GLB scene-info import and attach imported A0 scene totals to Nemisis mesh handles.
- [x] Surface imported asset and mesh/node totals in startup logs and the Assets debug page.
- [x] Add NovaCore CPU GLB mesh extraction for positions, optional normals/UVs, and indices.
- [x] Attach extracted A0 mesh data to Nemisis mesh handles and surface primitive/vertex/index totals.
- [x] Add NovaCore Vulkan runtime/device probe and surface detected GPU/runtime in logs/debug UI.
- [x] Add A0 environment GLB wireframe preview from extracted CPU mesh data.
- [x] Expose Vulkan SDK `F:\VulkanSDK\1.4.350.0` to the CMake build.
- [x] Add NovaCore compiled Vulkan backend with swapchain, render pass, framebuffers, sync, clear/present, and first graphics pipeline.
- [x] Add Vulkan shader compilation through `glslc`.
- [x] Add Nemisis `--vulkan`/`--vulkan-smoke-test` and CTest coverage.
- [x] Add NovaCore depth-tested Vulkan world box primitive path.
- [x] Feed Nemisis Dev Range greybox primitives into the Vulkan 3D renderer.
- [x] Add `--vulkan-dev-range-smoke-test` and CTest coverage.

## Doing

- [ ] Move from debug-map greybox to in-world mesh greybox rendering.
- [ ] Add renderer-side mesh upload and draw submission from extracted glTF buffers.

## Next Core

- [ ] Config-loaded sensitivity, cursor policy, and controller response curves.
- [ ] Packet simulation for latency, jitter, loss, duplication, and reorder.
- [ ] Vulkan swapchain resize/recreate and validation debug labels.
- [ ] Uploaded GLB mesh draw path with vertex/index buffers.
- [ ] Full KCC collision foundation for ramps, ledges, slope normals, slide/mantle tests.
- [ ] Shared server validation for greybox collision and movement corrections.

## Next Gameplay

- [ ] Dev shooting range mode shell with spawn/reset flow.
- [ ] TDM placeholder match lifecycle.
- [ ] Control placeholder objective lifecycle.
- [ ] More target dummies and range markers.
- [ ] First HUD widgets for ammo, health, target damage, and net status.
- [ ] Wire player health into real hit events, respawn, and server validation.

## Blocked

- [ ] No current engine-core blocker. GitHub Projects sync still depends on repository/project permissions for each board mutation.
