# 15 Playable Dev Sandbox

## Purpose

The playable dev sandbox is the first runtime loop for testing Nemisis features while the deeper engine backbone is still being built.

It is not a vertical slice yet. It is a developer playground for validating:

- Barebones main menu and mode selection.
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

## Run

When the local C++ toolchain exists:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
.\build\windows-msvc-debug\Debug\nemisis_game.exe
```

Current environment note:

- CMake is available.
- `windows-msvc-debug` no longer requires Ninja or vcpkg.
- A C++ compiler is not currently visible in PATH or to CMake.
- Configure currently fails until Visual Studio Build Tools/MSVC are installed or exposed through a Developer shell.

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
- Controller `A`: confirm.
- Controller `B`: back.
- Controller D-pad up/down: move menu selection.

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
- Current screen.
- Asset preload queue size.

The renderer clear color also changes by state for early visual feedback:

- Dark idle/grounded.
- Orange when firing.
- Red when firing and hitting the debug target.
- Blue while reloading.
- Cyan when dashing.
- Green while sliding.
- Purple while airborne.

## Current Limits

- Renderer has SDL debug visuals when SDL3 is available, but the Vulkan renderer is still a placeholder.
- There is no 3D world mesh or general collision yet.
- Asset ids and preload requests exist, but glTF import/GPU mesh handles are not implemented yet.
- Mouse look exists, but cursor capture/raw mouse mode is not implemented yet.
- Debug target hit resolution is a focused sphere test, not full scene collision.
- The command bridge is loopback only; real UDP transport, prediction/reconciliation, and remote snapshots are not implemented yet.

## Next Dev Sandbox Upgrades

- Raw mouse/cursor capture.
- Config-loaded sensitivity and response curves.
- More debug targets and measured TTK tests.
- On-screen debug HUD once UI text rendering exists.
- Real client/server packet transport after the loopback bridge is stable.
