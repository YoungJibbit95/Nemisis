# Nemisis

A 6v6 competitive arena shooter built on the **NovaCore Engine**. Fast-paced gameplay with advanced movement mechanics, precise gunplay, and skill-based combat.

## Overview

Nemisis is a ground-up game project targeting a commercial-ready competitive FPS experience with:

- 6v6 team-based gameplay
- Advanced movement system blending Titanfall, Apex-style flow, and BO-like TTK
- Precise gunplay and weapon balance
- Multiple game modes
- Custom networking architecture
- Cross-platform support (Windows/Linux first)

## Project Structure

- `game/include/nemisis/` - Game-layer headers
- `game/src/` - Game implementation and executable entrypoint
- `configs/` - Game configuration files (modes, weapons, movement, input)
- `assets/` - Game assets (models, textures, animations)
- `docs/` - Game design and implementation documentation

## Build Requirements

- NovaCore Engine (separate repository)
- C++23 compatible compiler
- CMake 3.27+
- Ninja

## Build Instructions

The game resolves NovaCore from `NOVACORE_ENGINE_ROOT`, then `../Novacore-Engine`, then an installed `Novacore` CMake package.

### With full support:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
```

### Sibling checkout fallback:

```powershell
cmake --preset local-debug-no-deps
cmake --build --preset local-debug-no-deps
```

## Documentation

See `docs/` for game design, mechanics, and implementation guides.

Start with:

- `docs/PROJECT_STATUS.md` for the current GitHub-visible progress log.
- `docs/15_PLAYABLE_DEV_SANDBOX.md` for the first playable developer sandbox, controls, telemetry, and current runtime limits.

## Current Dev Sandbox

The first playable dev loop is wired through `nemisis_game`: NovaCore window input snapshots update Nemisis actions, fixed ticks simulate the local player entity, and dev telemetry logs movement, weapon, command queue, and shot trace data.

At the moment, local configure still requires Ninja plus a visible C++ compiler in PATH.

## Related Projects

- **NovaCore Engine**: https://github.com/YoungJibbit95/Novacore-Engine

## License

See LICENSE file for details.





