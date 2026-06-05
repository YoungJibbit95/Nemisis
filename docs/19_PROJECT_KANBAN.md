# 19 Project Kanban

This file mirrors the active work while GitHub Projects access is blocked by the missing `read:project` token scope.

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

## Doing

- [ ] Move from debug-map greybox to in-world mesh greybox rendering.
- [ ] Keep renderer work moving toward real Vulkan swapchain/shaders.

## Next Core

- [ ] Config-loaded sensitivity, cursor policy, and controller response curves.
- [ ] Packet simulation for latency, jitter, loss, duplication, and reorder.
- [ ] glTF metadata/import shim and mesh handle placeholders.
- [ ] Renderer mesh/shader pipeline foundation.
- [ ] KCC collision foundation for ramps, ledges, and slide/mantle tests.
- [ ] Greybox primitive collision queries shared by movement and server validation.

## Next Gameplay

- [ ] Dev shooting range mode shell with spawn/reset flow.
- [ ] TDM placeholder match lifecycle.
- [ ] Control placeholder objective lifecycle.
- [ ] More target dummies and range markers.
- [ ] First HUD widgets for ammo, health, target damage, and net status.
- [ ] Wire player health into real hit events, respawn, and server validation.

## Blocked

- [ ] GitHub Projects/Kanban sync: blocked until GitHub CLI auth includes `read:project`.
