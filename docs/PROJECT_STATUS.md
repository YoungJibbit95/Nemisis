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
- Dev telemetry logs movement, weapon, pending command, and shot trace data while the game runs.
- Deterministic `ShotTraceResult` data now carries seed, direction, range, spread, and damage for future hitscan validation.
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Input command, weapon simulation, weapon shot, dev sandbox, player spawn, and command queue tests cover the newest gameplay bridge.

## Added In Latest Block

- Added `DevSandbox` for playable runtime telemetry and state-colored renderer clear feedback.
- `GameApp::onFrame` now updates game actions from `Window::inputSnapshot`.
- `GameApp::onFixedTick` now emits dev samples after command, movement, weapon, and network metadata updates.
- Added `WeaponShot` deterministic shot trace generation with seed, spread, recoil offsets, range, and damage.
- Weapon config now includes range, spread, and recoil tuning values.
- Added `nemisis_dev_sandbox_tests` and `nemisis_weapon_shot_tests` CMake targets.
- Added `docs/15_PLAYABLE_DEV_SANDBOX.md`.

## Next Game Blocks

- Start client/server command packet serialization and server-authoritative acknowledgement handoff.
- Add player health/damage components for measured TTK tests.
- Add camera-relative movement and mouse-look snapshot support.
- Add a debug target dummy and first hitscan damage application.
