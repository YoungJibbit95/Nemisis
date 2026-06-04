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
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Input command and weapon simulation tests cover the newest gameplay bridge.

## Added In Latest Block

- Added `InputCommandBuilder` to convert NovaCore `InputActionMap` states into `PlayerInputCommand`.
- Added default controller bindings for left-stick movement, trigger fire/ADS, jump, dash, slide, sprint, mantle, and reload.
- `GameApp::onFixedTick` now advances local movement from real input command data.
- Added `WeaponSimulation` with deterministic fire, cooldown, reload, dry-fire, and shot-index behavior.
- Weapon config now includes `reload_time` for the core prototype roster.
- `GameApp::onFixedTick` now advances the active weapon from command fire/reload input.
- Added `nemisis_input_command_tests` and `nemisis_weapon_simulation_tests` CMake targets.

## Next Game Blocks

- Add player entity/component setup around movement and weapons.
- Add hit-scan request/result types and deterministic recoil seed plumbing.
- Start server-authoritative command queue handoff.
