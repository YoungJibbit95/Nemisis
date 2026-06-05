# Nemisis Project Status

## Current Foundation

- Game repository consumes NovaCore through `Novacore::Engine`.
- `main.cpp` is thin and delegates to `nemisis::game::GameApp`.
- Game modules exist for input, player commands, movement, and weapons.
- Movement supports sprint, tactical sprint tuning, jump, double jump, dash, slide, and future wall-run values.
- Weapon registry supports AR, SMG, shotgun, and sidearm definitions.
- Input actions are translated into `PlayerInputCommand` for fixed-tick gameplay.
- Default input bindings cover MKB plus controller stick/buttons/triggers.
- Weapon runtime state supports ammo, shot index, fire cooldown, reload timer, and dry fire.
- Local player spawning creates a NovaCore ECS entity with identity, local-input, network, loadout, movement, and weapon runtime components.
- A tick-ordered `PlayerCommandQueue` keeps unacknowledged local commands for future server reconciliation.
- The playable dev sandbox updates actions from NovaCore window input snapshots.
- Dev telemetry logs movement, view, weapon, target, pending command, and shot trace data while the game runs.
- Mouse and right-stick look update player yaw/pitch.
- Movement is camera-relative through `PlayerViewComponent`.
- A debug target dummy supports first hitscan damage, health, hit count, elimination, and auto-respawn.
- Deterministic `ShotTraceResult` data now carries seed, direction, range, spread, and damage for future hitscan validation.
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Input command, weapon simulation, weapon shot, player view, debug target, dev sandbox, player spawn, and command queue tests cover the newest gameplay bridge.

## Added In Latest Block

- Added look actions for mouse delta and controller right stick.
- Added `PlayerViewComponent` and view helpers for yaw/pitch, forward vectors, and camera-relative movement.
- `GameApp::onFixedTick` now applies look before movement and fires shot traces along view forward.
- Added `DebugTarget` with sphere hit testing, damage, elimination, and reset behavior.
- Dev sandbox telemetry now reports target health, hits, hit result, and elimination state.
- Added `nemisis_player_view_tests` and `nemisis_debug_target_tests` CMake targets.

## Next Game Blocks

- Start client/server command packet serialization and server-authoritative acknowledgement handoff.
- Add player health/damage components and measured TTK tests.
- Add raw mouse capture/cursor lock and configurable sensitivity loading.
- Add server loopback acknowledgement for command queue trimming.
