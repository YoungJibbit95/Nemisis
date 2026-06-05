# 15 Playable Dev Sandbox

## Purpose

The playable dev sandbox is the first runtime loop for testing Nemisis features while the deeper engine backbone is still being built.

It is not a vertical slice yet. It is a developer playground for validating:

- Window event input.
- MKB and controller action mapping.
- Fixed-tick player movement.
- Weapon fire, cooldown, reload, ammo, and dry fire.
- Deterministic shot traces with seed, range, direction, spread, and damage.
- Pending command queue metadata for future server acknowledgement.

## Run

When the local C++ toolchain exists:

```powershell
cmake --preset local-debug-no-deps
cmake --build --preset local-debug-no-deps
.\build\local-debug-no-deps\nemisis_game.exe
```

Current environment note:

- CMake is available.
- Ninja is not currently in PATH.
- A C++ compiler is not currently in PATH.
- Configure currently fails until Ninja and a compiler are installed or exposed.

## Controls

MKB:

- `WASD`: move.
- `Space`: jump, double jump, mantle request.
- `LeftAlt`: dash.
- `C`: slide.
- `LeftShift`: sprint and tactical sprint placeholder.
- `MouseLeft`: fire.
- `MouseRight`: ADS.
- `R`: reload.

Controller:

- `LeftStick`: move.
- `A`: jump, double jump, mantle request.
- `B`: dash and slide placeholder.
- `LeftStickPress`: sprint and tactical sprint placeholder.
- `RightTrigger`: fire.
- `LeftTrigger`: ADS.
- `X`: reload.

## Telemetry

The sandbox logs every 0.5 seconds through NovaCore logging:

- Tick.
- Movement mode.
- Input device.
- Move vector.
- Position and velocity.
- Weapon id.
- Ammo.
- Shot index.
- Reload state.
- Fire/dry-fire result.
- Pending command count.
- Shot trace seed, range, and direction when a shot fires.

The renderer clear color also changes by state for early visual feedback:

- Dark idle/grounded.
- Orange when firing.
- Blue while reloading.
- Cyan when dashing.
- Green while sliding.
- Purple while airborne.

## Current Limits

- Renderer is still a Vulkan/null placeholder.
- There is no 3D world, collision, enemies, target dummies, or hit resolution yet.
- Mouse look is not implemented yet.
- Shot trace uses forward direction until camera aim exists.
- The command queue is local only; packet serialization and server acknowledgements are next.

## Next Dev Sandbox Upgrades

- Mouse look and camera-relative movement.
- Simple debug target dummy with health.
- Hitscan overlap against debug target primitives.
- On-screen debug HUD once UI text rendering exists.
- Server loopback command acknowledgement.
