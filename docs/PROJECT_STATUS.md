# Nemisis Project Status

## Current Foundation

- Game repository consumes NovaCore through `Novacore::Engine`.
- `main.cpp` is thin and delegates to `nemisis::game::GameApp`.
- Game modules exist for input, player commands, movement, and weapons.
- Movement supports sprint, tactical sprint tuning, jump, double jump, dash, slide, and future wall-run values.
- Weapon registry supports AR, SMG, shotgun, and sidearm definitions.
- Movement replay tests cover sprint distance, jump/double-jump, dash cooldown, and config-driven tuning.

## Added In Latest Block

- Movement tuning now loads from `configs/movement/player_movement.json`.
- Weapon definitions now load from `configs/weapons/core_trio.json`.
- GameApp uses NovaCore `ConfigRegistry` for watched JSON config reloads.
- Config reload events reapply movement and weapon settings at runtime.
- Project status documentation added for GitHub-visible progress tracking.

## Next Game Blocks

- Convert `InputActionMap` states into `PlayerInputCommand`.
- Add controller bindings and response curve data.
- Add weapon fire simulation foundation.
- Add player entity/component setup around movement and weapons.
