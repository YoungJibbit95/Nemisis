# 01 Gameplay Architecture

Nemisis game code is split by gameplay responsibility and consumes NovaCore through public `novacore/...` headers only.

## Current Game Modules

- `nemisis::game`: app bootstrap and high-level game lifetime.
- `nemisis::player`: normalized player input command data.
- `nemisis::input`: default game action names and MKB bindings.
- `nemisis::movement`: movement tuning, state, and deterministic simulation foundation.
- `nemisis::weapons`: weapon definitions and prototype weapon registry.

## Direction

- Input devices produce `PlayerInputCommand`.
- NovaCore input actions map raw controls into game actions.
- Movement consumes commands and fixed delta time.
- Weapons are registered through data-like definitions before real hot-load parsing arrives.
- Game code may create NovaCore entities, but must not reach into NovaCore internals.

## First Acceptance

- `main.cpp` stays thin.
- `GameApp` owns game subsystem instances.
- Movement and weapon code compile as separate sources.
- Movement replay tests protect deterministic feel changes.
- Config files match the planned movement and weapon vocabulary.
