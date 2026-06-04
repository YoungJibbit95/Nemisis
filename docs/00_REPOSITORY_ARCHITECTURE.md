# 00 Repository Architecture

Nemisis is the game repository. It owns game-specific code, assets, tuning, movement identity, weapons, utility, modes, HUD/menu design, and player-facing behavior. It consumes NovaCore as an external engine dependency.

## Boundaries

- `game/include/nemisis`: public game-layer headers used inside this repository.
- `game/src`: Nemisis game implementation and executable entrypoint.
- `configs`: game tuning for movement, weapons, input, and modes.
- `assets`: game-owned meshes, materials, animation, audio, maps, and UI art.
- `docs`: game design and implementation plans.

Nemisis must not fork or copy NovaCore engine internals. Shared runtime improvements go to `YoungJibbit95/Novacore-Engine` first, then Nemisis consumes them.

## Code Quality Rules

- Keep `main.cpp` thin; put game behavior in named systems/classes.
- Keep game rules data-driven where tuning speed matters.
- Keep movement, weapons, utility, and modes separated by subsystem.
- Prefer clear component/data boundaries over inheritance-heavy actor code.
- Do not put dedicated server runtime defaults in this repo.

## Build Rules

Nemisis resolves NovaCore in this order:

1. `NOVACORE_ENGINE_ROOT` CMake cache path.
2. Sibling checkout at `../Novacore-Engine`.
3. Installed CMake package `Novacore`.

The linked engine target is always `Novacore::Engine`.

## Push Rule

Large game work should be committed and pushed to `YoungJibbit95/Nemisis` after each meaningful milestone. Engine changes must be pushed to NovaCore first.

