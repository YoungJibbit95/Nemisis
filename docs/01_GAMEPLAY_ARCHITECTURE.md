# 01 Gameplay Architecture

Nemisis game code is split by gameplay responsibility and consumes NovaCore through public `novacore/...` headers only.

## Current Game Modules

- `nemisis::game`: app bootstrap and high-level game lifetime.
- `nemisis::player`: normalized player input command data.
- `nemisis::input`: default game action names and MKB bindings.
- `nemisis::movement`: movement tuning, state, and deterministic simulation foundation.
- `nemisis::weapons`: weapon definitions, prototype registry, and deterministic fire/reload simulation.

## Direction

- MKB and controller devices produce `PlayerInputCommand`.
- NovaCore input actions map raw controls, buttons, sticks, and triggers into game actions.
- Movement consumes commands and fixed delta time.
- Weapons consume commands through `FireRequest` and advance fixed-tick runtime state.
- Movement and weapon systems now consume NovaCore parsed config documents.
- Config reloads reapply tuning at runtime through `ConfigRegistry`.
- Game code may create NovaCore entities, but must not reach into NovaCore internals.

## First Acceptance

- `main.cpp` stays thin.
- `GameApp` owns game subsystem instances.
- Movement and weapon code compile as separate sources.
- Movement replay tests protect deterministic feel changes.
- Input command tests protect raw action-to-command mapping.
- Weapon simulation tests protect fire cadence, cooldown, dry fire, and reload behavior.
- Config files match the planned movement and weapon vocabulary.
