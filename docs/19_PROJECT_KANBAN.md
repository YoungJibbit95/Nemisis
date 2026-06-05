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

## Doing

- [ ] Make the first visible test loop feel like an actual dev shooting range.
- [ ] Expand debug UI pages for input, movement, weapon, net, render, and assets.
- [ ] Keep renderer work moving toward real Vulkan swapchain/shaders.

## Next Core

- [ ] Raw mouse capture and cursor lock.
- [ ] Player health/damage components and measured TTK tests.
- [ ] Net bitstream writer/reader and packet simulation.
- [ ] glTF metadata/import shim and mesh handle placeholders.
- [ ] Renderer mesh/shader pipeline foundation.
- [ ] KCC collision foundation for ramps, ledges, and slide/mantle tests.

## Next Gameplay

- [ ] Dev shooting range mode shell with spawn/reset flow.
- [ ] TDM placeholder match lifecycle.
- [ ] Control placeholder objective lifecycle.
- [ ] More target dummies and range markers.
- [ ] First HUD widgets for ammo, health, target damage, and net status.

## Blocked

- [ ] GitHub Projects/Kanban sync: blocked until GitHub CLI auth includes `read:project`.
- [ ] Local compile/run verification: blocked until MSVC Build Tools or another C++ compiler is visible to CMake.
- [ ] Blender asset generation: blocked until Blender CLI is installed or provided through a tool/plugin.
