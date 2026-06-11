# 15 Playable Dev Sandbox

## Purpose

The playable dev sandbox is the first runtime loop for testing Nemisis features while the deeper engine backbone is still being built.

It is not a vertical slice yet. It is a developer playground for validating:

- Vulkan-first Dev Shooting Range boot.
- Barebones main menu and mode selection through the explicit legacy/debug path.
- Window event input.
- MKB and controller action mapping.
- Mouse and right-stick look.
- Camera-relative movement.
- Fixed-tick player movement.
- Weapon fire, cooldown, reload, ammo, and dry fire.
- Deterministic shot traces with seed, range, direction, spread, and damage.
- Debug target hits, damage, health, elimination, and respawn.
- Pending command queue metadata plus loopback server acknowledgement.
- Game asset catalog load and dev-sandbox preload request setup.
- Relative mouse mode activation while the Dev Shooting Range is active.
- Multi-page on-screen debug telemetry for gameplay, network, and asset/render state.
- Deterministic greybox world data for the first shooting range.
- Visible in-world Vulkan 3D range with player spawn, target lane, cover, ramps, walls, GLB props, and weapon proxies.
- Budgeted A0 environment GLB wireframe preview drawn from extracted CPU mesh data.

## Run

When the local C++ toolchain exists:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
.\build\windows-msvc-debug\Debug\nemisis_game.exe
```

Current environment note:

- `cmake-build-debug` has been verified with CLion's bundled MinGW/Ninja.
- NovaCore fetches SDL3 automatically when no installed SDL3 package is found.
- Plain `nemisis_game.exe` now starts the Vulkan Dev Range directly.
- The smoke run now reports `Launch profile: renderer=vulkan require_vulkan=true start_screen=dev_range lock_dev_range=true`.
- Vulkan SDK is now visible at `F:\VulkanSDK\1.4.350.0`.
- `nemisis_game --sdl-debug` starts the old SDL debug menu path explicitly.
- `nemisis_game --menu --sdl-debug` is the current way to inspect the old menu/debug UI.
- `nemisis_game --vulkan-smoke-test` still validates Vulkan without auto-entering the Dev Range.
- `nemisis_game --vulkan-dev-range-smoke-test` remains a named explicit version of the default 3D smoke.
- Generated A0 and prototype-pack assets are rendered in-world through NovaCore's Vulkan GLB mesh path.
- The generated A0 proxy metadata, GLB scene info, and CPU mesh data are now loaded at startup and registered into NovaCore mesh handles.

## Controls

MKB:

- `WASD`: move.
- `Mouse`: look.
- `Space`: jump, double jump, mantle request.
- `LeftAlt`: dash.
- `C`: slide.
- `LeftShift`: sprint and tactical sprint placeholder.
- `MouseLeft`: fire.
- `MouseRight`: ADS.
- `R`: reload.

Controller:

- `LeftStick`: move.
- `RightStick`: look.
- `A`: jump, double jump, mantle request.
- `B`: dash and slide placeholder.
- `LeftStickPress`: sprint and tactical sprint placeholder.
- `RightTrigger`: fire.
- `LeftTrigger`: ADS.
- `X`: reload.

Menu:

- `1`: Dev Shooting Range.
- `2`: Team Deathmatch placeholder.
- `3`: Control placeholder.
- `Up/Down`: move menu selection.
- `Enter`: load selected menu item.
- `Esc`: return to main menu.
- `F1`: toggle debug overlay.
- `Tab`: cycle debug overlay pages.
- Controller `A`: confirm.
- Controller `B`: back.
- Controller D-pad up/down: move menu selection.
- Controller Start/Menu: cycle debug overlay page when the overlay is visible.

## Telemetry

The sandbox logs every 0.5 seconds through NovaCore logging:

- Tick.
- Movement mode.
- Input device.
- Move vector.
- Position and velocity.
- View-relative aiming state through shot trace direction.
- Weapon id.
- Ammo.
- Shot index.
- Reload state.
- Fire/dry-fire result.
- Pending command count.
- Sent command packet count.
- Received acknowledgement count.
- Last server-acknowledged tick.
- Shot trace seed, range, and direction when a shot fires.
- Debug target health.
- Debug target hit count.
- Hit/elimination result.
- Renderer backend.
- Vulkan runtime/device summary.
- Current screen.
- Asset preload queue size.
- Active debug page.
- Dev mesh asset readiness.
- Greybox collision hit/block state.

The on-screen debug overlay currently has these pages:

- Gameplay: screen, movement mode, tick, input device, position, and collision state.
- Network: command packet counters, acknowledgement counters, pending command count, and last acknowledged tick.
- Assets: renderer backend, Vulkan runtime summary, queued asset count, required mesh handles, extracted asset count, primitive count, vertex count, and index count.

The Dev Shooting Range screen also draws a greybox range map:

- Floor bounds, walls, cover, ramps, spawn points, range markers, and target marker.
- A budgeted wireframe overlay from the imported A0 environment GLB.
- Local player square and current yaw direction.
- Line from player to the current target lane.

The renderer clear color also changes by state for early visual feedback:

- Dark idle/grounded.
- Orange when firing.
- Red when firing and hitting the debug target.
- Blue while reloading.
- Cyan when dashing.
- Green while sliding.
- Purple while airborne.

## Current Limits

- Renderer defaults to Vulkan 3D in normal game launches.
- SDL debug visuals are a legacy fallback/debug path behind `--sdl-debug`.
- The compiled Vulkan backend can create a window swapchain, depth buffer, world box pipeline, world mesh pipeline, and indexed GLB mesh draws through the default launch profile.
- The world is represented by deterministic greybox data, Vulkan world boxes, uploaded GLB meshes, first-person proxy meshes, and a 3D aim marker.
- Current collision is a first capsule/AABB-style greybox resolver, not the final KCC with slope normals, step height, mantle probes, or ramp behavior.
- Asset ids, preload requests, generated `.glb` exports, metadata, GLB scene-info imports, CPU mesh extraction, and mesh handles exist, but GPU upload/draw submission is not implemented yet.
- Relative mouse mode is requested for the dev range, but sensitivity, cursor policy settings, and raw input config are not data-driven yet.
- Debug target hit resolution is a focused sphere test, not full scene collision.
- The command bridge is loopback only; real UDP transport, prediction/reconciliation, and remote snapshots are not implemented yet.

## Next Dev Sandbox Upgrades

- Config-loaded sensitivity and response curves.
- HUD health/ammo panels backed by player health and weapon state.
- More debug targets and measured TTK tests.
- Full KCC collision against greybox floors, walls, cover, ramps, and ledges.
- Renderer-owned resource handles, upload queues, and deferred destruction for current GLB meshes.
- Real UI text rendering after the SDL debug text path is replaced by the custom UI renderer.
- Real client/server packet transport after the loopback bridge is stable.
