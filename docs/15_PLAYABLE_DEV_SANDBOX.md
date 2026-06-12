# 15 Playable Dev Sandbox

## Purpose

The playable dev sandbox is the first runtime loop for testing Nemisis features while the deeper engine backbone is still being built.

It is not a vertical slice yet. It is a developer playground for validating:

- Vulkan-first Dev Shooting Range boot.
- Vulkan-first Main Menu boot with Play, Gamemodes, Loadout, Character, Settings, Account, and Loading screens.
- Dev Shooting Range direct boot through `--dev-range`.
- Window event input.
- MKB and controller action mapping.
- Mouse and right-stick look.
- Camera-relative movement.
- Acceleration/friction based fixed-tick player movement.
- Slide duration, slide steering, dash duration, air drag, and slide-jump momentum.
- Smoothed FPS camera rig with FOV kick, head bob, recoil view offsets, and weapon sway.
- Weapon fire, cooldown, ADS blend, recoil recovery, reload progress, ammo, burst tracking, movement spread, and dry fire.
- Deterministic shot traces with seed, range, direction, spread, runtime recoil, and damage.
- Debug target hits, damage, health, elimination, and respawn.
- Pending command queue metadata plus loopback server acknowledgement.
- Game asset catalog load and dev-sandbox preload request setup.
- Relative mouse mode activation while the Dev Shooting Range is active.
- Multi-page on-screen debug telemetry for gameplay, network, and asset/render state.
- `UiCanvas` command bridge for menu/debug/HUD primitives before the final Vulkan-native UI backend.
- UI image placeholder commands backed by committed SVG/PNG-style UI art paths.
- Live settings for mouse/controller sensitivity, ADS look scaling, HUD scale, damage-number visibility, aim-assist flags, and debug world-line visibility.
- Six-slot attachment loadouts with effective weapon summaries for ADS, recoil, spread, reload, range, view kick, magazine size, and mobility.
- Account/Profile stats for K/D, win rate, best weapon, best operator, and accuracy.
- Persisted user settings/loadout snapshot under `configs/user/nemisis_user.json` after live runtime edits.
- Deterministic greybox world data for the first shooting range.
- Visible in-world Vulkan 3D range with player spawn, target lane, cover, ramps, walls, GLB props, and weapon proxies.
- Active first-person weapon and arms rendering driven by camera/view, ADS, weapon sway, recoil, and current loadout.
- Dev Range session scoring for shots fired, hits, eliminations, damage dealt, accuracy, current streak, best streak, range resets, target respawn, and session event text.
- Four-lane Dev Target Range with nearest-target hit selection, active lane tracking, per-lane respawn timers, and alive/down counts.
- HUD health panel backed by `PlayerHealthComponent`.
- Configurable Dev Range render tuning for lighting, FOV, clip planes, and world debug line visibility.
- World-space Vulkan debug lines for aim rays and KCC ground normals.
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
- Plain `nemisis_game.exe` now starts the Vulkan Main Menu.
- `nemisis_game.exe --dev-range` starts the Vulkan Dev Range directly.
- The smoke run now reports `Launch profile: renderer=vulkan require_vulkan=true start_screen=menu lock_dev_range=false`.
- Vulkan SDK is now visible at `F:\VulkanSDK\1.4.350.0`.
- `nemisis_game --sdl-debug` starts the old SDL debug menu path explicitly.
- `nemisis_game --sdl-debug` is the current way to inspect the old SDL debug path.
- `nemisis_game --vulkan-smoke-test` still validates Vulkan without auto-entering the Dev Range.
- `nemisis_game --vulkan-dev-range-smoke-test` remains a named explicit version of the default 3D smoke.
- Generated A0 and prototype-pack assets are registered as NovaCore renderer mesh resources and rendered in-world through the Vulkan GLB mesh path.
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
- `P`: reset Dev Shooting Range.

Controller:

- `LeftStick`: move.
- `RightStick`: look.
- `A`: jump, double jump, mantle request.
- `B`: dash and slide placeholder.
- `LeftStickPress`: sprint and tactical sprint placeholder.
- `RightTrigger`: fire.
- `LeftTrigger`: ADS.
- `X`: reload.
- `Y`: reset Dev Shooting Range.

Menu:

