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
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.
- Input command, weapon simulation, player spawn, and command queue tests cover the newest gameplay bridge.

## Added In Latest Block

- Added player ECS components for identity, local ownership, network metadata, and loadout.
- Added `spawnLocalPlayer` to create the first local player entity through NovaCore ECS.
- Moved local movement and weapon runtime state onto the player entity.
- Added `PlayerCommandQueue` for monotonic tick commands, duplicate replacement, capacity trimming, and server acknowledgements.
- `GameApp::onFixedTick` now pushes commands, simulates component-backed movement/weapons, mirrors movement into transform, and updates pending-command network metadata.
- Added `nemisis_player_command_queue_tests` and `nemisis_player_spawn_tests` CMake targets.

## Next Game Blocks

- Add hit-scan request/result types and deterministic recoil seed plumbing.
- Start client/server command packet serialization and server-authoritative acknowledgement handoff.
- Add player health/damage components for measured TTK tests.