- `1`: Dev Shooting Range.
- `2`: Team Deathmatch placeholder.
- `3`: Control placeholder.
- `Up/Down`: move menu selection.
- `Left/Right`: adjust selected Loadout/Settings values.
- `Q/E`: switch top-level menu tabs.
- `Enter`: load selected menu item.
- `Esc`: return to main menu.
- `F1`: toggle debug overlay.
- `Tab`: cycle debug overlay pages.
- Controller `A`: confirm.
- Controller `B`: back.
- Controller D-pad up/down: move menu selection.
- Controller D-pad left/right: adjust selected Loadout/Settings values.
- Controller shoulders: switch top-level menu tabs.
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
- Active attachment/effective weapon state through HUD and loading UI.
- Ammo.
- Shot index.
- ADS alpha.
- Runtime recoil offsets.
- Reload state.
- Fire/dry-fire result.
- Pending command count.
- Sent command packet count.
- Received acknowledgement count.
- Last server-acknowledged tick.
- Shot trace seed, range, and direction when a shot fires.
- Player health and down-state sample.
- Dev Range score, accuracy, streak, best streak, damage dealt, and reset count.
- Dev Target Range alive/down counts and active lane.
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

- Gameplay: screen, movement mode, tick, input device, position, collision state, player HP, range eliminations, accuracy, and streak.
- Network: command packet counters, acknowledgement counters, pending command count, and last acknowledged tick.
- Assets: renderer backend, Vulkan runtime summary, queued asset count, CPU/GPU mesh-resource residency, upload queue length, failed/deferred counts, primitive count, vertex count, and index count.

The Dev Shooting Range HUD currently shows:

- Weapon/ammo state and active attachment build.
- Player health progress and current HP.
- Range eliminations, accuracy, current streak, and best streak.
- Target lane alive/down count and active lane name.
- Target HP and target respawn timer.
- Short session event text for hits, eliminations, reloads, and resets.
- `P / Y` reset hint.

The Dev Shooting Range screen also draws a greybox range map:

- Floor bounds, walls, cover, ramps, low step, mid ledge, spawn points, range markers, and target marker.
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
- The compiled Vulkan backend can create a window swapchain, depth buffer, world box pipeline, world line pipeline, world mesh pipeline, renderer-owned mesh resources, upload queued indexed GLB draws, and deferred GPU mesh destruction through the default launch profile.
- The world is represented by deterministic greybox data, Vulkan world boxes, uploaded GLB meshes, first-person proxy meshes, and a 3D aim marker.
- First-person weapon rendering now uses the active loadout asset instead of a fixed independent debug weapon.
- Settings and loadout edits are persisted through the user settings snapshot after live menu changes.
- Dev Range reset restores player movement, transform, camera view, health, weapon runtime, command queue, all target lanes, and session feedback.
- Dev Range render composition is isolated in `DevRangeRenderSceneBuilder`, with `GameApp` only collecting player state and orchestrating frame flow.
- Current collision supports floor grounding, bounds, AABB blockers, walkable ramp height sampling, low-step handling, ground normals, and ledges that block until mantle exists.
- Asset ids, preload requests, generated `.glb` exports, metadata, GLB scene-info imports, CPU mesh extraction, renderer-owned mesh handles, Vulkan upload queues, and indexed draw submission exist for the current dev assets.
- Vulkan Dev Range smoke currently submits 29 world boxes, 19 mesh instances, and at least one debug line.
- Relative mouse mode is requested for the dev range; mouse/controller sensitivity is data-driven, while cursor policy persistence and raw-input options are still pending.
- Debug target hit resolution is a focused sphere test, not full scene collision.
- The command bridge is loopback only; real UDP transport, prediction/reconciliation, and remote snapshots are not implemented yet.
- UI currently reaches Vulkan through the debug primitive bridge; the final SDF/MSDF text and vector path is still pending.

## Next Dev Sandbox Upgrades

- Recoil/ADS debug HUD widgets fed from `WeaponRuntimeState` and `PlayerCameraRig`.
- Timed range drills, measured TTK panels, recoil-control scoring, and lane-specific score breakdowns.
- Player damage sources, down-state, and respawn flow driven by authoritative hit events.
- Full KCC collision against greybox floors, walls, cover, ramps, and ledges.
- Renderer-owned resource handles, upload queues, and deferred destruction for current GLB meshes.
- Real UI text rendering after the SDL debug text path is replaced by the custom UI renderer.
- Real client/server packet transport after the loopback bridge is stable.
